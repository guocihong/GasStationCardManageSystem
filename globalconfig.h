#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include "CommonSetting.h"

class GlobalConfig
{
public:
    enum ConnectType{
        ShortConnection = 0,//短连接
        LongConnection = 1//长连接
    };

    static QString ServerIP;                  //服务器IP
    static QString ServerPort;                //服务器端口
    static enum ConnectType TcpConnectType;//tcp连接类型：短连接,长连接

    static quint8  SwipCardIntervalTime;      //刷卡间隔时间      单位是秒
    static quint8  HeartIntervalTime;         //检测网络联通间隔时间，单位是秒
    static quint8  DeviceHeartIntervalTime;   //心跳间隔时间		单位是分钟
    static QString DeviceID;                  //设备编号

    static QString LocalHostIP;
    static QString Mask;
    static QString Gateway;
    static QString MAC;

    static QString ConfigFileName;

    static QString Name;
    static QString Sex;
    static QString Nation;
    static QString Birthday;
    static QString Address;
    static QString IDCode;
    static QString Department;
    static QString StartDate;
    static QString EndDate;

    static void init();
};

#endif // GLOBALCONFIG_H
