#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class WelcomePage;
class ModulePlaceholderPage;
class PipelinePage;
class YoloWorkbenchPage;
namespace Ui { class MainWindow; }

/// Application shell that switches between the Designer-built pages.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void openModule(const QString &moduleId);
    void returnToWelcome();

private:
    Ui::MainWindow *ui;
    WelcomePage *m_welcomePage;
    PipelinePage *m_pipelinePage;
    YoloWorkbenchPage *m_yoloPage;
    ModulePlaceholderPage *m_modulePage;
};

#endif // MAINWINDOW_H
