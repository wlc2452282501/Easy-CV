#ifndef PIPELINEPAGE_H
#define PIPELINEPAGE_H

#include <QVector>
#include <QImage>
#include <QWidget>

#include <opencv2/core.hpp>

#include "../core/pipeline.h"

class QComboBox;
class QLabel;
class QListWidget;
class QTextEdit;
namespace Ui { class PipelinePage; }

/// OpenCV node pipeline page; layout and widget names live in pipelinepage.ui.
class PipelinePage : public QWidget
{
    Q_OBJECT

public:
    explicit PipelinePage(QWidget *parent = 0);
    ~PipelinePage();

signals:
    void backRequested();

private slots:
    void loadImage();
    void addNode();
    void removeNode();
    void runPipeline();
    void updateSelectedNode(int row);

private:
    void createDemoImage();
    void refreshNodeList();
    void showMat(QLabel *target, const cv::Mat &image);
    static QImage matToImage(const cv::Mat &image);

    Ui::PipelinePage *ui;
    Pipeline m_pipeline;
    cv::Mat m_inputImage;
    QVector<Pipeline::NodeResult> m_results;
};

#endif // PIPELINEPAGE_H
