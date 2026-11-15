QT += widgets concurrent testlib
CONFIG += console testcase c++11
greaterThan(QT_MAJOR_VERSION, 5): CONFIG += c++17
TEMPLATE = app
TARGET = face_recognition_tests
INCLUDEPATH += $$PWD/..

SOURCES += face_recognition_tests.cpp \
    ../core/pcafacerecognizer.cpp \
    ../core/visionnode.cpp ../core/pipeline.cpp ../core/noderegistry.cpp \
    ../app/facerecognitionpage.cpp ../app/opencvpage.cpp \
    ../app/pipelinepage.cpp ../app/mainwindow.cpp ../app/welcomepage.cpp \
    ../app/moduleplaceholderpage.cpp ../app/yoloworkbenchpage.cpp
HEADERS += ../core/pcafacerecognizer.h \
    ../app/facerecognitionpage.h ../app/opencvpage.h \
    ../app/pipelinepage.h ../app/mainwindow.h ../app/welcomepage.h \
    ../app/moduleplaceholderpage.h ../app/yoloworkbenchpage.h
FORMS += ../ui/facerecognitionpage.ui ../ui/opencvpage.ui ../ui/pipelinepage.ui \
    ../ui/mainwindow.ui ../ui/welcomepage.ui ../ui/moduleplaceholderpage.ui \
    ../ui/yoloworkbenchpage.ui
RESOURCES += ../resources/resources.qrc

unix {
    INCLUDEPATH += /usr/local/include/opencv4
    LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_objdetect
    QMAKE_LFLAGS += -Wl,-rpath,/usr/local/lib
}
# Optional native Windows verification against an official OpenCV build.
win32 {
    isEmpty(OPENCV_PREFIX): error("Pass OPENCV_PREFIX=<OpenCV build directory>")
    isEmpty(OPENCV_WORLD_VERSION): error("Pass OPENCV_WORLD_VERSION=<version suffix, e.g. 4110>")
    INCLUDEPATH += $$OPENCV_PREFIX/include
    LIBS += -L$$OPENCV_PREFIX/x64/vc16/lib -lopencv_world$$OPENCV_WORLD_VERSION
}
