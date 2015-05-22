#ifndef TCPCOMMUNICATE_H
#define TCPCOMMUNICATE_H

#include "CommonSetting.h"

class TcpCommunicate : public QObject
{
    Q_OBJECT
public:
    explicit TcpCommunicate(QObject *parent = 0);
    void slotGetCardTypeName();
    void slotOperationCmd();
    void SendDataPackage(QString PathPrefix, QString CardID, QString TriggerTime);
    void SendCommonCode(QString MessageMerge);
    void PareseSendMsgType();
    void ParseServerMessage(QString XmlData);

public slots:
    //与深广服务器通信
    void slotSendHeartData();
    void slotGetIDList();
    void slotCheckNetWorkState();
    void slotParseServerMessage();//深广服务器返回
    void slotCloseConnection();
    void slotEstablishConnection();
    void slotDisplayError(QAbstractSocket::SocketError socketError);
    void slotSendLogInfo(QString info);//用来上传刷卡记录

public:
    enum SendMsgType{
        DeviceHeart,//设备心跳
        GetCardTypeName,//获取卡类型
        OperationCmd//加钞,巡检,接警
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
    QTimer *GetCardIDListTimer;
    QTimer *CheckNetWorkTimer;
    QTcpSocket *tcpSocket;
    QFileSystemWatcher *dirWatcher;
    QSqlQuery query;
    volatile bool isGetCardIDList;
    volatile bool isGetCardOrderList;

    QString ServerIpAddress;
    QString ServerListenPort;
};

#endif // TCPCOMMUNICATE_H
