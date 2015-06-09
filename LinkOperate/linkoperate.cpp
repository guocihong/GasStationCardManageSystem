#include "linkoperate.h"

LinkOperate::LinkOperate(QObject *parent) :
    QObject(parent)
{
    OpenDevice();

    BuzzerTimer = new QTimer(this);
    BuzzerTimer->setInterval(5000);
    connect(BuzzerTimer,SIGNAL(timeout()),this,SLOT(slotBuzzerOff()));

    //检测防拆是否被打开
    DoorTimer = new QTimer(this);
    DoorTimer->setInterval(1000);
    connect(DoorTimer,SIGNAL(timeout()),this,SLOT(slotDoorState()));
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

    DoorFd = open("/dev/s5pv210_door",O_RDWR);
    if(DoorFd == -1){
        qDebug() << "/dev/s5pv210_door failed.Exit the Program";
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
    DoorTimer->stop();
    BuzzerOn2Times();
    DoorTimer->start();
}

void LinkOperate::ValidUserDisable()
{
    ioctl(LedFd,Led2_Red_Off);
}

void LinkOperate::InValidUserEnable()
{
    ioctl(LedFd,Led2_Green_On);
    DoorTimer->stop();
    BuzzerOn5Times();
    DoorTimer->start();
}

void LinkOperate::InValidUserDisable()
{
    ioctl(LedFd,Led2_Green_Off);
}

void LinkOperate::slotDoorState()
{
    char CurrentDoorState;
    static char PreDoorState = 1;//1不报警，0报警

    read(DoorFd,&CurrentDoorState,1);
    if((PreDoorState == 0) && (CurrentDoorState == 1)){
        this->slotBuzzerOff();
        PreDoorState = 1;
    }else if((PreDoorState == 1) && (CurrentDoorState == 0)){
        this->BuzzerOn();
        PreDoorState = 0;
    }
}
