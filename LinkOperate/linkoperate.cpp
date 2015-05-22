#include "linkoperate.h"

LinkOperate::LinkOperate(QObject *parent) :
    QObject(parent)
{
    OpenDevice();

    BuzzerTimer = new QTimer(this);
    BuzzerTimer->setInterval(5000);
    connect(BuzzerTimer,SIGNAL(timeout()),this,SLOT(slotBuzzerOff()));
}

void LinkOperate::OpenDevice()
{
    BuzzerFd = open("/dev/s5pv210_buzzer",O_RDWR);
    if(BuzzerFd == -1){
        qDebug() << "/dev/s5pv210_buzzer failed.Exit the Program";
        exit(0);
    }

    LedFd = open("/dev/s5pv210_led",O_RDWR);
    if(LedFd == -1){
        qDebug() << "/dev/s5pv210_led failed.Exit the Program";
        exit(0);
    }
}

void LinkOperate::BuzzerOn()
{
    ioctl(BuzzerFd,1,3000);
}

void LinkOperate::slotBuzzerOff()
{
    BuzzerTimer->stop();
    ioctl(BuzzerFd,0);
}

void LinkOperate::BuzzerOn1Times()
{
    ioctl(BuzzerFd,1,3000);
    CommonSetting::Sleep(200);
    ioctl(BuzzerFd,0);
}

void LinkOperate::BuzzerOn2Times()
{
    ioctl(BuzzerFd,1,3000);
    CommonSetting::Sleep(1000);
    ioctl(BuzzerFd,0);
    CommonSetting::Sleep(1000);

    ioctl(BuzzerFd,1,3000);
    CommonSetting::Sleep(1000);
    ioctl(BuzzerFd,0);
}

void LinkOperate::BuzzerOn5Times()
{
    for(int i = 0; i < 5; i++){
        ioctl(BuzzerFd,1,3000);
        CommonSetting::Sleep(1000);
        ioctl(BuzzerFd,0);
        CommonSetting::Sleep(1000);
    }
}

void LinkOperate::ValidUserEnable()
{
    ioctl(LedFd,Led2_Red_On);
    BuzzerOn2Times();
}

void LinkOperate::ValidUserDisable()
{
    ioctl(LedFd,Led2_Red_Off);
}

void LinkOperate::InValidUserEnable()
{
    ioctl(LedFd,Led2_Green_On);
    BuzzerOn5Times();
}

void LinkOperate::InValidUserDisable()
{
    ioctl(LedFd,Led2_Green_Off);
}
