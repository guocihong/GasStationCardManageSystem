#include "tcpcommunicate.h"
#define WITH_WRITE_FILE1

TcpCommunicate::TcpCommunicate(QObject *parent) :
    QObject(parent)
{
    isGetCardIDList = false;

    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket,SIGNAL(readyRead()),
            this,SLOT(slotParseServerMessage()));
    connect(tcpSocket,SIGNAL(disconnected()),
            this,SLOT(slotCloseConnection()));
    connect(tcpSocket,SIGNAL(connected()),
            this,SLOT(slotEstablishConnection()));
    connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotDisplayError(QAbstractSocket::SocketError)));

    ConnectStateFlag = TcpCommunicate::DisConnectedState;

    ServerIpAddress = CommonSetting::ReadSettings("/bin/config.ini","ServerNetwork/IP");
    ServerListenPort = CommonSetting::ReadSettings("/bin/config.ini","ServerNetwork/PORT");

    HeartTimer = new QTimer(this);
    quint32 HeartIntervalTime =
            CommonSetting::ReadSettings("/bin/config.ini","time/HeartIntervalTime").toUInt() * 60 * 1000;
    HeartTimer->setInterval(HeartIntervalTime);
    connect(HeartTimer,SIGNAL(timeout()),this,SLOT(slotSendHeartData()));

    GetCardIDListTimer = new QTimer(this);
    GetCardIDListTimer->setInterval(3000);
    connect(GetCardIDListTimer,SIGNAL(timeout()),this,SLOT(slotGetIDList()));

    CheckNetWorkTimer = new QTimer(this);
    CheckNetWorkTimer->setInterval(1000);
    connect(CheckNetWorkTimer,SIGNAL(timeout()),this,SLOT(slotCheckNetWorkState()));

    slotSendHeartData();//开机发送心跳

    HeartTimer->start();
    GetCardIDListTimer->start();
    CheckNetWorkTimer->start();

    //创建一个监视器，用于监视目录/mnt的变化
    dirWatcher = new QFileSystemWatcher(this);
    dirWatcher->addPath("/mnt");
    connect(dirWatcher,SIGNAL(directoryChanged(QString)), this,SLOT(slotSendLogInfo(QString)));
}

void TcpCommunicate::slotSendHeartData()
{
    SendMsgTypeFlag = TcpCommunicate::DeviceHeart;
    SendDataPackage("","","");

    CommonSetting::Sleep(1000);
    tcpSocket->disconnectFromHost();
    tcpSocket->abort();
}

void TcpCommunicate::slotGetIDList()
{
    if(!isGetCardIDList){
        SendMsgTypeFlag = TcpCommunicate::DeviceHeart;
        SendDataPackage("","","");
        qDebug() << "isGetCardIDList";
    }else{
        GetCardIDListTimer->stop();
        CommonSetting::Sleep(1000);
        tcpSocket->disconnectFromHost();
        tcpSocket->abort();
        qDebug() << "stop GetCardIDListTimer";
    }
}

void TcpCommunicate::slotCheckNetWorkState()
{
    system("ifconfig eth0 > /bin/network_info.txt");
    QString network_info =
            CommonSetting::ReadFile("/bin/network_info.txt");
    if(!network_info.contains("RUNNING")){
        ConnectStateFlag = TcpCommunicate::DisConnectedState;
    }
}

void TcpCommunicate::slotParseServerMessage()
{
    CommonSetting::Sleep(300);

    QByteArray data = tcpSocket->readAll();
    QString XmlData = QString(data).mid(20);
#if defined(WITH_WRITE_FILE)
    CommonSetting::WriteXmlFile("/bin/TcpReceiveMessage.txt",XmlData);
#endif
    ParseServerMessage(XmlData);//解析服务返回信息
}

void TcpCommunicate::ParseServerMessage(QString XmlData)
{
    QString NowTime,CardListUpTime;
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
            NowTime = CardListUpTime =
                    RootElement.attributeNode("NowTime").value();
            CommonSetting::SettingSystemDateTime(NowTime);
        }

        QDomNode firstChildNode =
                RootElement.firstChild();//第一个子节点
        while(!firstChildNode.isNull()){
            if(firstChildNode.nodeName() == "Caption"){
                qDebug() << "Caption";
                CaptionFlag = true;
                QDomElement firstChildElement =
                        firstChildNode.toElement();
                QString firstChildElementText =
                        firstChildElement.text();
                if(firstChildElementText.split(",").count() == 4){
                    LatestCardIDList <<
                                        firstChildElementText.split(",").at(0);
                    LatestCardTypeList <<
                                          firstChildElementText.split(",").at(1);
                    LatestCardStatusList <<
                                            firstChildElementText.split(",").at(2);
                    LatestCardValidTimeList <<
                                               firstChildElementText.split(",").at(3);
                }
            }else if(firstChildNode.nodeName() == "CardState"){
                qDebug() << "CardState";
                CardStateFlag = true;
                QDomElement firstChildElement =
                        firstChildNode.toElement();
                QString firstChildElementText =
                        firstChildElement.text();
                if(firstChildElementText.split(",").count() == 4){
                    CurrentCardIDList <<
                                         firstChildElementText.split(",").at(0);
                    CurrentCardTypeList <<
                                           firstChildElementText.split(",").at(1);
                    CurrentCardStatusList <<
                                             firstChildElementText.split(",").at(2);
                    CurrentCardValidTimeList <<
                                                firstChildElementText.split(",").at(3);
                }
            }

            firstChildNode =
                    firstChildNode.nextSibling();//下一个节点
        }
        if(CaptionFlag){
            isGetCardIDList = true;
            CommonSetting::WriteSettings("/bin/config.ini","time/CardListUpTime",CardListUpTime);
            // 开始启动事务
            QSqlDatabase::database().transaction();
            query.exec("delete from [卡号表]");//将原有卡号列表删除，重新下载最新卡号列表
            for(int i = 0; i < LatestCardIDList.count(); i++){
                query.exec(tr("INSERT INTO [卡号表] ([卡号], [功能号], [状态], [有效期限]) VALUES (\"%1\",\"%2\",\"%3\",\"%4\");")
                           .arg(LatestCardIDList.at(i)).arg(LatestCardTypeList.at(i)).arg(LatestCardStatusList.at(i)).arg(LatestCardValidTimeList.at(i)));
//                qDebug() << query.lastError().text();
            }
            // 提交事务，这个时候才是真正打开文件执行SQL语句的时候
            QSqlDatabase::database().commit();
        }

        if(CardStateFlag){
            // 开始启动事务
            QSqlDatabase::database().transaction();
            for(int i = 0; i < CurrentCardIDList.count(); i++){
                query.exec(tr("UPDATE [卡号表] SET [功能号] = \"%1\", [状态] = \"%2\", [有效期限] = \"%3\" WHERE [卡号] = \"%4\"")
                           .arg(CurrentCardTypeList.at(i)).arg(CurrentCardStatusList.at(i)).arg(CurrentCardValidTimeList.at(i)).arg(CurrentCardIDList.at(i)));
//                qDebug() << query.lastError().text();
            }
            // 提交事务，这个时候才是真正打开文件执行SQL语句的时候
            QSqlDatabase::database().commit();
        }
    }
}

void TcpCommunicate::slotCloseConnection()
{
    qDebug() << "close connection";
    ConnectStateFlag = TcpCommunicate::DisConnectedState;
    tcpSocket->abort();
}

void TcpCommunicate::slotEstablishConnection()
{
    qDebug() << "connect to server succeed";
    ConnectStateFlag = TcpCommunicate::ConnectedState;
}

void TcpCommunicate::slotDisplayError(QAbstractSocket::SocketError socketError)
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

    ConnectStateFlag = TcpCommunicate::DisConnectedState;
    tcpSocket->abort();
}

void TcpCommunicate::SendDataPackage(QString PathPrefix, QString CardID, QString TriggerTime)
{
    QString MessageMerge;
    QDomDocument dom;
    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    dom.appendChild(dom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement =
            dom.createElement("Device");

    QString DeviceID =
            CommonSetting::ReadSettings("/bin/config.ini","DeviceID/ID");
    QString MacAddress =
            CommonSetting::ReadMacAddress();
    QString CardListUpTime =
            CommonSetting::ReadSettings("/bin/config.ini","time/CardListUpTime");

    RootElement.setAttribute("ID",DeviceID);//设置属性
    RootElement.setAttribute("Mac",MacAddress);
    RootElement.setAttribute("Type","IdentifierCard");
    RootElement.setAttribute("Ver","1.2.0.66");

    dom.appendChild(RootElement);

    if(SendMsgTypeFlag == TcpCommunicate::DeviceHeart){
        //创建第一个子元素
        QDomElement firstChildElement = dom.createElement("DeviceHeart");
        firstChildElement.setAttribute("State","1");
        //        firstChildElement.setAttribute("CardListUpTime",CardListUpTime);
        firstChildElement.setAttribute("CardListUpTime","2012-01-01 00:00:00");

        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }else if(SendMsgTypeFlag == TcpCommunicate::OperationCmd){
        QDomElement firstChildElement =
                dom.createElement("OperationCmd");
        firstChildElement.setAttribute("Type","7");
        firstChildElement.setAttribute("CardID",CardID);
        firstChildElement.setAttribute("TriggerTime",TriggerTime);
        QString fileName = PathPrefix + "Base64_" + CardID + "_" + TriggerTime + ".txt";
        QString imgBase64 =
                CommonSetting::ReadFile(fileName);
        QDomText firstChildElementText =
                dom.createTextNode(imgBase64);//base64图片数据
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
    SendCommonCode(MessageMerge);
}

//tcp短连接
void TcpCommunicate::SendCommonCode(QString MessageMerge)
{
    if(ConnectStateFlag == TcpCommunicate::ConnectedState){      
        tcpSocket->write(MessageMerge.toAscii());
        if(tcpSocket->waitForBytesWritten(1000)){
            DataSendStateFlag = TcpCommunicate::SendSucceed;
            PareseSendMsgType();
        }
        return;
    }else if(ConnectStateFlag == TcpCommunicate::DisConnectedState){
        tcpSocket->disconnectFromHost();
        tcpSocket->abort();
        for(int i = 0; i < 3; i++){           
            tcpSocket->connectToHost(ServerIpAddress,
                                     ServerListenPort.toUInt());
            CommonSetting::Sleep(1000);
            if(ConnectStateFlag == TcpCommunicate::ConnectedState){
                tcpSocket->write(MessageMerge.toAscii());
                if(tcpSocket->waitForBytesWritten(1000)){
                    DataSendStateFlag = TcpCommunicate::SendSucceed;
                    PareseSendMsgType();
                }
                return;
            }
            tcpSocket->disconnectFromHost();
            tcpSocket->abort();
        }
        DataSendStateFlag = TcpCommunicate::SendFailed;
        qDebug() << "上传失败:与服务器连接未成功。请检查网线是否插好,本地网络配置,服务器IP,服务器监听端口配置是否正确.\n";
    }
}

void TcpCommunicate::PareseSendMsgType()
{
    if(SendMsgTypeFlag == TcpCommunicate::DeviceHeart){
        qDebug() << "Tcp:Send DeviceHeart Succeed";
    }else if(SendMsgTypeFlag == TcpCommunicate::OperationCmd){
        qDebug() << "Tcp:Picture And CardID Already Succeed Send";
    }
}

void TcpCommunicate::slotSendLogInfo(QString info)
{
    dirWatcher->removePath("/mnt");

    tcpSocket->connectToHost(ServerIpAddress,ServerListenPort.toUInt());
    tcpSocket->waitForConnected();

    QStringList tempCardIDList;
    QStringList tempTriggerTimeList;
    char txFailedCount = 0;//用来统计发送失败次数
    char txSucceedCount = 0;//用来统计发送成功的次数

    QStringList dirlist = CommonSetting::GetDirNames("/mnt");
    if(!dirlist.isEmpty()){
        foreach(const QString &dirName,dirlist){
            QStringList filelist =
                    CommonSetting::GetFileNames("/mnt/" + dirName,"*.txt");
            if(!filelist.isEmpty()){
                foreach(const QString &fileName,filelist){
                    tempCardIDList << fileName.split("_").at(1);
                    QString temp = fileName.split("_").at(2);
                    tempTriggerTimeList << temp.split(".").at(0);
                }
                SendMsgTypeFlag = TcpCommunicate::OperationCmd;
                SendDataPackage(QString("/mnt/") + dirName + QString("/"),tempCardIDList.join(","),tempTriggerTimeList.join(","));
                //日志信息发送成功:删除本地记录
                if(DataSendStateFlag == TcpCommunicate::SendSucceed){
                    system(tr("rm -rf /mnt/%1").arg(dirName).toAscii().data());
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

    CommonSetting::Sleep(1000);
    tcpSocket->disconnectFromHost();
    tcpSocket->abort();

    dirWatcher->addPath("/mnt");
}
