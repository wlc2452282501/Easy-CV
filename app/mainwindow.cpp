#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "moduleplaceholderpage.h"
#include "pipelinepage.h"
#include "welcomepage.h"
#include "yoloworkbenchpage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_welcomePage(new WelcomePage(this)),
      m_pipelinePage(new PipelinePage(this)),
      m_yoloPage(new YoloWorkbenchPage(this)),
      m_modulePage(new ModulePlaceholderPage(this))
{
    ui->setupUi(this);

    ui->pages->addWidget(m_welcomePage);
    ui->pages->addWidget(m_pipelinePage);
    ui->pages->addWidget(m_yoloPage);
    ui->pages->addWidget(m_modulePage);
    ui->pages->setCurrentWidget(m_welcomePage);

    connect(m_welcomePage, SIGNAL(moduleRequested(QString)),
            this, SLOT(openModule(QString)));
    connect(m_modulePage, SIGNAL(backRequested()),
            this, SLOT(returnToWelcome()));
    connect(m_pipelinePage, SIGNAL(backRequested()),
            this, SLOT(returnToWelcome()));
    connect(m_yoloPage, SIGNAL(backRequested()),
            this, SLOT(returnToWelcome()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openModule(const QString &moduleId)
{
    if (moduleId == QStringLiteral("pipeline")) {
        ui->pages->setCurrentWidget(m_pipelinePage);
        return;
    }
    if (moduleId == QStringLiteral("yolov8")) {
        ui->pages->setCurrentWidget(m_yoloPage);
        return;
    }
    m_modulePage->setModule(moduleId);
    ui->pages->setCurrentWidget(m_modulePage);
}

void MainWindow::returnToWelcome()
{
    ui->pages->setCurrentWidget(m_welcomePage);
}
