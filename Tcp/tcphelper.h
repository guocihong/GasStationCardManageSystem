#ifndef TCPHELPER
#define TCPHELPER

#include "CommonSetting.h"

class TcpHelper : public QObject
{
    Q_OBJECT
public:
    explicit TcpHelper(QObject *parent = 0);
    void SendDoorStatusInfo(QString DoorStatus);//发送门磁状态信息
    void SendDataPackage(QString PathPrefix, QString CardID, QString TriggerTime);
    void SendCommonCode(QString MessageMerge);
    void PareseSendMsgType();
    void ParseServerMessage(QString XmlData);

public slots:
    //与深广服务器通信
    void slotSendHeart();
    void slotSendDeviceHeart();
    void slotGetIDList();
    void slotCheckNetWorkState();
    void slotSendMsg();
    void slotProcessMsg();
    void slotRecvServerMsg();//深广服务器返回
    void slotCloseConnection();
    void slotEstablishConnection();
    void slotDisplayError(QAbstractSocket::SocketError socketError);
    void slotSendLogInfo(QString info);//用来上传刷卡记录

signals:
    void signalUpdateNetWorkStatusInfo(quint8 NetWorkStatus);

public:
    enum SendMsgType{
        Heart,//检测网络联通
        DeviceHeart,//设备心跳
        DoorStatusInfo,//门磁的状态信息
        OperationCmd//图片Base64数据
    };
    volatile enum SendMsgType SendMsgTypeFlag;

    enum ConnectState{
        ConnectedState,
        DisConnectedState
    };
    volatile enum ConnectState ConnectStateFlag;

    enum DataSendState{
        SendSucceed,
        SendFailed
    };
    volatile enum DataSendState DataSendStateFlag;

    QTimer *HeartTimer;
    QTimer *DeviceHeartTimer;
    QTimer *GetCardIDListTimer;
    QTimer *CheckNetWorkTimer;
    QTimer *SendMsgTimer;//专门用来发送数据包到服务器
    QTimer *ProcessMsgTimer;//专门用来解析服务器返回的数据包

    QList<QString> SendMsgBuffer;
    QByteArray RecvMsgBuffer;

    QTcpSocket *tcpSocket;
    QFileSystemWatcher *dirWatcher;
    QSqlQuery query;
    volatile bool isGetCardIDList;

    volatile bool isNetWorkNormal;
    volatile bool isInsertCable;//是否插网线
    volatile quint8 LossPacketCount;//丢包次数
};

#endif // TCPHELPER
