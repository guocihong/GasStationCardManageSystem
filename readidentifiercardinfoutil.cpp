#include "readidentifiercardinfoutil.h"

ReadIdentifierCardInfoUtil::ReadIdentifierCardInfoUtil(QObject *parent) :
    QObject(parent)
{
    SendCmdTimer = new QTimer (this);
    SendCmdTimer->setInterval(2000);
    connect(SendCmdTimer,SIGNAL(timeout()),this,SLOT(slotSendCmd()));

    RecvMsgTimer = new QTimer(this);
    RecvMsgTimer->setInterval(100);
    connect(RecvMsgTimer,SIGNAL(timeout()),this,SLOT(slotReadMsg()));
}

bool ReadIdentifierCardInfoUtil::Open(QString SerialNumber)
{
    Serial = new QextSerialPort(SerialNumber);
    if (Serial->open(QIODevice::ReadWrite)){
        Serial->setBaudRate(BAUD115200);
        Serial->setDataBits(DATA_8);
        Serial->setParity(PAR_NONE);
        Serial->setStopBits(STOP_1);
        Serial->setFlowControl(FLOW_OFF);
        Serial->setTimeout(10);

        qDebug() << SerialNumber << " Open Succeed";

        SendCmdTimer->start();
        RecvMsgTimer->start();

        return true;
    } else {
        qDebug() << SerialNumber << " Open Failed";

        return false;
    }
}

void ReadIdentifierCardInfoUtil::Close()
{
    Serial->close();

    SendCmdTimer->stop();
    RecvMsgTimer->stop();
}

/*
功能说明:查看模块状态。
返回值:0x90 状态正常;0x60 自检失败,不能接收命令。
*/
void ReadIdentifierCardInfoUtil::SDT_GetSAMStatus()
{
    Buffer.clear();

    //AA AA AA 96 69 00 03 11 FF ED
    QByteArray ControlCode;
    ControlCode[0] = 0xAA;
    ControlCode[1] = 0xAA;
    ControlCode[2] = 0xAA;
    ControlCode[3] = 0x96;
    ControlCode[4] = 0x69;
    ControlCode[5] = 0x00;
    ControlCode[6] = 0x03;
    ControlCode[7] = 0x11;
    ControlCode[8] = 0xFF;
    ControlCode[9] = 0xED;
    Serial->write(ControlCode);
}

/*
功能说明:对模块进行复位,相当于对模块重新上电。
返回值:0x90 成功
*/
void ReadIdentifierCardInfoUtil::SDT_ResetSAM()
{
    Buffer.clear();

    //AA AA AA 96 69 00 03 10 FF EC
    QByteArray ControlCode;
    ControlCode[0] = 0xAA;
    ControlCode[1] = 0xAA;
    ControlCode[2] = 0xAA;
    ControlCode[3] = 0x96;
    ControlCode[4] = 0x69;
    ControlCode[5] = 0x00;
    ControlCode[6] = 0x03;
    ControlCode[7] = 0x10;
    ControlCode[8] = 0xFF;
    ControlCode[9] = 0xEC;
    Serial->write(ControlCode);
}

/*
功能说明:对放入感应区的居民身份证做找卡操作
返回值:0x9f 找卡成功;0x80 找卡失败
*/
void ReadIdentifierCardInfoUtil::SDT_SearchCard()
{
    Buffer.clear();

    //AA AA AA 96 69 00 03 20 01 22
    QByteArray ControlCode;
    ControlCode[0] = 0xAA;
    ControlCode[1] = 0xAA;
    ControlCode[2] = 0xAA;
    ControlCode[3] = 0x96;
    ControlCode[4] = 0x69;
    ControlCode[5] = 0x00;
    ControlCode[6] = 0x03;
    ControlCode[7] = 0x20;
    ControlCode[8] = 0x01;
    ControlCode[9] = 0x22;
    Serial->write(ControlCode);
}

/*
功能说明:对放入感应区的居民身份证做选卡操作
返回值:0x90 选卡成功;0x81 选卡失败
*/
void ReadIdentifierCardInfoUtil::SDT_SelectCard()
{
    Buffer.clear();

    //AA AA AA 96 69 00 03 20 02 21
    QByteArray ControlCode;
    ControlCode[0] = 0xAA;
    ControlCode[1] = 0xAA;
    ControlCode[2] = 0xAA;
    ControlCode[3] = 0x96;
    ControlCode[4] = 0x69;
    ControlCode[5] = 0x00;
    ControlCode[6] = 0x03;
    ControlCode[7] = 0x20;
    ControlCode[8] = 0x02;
    ControlCode[9] = 0x21;
    Serial->write(ControlCode);
}

/*
功能说明:读取居民身份证机读文字信息和相片信息。
返回值:0x90 读文字信息和相片信息成功；0x41 读卡失败
*/
void ReadIdentifierCardInfoUtil::SDT_ReadBaseMsg()
{
    Buffer.clear();

    //AA AA AA 96 69 00 03 30 01 32
    QByteArray ControlCode;
    ControlCode[0] = 0xAA;
    ControlCode[1] = 0xAA;
    ControlCode[2] = 0xAA;
    ControlCode[3] = 0x96;
    ControlCode[4] = 0x69;
    ControlCode[5] = 0x00;
    ControlCode[6] = 0x03;
    ControlCode[7] = 0x30;
    ControlCode[8] = 0x01;
    ControlCode[9] = 0x32;
    Serial->write(ControlCode);
}

void ReadIdentifierCardInfoUtil::Parse()
{
    QStringList info;

    //分别取出 头部固定14字节 256字节文字信息 1024照片数据 CRC校验
    //AA AA AA 96 69 05 08 00 00 90 01 00 04 00 +（ 256 字节文字信息 ） +（ 1024 字节照片信息） +（ 1 字节 CRC）
    QByteArray head = Buffer.mid(0, 14);
    QByteArray IDInfo = Buffer.mid(14, 256);
    QByteArray temp;

    if (head.toHex() == "aaaaaa9669050800009001000400") {
        //姓名
        temp = IDInfo.mid(0, 30);
        info.append(getName(temp));

        //性别
        temp = IDInfo.mid(30, 2);
        info.append(getSex(temp));

        //名族
        temp = IDInfo.mid(32, 4);
        info.append(getNation(temp));

        //出生年月
        temp = IDInfo.mid(36, 16);
        info.append(unicodeToUtf8(temp));

        //住址
        temp = IDInfo.mid(52, 70);
        info.append(unicodeToUtf8(temp));

        //公民身份号码
        temp = IDInfo.mid(122, 36);
        info.append(unicodeToUtf8(temp));

        //签发机关
        temp = IDInfo.mid(158, 30);
        info.append(unicodeToUtf8(temp));

        //有效期起始日期
        temp = IDInfo.mid(188, 16);
        info.append(unicodeToUtf8(temp));

        //有效期结束日期
        temp = IDInfo.mid(204, 16);
        info.append(unicodeToUtf8(temp));

        //备用
        temp = IDInfo.mid(220, 30);
        info.append(unicodeToUtf8(temp));

        //解析身份证头像图片数据
        QByteArray IDPhoto = Buffer.mid(270, 1024);
        QString wltFile = QString("/bin/zp.wlt");
        QString bmpFile =  QString("bin/zp.bmp");
        QString outFile = QString("cd /bin;./a.out");

        QFile file(wltFile);
        if (file.open(QFile::ReadWrite | QFile::Truncate)) {
            file.write(IDPhoto);
            file.close();
            system(outFile.toLatin1().data());
        }

        emit signalCardInfo(info);
    }
}

void ReadIdentifierCardInfoUtil::slotSendCmd()
{
    this->SDT_GetSAMStatus();
}

void ReadIdentifierCardInfoUtil::slotReadMsg()
{
    Buffer.append(Serial->readAll());

    QByteArray HexData = Buffer.toHex();

    //1、读取安全模块的状态
    if (HexData == "aaaaaa9669000400009094") {//读取模块状态成功
        //检测模块成功后立即寻找身份证
        this->SDT_SearchCard();
        return;
    } else if (HexData == "aaaaaa9669000400006064") {//SAM_A 自检失败,不能接收命令
//        qDebug() << "SAM_A check failed";
        return;
    }

    //2、寻找居民身份证
    if (HexData == "aaaaaa9669000800009f0000000097") {//寻找居民身份证成功
        //寻找身份证成功则立即选取身份证
        this->SDT_SelectCard();
        return;
    } else if (HexData == "aaaaaa9669000400008084") {//寻找居民身份证失败
        qDebug() << "search card failed";
        return;
    }

    //3、选取居民身份证成功
    if (HexData == "aaaaaa9669000c00009000000000000000009c") {//选取居民身份证成功
        //选取身份证成功则读取身份证信息
        this->SDT_ReadBaseMsg();
        return;
    } else if (HexData == "aaaaaa9669000400008185") {//选取居民身份证失败
        qDebug() << "select card failed";
        return;
    }

    //4、读取身份证信息
    if (Buffer.size() == 1295) {//读取身份证成功后会返回 1295 字节数据身份证信息
        qDebug() << "read card succeed";
        this->Parse();
        Buffer.clear();
    } else if (HexData == "aaaaaa9669000400004145") {//读居民身份证操作失败
        qDebug() << "read card failed";
        Buffer.clear();
    } else if (Buffer.size() > 1295) {
        Buffer.clear();
    }
}

QString ReadIdentifierCardInfoUtil::unicodeToUtf8(const QByteArray &data)
{
    QString result;

    int len = data.count();
    QString strHex;

    //按照内存编码格式,将每两位的顺序调换
    for (int i = 0; i < len; i = i + 2) {
        quint8 first = data.at(i);
        quint8 second = data.at(i + 1);
        QString s = QString("%1%2").arg(second, 2, 16, QChar('0')).arg(first, 2, 16, QChar('0'));
        strHex += s;
    }

    QStringList list;
    for (int i = 0; i < strHex.length(); i = i + 4) {
        list.append(strHex.mid(i, 4));
    }

    QString strUnicode;
    foreach (QString str, list) {
        strUnicode.append(str.toUShort(0, 16));
    }

    QTextCodec *codec = QTextCodec::codecForName("utf-8");
    result = codec->fromUnicode(strUnicode);

    return result.trimmed();
}


QString ReadIdentifierCardInfoUtil::getName(const QByteArray &data)
{
    QString name = unicodeToUtf8(data);
    return name;
}

QString ReadIdentifierCardInfoUtil::getSex(const QByteArray &data)
{
    QString sex = unicodeToUtf8(data);

    if (sex == "0") {
        sex = "未知";
    } else if (sex == "1") {
        sex = "男";
    } else if (sex == "2") {
        sex = "女";
    } else if (sex == "9") {
        sex = "未说明";
    }

    return sex;
}

QString ReadIdentifierCardInfoUtil::getNation(const QByteArray &data)
{
    QString nation = unicodeToUtf8(data);

    if (nation == "01") {
        nation = "汉族";
    } else if (nation == "02") {
        nation = "蒙古族";
    } else if (nation == "03") {
        nation = "回族";
    } else if (nation == "04") {
        nation = "藏族";
    } else if (nation == "05") {
        nation = "维吾尔族";
    } else if (nation == "06") {
        nation = "苗族";
    } else if (nation == "07") {
        nation = "彝族";
    } else if (nation == "08") {
        nation = "壮族";
    } else if (nation == "09") {
        nation = "布依族";
    } else if (nation == "10") {
        nation = "朝鲜族";
    } else {
        nation = "其他";
    }

    return nation;
}
