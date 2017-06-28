#ifndef READIDENTIFIERCARDINFOUTIL_H
#define READIDENTIFIERCARDINFOUTIL_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QStringList>
#include <QFile>
#include <QTextCodec>
#include "Qextserialport/qextserialport.h"

class ReadIdentifierCardInfoUtil : public QObject
{
    Q_OBJECT
public:
    explicit ReadIdentifierCardInfoUtil(QObject *parent = 0);

    bool Open(QString SerialNumber);
    void Close();

    void SDT_GetSAMStatus();
    void SDT_ResetSAM();
    void SDT_SearchCard();
    void SDT_SelectCard();
    void SDT_ReadBaseMsg();

    void Parse();

    QString unicodeToUtf8(const QByteArray &data);

    QString getName(const QByteArray &data);
    QString getSex(const QByteArray &data);
    QString getNation(const QByteArray &data);

signals:
    void signalCardInfo(QStringList &info);

public slots:
    void slotSendCmd();
    void slotReadMsg();

public:
    QextSerialPort *Serial;

    QTimer *SendCmdTimer;
    QTimer *RecvMsgTimer;

    QByteArray Buffer;
};

#endif // READIDENTIFIERCARDINFOUTIL_H
