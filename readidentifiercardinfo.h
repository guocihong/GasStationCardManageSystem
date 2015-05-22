#ifndef READIDENTIFIERCARDINFO_H
#define READIDENTIFIERCARDINFO_H

#include <QWidget>
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
    void slotPolling();
    void slotEnableSwipCard();

private:
    Ui::ReadIdentifierCardInfo *ui;

    ListenSerial *listen_serial;
    LinkOperate *link_operate;
    OperateCamera *operate_camera;

    QTimer *PollingTimer;
    QSqlQuery query;

    bool flag;
};

#endif // READIDENTIFIERCARDINFO_H
