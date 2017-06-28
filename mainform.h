#ifndef MainForm_H
#define MainForm_H

#include <QWidget>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include "readidentifiercardinfoutil.h"
#include "Tcp/tcphelper.h"
#include "LinkOperate/linkoperate.h"
#include "OperateCamera/operatecamera.h"

namespace Ui {
class MainForm;
}

class MainForm : public QWidget
{
    Q_OBJECT

public:
    explicit MainForm(QWidget *parent = 0);
    ~MainForm();

    void Parse();

public slots:
    void slotCardInfo(QStringList &info);

    void slotUpdateNetWorkStatusInfo(quint8 NetWorkStatus);
    void slotDoorStatusChanged(QString DoorStatus);

    void slotEnableSwipCard();

private:
    Ui::MainForm *ui;

    ReadIdentifierCardInfoUtil *readid;
    TcpHelper *tcphelper;
    LinkOperate *link_operate;
    OperateCamera *operate_camera;

    QSqlQuery query;
};

#endif // MainForm_H
