#include "pcafacerecognizer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

namespace {
void fail(const QString &message)
{
    throw std::runtime_error(message.toUtf8().constData());
}

cv::Mat rowVector(const cv::Mat &face)
{
    if (face.type() != CV_8UC1 || face.rows != PcaFaceRecognizer::FaceSize
        || face.cols != PcaFaceRecognizer::FaceSize) {
        fail(QStringLiteral("PCA 输入必须是经过预处理的 100×100 灰度人脸。"));
    }
    cv::Mat row;
    face.clone().reshape(1, 1).convertTo(row, CV_32F);
    return row;
}
}

cv::Mat PcaFaceRecognizer::readImage(const QString &path)
{
    // QFile + imdecode also handles Chinese paths on Windows.
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("无法读取图像：%1").arg(path));
    }
    const QByteArray bytes = file.readAll();
    const std::vector<uchar> buffer(bytes.begin(), bytes.end());
    cv::Mat image;
    if (!buffer.empty()) {
        image = cv::imdecode(buffer, cv::IMREAD_GRAYSCALE);
    }
    if (image.empty()) {
        fail(QStringLiteral("图像损坏或格式不支持：%1").arg(path));
    }
    return image;
}

cv::Mat PcaFaceRecognizer::preprocess(const cv::Mat &image,
                                     const FacePreprocessing &options)
{
    if (image.empty() || image.depth() != CV_8U) {
        fail(QStringLiteral("需要非空的 8 位图像。"));
    }
    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image;
    } else if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else if (image.channels() == 4) {
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    } else {
        fail(QStringLiteral("不支持此图像通道数。"));
    }
    cv::Mat crop = gray;
    if (options.detectFace) {
        // Load from memory so classifier paths can also contain non-ASCII text.
        QFile file(options.cascadePath);
        if (!file.open(QIODevice::ReadOnly)) {
            fail(QStringLiteral("无法读取 Haar XML；请先选择人脸检测分类器。"));
        }
        const QByteArray xml = file.readAll();
        cv::FileStorage storage(xml.toStdString(), cv::FileStorage::READ | cv::FileStorage::MEMORY);
        cv::CascadeClassifier classifier;
        if (!storage.isOpened() || !classifier.read(storage.getFirstTopLevelNode())) {
            fail(QStringLiteral("Haar XML 分类器格式不正确。"));
        }
        cv::Mat detectionImage;
        cv::equalizeHist(gray, detectionImage);
        std::vector<cv::Rect> faces;
        classifier.detectMultiScale(detectionImage, faces, 1.1, 5, 0, cv::Size(40, 40));
        if (faces.size() != 1) {
            fail(QStringLiteral("需要恰好一张正面人脸，当前检测到 %1 张；请换图或使用已裁剪人脸模式。")
                 .arg(static_cast<int>(faces.size())));
        }
        crop = gray(faces.front());
    }
    cv::Mat resized, normalized;
    cv::resize(crop, resized, cv::Size(FaceSize, FaceSize), 0, 0, cv::INTER_AREA);
    // Histogram normalization required by the assignment: global equalization.
    cv::equalizeHist(resized, normalized);
    if (cv::norm(normalized, cv::NORM_INF) == 0 || cv::countNonZero(normalized != normalized.at<uchar>(0, 0)) == 0) {
        fail(QStringLiteral("图像没有有效灰度变化，请选择清晰的人脸图像。"));
    }
    return normalized;
}

QStringList PcaFaceRecognizer::people(const QVector<FaceSample> &samples)
{
    QSet<QString> names;
    for (const FaceSample &sample : samples) {
        names.insert(sample.person);
    }
    QStringList result = names.values();
    result.sort();
    return result;
}

QVector<FaceSample> PcaFaceRecognizer::readPeopleDirectory(
    const QString &root, const FacePreprocessing &options, QStringList *warnings)
{
    if (!QFileInfo(root).isDir()) {
        fail(QStringLiteral("人员库目录不存在：%1").arg(root));
    }
    QVector<FaceSample> result;
    const QFileInfoList directories = QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                               QDir::Name);
    const QStringList extensions = QStringList() << "png" << "jpg" << "jpeg" << "bmp"
                                                 << "pgm" << "tif" << "tiff" << "webp";
    for (const QFileInfo &directory : directories) {
        const QFileInfoList files = QDir(directory.absoluteFilePath()).entryInfoList(QDir::Files,
                                                                                    QDir::Name);
        int count = 0;
        for (const QFileInfo &file : files) {
            if (!extensions.contains(file.suffix().toLower())) {
                continue;
            }
            try {
                FaceSample sample;
                sample.person = directory.fileName();
                sample.path = file.canonicalFilePath();
                sample.face = preprocess(readImage(sample.path), options);
                result.append(sample);
                ++count;
            } catch (const std::exception &error) {
                if (warnings) {
                    warnings->append(QStringLiteral("%1：%2")
                                     .arg(file.absoluteFilePath(), QString::fromUtf8(error.what())));
                }
            }
        }
        if (count == 0 && warnings) {
            warnings->append(QStringLiteral("人员 %1 没有可用样本，未计入人数。").arg(directory.fileName()));
        }
    }
    return result;
}

QString PcaFaceRecognizer::defaultCascadePath()
{
    const QString file = QStringLiteral("haarcascade_frontalface_default.xml");
    const QStringList candidates = QStringList()
        << QCoreApplication::applicationDirPath() + "/resources/" + file
        << QDir::currentPath() + "/resources/" + file
        << "/usr/local/share/opencv4/haarcascades/" + file
        << "/usr/share/opencv4/haarcascades/" + file
        << "/usr/share/opencv/haarcascades/" + file;
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return QString();
}

void PcaFaceRecognizer::train(const QVector<FaceSample> &samples, int components)
{
    if (people(samples).size() < MinimumPeople) {
        fail(QStringLiteral("课程要求至少 %1 位不同人员，当前只有 %2 位有效人员。")
             .arg(MinimumPeople).arg(people(samples).size()));
    }
    if (components < 0) {
        fail(QStringLiteral("PCA 主成分数不能为负。"));
    }
    cv::Mat data;
    for (const FaceSample &sample : samples) {
        if (sample.person.trimmed().isEmpty()) {
            fail(QStringLiteral("人员姓名不能为空。"));
        }
        data.push_back(rowVector(sample.face));
    }
    // Train into temporaries: a failed training never leaves a partial model.
    cv::PCA pca;
    const int maximum = std::min(data.rows - 1, data.cols);
    pca(data, cv::Mat(), cv::PCA::DATA_AS_ROW,
        components == 0 ? maximum : std::min(components, maximum));
    cv::Mat centered = data - cv::repeat(pca.mean, data.rows, 1);
    const double totalVariance = cv::norm(centered, cv::NORM_L2SQR) / data.rows;
    if (totalVariance <= 1e-6 || pca.eigenvectors.empty()) {
        fail(QStringLiteral("样本完全相同，无法提取 PCA 特征；请录入不同人员的人脸。"));
    }
    if (components == 0) {
        // Select explicitly: some OpenCV versions exclude the component that
        // crosses the retained-variance target in the double overload of PCA.
        double cumulative = 0.0;
        int retained = 0;
        while (retained < pca.eigenvalues.rows && cumulative < 0.95 * totalVariance) {
            cumulative += std::max(0.0f, pca.eigenvalues.at<float>(retained));
            ++retained;
        }
        pca.eigenvalues = pca.eigenvalues.rowRange(0, retained).clone();
        pca.eigenvectors = pca.eigenvectors.rowRange(0, retained).clone();
    }
    cv::Mat projections;
    pca.project(data, projections);
    m_pca = pca;
    m_projections = projections;
    m_samples = samples;
    m_retainedVariance = std::min(1.0, cv::sum(pca.eigenvalues)[0] / totalVariance);
}

FacePrediction PcaFaceRecognizer::predict(const cv::Mat &face, double threshold) const
{
    if (!isTrained()) {
        fail(QStringLiteral("请先训练 PCA 模型。"));
    }
    if (!std::isfinite(threshold) || threshold <= 0.0) {
        fail(QStringLiteral("识别阈值必须大于 0。"));
    }
    FacePrediction result;
    m_pca.project(rowVector(face), result.projection);
    QMap<QString, FaceMatch> bestPerPerson;
    for (int i = 0; i < m_projections.rows; ++i) {
        FaceMatch match;
        match.person = m_samples.at(i).person;
        match.sampleIndex = i;
        match.distance = cv::norm(result.projection, m_projections.row(i), cv::NORM_L2)
            / std::sqrt(static_cast<double>(FaceSize * FaceSize));
        if (!bestPerPerson.contains(match.person)
            || match.distance < bestPerPerson.value(match.person).distance) {
            bestPerPerson.insert(match.person, match);
        }
    }
    for (auto it = bestPerPerson.constBegin(); it != bestPerPerson.constEnd(); ++it) {
        result.ranking.append(it.value());
    }
    std::sort(result.ranking.begin(), result.ranking.end(),
              [](const FaceMatch &a, const FaceMatch &b) { return a.distance < b.distance; });
    result.accepted = !result.ranking.isEmpty() && result.ranking.first().distance <= threshold;
    cv::Mat reconstructed = m_pca.backProject(result.projection).reshape(1, FaceSize);
    reconstructed.convertTo(result.reconstruction, CV_8U);
    return result;
}

cv::Mat PcaFaceRecognizer::meanFace() const
{
    cv::Mat image;
    if (isTrained()) {
        m_pca.mean.reshape(1, FaceSize).convertTo(image, CV_8U);
    }
    return image;
}

cv::Mat PcaFaceRecognizer::eigenFace(int index) const
{
    cv::Mat image;
    if (isTrained() && index >= 0 && index < components()) {
        cv::normalize(m_pca.eigenvectors.row(index).reshape(1, FaceSize), image,
                      0, 255, cv::NORM_MINMAX, CV_8U);
    }
    return image;
}
