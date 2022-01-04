# Ubuntu 18.04 Install packages
# sudo apt install \
#  libopencv-core-dev       libopencv-dev
#  libopencv-calib3d-dev    libopencv-contrib-dev \
#  libopencv-features2d-dev libopencv-flann-dev \
#  libopencv-highgui-dev    libopencv-imgproc-dev \
#  libopencv-imgcodecs-dev  libopencv-ml-dev \
#  libopencv-objdetect-dev  libopencv-photo-dev \
#  libopencv-shape-dev      libopencv-stitching-dev \
#  libopencv-superres-dev   libopencv-ts-dev \
#  libopencv-video-dev      libopencv-videoio-dev \
#  libopencv-videostab-dev  libopencv-viz-dev


TARGET = rsmc-ortho
CONFIG += c++11 console
CONFIG -= qt

SOURCES += rsmc-ortho.cpp

# INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/opencv2
INCLUDEPATH += /usr/include/boost

unix: LIBS += -L/usr/lib/ -lgdal
unix: LIBS += -L/usr/lib/x86_64-linux-gnu/ \
      -lopencv_core \
      -lopencv_imgproc \
      -lopencv_imgcodecs \
      -lboost_system \
      -lboost_filesystem

DEPENDPATH  += /usr/include
DEPENDPATH  += /usr/include/opencv2


unix: PRE_TARGETDEPS += /usr/lib/libgdal.a
unix: PRE_TARGETDEPS += /usr/lib/x86_64-linux-gnu/libopencv_core.a
unix: PRE_TARGETDEPS += /usr/lib/x86_64-linux-gnu/libboost_filesystem.a
unix: PRE_TARGETDEPS += /usr/lib/x86_64-linux-gnu/libboost_system.a
unix: PRE_TARGETDEPS += /usr/lib/x86_64-linux-gnu/libopencv_core.a
unix: PRE_TARGETDEPS += /usr/lib/x86_64-linux-gnu/libopencv_imgcodecs.a
unix: PRE_TARGETDEPS += /usr/lib/x86_64-linux-gnu/libopencv_imgproc.a

HEADERS +=

