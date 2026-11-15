#ifndef FACERECOGNITIONPAGE_H
#define FACERECOGNITIONPAGE_H

#include <QFutureWatcher>
#include <QWidget>
#include <functional>
#include "../core/pcafacerecognizer.h"

class QLabel;
namespace Ui { class FaceRecognitionPage; }

struct FaceEvaluationRow
{
    QString path;
    QString truth;
    QString prediction;
    double distance = 0.0;
    bool correct = false;
};

// Workers operate on snapshots only; widgets and live state stay on the GUI thread.
struct FaceTaskResult
{
    QVector<FaceSample> samples;
    PcaFaceRecognizer model;
    QVector<FaceEvaluationRow> evaluation;
    QStringList warnings;
    QString summary;
    QString error;
};

class FaceRecognitionPage : public QWidget
{
    Q_OBJECT
public:
    explicit FaceRecognitionPage(QWidget *parent = 0);
    ~FaceRecognitionPage();
signals:
    void backRequested();
private:
    FacePreprocessing preprocessing() const;
    void importDirectory();
    void addPerson();
    void removePerson();
    void train();
    void loadQuery();
    void recognize();
    void evaluate();
    void exportEvaluation();
    void preprocessingChanged();
    void invalidateModel();
    void clearEvaluation();
    void clearPrediction();
    void refreshGallery();
    void updateSamplePreview();
    void updateEigenPreview();
    void setBusy(bool busy);
    void runTask(const QString &message, const std::function<FaceTaskResult()> &task,
                 const std::function<void(const FaceTaskResult &)> &apply);
    void showImage(QLabel *label, const cv::Mat &image);
    void log(const QString &text);

    Ui::FaceRecognitionPage *ui;
    QVector<FaceSample> m_samples;
    PcaFaceRecognizer m_model;
    cv::Mat m_query;
    QVector<FaceEvaluationRow> m_evaluation;
    QFutureWatcher<FaceTaskResult> m_watcher;
    bool m_busy = false;
};
#endif
