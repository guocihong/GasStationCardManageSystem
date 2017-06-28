#include "mainform.h"
#include "ui_mainform.h"
#include "globalconfig.h"

MainForm::MainForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainForm)
{
    ui->setupUi(this);

    readid = new ReadIdentifierCardInfoUtil(this);
    connect(readid,SIGNAL(signalCardInfo(QStringList&)),this,SLOT(slotCardInfo(QStringList&)));
    readid->Open("/dev/ttySAC2");

    tcphelper = new TcpHelper(this);
    connect(tcphelper,SIGNAL(signalUpdateNetWorkStatusInfo(quint8)),this,SLOT(slotUpdateNetWorkStatusInfo(quint8)));

    operate_camera = new OperateCamera(this);

    link_operate = new LinkOperate(this);
    connect(link_operate,SIGNAL(signalDoorStatusChanged(QString)),this,SLOT(slotDoorStatusChanged(QString)));

    link_operate->BuzzerOn();
    link_operate->BuzzerTimer->start();
    link_operate->DoorTimer->start();
}

MainForm::~MainForm()
{
    delete ui;
}

void MainForm::slotCardInfo(QStringList &info)
{
    ui->labName->setText(info.at(0));
    GlobalConfig::Name = info.at(0);

    ui->labSex->setText(info.at(1));
    if (info.at(1) == "男") {
        GlobalConfig::Sex = "1";
    } else if (info.at(1) == "女") {
        GlobalConfig::Sex = "2";
    }

    ui->labNation->setText(info.at(2));
    GlobalConfig::Nation = info.at(2);

    QString str = info.at(3);
    ui->labYear->setText(str.mid(0, 4));
    ui->labMonth->setText(str.mid(4, 2));
    ui->labDay->setText(str.mid(6, 2));
    GlobalConfig::Birthday = str.mid(0,4) + "-" + str.mid(4,2) + "-" + str.mid(6,2);

    ui->labAddress->setText(info.at(4));
    GlobalConfig::Address = info.at(4);

    ui->labIDCode->setText(info.at(5));
    GlobalConfig::IDCode = info.at(5);

    ui->labDepartment->setText(info.at(6));
    GlobalConfig::Department = info.at(6);

    QString str1 = info.at(7);
    QString str2 = info.at(8);
    QString s = QString("%1.%2.%3 - %4.%5.%6")
                .arg(str1.mid(0, 4)).arg(str1.mid(4, 2)).arg(str1.mid(6, 2))
                .arg(str2.mid(0, 4)).arg(str2.mid(4, 2)).arg(str2.mid(6, 2));
    ui->labVaildTime->setText(s);

    GlobalConfig::StartDate = str1.mid(0,4) + "-" + str1.mid(4,2) + "-" + str1.mid(6,2);
    GlobalConfig::EndDate = str2.mid(0,4) + "-" + str2.mid(4,2) + "-" + str2.mid(6,2);


    ui->labCertificateImage->setPixmap(QPixmap("/bin/zp.bmp"));

    this->Parse();
}

void MainForm::slotUpdateNetWorkStatusInfo(quint8 NetWorkStatus)
{
    if (NetWorkStatus == 1) {//插有网线,并且与服务器联通,网络正常，ST3亮绿灯
        link_operate->ST3_GreenON();
    } else if (NetWorkStatus == 2) {//插有网线,但是与服务器不联通，ST3亮红灯
        link_operate->ST3_RedON();
    } else if (NetWorkStatus == 3) {//没有插网线，ST3不亮
        link_operate->ST3_OFF();
    }
}

void MainForm::slotDoorStatusChanged(QString DoorStatus)
{
    tcphelper->SendDoorStatusInfo(DoorStatus);
}

void MainForm::Parse()
{
    //禁止刷卡
    readid->SendCmdTimer->stop();

    //禁止测试网络联通
    tcphelper->HeartTimer->stop();

    QString IdentifierCardNumber = GlobalConfig::IDCode;
    QString TriggerTime = CommonSetting::GetCurrentDateTime();

    operate_camera->StartCamera(IdentifierCardNumber,TriggerTime);

    QString fileName = "/opt/" + IdentifierCardNumber + ".jpg";
    ui->labSnapImage->setPixmap(QPixmap(fileName));

    bool isRegister = false;//判断身份证号码是否注册
    bool isAllowAddOil = false;//是否允许加油
    int IdentifierCardNumberType = 0;//未注册的身份证号码
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
                IdentifierCardNumberType = 0;//只要不允许加油，全部填0，不管身份证是否注册
            }
        }
    }

    if(isRegister){//身份证号码注册
        if(isAllowAddOil){//允许加油
            link_operate->AllowAddOil(IdentifierCardNumberType);

            if (IdentifierCardNumberType == 1) {//个人加油
                ui->labType->setText("类型:个人");
                ui->labOilCount->setText("升数:5升");
            } else if (IdentifierCardNumberType == 2) {//单位加油
                ui->labType->setText("类型:单位");
                ui->labOilCount->setText("升数:60升");
            }

            ui->labStatus->setText("状态:注册,允许加油");
            ui->labStatus->setStyleSheet("border:0px solid rgb(0, 129, 255);"
                    "border-radius:0px;"
                    "color: rgb(92, 174, 37);"
                    "font: 30pt \"Ubuntu\"");
        }else{//不允许加油
            link_operate->RejectAddOil();
            ui->labStatus->setText("状态:不允许加油，卡号过期/加油次数用完");
            ui->labStatus->setStyleSheet("border:0px solid rgb(0, 129, 255);"
                    "border-radius:0px;"
                    "color: red;"
                    "font: 30pt \"Ubuntu\"");
        }
    }else{//身份证号码未注册
        link_operate->InValidUser();
        ui->labStatus->setText("状态:未注册，不允许加油");
        ui->labStatus->setStyleSheet("border:0px solid rgb(0, 129, 255);"
                "border-radius:0px;"
                "color: red;"
                "font: 30pt \"Ubuntu\"");
    }

    //将图片名字Base64_341125199005272375_2016-07-12_16-03-01.txt重命名为Base64_341125199005272375_2016-07-12_16-03-01_人员类型.txt
    system(tr("mv /opt/Base64_%1_%2.txt /opt/Base64_%3_%4_%5.txt").arg(IdentifierCardNumber).arg(TriggerTime).arg(IdentifierCardNumber).arg(TriggerTime).arg(IdentifierCardNumberType).toAscii().data());

    //移动/opt目录下的图片到/sdcard目录下
    QString StrId = QUuid::createUuid().toString();
    QString DirName = StrId.mid(1,StrId.length() - 2);
    CommonSetting::CreateFolder("/opt",DirName);
    system(tr("mv /opt/*.txt /opt/%1").arg(DirName).toAscii().data());
    system(tr("mv /opt/%1 /sdcard").arg(DirName).toAscii().data());

    QTimer::singleShot(GlobalConfig::SwipCardIntervalTime * 1000,this,SLOT(slotEnableSwipCard()));
}

void MainForm::slotEnableSwipCard()
{
    //ST1,ST2都不亮灯
    link_operate->Restore();

    //恢复刷卡
    readid->SendCmdTimer->start();

    //恢复测试网络联通
    tcphelper->HeartTimer->start();
}
