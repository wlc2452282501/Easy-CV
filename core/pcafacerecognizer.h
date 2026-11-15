#ifndef PCAFACERECOGNIZER_H
#define PCAFACERECOGNIZER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <opencv2/core.hpp>

// One row of the gallery is one already normalized, single-person face.
struct FaceSample
{
    QString person;
    QString path;
    cv::Mat face;
};

struct FacePreprocessing
{
    bool detectFace = false; // false: the input is an explicitly cropped portrait
    QString cascadePath;
};

struct FaceMatch
{
    QString person;
    int sampleIndex = -1;
    double distance = 0.0;
};

struct FacePrediction
{
    QVector<FaceMatch> ranking; // one nearest sample per identity, ascending distance
    bool accepted = false;
    cv::Mat projection;
    cv::Mat reconstruction;
};

// Eigenfaces implemented with cv::PCA, with the same preprocessing for all inputs.
// Distances are Euclidean distances in PCA space divided by sqrt(pixel count).
class PcaFaceRecognizer
{
public:
    static const int FaceSize = 100;
    static const int MinimumPeople = 10;

    static cv::Mat readImage(const QString &path);
    static cv::Mat preprocess(const cv::Mat &image, const FacePreprocessing &options);
    static QVector<FaceSample> readPeopleDirectory(const QString &root,
                                                  const FacePreprocessing &options,
                                                  QStringList *warnings);
    static QStringList people(const QVector<FaceSample> &samples);
    static QString defaultCascadePath();

    // Zero components retains 95% of variance. Explicit counts are capped at N-1.
    void train(const QVector<FaceSample> &samples, int components);
    FacePrediction predict(const cv::Mat &normalizedFace, double threshold) const;
    bool isTrained() const { return !m_pca.eigenvectors.empty(); }
    int components() const { return m_pca.eigenvectors.rows; }
    double retainedVariance() const { return m_retainedVariance; }
    const QVector<FaceSample> &samples() const { return m_samples; }
    cv::Mat meanFace() const;
    cv::Mat eigenFace(int index) const;

private:
    cv::PCA m_pca;
    cv::Mat m_projections;
    QVector<FaceSample> m_samples;
    double m_retainedVariance = 0.0;
};

#endif
