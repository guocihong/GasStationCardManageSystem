#ifndef LINKOPERATE_H
#define LINKOPERATE_H

#include "CommonSetting.h"
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

#define Led0_Green_On	0
#define Led0_Red_On		1
#define Led1_Green_On	2
#define Led1_Red_On		3
#define Led2_Green_On	4
#define Led2_Red_On		5

#define Led0_Green_Off	6
#define Led0_Red_Off	7
#define Led1_Green_Off	8
#define Led1_Red_Off	9
#define Led2_Green_Off	10
#define Led2_Red_Off	11

class LinkOperate : public QObject
{
    Q_OBJECT
public:
    explicit LinkOperate(QObject *parent = 0);
    void OpenDevice();

    void BuzzerOn();
    void BuzzerOn1Times();//读身份证成功，蜂鸣器鸣叫1次
    void BuzzerOn2Times();//有效身份证,蜂鸣器鸣叫2次
    void BuzzerOn5Times();//无效身份证，蜂鸣器鸣叫5次

    void PowerGreenLedOn();
    void PowerGreenLedOff();
    void PowerRedLedOn();
    void PowerRedLedOff();

    void ValidUserEnable();
    void ValidUserDisable();

    void InValidUserEnable();
    void InValidUserDisable();

public slots:
    void slotBuzzerOff();

public:
    int BuzzerFd;//蜂鸣器
    int LedFd;//指示灯

    QTimer *BuzzerTimer;
};

#endif // LINKOPERATE_H
