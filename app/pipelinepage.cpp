#include "pipelinepage.h"

#include <QComboBox>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QStringList>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "../core/noderegistry.h"
#include "ui_pipelinepage.h"

namespace {

QString statusText(const QString &prefix, const QString &detail)
{
    return prefix + (detail.isEmpty() ? QString() : QStringLiteral("：") + detail);
}

} // namespace

PipelinePage::PipelinePage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::PipelinePage)
{
    ui->setupUi(this);

    const QVector<NodeDescriptor> descriptors = NodeRegistry::descriptors();
    for (const NodeDescriptor &descriptor : descriptors) {
        ui->nodeType->addItem(descriptor.displayName, static_cast<int>(descriptor.type));
    }

    // Only behaviour is wired here; dimensions and layout are editable in Designer.
    connect(ui->backButton, &QPushButton::clicked, this, &PipelinePage::backRequested);
    connect(ui->loadButton, &QPushButton::clicked, this, &PipelinePage::loadImage);
    connect(ui->addButton, &QPushButton::clicked, this, &PipelinePage::addNode);
    connect(ui->removeButton, &QPushButton::clicked, this, &PipelinePage::removeNode);
    connect(ui->runButton, &QPushButton::clicked, this, &PipelinePage::runPipeline);
    connect(ui->nodeList, &QListWidget::currentRowChanged,
            this, &PipelinePage::updateSelectedNode);

    setStyleSheet(
        QStringLiteral("QWidget { background: #f5f7fa; color: #1f2937; }"
                       "QPushButton, QComboBox { padding: 8px 12px; background: white;"
                       " border: 1px solid #94a3b8; border-radius: 5px; }"
                       "QPushButton:hover { border-color: #2563eb; background: #eff6ff; }"
                       "QListWidget { background: white; border: 1px solid #cbd5e1; }"
                       "QLabel#inputTitle, QLabel#nodeTitle, QLabel#resultTitle { font-weight: 600; }"
                       "QLabel#originalPreview, QLabel#nodePreview, QLabel#resultPreview {"
                       " background: white; border: 1px solid #cbd5e1; }"
                       "QTextEdit { background: #111827; color: #e5e7eb; padding: 8px; }"
                       "QLabel#status { color: #52606d; padding: 6px; }"));

    createDemoImage();
    ui->status->setText(QStringLiteral("已载入演示图像，可添加节点后运行流程"));
}

PipelinePage::~PipelinePage()
{
    delete ui;
}

void PipelinePage::createDemoImage()
{
    m_inputImage = cv::Mat(420, 700, CV_8UC3, cv::Scalar(235, 242, 250));
    cv::rectangle(m_inputImage, cv::Rect(70, 70, 210, 210), cv::Scalar(55, 115, 220), 4);
    cv::circle(m_inputImage, cv::Point(480, 170), 95, cv::Scalar(50, 180, 100), 4);
    cv::putText(m_inputImage, "OpenCV demo", cv::Point(230, 360),
                cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(40, 60, 90), 2);
    showMat(ui->originalPreview, m_inputImage);
}

void PipelinePage::loadImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择图像"), QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty()) {
        return;
    }

    const cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (image.empty()) {
        ui->status->setText(QStringLiteral("无法读取图像：%1").arg(path));
        return;
    }
    m_inputImage = image;
    m_results.clear();
    showMat(ui->originalPreview, m_inputImage);
    ui->nodePreview->clear();
    ui->resultPreview->clear();
    ui->codeView->clear();
    ui->status->setText(QStringLiteral("已载入：%1").arg(path));
}

void PipelinePage::addNode()
{
    const VisionNode::Type type = static_cast<VisionNode::Type>(
        ui->nodeType->currentData().toInt());
    m_pipeline.addNode(NodeRegistry::create(type));
    m_results.clear();
    refreshNodeList();
    ui->nodeList->setCurrentRow(ui->nodeList->count() - 1);
    ui->status->setText(QStringLiteral("已添加节点：%1").arg(VisionNode(type).name()));
}

void PipelinePage::removeNode()
{
    const int row = ui->nodeList->currentRow();
    if (row < 0) {
        return;
    }
    m_pipeline.removeNode(row);
    m_results.clear();
    refreshNodeList();
    ui->codeView->clear();
    ui->nodePreview->clear();
    ui->resultPreview->clear();
    ui->status->setText(QStringLiteral("已删除节点，运行流程后更新预览"));
}

void PipelinePage::runPipeline()
{
    if (m_inputImage.empty()) {
        ui->status->setText(QStringLiteral("请先载入图像"));
        return;
    }
    if (m_pipeline.size() == 0) {
        ui->status->setText(QStringLiteral("请先添加至少一个节点"));
        return;
    }

    m_results = m_pipeline.execute(m_inputImage);
    if (m_results.isEmpty()) {
        ui->status->setText(QStringLiteral("流程没有输出"));
        return;
    }

    const Pipeline::NodeResult &last = m_results.last();
    if (!last.error.isEmpty()) {
        ui->status->setText(statusText(
            QStringLiteral("流程在第 %1 个节点停止").arg(m_results.size()), last.error));
    } else {
        ui->status->setText(QStringLiteral("流程完成，共执行 %1 个节点").arg(m_results.size()));
    }
    showMat(ui->resultPreview, last.image);
    updateSelectedNode(ui->nodeList->currentRow());
}

void PipelinePage::updateSelectedNode(int row)
{
    if (row < 0 || row >= m_pipeline.size()) {
        return;
    }
    ui->codeView->setPlainText(m_pipeline.nodeAt(row).codeSnippet());

    if (!m_inputImage.empty() && m_results.size() != m_pipeline.size()) {
        m_results = m_pipeline.execute(m_inputImage);
        if (!m_results.isEmpty()) {
            showMat(ui->resultPreview, m_results.last().image);
        }
    }
    if (row < m_results.size() && !m_results.at(row).image.empty()) {
        showMat(ui->nodePreview, m_results.at(row).image);
    } else {
        ui->nodePreview->clear();
    }
}

void PipelinePage::refreshNodeList()
{
    ui->nodeList->clear();
    for (int i = 0; i < m_pipeline.size(); ++i) {
        ui->nodeList->addItem(QStringLiteral("%1. %2")
                                  .arg(i + 1)
                                  .arg(m_pipeline.nodeAt(i).name()));
    }
}

void PipelinePage::showMat(QLabel *target, const cv::Mat &image)
{
    const QImage converted = matToImage(image);
    if (converted.isNull()) {
        target->clear();
        return;
    }
    target->setPixmap(QPixmap::fromImage(converted).scaled(
        target->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QImage PipelinePage::matToImage(const cv::Mat &image)
{
    if (image.empty()) {
        return QImage();
    }
    if (image.channels() == 1) {
        return QImage(image.data, image.cols, image.rows, image.step,
                      QImage::Format_Grayscale8).copy();
    }
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step,
                  QImage::Format_RGB888).copy();
}
