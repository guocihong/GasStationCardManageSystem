#ifndef OPERATECAMERA_H
#define OPERATECAMERA_H

#include <QObject>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <QDebug>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>


#define VideoDeviceName "/dev/video0"

class OperateCamera : public QObject
{
    Q_OBJECT
public:
    explicit OperateCamera(QObject *parent = 0);
    ~OperateCamera();

    //摄像头相关操作
    void OpenDevice();
    void QueryDeviceCapability();
    void SetPictureFormat();
    void RequestBuffer();
    void StartCamera(QString CardID, QString TriggerTime);

public slots:

public:
    int Width;
    int Height;
    int VideoFd;
    struct v4l2_capability cap;
    struct v4l2_input input;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt;
    struct v4l2_streamparm StreamParam;
    struct v4l2_requestbuffers req;
    struct buffer *buffers;
    struct v4l2_buffer buf;
    bool CouldSetFrameRate;
    bool Valid;
    int CAPTURE_BUFFER_NUMBER;

};

//定义一个结构体来映射每个缓冲帧
struct buffer
{
    void *start;
    unsigned int length;

};
#endif // OPERATECAMERA_H
