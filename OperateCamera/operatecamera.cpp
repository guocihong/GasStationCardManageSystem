#include "operatecamera.h"

OperateCamera::OperateCamera(QObject *parent) :
    QObject(parent)
{
    Width = 640;
    Height = 480;
    CAPTURE_BUFFER_NUMBER = 1;

    OpenDevice();
    QueryDeviceCapability();
    SetPictureFormat();
    RequestBuffer();
    StartCamera("first","cardid");
    system("rm -rf /opt/Base64_first_cardid.txt");
}

OperateCamera::~OperateCamera()
{
    for(int i = 0; i < CAPTURE_BUFFER_NUMBER; i++){
        munmap(buffers[i].start,buffers[i].length);
    }
    free(buffers);

    ::close(VideoFd);
}

void OperateCamera::OpenDevice()
{
    Valid = true;

    VideoFd = open(VideoDeviceName,O_RDWR | O_NONBLOCK);//打开摄像头
    if (VideoFd < 0){
        Valid = false;
        qDebug() << QString("Could not open %s\n").arg(VideoDeviceName);
        return;
    }
    qDebug() << VideoDeviceName << "Open Succeed" << VideoFd;
}

void OperateCamera::QueryDeviceCapability()//查询设备属性
{
    if(::ioctl(VideoFd, VIDIOC_QUERYCAP, &cap) < 0){
        Valid = false;
        qDebug() << "Could not query capability\n";
        return;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        Valid = false;
        qDebug() << "not a video capture device\n";
        return;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)){
        Valid = false;
        qDebug() << "not support streaming\n";
        return;
    }

    input.index = 0;
    if (::ioctl(VideoFd, VIDIOC_ENUMINPUT, &input) != 0){
        Valid = false;
        qDebug() << "No matching index found\n";
        return;
    }
    if (!input.name){
        Valid = false;
        qDebug() << "No matching index found\n";
        return;
    }
    if (::ioctl(VideoFd, VIDIOC_S_INPUT, &input) < 0){
        Valid = false;
        qDebug() << "VIDIOC_S_INPUT failed\n";
        return;
    }

    qDebug() << "QueryDeviceCapability succeed";
}

void OperateCamera::SetPictureFormat()
{
    //设置当前桢格式VIDIOC_S_FMT
    memset(&fmt,0,sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = Width;
    fmt.fmt.pix.height = Height;
    fmt.fmt.pix.pixelformat =  V4L2_PIX_FMT_RGB565;
    fmt.fmt.pix.sizeimage = (fmt.fmt.pix.width * fmt.fmt.pix.height * 16) / 8;
    fmt.fmt.pix.priv = 1;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if(::ioctl(VideoFd, VIDIOC_S_FMT, &fmt) < 0){
        Valid = false;
        qDebug() << "VIDIOC_S_FMT failed\n";
        return;
    }

    //查看当前格式:VIDIOC_G_FMT,
    memset(&fmt,0,sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(VideoFd, VIDIOC_G_FMT, &fmt);
    printf("current pixelformat = %c%c%c%c\n",
           (fmt.fmt.pix.pixelformat & 0xFF),
           ((fmt.fmt.pix.pixelformat >> 8) & 0xFF),
           ((fmt.fmt.pix.pixelformat >> 16) & 0xFF),
           ((fmt.fmt.pix.pixelformat >> 24) & 0xFF));
}

void OperateCamera::RequestBuffer()
{
    //申请一个拥有两个缓冲帧的缓冲区
    req.count = CAPTURE_BUFFER_NUMBER;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if(ioctl(VideoFd, VIDIOC_REQBUFS, &req) < 0){
        Valid = false;
        qDebug() << "request capture buffer failed\n";
        return;
    }
    if(int(req.count) != CAPTURE_BUFFER_NUMBER){
        Valid = false;
        qDebug() << "capture buffer number is wrong\n";
        return;
    }
    //将两个已申请到的内核缓冲帧映射到应用程序，用buffers指针记录。
    buffers = (struct buffer *)calloc(req.count,sizeof(struct buffer));
    if(buffers == NULL){
        Valid = false;
        qDebug() << "calloc request memory failed\n";
        return;
    }
    // 映射
    for(int n_buffers = 0;
        n_buffers < CAPTURE_BUFFER_NUMBER;
        ++n_buffers){
        memset(&buf,0,sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        // 查询序号为n_buffers的缓冲区，得到其起始物理地址和大小
        if(-1 == ioctl(VideoFd, VIDIOC_QUERYBUF, &buf)){
            Valid = false;
            qDebug() << "query capture buffer failed\n";
            return;
        }
        buffers[n_buffers].length = buf.length;
        // 映射内存
        buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ | PROT_WRITE
                     ,MAP_SHARED,VideoFd, buf.m.offset);
        if(MAP_FAILED == buffers[n_buffers].start){
            Valid = false;
            qDebug() << "unable to map capture buffer\n";
            return;
        }
    }

    //把两个缓冲帧放入队列
    for(int i = 0; i < CAPTURE_BUFFER_NUMBER; ++i){
        memset(&buf,0,sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if(ioctl(VideoFd, VIDIOC_QBUF, &buf) < 0){
            Valid = false;
            qDebug() << "queue capture failed\n";
            return;
        }
    }
    qDebug() << "request capture buffer succeed";
}

void OperateCamera::StartCamera(QString CardID,QString TriggerTime)
{
    //启动图像采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(VideoFd, VIDIOC_STREAMON, &type) < 0){
        Valid = false;
        qDebug() << "Could not start stream\n";
        return;
    }

    if(Valid){
        qDebug() << "StartStream OK!\n";
    }
    //等待摄像头采集到一桢数据
    for(;;){
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds);//将指定的文件描述符集清空
        FD_SET(VideoFd, &fds);//在文件描述符集合中增加一个新的文件描述符
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        int r = select(VideoFd + 1, &fds, NULL, NULL, &tv);//判断是否可读（即摄像头是否准备好），tv是定时
        if(-1 == r) {
            if(EINTR == errno)
                continue;
            qDebug() << "select error\n";
            return;
        }else if(0 == r) {
            qDebug() << "select timeout\n";
            return;
        }else{//采集到一张图片
            break;
        }
    }

    //从缓冲区取出一个缓冲帧
    memset(&buf,0,sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(ioctl(VideoFd, VIDIOC_DQBUF, &buf) < 0){
        qDebug() << "cannot fetch picture(VIDIOC_DQBUF failed)\n";
        return;
    }

    //停止图像采集
    if(ioctl(VideoFd, VIDIOC_STREAMOFF, &type) < 0){
        qDebug() << "cannot stop stream\n";
        return;
    }

    QString Base64 = QString("/opt/Base64_") + CardID + QString("_") + TriggerTime + QString(".txt");
    QImage image_rgb565((const uchar *)buffers[buf.index].start,Width,Height,QImage::Format_RGB16);

    QByteArray tempData;
    QBuffer tempBuffer(&tempData);
    image_rgb565.save(&tempBuffer,"JPG");//按照JPG解码保存数据
//    image_rgb565.save("/opt/" + CardID + ".jpg","JPG");
    QByteArray Base64Data = tempData.toBase64();

    QFile file(Base64);
    if(file.open(QFile::WriteOnly | QFile::Truncate)){
        file.write(Base64Data);
        file.close();
    }
    //将取出的缓冲帧放回缓冲区
    if(ioctl(VideoFd, VIDIOC_QBUF, &buf) < 0){
        qDebug() << "cannot VIDIOC_QBUF failed\n";
        return;
    }
}
