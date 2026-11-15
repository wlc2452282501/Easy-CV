#include "opencvpage.h"
#include "ui_opencvpage.h"

OpenCvPage::OpenCvPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::OpenCvPage)
{
    ui->setupUi(this);
    connect(ui->facePage, &FaceRecognitionPage::backRequested, this, &OpenCvPage::backRequested);
    connect(ui->pipelinePage, &PipelinePage::backRequested, this, &OpenCvPage::backRequested);
}

OpenCvPage::~OpenCvPage()
{
    delete ui;
}
