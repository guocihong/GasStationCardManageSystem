#ifndef LISTENSERIAL_H
#define LISTENSERIAL_H

#include "qextserialport.h"
#include "CommonSetting.h"

class ListenSerial : public QObject
{
    Q_OBJECT
public:
    explicit ListenSerial(QObject *parent = 0);
    void FDX3S_CheckSecurityModuleState();
    void FDX3S_SearchCard();
    void FDX3S_SelectCard();
    void FDX3S_ReadBaseInfo();
    void FDX3S_GetManuID(int *pID);//读取设备模块号码

    QString ReadSerial();

public:
    QextSerialPort *mySerial;
    QByteArray AsciiCode;
};

#endif // LISTENSERIAL_H
