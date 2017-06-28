#ifndef LINKOPERATE_H
#define LINKOPERATE_H

#include "CommonSetting.h"
#include "Tcp/tcphelper.h"
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

#define LED_IOCTL_BASE   'W'

#define Led1_Red_On      _IOWR(LED_IOCTL_BASE, 0, int)
#define Led1_Green_On    _IOWR(LED_IOCTL_BASE, 1, int)
#define Led2_Red_On      _IOWR(LED_IOCTL_BASE, 2, int)
#define Led2_Green_On    _IOWR(LED_IOCTL_BASE, 3, int)
#define Led3_Red_On      _IOWR(LED_IOCTL_BASE, 4, int)
#define Led3_Green_On    _IOWR(LED_IOCTL_BASE, 5, int)

#define Led1_Red_Off     _IOWR(LED_IOCTL_BASE, 6, int)
#define Led1_Green_Off   _IOWR(LED_IOCTL_BASE, 7, int)
#define Led2_Red_Off     _IOWR(LED_IOCTL_BASE, 8, int)
#define Led2_Green_Off   _IOWR(LED_IOCTL_BASE, 9, int)
#define Led3_Red_Off     _IOWR(LED_IOCTL_BASE, 10, int)
#define Led3_Green_Off   _IOWR(LED_IOCTL_BASE, 11, int)

class LinkOperate : public QObject
{
    Q_OBJECT
public:
    explicit LinkOperate(QObject *parent = 0);
    void OpenDevice();

    void BuzzerOn();
    void BuzzerOn2Times();//有效身份证,蜂鸣器鸣叫2次
    void BuzzerOn5Times();//无效身份证，蜂鸣器鸣叫5次

    void AllowAddOil(int type);//注册用户，允许加油
    void RejectAddOil();//注册用户，不允许加油
    void InValidUser();//未注册用户
    void Restore();//ST1,ST2都不亮灯

    void ST3_GreenON();//网络正常,ST3亮绿灯(插有网线,并且与服务器联通)
    void ST3_RedON();//网络异常,ST3亮红灯(插有网线，并且与服务器不联通)
    void ST3_OFF();//网络异常,ST3不亮灯(没有插网线)

public slots:
    void slotBuzzerOff();
    void slotReadDoorState();

signals:
    void signalDoorStatusChanged(QString DoorStatus);

public:
    int BuzzerFd;//蜂鸣器
    int LedFd;//指示灯
    int DoorFd;//防拆开关

    QTimer *BuzzerTimer;
    QTimer *DoorTimer;
};

#endif // LINKOPERATE_H
