#include "yoloworkbenchpage.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTextEdit>

#include "ui_yoloworkbenchpage.h"

YoloWorkbenchPage::YoloWorkbenchPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::YoloWorkbenchPage),
      m_process(new QProcess(this))
{
    ui->setupUi(this);

    ui->action->addItem(QStringLiteral("训练"), QStringLiteral("train"));
    ui->action->addItem(QStringLiteral("验证"), QStringLiteral("val"));
    ui->action->addItem(QStringLiteral("实时/视频推理"), QStringLiteral("predict"));
    ui->action->addItem(QStringLiteral("导出 ONNX"), QStringLiteral("export"));
    ui->action->addItem(QStringLiteral("INT8 量化（OpenVINO）"), QStringLiteral("quantize"));
    ui->device->addItem(QStringLiteral("自动检测"), QStringLiteral("auto"));
    ui->device->addItem(QStringLiteral("CPU"), QStringLiteral("cpu"));
    ui->device->addItem(QStringLiteral("CUDA 0"), QStringLiteral("0"));

    // The .ui file owns layout; this class owns process lifecycle and data flow.
    connect(ui->backButton, &QPushButton::clicked, this, &YoloWorkbenchPage::backRequested);
    connect(ui->prepareButton, &QPushButton::clicked,
            this, &YoloWorkbenchPage::prepareEnvironment);
    connect(ui->inspectButton, &QPushButton::clicked,
            this, &YoloWorkbenchPage::inspectEnvironment);
    connect(ui->runButton, &QPushButton::clicked, this, &YoloWorkbenchPage::runAction);
    connect(ui->modelButton, &QPushButton::clicked, this, &YoloWorkbenchPage::chooseModel);
    connect(ui->dataButton, &QPushButton::clicked, this, &YoloWorkbenchPage::chooseData);
    connect(ui->sourceButton, &QPushButton::clicked, this, &YoloWorkbenchPage::chooseSource);
    connect(ui->action, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &YoloWorkbenchPage::updateActionForm);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &YoloWorkbenchPage::readStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &YoloWorkbenchPage::readStandardError);
    connect(m_process, &QProcess::errorOccurred,
            this, &YoloWorkbenchPage::processError);
    connect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &YoloWorkbenchPage::processFinished);

    setStyleSheet(
        QStringLiteral("QWidget { background: #f5f7fa; color: #1f2937; }"
                       "QPushButton, QComboBox, QLineEdit, QSpinBox, QDoubleSpinBox {"
                       " padding: 7px 10px; background: white; border: 1px solid #94a3b8;"
                       " border-radius: 5px; }"
                       "QPushButton:hover { border-color: #2563eb; background: #eff6ff; }"
                       "QLabel#pageTitle { font-size: 26px; font-weight: 600; }"
                       "QTextEdit { background: #111827; color: #e5e7eb; padding: 8px; }"
                       "QLabel#status { color: #52606d; padding: 6px; }"));
    updateActionForm(0);
    ui->status->setText(QStringLiteral("可先检查环境，再选择训练、验证、推理、导出或量化动作"));
}

YoloWorkbenchPage::~YoloWorkbenchPage()
{
    delete ui;
}

QString YoloWorkbenchPage::scriptPath(const QString &fileName) const
{
    const QStringList candidates = QStringList()
        << QCoreApplication::applicationDirPath() + QStringLiteral("/scripts/") + fileName
        << QCoreApplication::applicationDirPath() + QStringLiteral("/../scripts/") + fileName
        << QDir::currentPath() + QStringLiteral("/scripts/") + fileName
        << QDir::currentPath() + QStringLiteral("/../scripts/") + fileName;
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).canonicalFilePath();
        }
    }
    return QString();
}

QString YoloWorkbenchPage::pythonExecutable() const
{
    return QStringLiteral("python3");
}

void YoloWorkbenchPage::prepareEnvironment()
{
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }
    const QString script = scriptPath(QStringLiteral("prepare_yolov8_env.sh"));
    if (script.isEmpty()) {
        ui->status->setText(QStringLiteral("找不到 scripts/prepare_yolov8_env.sh"));
        return;
    }
    ui->log->clear();
    ui->log->append(QStringLiteral("开始准备 YOLOv8 环境..."));
    setBusy(true);
    m_process->setProgram(QStringLiteral("bash"));
    m_process->setArguments(QStringList() << script);
    m_process->start();
}

void YoloWorkbenchPage::inspectEnvironment()
{
    startPythonTask(QStringList() << QStringLiteral("--task") << QStringLiteral("env"));
}

void YoloWorkbenchPage::runAction()
{
    QStringList arguments;
    const QString action = ui->action->currentData().toString();
    arguments << QStringLiteral("--task") << action
              << QStringLiteral("--model") << ui->model->text()
              << QStringLiteral("--device") << ui->device->currentData().toString()
              << QStringLiteral("--imgsz") << QString::number(ui->imageSize->value())
              << QStringLiteral("--output-dir") << ui->outputDir->text();
    if (action == QStringLiteral("train")) {
        arguments << QStringLiteral("--data") << ui->data->text()
                  << QStringLiteral("--epochs") << QString::number(ui->epochs->value())
                  << QStringLiteral("--batch") << QString::number(ui->batch->value());
    } else if (action == QStringLiteral("val")) {
        arguments << QStringLiteral("--data") << ui->data->text();
    } else if (action == QStringLiteral("predict")) {
        arguments << QStringLiteral("--source") << ui->source->text()
                  << QStringLiteral("--conf") << QString::number(ui->confidence->value())
                  << QStringLiteral("--iou") << QString::number(ui->iou->value());
    } else if (action == QStringLiteral("quantize")) {
        arguments << QStringLiteral("--data") << ui->data->text();
    }
    startPythonTask(arguments);
}

void YoloWorkbenchPage::chooseModel()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择模型"), QString(),
        QStringLiteral("Models (*.pt *.onnx)"));
    if (!path.isEmpty()) {
        ui->model->setText(path);
    }
}

void YoloWorkbenchPage::chooseData()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择数据集 YAML"), QString(),
        QStringLiteral("YAML (*.yaml *.yml)"));
    if (!path.isEmpty()) {
        ui->data->setText(path);
    }
}

void YoloWorkbenchPage::chooseSource()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择推理源"), QString(),
        QStringLiteral("Media (*.jpg *.jpeg *.png *.mp4 *.avi)"));
    if (!path.isEmpty()) {
        ui->source->setText(path);
    }
}

void YoloWorkbenchPage::updateActionForm(int index)
{
    const QString action = ui->action->itemData(index).toString();
    const bool train = action == QStringLiteral("train");
    const bool predict = action == QStringLiteral("predict");
    ui->data->setEnabled(train || action == QStringLiteral("val") ||
                         action == QStringLiteral("quantize"));
    ui->source->setEnabled(predict);
    ui->epochs->setEnabled(train);
    ui->batch->setEnabled(train);
    ui->confidence->setEnabled(predict);
    ui->iou->setEnabled(predict);
}

void YoloWorkbenchPage::startPythonTask(const QStringList &arguments)
{
    if (m_process->state() != QProcess::NotRunning) {
        ui->status->setText(QStringLiteral("已有任务正在运行"));
        return;
    }
    const QString script = scriptPath(QStringLiteral("yolov8_runner.py"));
    if (script.isEmpty()) {
        ui->status->setText(QStringLiteral("找不到 scripts/yolov8_runner.py"));
        return;
    }
    ui->log->clear();
    setBusy(true);
    QStringList processArguments;
    processArguments << script;
    processArguments += arguments;
    m_process->setProgram(pythonExecutable());
    m_process->setArguments(processArguments);
    m_process->start();
}

void YoloWorkbenchPage::appendLog(const QString &text)
{
    if (!text.trimmed().isEmpty()) {
        ui->log->append(text.trimmed());
    }
}

void YoloWorkbenchPage::readStandardOutput()
{
    appendLog(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
}

void YoloWorkbenchPage::readStandardError()
{
    appendLog(QString::fromLocal8Bit(m_process->readAllStandardError()));
}

void YoloWorkbenchPage::processError(QProcess::ProcessError error)
{
    setBusy(false);
    if (error == QProcess::FailedToStart) {
        ui->status->setText(QStringLiteral("无法启动外部进程，请检查 Python 或 bash 路径"));
    } else {
        ui->status->setText(QStringLiteral("外部进程发生错误：%1").arg(static_cast<int>(error)));
    }
}

void YoloWorkbenchPage::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    setBusy(false);
    if (exitStatus == QProcess::CrashExit) {
        ui->status->setText(QStringLiteral("外部进程异常退出"));
    } else if (exitCode == 0) {
        ui->status->setText(QStringLiteral("任务完成"));
    } else {
        ui->status->setText(QStringLiteral("任务失败，退出码：%1").arg(exitCode));
    }
}

void YoloWorkbenchPage::setBusy(bool busy)
{
    ui->status->setText(busy ? QStringLiteral("任务运行中...") : QStringLiteral("任务已结束"));
}
