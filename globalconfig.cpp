#include "globalconfig.h"

QString GlobalConfig::ServerIP = QString("sgdzpic.3322.org");
QString GlobalConfig::ServerPort = QString("65001");
enum GlobalConfig::ConnectType GlobalConfig::TcpConnectType = GlobalConfig::LongConnection;

quint8  GlobalConfig::SwipCardIntervalTime = 60;
quint8  GlobalConfig::HeartIntervalTime = 3;
quint8  GlobalConfig::DeviceHeartIntervalTime = 30;
QString GlobalConfig::DeviceID = QString("SGT1TEST0001");

QString GlobalConfig::LocalHostIP = CommonSetting::GetLocalHostIP();
QString GlobalConfig::Mask = CommonSetting::GetMask();
QString GlobalConfig::Gateway = CommonSetting::GetGateway();
QString GlobalConfig::MAC = CommonSetting::ReadMacAddress();

QString GlobalConfig::ConfigFileName = "";

void GlobalConfig::init()
{
    GlobalConfig::ConfigFileName = QString("/bin/config.ini");

    bool retval = CommonSetting::FileExists(GlobalConfig::ConfigFileName);
    if (retval) {//配置文件存在
        QString str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"ServerNetwork/IP");
        if(!str.isEmpty()){
            GlobalConfig::ServerIP = str;
        }

        str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"ServerNetwork/PORT");
        if(!str.isEmpty()){
            GlobalConfig::ServerPort = str;
        }

        str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"ServerNetwork/TcpConnectType");
        if(!str.isEmpty()){
            if(str.toUInt() == 0){
                GlobalConfig::TcpConnectType = GlobalConfig::ShortConnection;
            }else if(str.toUInt() == 1){
                GlobalConfig::TcpConnectType = GlobalConfig::LongConnection;
            }
        }

        str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"time/SwipCardIntervalTime");
        if(!str.isEmpty()){
            GlobalConfig::SwipCardIntervalTime = str.toUInt();
        }

        str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"time/HeartIntervalTime");
        if(!str.isEmpty()){
            GlobalConfig::HeartIntervalTime = str.toUInt();
        }

        str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"time/DeviceHeartIntervalTime");
        if(!str.isEmpty()){
            GlobalConfig::DeviceHeartIntervalTime = str.toUInt();
        }

        str = CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,"DeviceID/ID");
        if(!str.isEmpty()){
            GlobalConfig::DeviceID = str;
        }
    }else{//配置文件不存在,使用默认值生成配置文件
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"ServerNetwork/IP",GlobalConfig::ServerIP);
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"ServerNetwork/PORT",GlobalConfig::ServerPort);
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"ServerNetwork/TcpConnectType",QString::number(GlobalConfig::TcpConnectType));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"time/SwipCardIntervalTime",QString::number(GlobalConfig::SwipCardIntervalTime));
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"time/HeartIntervalTime",QString::number(GlobalConfig::HeartIntervalTime));
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"time/DeviceHeartIntervalTime",QString::number(GlobalConfig::DeviceHeartIntervalTime));
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,"DeviceID/ID",GlobalConfig::DeviceID);
    }
}
