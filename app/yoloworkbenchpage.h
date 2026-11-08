#ifndef YOLOWORKBENCHPAGE_H
#define YOLOWORKBENCHPAGE_H

#include <QProcess>
#include <QString>
#include <QStringList>
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QLabel;
class QSpinBox;
class QTextEdit;
namespace Ui { class YoloWorkbenchPage; }

/// Designer-built YOLOv8 control page; execution remains in external Python.
class YoloWorkbenchPage : public QWidget
{
    Q_OBJECT

public:
    explicit YoloWorkbenchPage(QWidget *parent = 0);
    ~YoloWorkbenchPage();

signals:
    void backRequested();

private slots:
    void prepareEnvironment();
    void inspectEnvironment();
    void runAction();
    void chooseModel();
    void chooseData();
    void chooseSource();
    void updateActionForm(int index);
    void readStandardOutput();
    void readStandardError();
    void processError(QProcess::ProcessError error);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString scriptPath(const QString &fileName) const;
    QString pythonExecutable() const;
    void startPythonTask(const QStringList &arguments);
    void appendLog(const QString &text);
    void setBusy(bool busy);

    Ui::YoloWorkbenchPage *ui;
    QProcess *m_process;
};

#endif // YOLOWORKBENCHPAGE_H
