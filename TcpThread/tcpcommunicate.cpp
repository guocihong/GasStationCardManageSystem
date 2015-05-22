#include "tcpcommunicate.h"
#define WITH_DEBUG1
#define WITH_WRITE_FILE1

TcpCommunicate::TcpCommunicate(QObject *parent) :
    QObject(parent)
{
#if defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::TcpCommunicate" << QThread::currentThreadId();
#endif

    isGetCardIDList = false;
    isGetCardOrderList = false;

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
    tcpSocket->connectToHost(ServerIpAddress,ServerListenPort.toUInt());
    CommonSetting::Sleep(1000);

    HeartTimer = new QTimer(this);
    quint32 HeartIntervalTime =
            CommonSetting::ReadSettings("/bin/config.ini","time/HeartIntervalTime").toUInt() * 60 * 1000;
    HeartTimer->setInterval(HeartIntervalTime);
    connect(HeartTimer,SIGNAL(timeout()),this,SLOT(slotSendHeartData()));

    GetCardIDListTimer = new QTimer(this);
    GetCardIDListTimer->setInterval(3000);
    connect(GetCardIDListTimer,SIGNAL(timeout()),this,SLOT(slotGetIDList()));

    CheckNetWorkTimer = new QTimer(this);
    CheckNetWorkTimer->setInterval(10000);
    connect(CheckNetWorkTimer,SIGNAL(timeout()),this,SLOT(slotCheckNetWorkState()));

    CommonSetting::Sleep(3000);
    slotSendHeartData();//开机发送心跳和slotGetCardTypeName

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
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotSendHeartData" <<
                QThread::currentThreadId();
#endif

    SendMsgTypeFlag = TcpCommunicate::DeviceHeart;
    SendDataPackage("","","");

    CommonSetting::Sleep(1000);
    slotGetCardTypeName();
}

void TcpCommunicate::slotGetIDList()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotGetIDList" <<
                QThread::currentThreadId();
#endif

    if(!isGetCardIDList){
        SendMsgTypeFlag = TcpCommunicate::DeviceHeart;
        SendDataPackage("","","");
        qDebug() << "isGetCardIDList";
    }else if(!isGetCardOrderList){
        SendMsgTypeFlag = TcpCommunicate::GetCardTypeName;
        SendDataPackage("","","");
        qDebug() << "isGetCardOrderList";
    }else{
        GetCardIDListTimer->stop();
        qDebug() << "stop GetCardIDListTimer";
    }
}

//这样命令仅仅只是为了方便
void TcpCommunicate::slotGetCardTypeName()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotGetCardTypeName" <<
                QThread::currentThreadId();
#endif

    SendMsgTypeFlag = TcpCommunicate::GetCardTypeName;
    SendDataPackage("","","");
}

void TcpCommunicate::slotCheckNetWorkState()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotCheckNetWorkState" <<
                QThread::currentThreadId();
#endif

    system("ifconfig eth0 > /bin/network_info.txt");
    QString network_info =
            CommonSetting::ReadFile("/bin/network_info.txt");
    if(!network_info.contains("RUNNING")){
        ConnectStateFlag = TcpCommunicate::DisConnectedState;
    }
}

void TcpCommunicate::slotParseServerMessage()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotParseServerMessage" <<
                QThread::currentThreadId();
#endif
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
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::ParseServerMessage" <<
                QThread::currentThreadId();
#endif

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
#if  defined(WITH_DEBUG)
        qDebug() << "Parse error at line " << errorLine <<","<< "column " << errorColumn << ","<< errorMsg;
#endif
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
                LatestCardIDList <<
                                    firstChildElementText.split(",").at(0);
                LatestCardTypeList <<
                                      firstChildElementText.split(",").at(1);
                LatestCardStatusList <<
                                        firstChildElementText.split(",").at(2);
                LatestCardValidTimeList <<
                                           firstChildElementText.split(",").at(3);
            }else if(firstChildNode.nodeName() == "CardState"){
                qDebug() << "CardState";
                CardStateFlag = true;
                QDomElement firstChildElement =
                        firstChildNode.toElement();
                QString firstChildElementText =
                        firstChildElement.text();
                CurrentCardIDList <<
                                     firstChildElementText.split(",").at(0);
                CurrentCardTypeList <<
                                       firstChildElementText.split(",").at(1);
                CurrentCardStatusList <<
                                         firstChildElementText.split(",").at(2);
                CurrentCardValidTimeList <<
                                            firstChildElementText.split(",").at(3);

#if  defined(WITH_DEBUG)
                qDebug() << "CardState";
#endif
            }else if(firstChildNode.nodeName() == "FunctionTypeName"){
                qDebug() << "FunctionTypeName";
                isGetCardOrderList = true;
                QDomElement firstChildElement =
                        firstChildNode.toElement();
                QString firstChildElementText =
                        firstChildElement.text();
                QStringList FunctionList =
                        firstChildElementText.split(";");
                // 开始启动事务
                QSqlDatabase::database().transaction();

                query.exec("delete from [功能表]");
                for(int i = 0; i < FunctionList.count();  i++){
                    QString FunctionNum = FunctionList.at(i).split(",").at(0);//功能号
                    QString ActionNum = FunctionList.at(i).split(",").at(2).split("#").at(0);//动作号
                    QString CardOrder = FunctionList.at(i).split("#").at(1);//刷卡序列
                    query.exec(tr("INSERT INTO [功能表] ([功能号], [动作号],[刷卡序列]) VALUES (\"%1\",\"%2\",\"%3\");").arg(FunctionNum).arg(ActionNum).arg(CardOrder));
#if  defined(WITH_DEBUG)
                    qDebug() << query.lastError().text();
#endif
                }
                // 提交事务，这个时候才是真正打开文件执行SQL语句的时候
                QSqlDatabase::database().commit();
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
#if  defined(WITH_DEBUG)
                qDebug() << query.lastError().text();
#endif
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
#if  defined(WITH_DEBUG)
                qDebug() << query.lastError().text();
#endif
            }
            // 提交事务，这个时候才是真正打开文件执行SQL语句的时候
            QSqlDatabase::database().commit();
        }
    }
}

void TcpCommunicate::slotCloseConnection()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotCloseConnection" <<
                QThread::currentThreadId();

    qDebug() << "server close connection!\n";
#endif

    ConnectStateFlag = TcpCommunicate::DisConnectedState;
    tcpSocket->abort();
}

void TcpCommunicate::slotEstablishConnection()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotEstablishConnection" <<
                QThread::currentThreadId();

    qDebug() << "connect to server succeed!\n";
#endif

    ConnectStateFlag = TcpCommunicate::ConnectedState;

}

void TcpCommunicate::slotDisplayError(QAbstractSocket::SocketError socketError)
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotDisplayError" <<
                QThread::currentThreadId();
#endif

    switch(socketError){
    case QAbstractSocket::ConnectionRefusedError:
#if  defined(WITH_DEBUG)
        qDebug() << "QAbstractSocket::ConnectionRefusedError\n";
#endif
        break;
    case QAbstractSocket::RemoteHostClosedError:
#if  defined(WITH_DEBUG)
        qDebug() << "QAbstractSocket::RemoteHostClosedError\n";
#endif
        break;
    case QAbstractSocket::HostNotFoundError:
#if  defined(WITH_DEBUG)
        qDebug() << "QAbstractSocket::HostNotFoundError\n";
#endif
        break;
    default:
#if  defined(WITH_DEBUG)
        qDebug() << "The following error occurred:"
                    + tcpSocket->errorString();
#endif
        break;
    }

    ConnectStateFlag = TcpCommunicate::DisConnectedState;
    tcpSocket->abort();
}

void TcpCommunicate::SendDataPackage(QString PathPrefix,QString CardID,QString TriggerTime)
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::SendDataPackage" <<
                QThread::currentThreadId();
#endif

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
    RootElement.setAttribute("Type","ICCard");
    RootElement.setAttribute("Ver","1.2.0.36");

    dom.appendChild(RootElement);

    if(SendMsgTypeFlag == TcpCommunicate::DeviceHeart){
        //创建第一个子元素
        QDomElement firstChildElement = dom.createElement("DeviceHeart");
        firstChildElement.setAttribute("State","1");
//        firstChildElement.setAttribute("CardListUpTime",CardListUpTime);
        firstChildElement.setAttribute("CardListUpTime","2012-01-01 00:00:00");

        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }else if(SendMsgTypeFlag == TcpCommunicate::GetCardTypeName){
        //创建第一个子元素
        QDomElement firstChildElement =
                dom.createElement("GetCardTypeName");
        //将元素添加到根元素后面
        RootElement.appendChild(firstChildElement);
    }else if(SendMsgTypeFlag == TcpCommunicate::OperationCmd){
        QStringList CardIDList = CardID.split(",");
        QStringList TriggerTimeList = TriggerTime.split(",");
        QString CardType;

        query.exec(tr("SELECT [功能号] from [卡号表] where [卡号] = \"%1\"").arg(CardIDList.at(0)));
        while(query.next()){
            CardType = query.value(0).toString();
        }

        for(int i = 0; i < CardIDList.count(); i++){
            QDomElement firstChildElement =
                    dom.createElement("OperationCmd");
            firstChildElement.setAttribute("Type",CardType);
            firstChildElement.setAttribute("CardID",CardIDList.at(i));
            firstChildElement.setAttribute("TriggerTime",TriggerTimeList.at(i));
            QString fileName = PathPrefix + "Base64_" + CardIDList.at(i) + "_" + TriggerTimeList.at(i) + ".txt";
            QString imgBase64 =
                    CommonSetting::ReadFile(fileName);
            QDomText firstChildElementText =
                    dom.createTextNode(imgBase64);//base64图片数据
            firstChildElement.appendChild(firstChildElementText);
            //将元素添加到根元素后面
            RootElement.appendChild(firstChildElement);
        }
    }

    QTextStream Out(&MessageMerge);
    dom.save(Out,4);

    MessageMerge = CommonSetting::AddHeaderByte(MessageMerge);
#if defined(WITH_WRITE_FILE)
    CommonSetting::WriteCommonFile("/bin/TcpSendMessage.txt",MessageMerge);
#endif
    SendCommonCode(MessageMerge);
}

//tcp长连接
void TcpCommunicate::SendCommonCode(QString MessageMerge)
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::SendCommonCode" <<
                QThread::currentThreadId();
#endif

    if(ConnectStateFlag == TcpCommunicate::ConnectedState){
#if  defined(WITH_DEBUG)
        qDebug() << "TcpCommunicate::ConnectedState";
#endif
        tcpSocket->write(MessageMerge.toAscii());
        DataSendStateFlag = TcpCommunicate::SendSucceed;
        PareseSendMsgType();
    }else if(ConnectStateFlag ==
             TcpCommunicate::DisConnectedState){
#if  defined(WITH_DEBUG)
        qDebug() << "TcpCommunicate::DisConnectedState";
#endif
        tcpSocket->disconnectFromHost();
        tcpSocket->abort();
        for(int i = 0; i < 3; i++){
            tcpSocket->connectToHost(ServerIpAddress,
                                     ServerListenPort.toUInt());
            CommonSetting::Sleep(1000);
            if(ConnectStateFlag == TcpCommunicate::ConnectedState){
                tcpSocket->write(MessageMerge.toAscii());
                DataSendStateFlag = TcpCommunicate::SendSucceed;
                PareseSendMsgType();
                return;
            }
            tcpSocket->disconnectFromHost();
            tcpSocket->abort();
        }
        DataSendStateFlag = TcpCommunicate::SendFailed;
#if  defined(WITH_DEBUG)
        qDebug() << "上传失败:与服务器连接未成功。请检查网线是否插好,本地网络配置,服务器IP,服务器监听端口配置是否正确.\n";
#endif
    }
}

void TcpCommunicate::PareseSendMsgType()
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::PareseSendMsgType" <<
                QThread::currentThreadId();
#endif
    if((SendMsgTypeFlag == TcpCommunicate::DeviceHeart) ||
            (SendMsgTypeFlag == TcpCommunicate::GetCardTypeName)){
#if  defined(WITH_DEBUG)
        qDebug() << "Tcp:Send DeviceHeart/GetCardTypeName Succeed\n";
#endif
    }else if(SendMsgTypeFlag == TcpCommunicate::OperationCmd){
#if  defined(WITH_DEBUG)
        qDebug() << "Tcp:Picture And CardID Already Succeed Send!\n";
#endif
    }
}

void TcpCommunicate::slotSendLogInfo(QString info)
{
#if  defined(WITH_DEBUG)
    qDebug() << "TcpCommunicate::slotSendLogInfo" <<
                QThread::currentThreadId();
#endif
    dirWatcher->removePath("/mnt");

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
#if defined(WITH_DEBUG)
                        qDebug() << "连续发送五次都没有成功，表示网络离线\n";
#endif
                        break;
                    }
                }
                tempCardIDList.clear();
                tempTriggerTimeList.clear();
                CommonSetting::Sleep(1000);
            }
        }
    }

    dirWatcher->addPath("/mnt");
}
