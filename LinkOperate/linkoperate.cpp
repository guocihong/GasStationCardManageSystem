#include "linkoperate.h"

LinkOperate::LinkOperate(QObject *parent) :
    QObject(parent)
{
    OpenDevice();

    BuzzerTimer = new QTimer(this);
    BuzzerTimer->setInterval(2000);
    connect(BuzzerTimer,SIGNAL(timeout()),this,SLOT(slotBuzzerOff()));

    //检测防拆是否被打开
    DoorTimer = new QTimer(this);
    DoorTimer->setInterval(500);
    connect(DoorTimer,SIGNAL(timeout()),this,SLOT(slotReadDoorState()));
}

void LinkOperate::OpenDevice()
{
    BuzzerFd = open("/dev/s5pv210_buzzer",O_RDWR);
    if(BuzzerFd == -1){
        qDebug() << "/dev/s5pv210_buzzer failed";
    } else {
        qDebug() << "/dev/s5pv210_buzzer succeed";
    }

    LedFd = open("/dev/s5pv210_led",O_RDWR);
    if(LedFd == -1){
        qDebug() << "/dev/s5pv210_led failed";
    } else {
        qDebug() << "/dev/s5pv210_led succeed";
    }

    DoorFd = open("/dev/s5pv210_door",O_RDWR);
    if(DoorFd == -1){
        qDebug() << "/dev/s5pv210_door failed";
    } else {
        qDebug() << "/dev/s5pv210_door succeed";
    }
}

void LinkOperate::BuzzerOn()
{
    ioctl(BuzzerFd,1,3000);
}

void LinkOperate::BuzzerOn2Times()
{
    ioctl(BuzzerFd,1,3000);
    CommonSetting::Sleep(100);
    ioctl(BuzzerFd,0);
    CommonSetting::Sleep(100);

    ioctl(BuzzerFd,1,3000);
    CommonSetting::Sleep(100);
    ioctl(BuzzerFd,0);
}

void LinkOperate::BuzzerOn5Times()
{
    for(int i = 0; i < 5; i++){
        ioctl(BuzzerFd,1,3000);
        CommonSetting::Sleep(100);
        ioctl(BuzzerFd,0);
        CommonSetting::Sleep(100);
    }
}

void LinkOperate::AllowAddOil(int type)
{
    //ST1亮绿灯
    ioctl(LedFd,Led1_Green_On);

    //ST2亮绿灯-单位加油,不亮灯-个人加油
    if(type == 1){//个人加油
        ioctl(LedFd,Led2_Green_Off);
        ioctl(LedFd,Led2_Red_Off);
    }else if(type == 2){//单位加油
        ioctl(LedFd,Led2_Green_On);
    }

    //蜂鸣器2声
    DoorTimer->stop();
    BuzzerOn2Times();
    DoorTimer->start();
}

void LinkOperate::RejectAddOil()
{
    //ST1亮红灯
    ioctl(LedFd,Led1_Red_On);

    //ST2亮红灯
    ioctl(LedFd,Led2_Red_On);

    //蜂鸣器5声
    DoorTimer->stop();
    BuzzerOn5Times();
    DoorTimer->start();
}

void LinkOperate::InValidUser()
{
    //ST1亮红灯
    ioctl(LedFd,Led1_Red_On);

    //ST2不亮灯
    ioctl(LedFd,Led2_Red_Off);
    ioctl(LedFd,Led2_Green_Off);

    //蜂鸣器5声
    DoorTimer->stop();
    BuzzerOn5Times();
    DoorTimer->start();
}

void LinkOperate::Restore()
{
    ioctl(LedFd,Led1_Green_Off);
    ioctl(LedFd,Led2_Green_Off);

    ioctl(LedFd,Led1_Red_Off);
    ioctl(LedFd,Led2_Red_Off);
}

void LinkOperate::ST3_GreenON()
{
    ioctl(LedFd,Led3_Green_On);
}

void LinkOperate::ST3_RedON()
{
    ioctl(LedFd,Led3_Red_On);
}

void LinkOperate::ST3_OFF()
{
    ioctl(LedFd,Led3_Green_Off);
    ioctl(LedFd,Led3_Red_Off);
}

void LinkOperate::slotReadDoorState()
{
    char CurrentDoorState;
    static char PreDoorState = 1;//1不报警，0报警

    read(DoorFd,&CurrentDoorState,1);
    if((PreDoorState == 0) && (CurrentDoorState == 1)){
        this->slotBuzzerOff();
        PreDoorState = 1;
        emit signalDoorStatusChanged("0");//防拆闭合，不报警
    }else if((PreDoorState == 1) && (CurrentDoorState == 0)){
        this->BuzzerOn();
        PreDoorState = 0;
        emit signalDoorStatusChanged("1");//防拆打开，报警
    }
}

void LinkOperate::slotBuzzerOff()
{
    BuzzerTimer->stop();
    ioctl(BuzzerFd,0);
}
