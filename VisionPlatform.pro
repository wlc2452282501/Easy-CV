QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = VisionPlatform

INCLUDEPATH += $$PWD
INCLUDEPATH += /usr/local/include/opencv4

SOURCES += \
    main.cpp \
    app/mainwindow.cpp \
    app/welcomepage.cpp \
    app/moduleplaceholderpage.cpp \
    app/pipelinepage.cpp \
    app/yoloworkbenchpage.cpp \
    core/pipeline.cpp \
    core/noderegistry.cpp \
    core/visionnode.cpp

HEADERS += \
    app/mainwindow.h \
    app/welcomepage.h \
    app/moduleplaceholderpage.h \
    app/pipelinepage.h \
    app/yoloworkbenchpage.h \
    core/pipeline.h \
    core/noderegistry.h \
    core/visionnode.h

FORMS += \
    ui/mainwindow.ui \
    ui/welcomepage.ui \
    ui/moduleplaceholderpage.ui \
    ui/pipelinepage.ui \
    ui/yoloworkbenchpage.ui

RESOURCES += resources/resources.qrc

# OpenCV modules confirmed by scripts/check_opencv.sh in the target Linux VM.
LIBS += -L/usr/local/lib \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_imgcodecs \
        -lopencv_highgui \
        -lopencv_face \
        -lopencv_objdetect \
        -lopencv_videoio \
        -lopencv_dnn

unix:QMAKE_LFLAGS += -Wl,-rpath,/usr/local/lib
