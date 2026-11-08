#include "moduleplaceholderpage.h"

#include "ui_moduleplaceholderpage.h"

ModulePlaceholderPage::ModulePlaceholderPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ModulePlaceholderPage)
{
    ui->setupUi(this);
    connect(ui->backButton, &QPushButton::clicked, this,
            &ModulePlaceholderPage::backRequested);

    setStyleSheet(
        QStringLiteral("QWidget { background: #f5f7fa; color: #1f2937; }"
                       "QLabel#moduleTitle { font-size: 28px; font-weight: 600; }"
                       "QLabel#moduleDescription { font-size: 16px; color: #52606d; }"
                       "QLabel#moduleStatus { padding: 18px; background: white;"
                       " border: 1px solid #d9e2ec; border-radius: 6px; }"
                       "QPushButton { padding: 10px 18px; border: 1px solid #94a3b8;"
                       " border-radius: 5px; background: white; }"
                       "QPushButton:hover { background: #eff6ff; border-color: #2563eb; }"));
}

ModulePlaceholderPage::~ModulePlaceholderPage()
{
    delete ui;
}

void ModulePlaceholderPage::setModule(const QString &moduleId)
{
    if (moduleId == QStringLiteral("pipeline")) {
        ui->moduleTitle->setText(QStringLiteral("OpenCV 流程编辑器"));
        ui->moduleDescription->setText(QStringLiteral(
            "后续将提供可连接节点、算子参数、节点级预览和完整流程预览。"));
    } else if (moduleId == QStringLiteral("yolov8")) {
        ui->moduleTitle->setText(QStringLiteral("YOLOv8 工作台"));
        ui->moduleDescription->setText(QStringLiteral(
            "后续将接入训练、验证、实时推理、ONNX 导出和量化流程。"));
    } else {
        ui->moduleTitle->setText(QStringLiteral("运行环境"));
        ui->moduleDescription->setText(QStringLiteral(
            "后续将展示 Qt 版本、OpenCV 模块、编译器和运行时检查结果。"));
    }
}
