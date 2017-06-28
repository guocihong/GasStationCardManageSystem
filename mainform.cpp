#include "readidentifiercardinfo.h"
#include "ui_readidentifiercardinfo.h"
#include "globalconfig.h"

/*
身份证信息结构:
AA AA AA 96 69 05 08 00 00 90 01 00 04 00 +（256 字节文字信息 ）+（1024 字节
照片信息）+（1 字节 CRC）

05 08:是"00 00 90 01 00 04 00 +（256 字节文字信息 ）+（1024 字节
照片信息）+（1 字节 CRC）"的长度
00 00 90:成功状态位
01 00:文字信息长度
04 00:照片信息长度

其中:
256个字节的文字信息以unicode编码-->GB13000 UCS-2其实就是unicode
1024个字节的图片信息经过小波变换压缩

注意：
1、若采用查询方式自动判断卡片是否放置，则间隔时间建议大于300ms
2、读完基本信息后，若需要立即读取最新住址信息或芯片管理号，在未移走卡片的情况下可以不用卡认证；
3、单独读取最新住址信息或芯片管理号时，需要先进行卡认证；
4、若卡片放置后发生读卡错误时，应移走卡片重新放置。
*/

#define NAME_ADDR_OFFSET  (14 * 2 + 14)
#define NAME_LENGTH       (30 * 2 + 29)
#define SEX_OFFSET        (44 * 2 + 44)
#define SEX_LENGTH        (2 * 2 + 1)
#define NATION_OFFSET     (46 * 2 + 46)
#define NATION_LENGTH     (4 * 2 + 3)
#define BIRTH_OFFSET      (50 * 2 + 50)
#define BIRTH_LENGTH      (16 * 2 + 15)
#define ADDRESS_OFFSET    (66 * 2 + 66)
#define ADDRESS_LENGTH    (70 * 2 +69)
#define ID_OFFSET         (136 * 2 + 136)
#define ID_LENGTH         (36 * 2 + 35)
#define ISSUE_OFFSET      (172 * 2 + 172)
#define ISSUE_LENGTH      (30 * 2 + 29)
#define EXPER1_OFFSET     (202 * 2 + 202)
#define EXPER1_LENGTH     (16 * 2 + 15)
#define EXPER2_OFFSET     (218 * 2 + 218)
#define EXPER2_LENGTH     (16 * 2 + 15)
#define PIC_OFFSET        (270 * 2 + 270)
#define PIC_LENGTH        (1024 * 2 + 1023)

ReadIdentifierCardInfo::ReadIdentifierCardInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReadIdentifierCardInfo)
{
    ui->setupUi(this);

    tcphelper = new TcpHelper(this);
    connect(tcphelper,SIGNAL(signalUpdateNetWorkStatusInfo(quint8)),this,SLOT(slotUpdateNetWorkStatusInfo(quint8)));
    listen_serial = new ListenSerial(this);
    operate_camera = new OperateCamera(this);
    link_operate = new LinkOperate(this);
    connect(link_operate,SIGNAL(signalDoorStatusChanged(QString)),this,SLOT(slotDoorStatusChanged(QString)));

    PollingTimer = new QTimer(this);
    PollingTimer->setInterval(1500);
    connect(PollingTimer,SIGNAL(timeout()),this,SLOT(slotPolling()));
    PollingTimer->start();


    link_operate->BuzzerOn();
    link_operate->BuzzerTimer->start();

    link_operate->DoorTimer->start();
}

ReadIdentifierCardInfo::~ReadIdentifierCardInfo()
{
    delete ui;
}

void ReadIdentifierCardInfo::slotUpdateNetWorkStatusInfo(quint8 NetWorkStatus)
{
    if(NetWorkStatus == 0){//没有插网线
        link_operate->ST3_RedON();
    }else if(NetWorkStatus == 1){//插有网线,但是与服务器不联通
        link_operate->ST3_GreenOFF();
    }else if(NetWorkStatus == 2){//插有网线,并且与服务器联通,网络正常
        link_operate->ST3_GreenON();
    }
}

void ReadIdentifierCardInfo::slotDoorStatusChanged(QString DoorStatus)
{
    tcphelper->SendDoorStatusInfo(DoorStatus);
}

void ReadIdentifierCardInfo::slotPolling()
{
    //1.检测安全模块状态
    listen_serial->FDX3S_CheckSecurityModuleState();
    QString SecurityModuleState = listen_serial->ReadSerial();
    if(SecurityModuleState == "AA AA AA 96 69 00 04 00 00 90 94"){
        //2.寻找卡片
        listen_serial->FDX3S_SearchCard();
        QString SearchCardResult = listen_serial->ReadSerial();
        if(SearchCardResult == "AA AA AA 96 69 00 08 00 00 9F 00 00 00 00 97"){
            //3.选择卡片
            listen_serial->FDX3S_SelectCard();
            QString SelectCardResult = listen_serial->ReadSerial();
//            if((SelectCardResult == "AA AA AA 96 69 00 0C 00 00 90 00 00 00 00 00 00 00 00 9C") ||  (SelectCardResult == "AA 00 AA AA 96 69 00 0C 00 00 90 00 00 00 00 00 00 9C"
//)){
            if(1){
                //"AA 00 AA AA 96 69 00 0C 00 00 90 00 00 00 00 00 00 9C"
                //"AA AA AA 96 69 00 0C 00 00 90 00 00 00 00 00 00 00 00 9C"

                //4.读取基本信息
                listen_serial->FDX3S_ReadBaseInfo();
                CommonSetting::Sleep(1000);
                QByteArray BaseInfo = listen_serial->ReadSerial().toAscii();
                if(BaseInfo.length() == 3884){
                    FDX3S_GetPeopleIDCode(BaseInfo);
                }
            }
        }
    }
}

void ReadIdentifierCardInfo::FDX3S_GetPeopleIDCode(QByteArray BaseInfo)
{
    //禁止刷卡
    PollingTimer->stop();

    //禁止测试网络联通
    tcphelper->HeartTimer->stop();

    QString IdentifierCardNumberUnicodeCode;
    QStringList IdentifierCardNumberHexCodeList =
            QString(BaseInfo.mid(ID_OFFSET,ID_LENGTH)).split(" ");
    for(int i = 0,j = 1; (i <= 34) && (j <= 35); i += 2,j += 2)
        IdentifierCardNumberUnicodeCode += IdentifierCardNumberHexCodeList.at(j) +
                IdentifierCardNumberHexCodeList.at(i);
    QString IdentifierCardNumber =
            CommonSetting::Unicode2UTF8(IdentifierCardNumberUnicodeCode);
    qDebug() << IdentifierCardNumber;
    operate_camera->StartCamera(IdentifierCardNumber,CommonSetting::GetCurrentDateTime());
    QString fileName = "/opt/" + IdentifierCardNumber + ".jpg";
    ui->pic_label->setPixmap(QPixmap(fileName));

    bool isRegister = false;//判断身份证号码是否注册
    bool isAllowAddOil = false;//是否允许加油
    int IdentifierCardNumberType;
    int OperateCount;
    QString ValidTime;

    query.exec(tr("SELECT [IdentifierCardNumberType],[OperateCount],[ValidTime] FROM [dtm_identifiercardnumber_table] WHERE [IdentifierCardNumber] = \"%1\"").arg(IdentifierCardNumber));
    while(query.next()){
        isRegister = true;
        IdentifierCardNumberType = query.value(0).toInt();
        OperateCount = query.value(1).toInt();
        ValidTime = query.value(2).toString();

        QSqlQuery sq;
        sq.exec(tr("SELECT julianday('now')- julianday('%1')").arg(ValidTime));
        while(sq.next()){
            //卡号没有过期并且加油次数没有用完
            if((sq.value(0).toInt() <= 0) && (OperateCount > 0)){
                isAllowAddOil = true;//允许加油
            }else{
                isAllowAddOil = false;//不允许加油
            }
        }
    }

    if(isRegister){//身份证号码注册
        if(isAllowAddOil){//允许加油
            link_operate->AllowAddOil(IdentifierCardNumberType);
        }else{//不允许加油
            link_operate->RejectAddOil();
        }
    }else{//身份证号码未注册
        link_operate->InValidUser();
    }

    //移动/opt目录下的图片到/sdcard目录下
    QString StrId = QUuid::createUuid().toString();
    QString DirName = StrId.mid(1,StrId.length() - 2);
    CommonSetting::CreateFolder("/opt",DirName);
    system(tr("mv /opt/*.txt /opt/%1").arg(DirName).toAscii().data());
    system(tr("mv /opt/%1 /sdcard").arg(DirName).toAscii().data());

    QTimer::singleShot(GlobalConfig::SwipCardIntervalTime * 1000,this,SLOT(slotEnableSwipCard()));
}

void ReadIdentifierCardInfo::slotEnableSwipCard()
{
    //ST1,ST2都不亮灯
    link_operate->Restore();

    //恢复刷卡
    PollingTimer->start();

    //恢复测试网络联通
    tcphelper->HeartTimer->start();
}

void ReadIdentifierCardInfo::on_btnLedAllRedOn_clicked()
{
    link_operate->LedAllRedOn();
}

void ReadIdentifierCardInfo::on_btnLedAllGreenOn_clicked()
{
    link_operate->LedAllGreenOn();
}

void ReadIdentifierCardInfo::on_btnLedAllOff_clicked()
{
    link_operate->LedAllOff();
}
