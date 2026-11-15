#include <QtTest>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QTimer>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "core/pcafacerecognizer.h"
#include "app/mainwindow.h"
#include "app/facerecognitionpage.h"

namespace {
// These artificial textures test algorithm plumbing; they are not a face benchmark.
cv::Mat texture(int identity, int variation)
{
    cv::Mat image(100, 100, CV_8U);
    cv::RNG rng(1000 + identity);
    rng.fill(image, cv::RNG::UNIFORM, 20, 230);
    if (variation) {
        image.at<uchar>(variation, variation) += 1;
    }
    return PcaFaceRecognizer::preprocess(image, FacePreprocessing());
}

QVector<FaceSample> gallery(int count = 10)
{
    QVector<FaceSample> samples;
    for (int person = 0; person < count; ++person) {
        for (int variant = 0; variant < 2; ++variant) {
            FaceSample sample;
            sample.person = QStringLiteral("person_%1").arg(person);
            sample.path = QStringLiteral("/synthetic/%1/%2").arg(person).arg(variant);
            sample.face = texture(person, variant);
            samples.append(sample);
        }
    }
    return samples;
}

void writeImage(const QString &path, const cv::Mat &image)
{
    std::vector<uchar> bytes;
    cv::imencode(".png", image, bytes);
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size()),
             static_cast<qint64>(bytes.size()));
}

void chooseDialogPath(const QString &path, bool directory)
{
    // Drive the public file chooser instead of bypassing the page's data lifecycle.
    QTimer::singleShot(50, [path, directory]() {
        auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        if (directory) {
            dialog->setDirectory(path);
        } else {
            dialog->setDirectory(QFileInfo(path).absolutePath());
        }
        // QFileSystemModel loads directory contents asynchronously.
        QTimer::singleShot(250, dialog, [dialog, path, directory]() {
            if (!directory) {
                auto *fileName = dialog->findChild<QLineEdit *>("fileNameEdit");
                QVERIFY(fileName);
                fileName->setText(QFileInfo(path).fileName());
            }
            QVERIFY(QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection));
        });
        QTimer::singleShot(3000, dialog, [dialog]() {
            if (dialog->isVisible()) {
                qWarning() << "Test file chooser did not accept" << dialog->selectedFiles();
                dialog->reject();
            }
        });
    });
}
}

class FaceRecognitionTests : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    }

    void histogramPreprocessing()
    {
        cv::Mat input(100, 100, CV_8UC3);
        cv::RNG rng(42);
        rng.fill(input, cv::RNG::UNIFORM, 80, 120);
        cv::Mat gray, expected;
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, expected);
        const cv::Mat actual = PcaFaceRecognizer::preprocess(input, FacePreprocessing());
        QCOMPARE(actual.type(), CV_8UC1);
        QCOMPARE(actual.rows, 100);
        QCOMPARE(cv::norm(actual, expected, cv::NORM_INF), 0.0);
        cv::Mat original = input.clone();
        PcaFaceRecognizer::preprocess(input, FacePreprocessing());
        QCOMPARE(cv::norm(original, input, cv::NORM_INF), 0.0);
    }

    void rejectsInvalidImages()
    {
        QVERIFY_EXCEPTION_THROWN(PcaFaceRecognizer::preprocess(cv::Mat(), FacePreprocessing()), std::exception);
        QVERIFY_EXCEPTION_THROWN(PcaFaceRecognizer::preprocess(cv::Mat::zeros(10, 10, CV_8U), FacePreprocessing()), std::exception);
        QVERIFY_EXCEPTION_THROWN(PcaFaceRecognizer::preprocess(cv::Mat::ones(10, 10, CV_32F), FacePreprocessing()), std::exception);
        FacePreprocessing detection;
        detection.detectFace = true;
        detection.cascadePath = QStringLiteral("/missing.xml");
        QVERIFY_EXCEPTION_THROWN(PcaFaceRecognizer::preprocess(texture(0, 0), detection), std::exception);
        const QString cascade = qEnvironmentVariable("FACE_TEST_CASCADE");
        if (!cascade.isEmpty()) {
            detection.cascadePath = cascade;
            try {
                PcaFaceRecognizer::preprocess(texture(0, 0), detection);
                QFAIL("Random texture must not be accepted as a face");
            } catch (const std::exception &error) {
                QVERIFY(QString::fromUtf8(error.what()).contains(QStringLiteral("当前检测到 0 张")));
            }
        }
    }

    void importUnicodeDirectoryAndCorruptImage()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString person = QStringLiteral("张三_001");
        QVERIFY(QDir(directory.path()).mkdir(person));
        const QString path = directory.path() + '/' + person + QStringLiteral("/正面.PNG");
        writeImage(path, texture(0, 0));
        QFile corrupt(directory.path() + '/' + person + "/bad.jpg");
        QVERIFY(corrupt.open(QIODevice::WriteOnly));
        corrupt.write("broken");
        corrupt.close();
        QStringList warnings;
        const auto samples = PcaFaceRecognizer::readPeopleDirectory(directory.path(), FacePreprocessing(), &warnings);
        QCOMPARE(samples.size(), 1);
        QCOMPARE(samples.first().person, person);
        QCOMPARE(warnings.size(), 1);
    }

    void minimumTenDistinctPeople()
    {
        PcaFaceRecognizer model;
        QVERIFY_EXCEPTION_THROWN(model.train(gallery(9), 0), std::exception);
        QVERIFY(!model.isTrained());
        auto onePerson = gallery();
        for (FaceSample &sample : onePerson) {
            sample.person = QStringLiteral("same_person");
        }
        QVERIFY_EXCEPTION_THROWN(model.train(onePerson, 0), std::exception);
    }

    void pcaAndTenPersonRanking()
    {
        PcaFaceRecognizer model;
        model.train(gallery(), 0);
        QVERIFY(model.isTrained());
        QVERIFY(model.components() > 0 && model.components() < 20);
        QVERIFY(model.retainedVariance() >= 0.95 && model.retainedVariance() <= 1.0);
        for (int person = 0; person < 10; ++person) {
            const auto prediction = model.predict(texture(person, 3), 45.0);
            QCOMPARE(prediction.ranking.size(), 10);
            QCOMPARE(prediction.ranking.first().person, QStringLiteral("person_%1").arg(person));
            QVERIFY(prediction.accepted);
            QCOMPARE(prediction.projection.cols, model.components());
            QCOMPARE(prediction.reconstruction.size(), cv::Size(100, 100));
            for (int i = 1; i < prediction.ranking.size(); ++i) {
                QVERIFY(prediction.ranking[i - 1].distance <= prediction.ranking[i].distance);
            }
        }
        QCOMPARE(model.meanFace().size(), cv::Size(100, 100));
        QCOMPARE(model.eigenFace(0).type(), CV_8UC1);
        QVERIFY(model.eigenFace(-1).empty());
    }

    void rejectThresholdAndComponentLimit()
    {
        PcaFaceRecognizer model;
        model.train(gallery(), 1000);
        QCOMPARE(model.components(), 19);
        const auto prediction = model.predict(texture(42, 0), 0.01);
        QVERIFY(!prediction.accepted);
        QVERIFY(prediction.ranking.first().distance > 0.01);
        const auto accepted = model.predict(texture(42, 0), 255.0);
        QVERIFY(accepted.accepted);
        QVERIFY_EXCEPTION_THROWN(model.predict(texture(0, 0), 0.0), std::exception);
        QVERIFY_EXCEPTION_THROWN(model.predict(cv::Mat::ones(50, 50, CV_8U), 45.0), std::exception);
    }

    void failedRetrainingPreservesModel()
    {
        PcaFaceRecognizer model;
        model.train(gallery(), 5);
        QVERIFY_EXCEPTION_THROWN(model.train(gallery(2), 0), std::exception);
        QCOMPARE(model.components(), 5);
        QVERIFY(model.isTrained());
        auto identical = gallery();
        for (FaceSample &sample : identical) {
            sample.face = identical.first().face;
        }
        QVERIFY_EXCEPTION_THROWN(model.train(identical, 0), std::exception);
        QCOMPARE(model.components(), 5);
    }

    void openCvNavigationAndInitialState()
    {
        MainWindow window;
        window.show();
        auto *pages = window.findChild<QStackedWidget *>("pages");
        auto *open = window.findChild<QPushButton *>("pipelineButton");
        QVERIFY(pages && open);
        QWidget *welcome = pages->currentWidget();
        QTest::mouseClick(open, Qt::LeftButton);
        QVERIFY(pages->currentWidget() != welcome);
        auto *tabs = pages->currentWidget()->findChild<QTabWidget *>("tabs");
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 2);
        auto *face = tabs->findChild<FaceRecognitionPage *>();
        QVERIFY(face);
        QVERIFY(!face->findChild<QPushButton *>("trainButton")->isEnabled());
        QVERIFY(!face->findChild<QPushButton *>("recognizeButton")->isEnabled());
        QVERIFY(!face->findChild<QPushButton *>("evaluateButton")->isEnabled());
        QVERIFY(!face->findChild<QPushButton *>("exportButton")->isEnabled());
        const QString screenshotDir = qEnvironmentVariable("FACE_TEST_SCREENSHOT_DIR");
        if (!screenshotDir.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshotDir));
            QTest::qWait(50);
            QVERIFY(window.grab().save(screenshotDir + "/face-gallery.png"));
            face->findChild<QTabWidget *>("workspaceTabs")->setCurrentIndex(1);
            QTest::qWait(50);
            QVERIFY(window.grab().save(screenshotDir + "/face-recognition.png"));
        }
        QTest::mouseClick(face->findChild<QPushButton *>("backButton"), Qt::LeftButton);
        QCOMPARE(pages->currentWidget(), welcome);
    }

    void completeImportTrainRecognizeEvaluateExport()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QDir root(directory.path());
        QVERIFY(root.mkdir("train"));
        QVERIFY(root.mkdir("test"));
        for (int person = 0; person < 10; ++person) {
            const QString name = QStringLiteral("person_%1").arg(person);
            QVERIFY(root.mkpath("train/" + name));
            for (int variant = 0; variant < 2; ++variant) {
                writeImage(root.filePath("train/" + name + QStringLiteral("/%1.png").arg(variant)),
                           texture(person, variant));
            }
        }
        QVERIFY(root.mkpath("test/person_0"));
        QVERIFY(root.mkpath("test/unknown"));
        writeImage(root.filePath("test/person_0/heldout.png"), texture(0, 3));
        writeImage(root.filePath("test/person_0/training_copy.png"), texture(0, 0));
        writeImage(root.filePath("test/unknown/heldout.png"), texture(42, 0));

        FaceRecognitionPage page;
        page.resize(1140, 700);
        page.show();
        auto *trainButton = page.findChild<QPushButton *>("trainButton");
        auto *queryButton = page.findChild<QPushButton *>("queryButton");
        auto *recognizeButton = page.findChild<QPushButton *>("recognizeButton");
        auto *evaluateButton = page.findChild<QPushButton *>("evaluateButton");
        auto *exportButton = page.findChild<QPushButton *>("exportButton");
        auto *tabs = page.findChild<QTabWidget *>("workspaceTabs");
        chooseDialogPath(root.filePath("train"), true);
        QTest::mouseClick(page.findChild<QPushButton *>("importButton"), Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(trainButton->isEnabled(), 10000);
        QCOMPARE(page.findChild<QTableWidget *>("peopleTable")->rowCount(), 10);
        QTest::mouseClick(trainButton, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(evaluateButton->isEnabled(), 10000);
        QVERIFY(page.findChild<QLabel *>("modelStats")->text().contains(QStringLiteral("训练完成")));

        tabs->setCurrentIndex(1);
        page.findChild<QDoubleSpinBox *>("threshold")->setValue(5.0);
        chooseDialogPath(root.filePath("test/person_0/heldout.png"), false);
        QTest::mouseClick(queryButton, Qt::LeftButton);
        QVERIFY(recognizeButton->isEnabled());
        QTest::mouseClick(recognizeButton, Qt::LeftButton);
        QCOMPARE(page.findChild<QTableWidget *>("rankingTable")->rowCount(), 10);
        QVERIFY(page.findChild<QLabel *>("prediction")->text().contains(QStringLiteral("识别结果：person_0")));

        tabs->setCurrentIndex(2);
        chooseDialogPath(root.filePath("test"), true);
        QTest::mouseClick(evaluateButton, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(exportButton->isEnabled(), 10000);
        QCOMPARE(page.findChild<QTableWidget *>("evaluationTable")->rowCount(), 2);
        QVERIFY(page.findChild<QLabel *>("evaluationSummary")->text().contains(QStringLiteral("排除训练重复 1 张")));
        const QString csvPath = root.filePath("evaluation.csv");
        chooseDialogPath(csvPath, false);
        QTest::mouseClick(exportButton, Qt::LeftButton);
        QFile csv(csvPath);
        QVERIFY(csv.open(QIODevice::ReadOnly));
        const QByteArray bytes = csv.readAll();
        QVERIFY(bytes.startsWith(QByteArray::fromHex("efbbbf")));
        QVERIFY(bytes.contains("person_0"));
        QVERIFY(bytes.contains("unknown"));

        // Changing the threshold clears old results, changing PCA invalidates the model.
        page.findChild<QDoubleSpinBox *>("threshold")->setValue(6.0);
        QVERIFY(!exportButton->isEnabled());
        QCOMPARE(page.findChild<QTableWidget *>("rankingTable")->rowCount(), 0);
        page.findChild<QSpinBox *>("components")->setValue(3);
        QVERIFY(!recognizeButton->isEnabled());
        QVERIFY(!evaluateButton->isEnabled());
        page.findChild<QComboBox *>("cropMode")->setCurrentIndex(1);
        QCOMPARE(page.findChild<QTableWidget *>("peopleTable")->rowCount(), 0);
        QVERIFY(!trainButton->isEnabled());
    }
};

QTEST_MAIN(FaceRecognitionTests)
#include "face_recognition_tests.moc"
