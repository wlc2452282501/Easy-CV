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
    // Pipeline and YOLOv8 are real pages and are routed by MainWindow. This
    // page is only used for modules without an implemented page yet.
    if (moduleId == QStringLiteral("environment")) {
        ui->moduleTitle->setText(QStringLiteral("运行环境"));
        ui->moduleDescription->setText(QStringLiteral(
            "后续将展示 Qt 版本、OpenCV 模块、编译器和运行时检查结果。"));
    } else {
        ui->moduleTitle->setText(QStringLiteral("模块暂未实现"));
        ui->moduleDescription->setText(QStringLiteral(
            "当前模块尚未接入页面，请返回首页选择已实现的功能。"));
    }
}
