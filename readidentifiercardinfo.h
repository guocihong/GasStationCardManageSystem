#ifndef READIDENTIFIERCARDINFO_H
#define READIDENTIFIERCARDINFO_H

#include <QWidget>
#include "Tcp/tcphelper.h"
#include "Qextserialport/listenserial.h"
#include "LinkOperate/linkoperate.h"
#include "OperateCamera/operatecamera.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>

namespace Ui {
class ReadIdentifierCardInfo;
}

class ReadIdentifierCardInfo : public QWidget
{
    Q_OBJECT

public:
    explicit ReadIdentifierCardInfo(QWidget *parent = 0);
    ~ReadIdentifierCardInfo();
    void FDX3S_GetPeopleIDCode(QByteArray BaseInfo);//得到卡号信息

public slots:
    void slotUpdateNetWorkStatusInfo(quint8 NetWorkStatus);
    void slotDoorStatusChanged(QString DoorStatus);

    void slotPolling();
    void slotEnableSwipCard();

    void on_btnLedAllRedOn_clicked();
    void on_btnLedAllGreenOn_clicked();
    void on_btnLedAllOff_clicked();

private:
    Ui::ReadIdentifierCardInfo *ui;

    TcpHelper *tcphelper;
    ListenSerial *listen_serial;
    LinkOperate *link_operate;
    OperateCamera *operate_camera;

    QTimer *PollingTimer;
    QSqlQuery query;
};

#endif // READIDENTIFIERCARDINFO_H
