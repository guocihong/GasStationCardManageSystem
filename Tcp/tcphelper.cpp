#include "tcphelper.h"
#include "globalconfig.h"
#define WITH_WRITE_FILE1

TcpHelper::TcpHelper(QObject *parent) :
    QObject(parent)
{
    isGetCardIDList = false;
    isNetWorkNormal = false;
    isInsertCable = true;
    LossPacketCount = 0;
    SendMsgBuffer.clear();

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket,SIGNAL(readyRead()),
            this,SLOT(slotRecvServerMsg()));
    connect(tcpSocket,SIGNAL(disconnected()),
            this,SLOT(slotCloseConnection()));
    connect(tcpSocket,SIGNAL(connected()),
            this,SLOT(slotEstablishConnection()));
    connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotDisplayError(QAbstractSocket::SocketError)));

    ConnectStateFlag = TcpHelper::DisConnectedState;

    HeartTimer = new QTimer(this);
    HeartTimer->setInterval(GlobalConfig::HeartIntervalTime * 1000);
    connect(HeartTimer,SIGNAL(timeout()),this,SLOT(slotSendHeart()));

    DeviceHeartTimer = new QTimer(this);
    DeviceHeartTimer->setInterval(GlobalConfig::DeviceHeartIntervalTime * 60 * 1000);
    connect(DeviceHeartTimer,SIGNAL(timeout()),this,SLOT(slotSendDeviceHeart()));

    GetCardIDListTimer = new QTimer(this);
    GetCardIDListTimer->setInterval(3000);
    connect(GetCardIDListTimer,SIGNAL(timeout()),this,SLOT(slotGetIDList()));

    CheckNetWorkTimer = new QTimer(this);
    CheckNetWorkTimer->setInterval(1000);
    connect(CheckNetWorkTimer,SIGNAL(timeout()),this,SLOT(slotCheckNetWorkState()));

    SendMsgTimer = new QTimer(this);
    SendMsgTimer->setInterval(200);
    connect(SendMsgTimer,SIGNAL(timeout()),this,SLOT(slotSendMsg()));

    ProcessMsgTimer = new QTimer(this);
    ProcessMsgTimer->setInterval(200);
    connect(ProcessMsgTimer,SIGNAL(timeout()),this,SLOT(slotProcessMsg()));

    HeartTimer->start();
    DeviceHeartTimer->start();
    GetCardIDListTimer->start();
    CheckNetWorkTimer->start();
    SendMsgTimer->start();
    ProcessMsgTimer->start();

    //创建一个监视器，用于监视目录/sdcard的变化
    dirWatcher = new QFileSystemWatcher(this);
    dirWatcher->addPath("/sdcard");
    connect(dirWatcher,SIGNAL(directoryChanged(QString)), this,SLOT(slotSendLogInfo(QString)));

    slotSendDeviceHeart();//开机发送心跳
}

void TcpHelper::SendDoorStatusInfo(QString DoorStatus)
{
    SendMsgTypeFlag = TcpHelper::DoorStatusInfo;
    SendDataPackage("",DoorStatus,"");
}

void TcpHelper::slotSendHeart()
{
    SendMsgTypeFlag = TcpHelper::Heart;
    SendDataPackage("","","");

    if(isNetWorkNormal){//网络正常
        emit signalUpdateNetWorkStatusInfo(2);
        LossPacketCount = 0;
    }else{//网络异常
        if(isInsertCable){//插了网线,表明网络是正常的,但是与服务器不联通,可能是配置问题或者服务器的问题
            LossPacketCount++;
            if(LossPacketCount == 5){
                emit signalUpdateNetWorkStatusInfo(1);
                LossPacketCount = 0;
            }
        }else{//没有插网线
            emit signalUpdateNetWorkStatusInfo(0);
        }
    }

    isNetWorkNormal = false;
}

void TcpHelper::slotSendDeviceHeart()
{
    SendMsgTypeFlag = TcpHelper::DeviceHeart;
    SendDataPackage("","","");
}

void TcpHelper::slotGetIDList()
{
    if(!isGetCardIDList){
        SendMsgTypeFlag = TcpHelper::DeviceHeart;
        SendDataPackage("","","");
        qDebug() << "isGetCardIDList";
    }else{
        GetCardIDListTimer->stop();
        qDebug() << "stop GetCardIDListTimer";
    }
}

void TcpHelper::slotCheckNetWorkState()
{
    system("ifconfig eth0 > /bin/network_info.txt");
    QString network_info = CommonSetting::ReadFile("/bin/network_info.txt");
    if(network_info.contains("RUNNING")){
        isInsertCable = true;//插了网线
    }else{
        isInsertCable = false;//没有插网线
        ConnectStateFlag = TcpHelper::DisConnectedState;
        tcpSocket->disconnectFromHost();
        tcpSocket->abort();
    }
}

void TcpHelper::slotSendMsg()
{
    if(SendMsgBuffer.size() == 0){
        return;
    }

    QString MsgInfo = SendMsgBuffer.takeFirst();
    SendCommonCode(MsgInfo);
}

void TcpHelper::slotRecvServerMsg()
{
    CommonSetting::Sleep(300);
    QByteArray data = tcpSocket->readAll();
    RecvMsgBuffer.append(data);
}

void TcpHelper::slotProcessMsg()
{
    if(RecvMsgBuffer.size() == 0){
        return;
    }

    QString MsgHeader = RecvMsgBuffer.mid(0,20);
    quint32 MsgSize = MsgHeader.mid(6,8).toUInt();

    if(RecvMsgBuffer.size() < MsgSize){//没有一个完整的数据包
        return;
    }

    QString XmlData = RecvMsgBuffer.mid(20,MsgSize - 20);
    RecvMsgBuffer = RecvMsgBuffer.mid(MsgSize);

    QString NowTime;
    QDomDocument dom;
    QString errorMsg;
    QStringList LatestCardIDList,LatestCardTypeList
            ,LatestCardStatusList,LatestCardValidTimeList;//下载最新的卡号信息
    QStringList CurrentCardIDList,CurrentCardTypeList
            ,CurrentCardStatusList,CurrentCardValidTimeList;//发送日志信息，服务器返回卡号的当前信息

    int errorLine,errorColumn;
    bool CaptionFlag = false,CardStateFlag = false;

    if(!dom.setContent(XmlData,&errorMsg,&errorLine,&errorColumn)){
        qDebug() << "Parse error at line " << errorLine <<","<< "column " << errorColumn << ","<< errorMsg;
        return;
    }

    QDomElement RootElement = dom.documentElement();//获取根元素
    if(RootElement.tagName() == "Server"){//根元素名称
        //判断根元素是否有这个属性
        if(RootElement.hasAttribute("NowTime")){
            //获得这个属性对应的值
            NowTime = RootElement.attributeNode("NowTime").value();
        }

        QDomNode firstChildNode =
                RootElement.firstChild();//第一个子节点
        while(!firstChildNode.isNull()){
            if(firstChildNode.nodeName() == "Caption"){
                qDebug() << "Caption";
                CaptionFlag = true;
                QDomElement firstChildElement = firstChildNode.toElement();
                QString firstChildElementText = firstChildElement.text();
                if(firstChildElementText.split(",").count() == 4){
                    LatestCardIDList        << firstChildElementText.split(",").at(0);
                    LatestCardTypeList      << firstChildElementText.split(",").at(1);
                    LatestCardStatusList    << firstChildElementText.split(",").at(2);
                    LatestCardValidTimeList << firstChildElementText.split(",").at(3);
                }
            }else if(firstChildNode.nodeName() == "CardState"){
                qDebug() << "CardState";
                CardStateFlag = true;
                QDomElement firstChildElement = firstChildNode.toElement();
                QString firstChildElementText = firstChildElement.text();
                if(firstChildElementText.split(",").count() == 4){
                    CurrentCardIDList        << firstChildElementText.split(",").at(0);
                    CurrentCardTypeList      << firstChildElementText.split(",").at(1);
                    CurrentCardStatusList    << firstChildElementText.split(",").at(2);
                    CurrentCardValidTimeList << firstChildElementText.split(",").at(3);
                }
            }else if(firstChildNode.nodeName() == "Msg"){
                QDomElement firstChildElement = firstChildNode.toElement();
                QString firstChildElementText = firstChildElement.text().toUpper();
                if(firstChildElementText == "OK"){
                    isNetWorkNormal = true;
                }

                if(firstChildElement.hasAttribute("id")){
                    QString id = firstChildElement.attributeNode("id").value();
                    qDebug() << "id =" << id << ",error info =" << firstChildElementText;
                }
            }

            firstChildNode = firstChildNode.nextSibling();//下一个节点
        }
        if(CaptionFlag){
            CommonSetting::SettingSystemDateTime(NowTime);

            isGetCardIDList = true;
            // 开始启动事务
            QSqlDatabase::database().transaction();
            query.exec("delete from [dtm_identifiercardnumber_table]");//将原有卡号列表删除，重新下载最新卡号列表
            for(int i = 0; i < LatestCardIDList.count(); i++){
                query.exec(tr("INSERT INTO [dtm_identifiercardnumber_table] ([IdentifierCardNumber], [IdentifierCardNumberType], [OperateCount], [ValidTime]) VALUES (\"%1\",\"%2\",\"%3\",\"%4\");")
                           .arg(LatestCardIDList.at(i)).arg(LatestCardTypeList.at(i)).arg(LatestCardStatusList.at(i)).arg(LatestCardValidTimeList.at(i)));
                qDebug() << query.lastError().text();
            }
            // 提交事务，这个时候才是真正打开文件执行SQL语句的时候
            QSqlDatabase::database().commit();
        }

        if(CardStateFlag){
            CommonSetting::SettingSystemDateTime(NowTime);

            // 开始启动事务
            QSqlDatabase::database().transaction();
            for(int i = 0; i < CurrentCardIDList.count(); i++){
                query.exec(tr("UPDATE [dtm_identifiercardnumber_table] SET [IdentifierCardNumberType] = \"%1\", [OperateCount] = \"%2\", [ValidTime] = \"%3\" WHERE [IdentifierCardNumber] = \"%4\"")
                           .arg(CurrentCardTypeList.at(i)).arg(CurrentCardStatusList.at(i)).arg(CurrentCardValidTimeList.at(i)).arg(CurrentCardIDList.at(i)));
                qDebug() << query.lastError().text();
            }
            // 提交事务，这个时候才是真正打开文件执行SQL语句的时候
            QSqlDatabase::database().commit();
        }
    }
}

void TcpHelper::slotCloseConnection()
{
    qDebug() << "close connection";
    ConnectStateFlag = TcpHelper::DisConnectedState;
    tcpSocket->disconnectFromHost();
    tcpSocket->abort();
}

void TcpHelper::slotEstablishConnection()
{
    qDebug() << "connect to server succeed";
    ConnectStateFlag = TcpHelper::ConnectedState;
}

void TcpHelper::slotDisplayError(QAbstractSocket::SocketError socketError)
{
    switch(socketError){
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() << "QAbstractSocket::ConnectionRefusedError";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        qDebug() << "QAbstractSocket::RemoteHostClosedError";
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() << "QAbstractSocket::HostNotFoundError";
        break;
    default:
        qDebug() << "The following error occurred:"
                    + tcpSocket->errorString();
        break;
    }

    ConnectStateFlag = TcpHelper::DisConnectedState;
    tcpSocket->disconnectFromHost();
    tcpSocket->abort();
}

void TcpHelper::SendDataPackage(QString PathPrefix, QString CardID, QString TriggerTime)
{
    QString MessageMerge;
    QDomDocument dom;
    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    dom.appendChild(dom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = dom.createElement("Device");
    RootElement.setAttribute("ID",GlobalConfig::DeviceID);//设置属性
    RootElement.setAttribute("Mac",GlobalConfig::MAC);
    RootElement.setAttribute("Type","IdentifierCard");
    RootElement.setAttribute("Ver","1.2.0.83");
    dom.appendChild(RootElement);

    if(SendMsgTypeFlag == TcpHelper::Heart){
        //创建第一个子元素
        QDomElement firstChildElement = dom.createElement("Heart");

        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }else if(SendMsgTypeFlag == TcpHelper::DeviceHeart){
        //创建第一个子元素
        QDomElement firstChildElement = dom.createElement("DeviceHeart");
        firstChildElement.setAttribute("State","0");
        firstChildElement.setAttribute("CardListUpTime","2012-01-01 00:00:00");

        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }else if(SendMsgTypeFlag == TcpHelper::DoorStatusInfo){
        //创建第一个子元素
        QDomElement firstChildElement = dom.createElement("DeviceHeart");
        firstChildElement.setAttribute("State",CardID);
        firstChildElement.setAttribute("CardListUpTime","");

        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }else if(SendMsgTypeFlag == TcpHelper::OperationCmd){
        QDomElement firstChildElement = dom.createElement("OperationCmd");
        firstChildElement.setAttribute("Type","1");
        firstChildElement.setAttribute("CardID",CardID);
        firstChildElement.setAttribute("TriggerTime",TriggerTime);//2016-02-19 15:07:36
        //Base64_432503199006177691_2016-02-19_14-34-05.txt
        QString fileName = PathPrefix + "Base64_" + CardID + "_" + TriggerTime.replace(" ","_").replace(":","-") + ".txt";
        QString imgBase64 = CommonSetting::ReadFile(fileName);
        QDomText firstChildElementText = dom.createTextNode(imgBase64);//base64图片数据
        firstChildElement.appendChild(firstChildElementText);
        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }

    QTextStream Out(&MessageMerge);
    dom.save(Out,4);

    MessageMerge = CommonSetting::AddHeaderByte(MessageMerge);
#if defined(WITH_WRITE_FILE)
    CommonSetting::WriteCommonFile("/bin/TcpSendMessage.txt",MessageMerge);
#endif

    SendMsgBuffer.append(MessageMerge);
}

//tcp短连接和长连接通用
void TcpHelper::SendCommonCode(QString MessageMerge)
{
    if(GlobalConfig::TcpConnectType == GlobalConfig::ShortConnection){//短连接
        //建立连接
        tcpSocket->connectToHost(GlobalConfig::ServerIP,                               GlobalConfig::ServerPort.toUInt());
        CommonSetting::Sleep(300);
        if(ConnectStateFlag == TcpHelper::ConnectedState){
            tcpSocket->write(MessageMerge.toAscii());
            if(tcpSocket->waitForBytesWritten(1000)){
                DataSendStateFlag = TcpHelper::SendSucceed;
            }else{
                DataSendStateFlag = TcpHelper::SendFailed;
            }
        }else{
            qDebug() << "connect to server failed";
            DataSendStateFlag = TcpHelper::SendFailed;
        }

        //打印信息
        PareseSendMsgType();

        //断开连接
        CommonSetting::Sleep(1000);
        tcpSocket->disconnectFromHost();
        tcpSocket->abort();
    }else if(GlobalConfig::TcpConnectType == GlobalConfig::LongConnection){//长连接
        if(ConnectStateFlag == TcpHelper::ConnectedState){
            tcpSocket->write(MessageMerge.toAscii());
            if(tcpSocket->waitForBytesWritten(1000)){
                DataSendStateFlag = TcpHelper::SendSucceed;
            }else{
                DataSendStateFlag = TcpHelper::SendFailed;
            }

            PareseSendMsgType();
        }else if(ConnectStateFlag == TcpHelper::DisConnectedState){
            //建立连接
            tcpSocket->disconnectFromHost();
            tcpSocket->abort();

            tcpSocket->connectToHost(GlobalConfig::ServerIP,                               GlobalConfig::ServerPort.toUInt());
            CommonSetting::Sleep(300);
            if(ConnectStateFlag == TcpHelper::ConnectedState){
                tcpSocket->write(MessageMerge.toAscii());
                if(tcpSocket->waitForBytesWritten(1000)){
                    DataSendStateFlag = TcpHelper::SendSucceed;
                }else{
                    DataSendStateFlag = TcpHelper::SendFailed;
                }
            }else{
                qDebug() << "connect to server failed";
                DataSendStateFlag = TcpHelper::SendFailed;
            }

            PareseSendMsgType();
        }
    }
}

void TcpHelper::PareseSendMsgType()
{
    if(SendMsgTypeFlag == TcpHelper::Heart){
        if(DataSendStateFlag == TcpHelper::SendSucceed){
//            qDebug() << "Tcp:Send Heart Succeed";
        }else if(DataSendStateFlag == TcpHelper::SendFailed){
//            qDebug() << "Tcp:Send Heart Failed";
        }
    }else if(SendMsgTypeFlag == TcpHelper::DeviceHeart){
        if(DataSendStateFlag == TcpHelper::SendSucceed){
            qDebug() << "Tcp:Send DeviceHeart Succeed";
        }else if(DataSendStateFlag == TcpHelper::SendFailed){
            qDebug() << "Tcp:Send DeviceHeart Failed";
        }
    }else if(SendMsgTypeFlag == TcpHelper::DoorStatusInfo){
        if(DataSendStateFlag == TcpHelper::SendSucceed){
            qDebug() << "Tcp:Send DoorStatusInfo Succeed";
        }else if(DataSendStateFlag == TcpHelper::SendFailed){
            qDebug() << "Tcp:Send DoorStatusInfo Failed";
        }
    }else if(SendMsgTypeFlag == TcpHelper::OperationCmd){
        if(DataSendStateFlag == TcpHelper::SendSucceed){
            qDebug() << "Tcp:Send Base64 Image Data Succeed";
        }else if(DataSendStateFlag == TcpHelper::SendFailed){
            qDebug() << "Tcp:Send Base64 Image Data Failed";
        }
    }
}

void TcpHelper::slotSendLogInfo(QString info)
{
    dirWatcher->removePath("/sdcard");
    CommonSetting::Sleep(1000);

    QStringList tempCardIDList;
    QStringList tempTriggerTimeList;
    char txFailedCount = 0;//用来统计发送失败次数
    char txSucceedCount = 0;//用来统计发送成功的次数

    QStringList dirlist = CommonSetting::GetDirNames("/sdcard");
    if(!dirlist.isEmpty()){
        foreach(const QString &dirName,dirlist){
            QStringList filelist =
                    CommonSetting::GetFileNames("/sdcard/" + dirName,"*.txt");
            if(!filelist.isEmpty()){
                foreach(const QString &fileName,filelist){
                    //Base64_432503199006177691_2016-02-19_14-34-05.txt
                    tempCardIDList << fileName.split("_").at(1);
                    QString Date = fileName.split("_").at(2);
                    QString Time = fileName.split("_").at(3).split(".").at(0);
                    Time = Time.replace(QRegExp("-"),QString(":"));
                    tempTriggerTimeList << Date + " " + Time;
                }
                SendMsgTypeFlag = TcpHelper::OperationCmd;
                SendDataPackage(QString("/sdcard/") + dirName + QString("/"),tempCardIDList.join(","),tempTriggerTimeList.join(","));

                CommonSetting::Sleep(500);
                //日志信息发送成功:删除本地记录
                if(DataSendStateFlag == TcpHelper::SendSucceed){
                    system(tr("rm -rf /sdcard/%1").arg(dirName).toAscii().data());
                    txSucceedCount++;
                    if(txSucceedCount == 5){
                        break;
                    }
                }else{
                    txFailedCount++;
                    if(txFailedCount == 5){
                        qDebug() << "连续发送五次都没有成功，表示网络离线\n";
                        break;
                    }
                }
                tempCardIDList.clear();
                tempTriggerTimeList.clear();
                CommonSetting::Sleep(1000);
            }
        }
    }

    dirWatcher->addPath("/sdcard");
}
