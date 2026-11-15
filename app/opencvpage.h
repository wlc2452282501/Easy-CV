#ifndef OPENCVPAGE_H
#define OPENCVPAGE_H

#include <QWidget>
namespace Ui { class OpenCvPage; }

class OpenCvPage : public QWidget
{
    Q_OBJECT
public:
    explicit OpenCvPage(QWidget *parent = 0);
    ~OpenCvPage();
signals:
    void backRequested();
private:
    Ui::OpenCvPage *ui;
};
#endif
