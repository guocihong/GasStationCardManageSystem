#include "listenserial.h"

ListenSerial::ListenSerial(QObject *parent) :
        QObject(parent)
{
    mySerial = new QextSerialPort("/dev/ttySAC2");
    if(mySerial->open(QIODevice::ReadWrite)){
        mySerial->setBaudRate(BAUD115200);
        mySerial->setDataBits(DATA_8);
        mySerial->setParity(PAR_NONE);
        mySerial->setStopBits(STOP_1);
        mySerial->setFlowControl(FLOW_OFF);
        mySerial->setTimeout(10);
        qDebug() << "/dev/ttySAC2 Open Succeed!";
    }
}

QString ListenSerial::ReadSerial()
{
    //延时100毫秒保证接收到的是一条完整的数据,而不是脱节的
    CommonSetting::Sleep(100);

    QByteArray Buffer = mySerial->readAll();
    AsciiCode = Buffer;

//    qDebug() << Buffer.toHex();
    QDataStream in(&Buffer,QIODevice::ReadWrite);
    QString strHex,RetValue;
    if(!Buffer.isEmpty()){
        while(!in.atEnd()){
            quint8 outChar;
            in >> outChar;
            strHex = QString::number(outChar,16);
            if(strHex.length() == 1)
                strHex = "0" + strHex;
            RetValue += strHex + " ";
        }
        RetValue = RetValue.simplified().trimmed().toUpper();
    }
//    qDebug() << QString(RetValue);

    return QString(RetValue);
}

void ListenSerial::FDX3S_CheckSecurityModuleState()
{
    //AA AA AA 96 69 00 03 11 FF ED
    char ControlCode[] = {0xAA,0xAA,0xAA,0x96,0x69,0x00,0x03,0x11,0xFF,0xED};
    mySerial->write(ControlCode,10);
}

void ListenSerial::FDX3S_SearchCard()
{
    //AA AA AA 96 69 00 03 20 01 22
    char ControlCode[] = {0xAA,0xAA,0xAA,0x96,0x69,0x00,0x03,0x20,0x01,0x22};
    mySerial->write(ControlCode,10);
}

void ListenSerial::FDX3S_SelectCard()
{
    //AA AA AA 96 69 00 03 20 02 21
    char ControlCode[] = {0xAA,0xAA,0xAA,0x96,0x69,0x00,0x03,0x20,0x02,0x21};
    mySerial->write(ControlCode,10);
}

void ListenSerial::FDX3S_ReadBaseInfo()
{
    //AA AA AA 96 69 00 03 30 01 32
    char ControlCode[] = {0xAA,0xAA,0xAA,0x96,0x69,0x00,0x03,0x30,0x01,0x32};
    mySerial->write(ControlCode,10);
}
