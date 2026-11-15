#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "moduleplaceholderpage.h"
#include "opencvpage.h"
#include "welcomepage.h"
#include "yoloworkbenchpage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_welcomePage(new WelcomePage(this)),
      m_opencvPage(new OpenCvPage(this)),
      m_yoloPage(new YoloWorkbenchPage(this)),
      m_modulePage(new ModulePlaceholderPage(this))
{
    ui->setupUi(this);

    ui->pages->addWidget(m_welcomePage);
    ui->pages->addWidget(m_opencvPage);
    ui->pages->addWidget(m_yoloPage);
    ui->pages->addWidget(m_modulePage);
    ui->pages->setCurrentWidget(m_welcomePage);

    // The welcome page emits a module id; route real pages directly and
    // reserve the placeholder page for modules that are not implemented yet.
    connect(m_welcomePage, &WelcomePage::moduleRequested,
            this, &MainWindow::openModule);
    connect(m_modulePage, SIGNAL(backRequested()),
            this, SLOT(returnToWelcome()));
    connect(m_opencvPage, SIGNAL(backRequested()),
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
        ui->pages->setCurrentWidget(m_opencvPage);
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
