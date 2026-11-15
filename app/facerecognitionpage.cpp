#include "facerecognitionpage.h"
#include "ui_facerecognitionpage.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QImage>
#include <QInputDialog>
#include <QMap>
#include <QPixmap>
#include <QSet>
#include <QtConcurrent/QtConcurrentRun>
#include <exception>

namespace {
QString imageFilter()
{
    return QStringLiteral("人脸图像 (*.png *.jpg *.jpeg *.bmp *.pgm *.tif *.tiff *.webp)");
}

QString csvField(QString text)
{
    // Quote all cells, and prevent spreadsheet applications interpreting names as formulas.
    if (!text.isEmpty() && QStringLiteral("=+-@\t\r\n").contains(text.at(0))) {
        text.prepend('\'');
    }
    text.replace('"', QStringLiteral("\"\""));
    return '"' + text + '"';
}
}

FaceRecognitionPage::FaceRecognitionPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::FaceRecognitionPage)
{
    ui->setupUi(this);
    ui->cascadePath->setText(PcaFaceRecognizer::defaultCascadePath());
    ui->logView->setMaximumBlockCount(500);
    ui->peopleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->rankingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->evaluationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(ui->backButton, &QPushButton::clicked, this, &FaceRecognitionPage::backRequested);
    connect(ui->importButton, &QPushButton::clicked, this, &FaceRecognitionPage::importDirectory);
    connect(ui->addPersonButton, &QPushButton::clicked, this, &FaceRecognitionPage::addPerson);
    connect(ui->removePersonButton, &QPushButton::clicked, this, &FaceRecognitionPage::removePerson);
    connect(ui->trainButton, &QPushButton::clicked, this, &FaceRecognitionPage::train);
    connect(ui->queryButton, &QPushButton::clicked, this, &FaceRecognitionPage::loadQuery);
    connect(ui->recognizeButton, &QPushButton::clicked, this, &FaceRecognitionPage::recognize);
    connect(ui->evaluateButton, &QPushButton::clicked, this, &FaceRecognitionPage::evaluate);
    connect(ui->exportButton, &QPushButton::clicked, this, &FaceRecognitionPage::exportEvaluation);
    connect(ui->peopleTable, &QTableWidget::itemSelectionChanged,
            this, &FaceRecognitionPage::updateSamplePreview);
    connect(ui->eigenIndex, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { updateEigenPreview(); });
    connect(ui->components, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { invalidateModel(); });
    connect(ui->threshold, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, [this](double) { clearPrediction(); clearEvaluation(); });
    connect(ui->cropMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { preprocessingChanged(); });
    connect(ui->cascadePath, &QLineEdit::textChanged,
            this, [this](const QString &) {
        if (ui->cropMode->currentIndex() == 1) {
            preprocessingChanged();
        }
    });
    connect(ui->cascadeButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择 Haar 分类器"),
                                                         QString(), QStringLiteral("XML (*.xml)"));
        if (!path.isEmpty()) {
            ui->cascadePath->setText(path);
        }
    });
    setStyleSheet(QStringLiteral(
        "QWidget { color: #1f2937; }"
        "QLabel#pageTitle { font-size: 23px; font-weight: 600; }"
        "QLabel#workflow { padding: 8px; background: #eff6ff; color: #1d4ed8; }"
        "QPushButton { padding: 7px 12px; }"
        "QLabel#prediction { padding: 8px; font-size: 16px; font-weight: 600; }"
        "QLabel#samplePreview, QLabel#meanPreview, QLabel#eigenPreview,"
        "QLabel#originalPreview, QLabel#normalizedPreview, QLabel#reconstructionPreview,"
        "QLabel#matchedPreview { background: #eef2f6; border: 1px solid #cbd5e1; }"
        "QPlainTextEdit { background: #111827; color: #e5e7eb; }"
        "QLabel#status { color: #52606d; }"));
    setBusy(false);
}

FaceRecognitionPage::~FaceRecognitionPage()
{
    // A worker owns only value snapshots, so waiting cannot call back into destroyed widgets.
    m_watcher.waitForFinished();
    delete ui;
}

FacePreprocessing FaceRecognitionPage::preprocessing() const
{
    FacePreprocessing options;
    options.detectFace = ui->cropMode->currentIndex() == 1;
    options.cascadePath = ui->cascadePath->text().trimmed();
    return options;
}

void FaceRecognitionPage::setBusy(bool busy)
{
    m_busy = busy;
    ui->preprocessingBox->setEnabled(!busy);
    ui->importButton->setEnabled(!busy);
    ui->addPersonButton->setEnabled(!busy);
    ui->removePersonButton->setEnabled(!busy && !m_samples.isEmpty());
    ui->components->setEnabled(!busy);
    ui->threshold->setEnabled(!busy);
    ui->trainButton->setEnabled(!busy && PcaFaceRecognizer::people(m_samples).size()
                                >= PcaFaceRecognizer::MinimumPeople);
    ui->queryButton->setEnabled(!busy);
    ui->recognizeButton->setEnabled(!busy && m_model.isTrained() && !m_query.empty());
    ui->evaluateButton->setEnabled(!busy && m_model.isTrained());
    ui->exportButton->setEnabled(!busy && !m_evaluation.isEmpty());
    ui->progress->setRange(0, busy ? 0 : 1);
    ui->progress->setValue(busy ? 0 : 1);
}

void FaceRecognitionPage::runTask(const QString &message,
                                  const std::function<FaceTaskResult()> &task,
                                  const std::function<void(const FaceTaskResult &)> &apply)
{
    if (m_busy) {
        return;
    }
    setBusy(true);
    log(message);
    disconnect(&m_watcher, nullptr, this, nullptr);
    connect(&m_watcher, &QFutureWatcher<FaceTaskResult>::finished, this, [this, apply]() {
        const FaceTaskResult result = m_watcher.result();
        if (result.error.isEmpty()) {
            apply(result);
            for (const QString &warning : result.warnings) {
                ui->logView->appendPlainText(warning);
            }
            log(result.summary);
        } else {
            log(QStringLiteral("操作失败：%1").arg(result.error));
        }
        setBusy(false);
    });
    m_watcher.setFuture(QtConcurrent::run([task]() {
        try {
            return task();
        } catch (const std::exception &error) {
            FaceTaskResult result;
            result.error = QString::fromUtf8(error.what());
            return result;
        } catch (...) {
            FaceTaskResult result;
            result.error = QStringLiteral("未知错误");
            return result;
        }
    }));
}

void FaceRecognitionPage::importDirectory()
{
    const QString root = QFileDialog::getExistingDirectory(this, QStringLiteral("选择人员库根目录"));
    if (root.isEmpty()) {
        return;
    }
    const FacePreprocessing options = preprocessing();
    runTask(QStringLiteral("正在导入并预处理人员库：%1").arg(root), [root, options]() {
        FaceTaskResult result;
        result.samples = PcaFaceRecognizer::readPeopleDirectory(root, options, &result.warnings);
        if (result.samples.isEmpty()) {
            result.error = QStringLiteral("目录内没有可用人脸。请检查“根目录 / 姓名 / 图片”结构与预处理模式。\n")
                + result.warnings.mid(0, 5).join('\n');
        }
        result.summary = QStringLiteral("人员库导入完成：%1 人，%2 张有效样本，%3 条提示。")
            .arg(PcaFaceRecognizer::people(result.samples).size())
            .arg(result.samples.size()).arg(result.warnings.size());
        return result;
    }, [this](const FaceTaskResult &result) {
        // Replace the in-memory gallery only after a successful, nonempty import.
        m_samples = result.samples;
        invalidateModel();
        refreshGallery();
    });
}

void FaceRecognitionPage::addPerson()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("录入人员"),
        QStringLiteral("姓名或学号（相同名称会追加样本）："), QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    const QStringList paths = QFileDialog::getOpenFileNames(this, QStringLiteral("选择此人的多张照片"),
                                                           QString(), imageFilter());
    if (paths.isEmpty()) {
        return;
    }
    const FacePreprocessing options = preprocessing();
    const QVector<FaceSample> existing = m_samples;
    runTask(QStringLiteral("正在录入 %1 的人脸照片……").arg(name), [name, paths, options, existing]() {
        FaceTaskResult result;
        result.samples = existing;
        QSet<QString> usedPaths;
        for (const FaceSample &sample : existing) {
            usedPaths.insert(sample.path);
        }
        int added = 0;
        for (const QString &path : paths) {
            const QString canonical = QFileInfo(path).canonicalFilePath();
            if (usedPaths.contains(canonical)) {
                result.warnings.append(QStringLiteral("已在库中，跳过重复路径：%1").arg(path));
                continue;
            }
            try {
                FaceSample sample;
                sample.person = name;
                sample.path = canonical;
                sample.face = PcaFaceRecognizer::preprocess(PcaFaceRecognizer::readImage(path), options);
                result.samples.append(sample);
                usedPaths.insert(canonical);
                ++added;
            } catch (const std::exception &error) {
                result.warnings.append(path + QStringLiteral("：") + QString::fromUtf8(error.what()));
            }
        }
        if (added == 0) {
            result.error = QStringLiteral("未新增有效样本。\n") + result.warnings.mid(0, 5).join('\n');
        }
        result.summary = QStringLiteral("%1 新增 %2 张有效样本。") .arg(name).arg(added);
        return result;
    }, [this](const FaceTaskResult &result) {
        m_samples = result.samples;
        invalidateModel();
        refreshGallery();
    });
}

void FaceRecognitionPage::removePerson()
{
    const int row = ui->peopleTable->currentRow();
    if (row < 0 || !ui->peopleTable->item(row, 0)) {
        return;
    }
    const QString name = ui->peopleTable->item(row, 0)->text();
    for (int i = m_samples.size() - 1; i >= 0; --i) {
        if (m_samples.at(i).person == name) {
            m_samples.removeAt(i);
        }
    }
    invalidateModel();
    refreshGallery();
    setBusy(false);
    log(QStringLiteral("已从当前人员库移除 %1，需重新训练。").arg(name));
}

void FaceRecognitionPage::train()
{
    const QVector<FaceSample> samples = m_samples;
    const int components = ui->components->value();
    runTask(QStringLiteral("正在执行 PCA 特征提取与训练……"), [samples, components]() {
        FaceTaskResult result;
        result.model.train(samples, components);
        result.summary = QStringLiteral("训练完成：%1 人 / %2 张，10000 维 → %3 维，保留方差 %4%。")
            .arg(PcaFaceRecognizer::people(samples).size()).arg(samples.size())
            .arg(result.model.components()).arg(result.model.retainedVariance() * 100.0, 0, 'f', 2);
        return result;
    }, [this](const FaceTaskResult &result) {
        m_model = result.model;
        clearPrediction();
        clearEvaluation();
        ui->modelStats->setText(result.summary);
        ui->eigenIndex->setMaximum(m_model.components());
        showImage(ui->meanPreview, m_model.meanFace());
        updateEigenPreview();
    });
}

void FaceRecognitionPage::loadQuery()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择待识别人脸"),
                                                     QString(), imageFilter());
    if (path.isEmpty()) {
        return;
    }
    try {
        const cv::Mat image = PcaFaceRecognizer::readImage(path);
        const cv::Mat face = PcaFaceRecognizer::preprocess(image, preprocessing());
        m_query = face;
        clearPrediction();
        showImage(ui->originalPreview, image);
        showImage(ui->normalizedPreview, face);
        setBusy(false);
        log(QStringLiteral("已预处理待识别图像：%1").arg(path));
    } catch (const std::exception &error) {
        log(QStringLiteral("无法载入待识别图像：%1").arg(QString::fromUtf8(error.what())));
    }
}

void FaceRecognitionPage::recognize()
{
    if (m_query.empty() || !m_model.isTrained()) {
        return;
    }
    try {
        const FacePrediction result = m_model.predict(m_query, ui->threshold->value());
        const FaceMatch &best = result.ranking.first();
        ui->rankingTable->setRowCount(result.ranking.size());
        for (int row = 0; row < result.ranking.size(); ++row) {
            const FaceMatch &match = result.ranking.at(row);
            const QStringList cells = QStringList() << QString::number(row + 1) << match.person
                << QString::number(match.distance, 'f', 3) << m_model.samples().at(match.sampleIndex).path;
            for (int col = 0; col < cells.size(); ++col) {
                auto *item = new QTableWidgetItem(cells.at(col));
                item->setToolTip(cells.at(col));
                ui->rankingTable->setItem(row, col, item);
            }
        }
        showImage(ui->reconstructionPreview, result.reconstruction);
        showImage(ui->matchedPreview, m_model.samples().at(best.sampleIndex).face);
        ui->prediction->setText(result.accepted
            ? QStringLiteral("识别结果：%1 ｜ 距离 %2 ≤ 阈值 %3 ｜ 已比对 %4 人")
                .arg(best.person).arg(best.distance, 0, 'f', 3).arg(ui->threshold->value())
                .arg(result.ranking.size())
            : QStringLiteral("未知人员（拒识）｜ 最近候选 %1，距离 %2 > 阈值 %3 ｜ 已比对 %4 人")
                .arg(best.person).arg(best.distance, 0, 'f', 3).arg(ui->threshold->value())
                .arg(result.ranking.size()));
        log(ui->prediction->text());
    } catch (const std::exception &error) {
        log(QString::fromUtf8(error.what()));
    }
}

void FaceRecognitionPage::evaluate()
{
    const QString root = QFileDialog::getExistingDirectory(this, QStringLiteral("选择独立测试集根目录"));
    if (root.isEmpty() || !m_model.isTrained()) {
        return;
    }
    const PcaFaceRecognizer model = m_model;
    const FacePreprocessing options = preprocessing();
    const double threshold = ui->threshold->value();
    runTask(QStringLiteral("正在评估独立测试集……"), [root, model, options, threshold]() {
        FaceTaskResult result;
        const QVector<FaceSample> tests = PcaFaceRecognizer::readPeopleDirectory(root, options,
                                                                               &result.warnings);
        const QStringList knownPeople = PcaFaceRecognizer::people(model.samples());
        int known = 0, knownCorrect = 0, unknown = 0, unknownCorrect = 0, excluded = 0;
        for (const FaceSample &test : tests) {
            bool overlap = false;
            for (const FaceSample &training : model.samples()) {
                if (test.path == training.path || cv::norm(test.face, training.face, cv::NORM_INF) == 0) {
                    overlap = true;
                    break;
                }
            }
            if (overlap) {
                ++excluded;
                result.warnings.append(QStringLiteral("排除训练照片或相同预处理副本：%1").arg(test.path));
                continue;
            }
            const FacePrediction prediction = model.predict(test.face, threshold);
            FaceEvaluationRow row;
            row.path = test.path;
            row.truth = test.person;
            row.prediction = prediction.accepted ? prediction.ranking.first().person
                                                 : QStringLiteral("未知人员（拒识）");
            row.distance = prediction.ranking.first().distance;
            if (knownPeople.contains(test.person)) {
                ++known;
                row.correct = prediction.accepted && prediction.ranking.first().person == test.person;
                knownCorrect += row.correct;
            } else {
                ++unknown;
                row.correct = !prediction.accepted;
                unknownCorrect += row.correct;
            }
            result.evaluation.append(row);
        }
        const QString knownRate = known ? QStringLiteral("%1/%2（%3%）")
            .arg(knownCorrect).arg(known).arg(100.0 * knownCorrect / known, 0, 'f', 2)
            : QStringLiteral("无已知人员测试样本");
        const QString unknownRate = unknown ? QStringLiteral("%1/%2（%3%）")
            .arg(unknownCorrect).arg(unknown).arg(100.0 * unknownCorrect / unknown, 0, 'f', 2)
            : QStringLiteral("无未知人员测试样本");
        result.summary = QStringLiteral("阈值 %1 ｜ 已知人员识别正确：%2 ｜ 未知人员正确拒识：%3 ｜ 排除训练重复 %4 张 ｜ 提示 %5 条")
            .arg(threshold).arg(knownRate, unknownRate).arg(excluded).arg(result.warnings.size());
        return result;
    }, [this](const FaceTaskResult &result) {
        m_evaluation = result.evaluation;
        ui->evaluationSummary->setText(result.summary);
        ui->evaluationTable->setRowCount(m_evaluation.size());
        for (int row = 0; row < m_evaluation.size(); ++row) {
            const FaceEvaluationRow &entry = m_evaluation.at(row);
            const QStringList cells = QStringList() << entry.path << entry.truth << entry.prediction
                << QString::number(entry.distance, 'f', 3)
                << (entry.correct ? QStringLiteral("正确") : QStringLiteral("错误"));
            for (int col = 0; col < cells.size(); ++col) {
                auto *item = new QTableWidgetItem(cells.at(col));
                item->setToolTip(cells.at(col));
                ui->evaluationTable->setItem(row, col, item);
            }
        }
    });
}

void FaceRecognitionPage::exportEvaluation()
{
    if (m_evaluation.isEmpty()) {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出评估结果"),
                                                     QStringLiteral("face_evaluation.csv"), "CSV (*.csv)");
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        log(QStringLiteral("无法写入：%1").arg(path));
        return;
    }
    QString csv = QStringLiteral("图像,真实人员,识别结果,PCA距离,判定,拒识阈值,PCA维数\r\n");
    for (const FaceEvaluationRow &row : m_evaluation) {
        csv += csvField(row.path) + ',' + csvField(row.truth) + ',' + csvField(row.prediction) + ','
            + QString::number(row.distance, 'f', 6) + ','
            + (row.correct ? QStringLiteral("正确") : QStringLiteral("错误")) + ','
            + QString::number(ui->threshold->value()) + ',' + QString::number(m_model.components()) + "\r\n";
    }
    const QByteArray bytes = QByteArray::fromHex("efbbbf") + csv.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        log(QStringLiteral("CSV 写入失败：%1").arg(file.errorString()));
        return;
    }
    log(QStringLiteral("评估结果已导出：%1").arg(path));
}

void FaceRecognitionPage::preprocessingChanged()
{
    m_samples.clear();
    m_query.release();
    ui->originalPreview->clear();
    ui->normalizedPreview->clear();
    invalidateModel();
    refreshGallery();
    setBusy(false);
    log(QStringLiteral("预处理设置已修改，请重新导入人员库与待识别图像。"));
}

void FaceRecognitionPage::clearPrediction()
{
    ui->rankingTable->setRowCount(0);
    ui->matchedPreview->clear();
    ui->reconstructionPreview->clear();
    ui->prediction->setText(QStringLiteral("请载入图像并点击“开始识别”。"));
}

void FaceRecognitionPage::clearEvaluation()
{
    m_evaluation.clear();
    ui->evaluationTable->setRowCount(0);
    ui->evaluationSummary->setText(QStringLiteral("尚未评估（模型或阈值改变后需重新评估）"));
    ui->exportButton->setEnabled(false);
}

void FaceRecognitionPage::invalidateModel()
{
    m_model = PcaFaceRecognizer();
    ui->modelStats->setText(QStringLiteral("模型尚未训练 / 数据或参数已改变"));
    ui->meanPreview->clear();
    ui->eigenPreview->clear();
    clearPrediction();
    clearEvaluation();
    if (!m_busy) {
        setBusy(false);
    }
}

void FaceRecognitionPage::refreshGallery()
{
    QMap<QString, int> counts;
    for (const FaceSample &sample : m_samples) {
        ++counts[sample.person];
    }
    ui->peopleTable->setRowCount(0);
    ui->peopleTable->setRowCount(counts.size());
    int row = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it, ++row) {
        ui->peopleTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        ui->peopleTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
    }
    ui->galleryStats->setText(QStringLiteral("%1 人 / %2 张 · %3")
        .arg(counts.size()).arg(m_samples.size())
        .arg(counts.size() >= PcaFaceRecognizer::MinimumPeople ? QStringLiteral("已满足至少 10 人")
                                                              : QStringLiteral("至少需要 10 人")));
    ui->samplePreview->clear();
    if (!counts.isEmpty()) {
        ui->peopleTable->selectRow(0);
    }
}

void FaceRecognitionPage::updateSamplePreview()
{
    const int row = ui->peopleTable->currentRow();
    if (row < 0 || !ui->peopleTable->item(row, 0)) {
        return;
    }
    const QString person = ui->peopleTable->item(row, 0)->text();
    for (const FaceSample &sample : m_samples) {
        if (sample.person == person) {
            showImage(ui->samplePreview, sample.face);
            return;
        }
    }
}

void FaceRecognitionPage::updateEigenPreview()
{
    showImage(ui->eigenPreview, m_model.eigenFace(ui->eigenIndex->value() - 1));
}

void FaceRecognitionPage::showImage(QLabel *label, const cv::Mat &image)
{
    if (image.empty()) {
        label->clear();
        return;
    }
    const QImage preview(image.data, image.cols, image.rows, static_cast<int>(image.step),
                         QImage::Format_Grayscale8);
    // Fixed thumbnail bounds keep layouts stable when switching tabs and resizing.
    label->setPixmap(QPixmap::fromImage(preview.copy()).scaled(130, 140, Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation));
}

void FaceRecognitionPage::log(const QString &text)
{
    ui->status->setText(text);
    ui->logView->appendPlainText(text);
}
