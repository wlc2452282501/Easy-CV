#include "welcomepage.h"

#include "ui_welcomepage.h"

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::WelcomePage)
{
    ui->setupUi(this);

    // Keep visual styling here so the layout remains editable in Qt Designer.
    setStyleSheet(
        QStringLiteral("QWidget { background: #f5f7fa; color: #1f2937; }"
                       "QLabel#title { font-size: 30px; font-weight: 600; }"
                       "QLabel#subtitle { font-size: 16px; color: #52606d; }"
                       "QLabel#sectionLabel { font-size: 18px; font-weight: 600; }"
                       "QLabel#footer { color: #7b8794; }"
                       "QLabel#heroImage { background: #dbeafe; border-radius: 8px; }"
                       "QPushButton { background: white; border: 1px solid #cbd5e1;"
                       " border-radius: 6px; padding: 16px; text-align: left; font-size: 15px; }"
                       "QPushButton:hover { border-color: #2563eb; background: #eff6ff; }"
                       "QPushButton:pressed { background: #dbeafe; }"));

    connect(ui->pipelineButton, &QPushButton::clicked, this, [this]() {
        emit moduleRequested(QStringLiteral("pipeline"));
    });
    connect(ui->yoloButton, &QPushButton::clicked, this, [this]() {
        emit moduleRequested(QStringLiteral("yolov8"));
    });
    connect(ui->environmentButton, &QPushButton::clicked, this, [this]() {
        emit moduleRequested(QStringLiteral("environment"));
    });
}

WelcomePage::~WelcomePage()
{
    delete ui;
}
