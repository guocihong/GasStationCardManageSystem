#ifndef COMMONSETTING_H
#define COMMONSETTING_H
#include <QObject>
#include <QBuffer>
#include <QComboBox>
#include <QListWidget>
#include <QDomDocument>
#include <QSettings>
#include <QSqlError>
#include <QMutex>
#include <QFileSystemWatcher>
#include <QDesktopServices>
#include <QWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QTcpServer>
#include <QTcpSocket>
#include <QStringList>
#include <QDesktopWidget>
#include <QList>
#include <QThread>
#include <QNetworkInterface>
#include <QFile>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextCodec>
#include <QMessageBox>
#include <QAbstractButton>
#include <QPushButton>
#include <QTime>
#include <QDateTime>
#include <QDate>
#include <QCoreApplication>
#include <QFileDialog>
#include <QProcess>
#include <QUrl>
#include <QDir>
#include <QCursor>
#include <QTimer>
#include <QLabel>
#include <QSound>
#include <QApplication>
#include <QStyleFactory>
#include <QTextStream>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <stdio.h>
#include <stdlib.h>

class CommonSetting : public QObject
{
public:
    CommonSetting();
    ~CommonSetting();

    //设置编码为GB2312
    static void SetGB2312Code()
    {
        QTextCodec *codec = QTextCodec::codecForName("GB2312");
        QTextCodec::setCodecForLocale(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForTr(codec);
    }

    //设置编码为UTF8
    static void SetUTF8Code()
    {
        QTextCodec *codec= QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForTr(codec);
    }

    static QString Unicode2UTF8(QString t)
    {
//        qDebug() << "Convert Before:" << t;
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QStringList s;
        for(int i = 0;i < t.length();i += 4) {
            s.append(t.mid(i,4));
        }
        QString t1;
        foreach (const QString &t0, s) {
            t1.append(t0.toUShort(0,16));
        }
        QString re = codec->fromUnicode(t1);
        QString utf8 = QString::fromUtf8(re.toAscii().data());
//        qDebug() << "Convert After:" << utf8;
        return utf8;
    }

    static void OpenDataBase()
    {
        QString dbFile = CommonSetting::GetCurrentPath() + QString("database/DataBase.db");
        if(!CommonSetting::FileExists(dbFile)){
            qDebug() << "数据库不存在，程序自动关闭";
            exit(-1);
        }
        QSqlDatabase DbConn =
                QSqlDatabase::addDatabase("QSQLITE");
        DbConn.setDatabaseName(dbFile);

        if(!DbConn.open()){
            qDebug() << "打开数据库失败！程序将自动关闭！";
            exit(-1);
        }
    }


    //延时处理
    static void DelayMs(int msc)
    {
        QTime t = QTime::currentTime().addMSecs(msc);
        while(QTime::currentTime() < t){
            QCoreApplication::processEvents(QEventLoop::AllEvents,100);
        }
    }

    //获取当前日期时间星期
    static QString GetCurrentDateTime()
    {
        QDateTime time = QDateTime::currentDateTime();
        return time.toString("yyyy-MM-dd_hh-mm-ss");
    }

    static QString GetCurrentDateTimeNoSpace()
    {
        QDateTime time = QDateTime::currentDateTime();
        return time.toString("yyyy-MM-dd_hh:mm:ss");
    }

    //读取文件内容
    static QString ReadFile(QString fileName)
    {
        QFile file(fileName);
        QByteArray fileContent;
        if (file.open(QIODevice::ReadOnly)){
            fileContent = file.readAll();
        }
        file.close();
        return QString(fileContent);
    }

    //写数据到文件
    static void WriteCommonFile(QString fileName,QString data)
    {
        QFile file(fileName);
        if(file.open(QFile::ReadWrite | QFile::Append)){
            file.write(data.toLocal8Bit().data());
            file.close();
        }
    }

    static void WriteCommonFileTruncate(QString fileName,QString data)
    {
        QFile file(fileName);
        if(file.open(QFile::ReadWrite | QFile::Truncate)){
            file.write(data.toAscii().data());
            file.close();
        }
    }

    static void WriteXmlFile(QString fileName,QString data)
    {
        QFile file(fileName);
        if(file.open(QFile::ReadWrite | QFile::Append)){
            QDomDocument dom;
            QTextStream out(&file);
            out.setCodec("UTF-8");
            dom.setContent(data);
            dom.save(out,4,QDomNode::EncodingFromTextStream);
            file.close();
        }
    }

    //创建文件夹
    static void CreateFolder(QString path,QString strFolder)
    {
        QDir dir(path);
        dir.mkdir(strFolder);
    }

    //删除文件夹
    static bool deleteFolder(const QString &dirName)
    {
        QDir directory(dirName);
        if (!directory.exists()){
            return true;
        }

        QString srcPath =
                QDir::toNativeSeparators(dirName);
        if (!srcPath.endsWith(QDir::separator()))
            srcPath += QDir::separator();

        QStringList fileNames =
                directory.entryList(QDir::AllEntries |
                                    QDir::NoDotAndDotDot | QDir::Hidden);
        bool error = false;
        for (QStringList::size_type i=0; i != fileNames.size(); ++i){
            QString filePath = srcPath +
                    fileNames.at(i);
            QFileInfo fileInfo(filePath);
            if(fileInfo.isFile() ||
                    fileInfo.isSymLink()){
                QFile::setPermissions(filePath, QFile::WriteOwner);
                if (!QFile::remove(filePath)){
                    qDebug() << "remove file" << filePath << " faild!";
                    error = true;
                }
            }else if (fileInfo.isDir()){
                if (!deleteFolder(filePath)){
                    error = true;
                }
            }
        }

        if (!directory.rmdir(
                    QDir::toNativeSeparators(
                        directory.path()))){
            qDebug() << "remove dir" << directory.path() << " faild!";
            error = true;
        }

        return !error;
    }

    //拷贝文件：
    static bool copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist)
    {
        if (sourceDir == toDir){
            return true;
        }
        if (!QFile::exists(sourceDir)){
            return false;
        }
        QDir *createfile     = new QDir;
        bool exist = createfile->exists(toDir);
        if (exist){
            if(coverFileIfExist){
                createfile->remove(toDir);
            }
        }//end if

        if(!QFile::copy(sourceDir, toDir))
        {
            return false;
        }
        return true;
    }

    //拷贝文件夹
    static bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
    {
        QDir sourceDir(fromDir);
        QDir targetDir(toDir);
        if(!targetDir.exists()){//如果目标目录不存在,则进行创建
            if(!targetDir.mkdir(
                        targetDir.absolutePath()))
                return false;
        }

        QFileInfoList fileInfoList =
                sourceDir.entryInfoList();
        foreach(QFileInfo fileInfo, fileInfoList){
            if(fileInfo.fileName() == "." ||
                    fileInfo.fileName() == "..")
                continue;

            if(fileInfo.isDir()){//当为目录时，递归的进行copy
                if(!copyDirectoryFiles(
                            fileInfo.filePath(),
                            targetDir.filePath(
                                fileInfo.fileName()),
                            coverFileIfExist))
                    return false;
            }else{//当允许覆盖操作时，将旧文件进行删除操作
                if(coverFileIfExist && targetDir.exists(
                            fileInfo.fileName())){
                    targetDir.remove(
                                fileInfo.fileName());
                }

                //进行文件copy
                if(!QFile::copy(fileInfo.filePath(),
                                targetDir.filePath(
                                    fileInfo.fileName()))){
                    return false;
                }
            }
        }
        return true;
    }

    //返回指定路径下符合筛选条件的文件，注意只返回文件名，不返回绝对路径
    static QStringList GetFileNames(QString path,QString filter)
    {
        QDir dir;
        dir.setPath(path);
        QStringList fileFormat(filter);
        dir.setNameFilters(fileFormat);
        dir.setFilter(QDir::Files);
        return dir.entryList();
    }
    //返回指定路径下文件夹的集合,注意只返回文件夹名，不返回绝对路径
    static QStringList GetDirNames(QString path)
    {
        QDir dir(path);
        dir.setFilter(QDir::Dirs);
        QStringList dirlist = dir.entryList();
        QStringList dirNames;
        foreach(const QString &dirName,dirlist){
            if(dirName == "." || dirName == "..")
                continue;
            dirNames << dirName;
        }
        return dirNames;
    }

    //返回文件文件时间
    static QString GetFileCreateTime(QString fileName)
    {
        QFileInfo file(fileName);
        QDateTime DateTime = file.created();
        return DateTime.toString("yyyy-MM-dd hh:mm:ss");
    }


    //将图片保存到QByteArray中
    static QByteArray SaveImageToQByteArrayInJPG(
            const QString &imageName)
    {
        QImage image(imageName);
        QByteArray tempData;
        QBuffer tempBuffer(&tempData);
        image.save(&tempBuffer,"JPG");
        return tempData;
    }

    //将JPG格式的图片数据保存到文件中
    static void SaveJPGDataToPic(const QByteArray &data,
                                 QString &picName)
    {
        QImage image;
        image.loadFromData(data);
        image.save(picName);
    }

    //QSetting应用
    static void WriteSettings(const QString &ConfigFile,
                              const QString &key,
                              const QString value)
    {

        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.setValue(key,value);
    }

    static QString ReadSettings(const QString &ConfigFile,
                                const QString &key)
    {
        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.setIniCodec("UTF-8");
        return settings.value(key).toString();
    }

    static QString ReadSettingsChinese(const QString &ConfigFile,
                                       const QString &key)
    {
        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.setIniCodec("UTF-8");
        return settings.value(key).toString();
    }

    static QStringList AllChildKeys(const QString &ConfigFile,const QString &beginGroup)
    {
        QSettings settings(ConfigFile,
                           QSettings::IniFormat);
        settings.beginGroup(beginGroup);
        return settings.childKeys();
    }

    static void RemoveSettingsKey(const QString &ConfigFile,
                                  const QString &key)
    {
        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.remove(key);
    }

    static void ClearSettings(const QString &ConfigFile)
    {
        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.clear();
    }

    //将指定路径下指定格式的文件返回(只返回文件名，不返回绝对路径)
    static QStringList fileFilter(const QString &path,
                                  const QString &filter)
    {
        QDir dir(path);
        QStringList fileFormat(filter);
        dir.setNameFilters(fileFormat);
        dir.setSorting(QDir::Time);
        dir.setFilter(QDir::Files);
        QStringList fileList = dir.entryList();
        return fileList;
    }


    //返回当前可执行程序的绝对路径,但不包括本身
    static QString GetExecutableProgramPath()
    {
        return QApplication::applicationDirPath();

    }

    //返回当前可执行程序的绝对路径,包括本身
    static QString GetExecutableProgramAbsolutePath()
    {
        return QApplication::applicationFilePath();
    }

    //获取当前路径
    static QString GetCurrentPath()
    {
        return QString(QCoreApplication::applicationDirPath()+"/");
    }

    static void QMessageBoxOnlyOk_Information(const QString &info)
    {
        QMessageBox msg;
        msg.setWindowTitle(tr("提示"));
        msg.setText(info);
        msg.setIcon(QMessageBox::Information);
        msg.addButton(tr("确定"),QMessageBox::ActionRole);
        msg.exec();
    }

    static void QMessageBoxOnlyOk_Error(const QString &info)
    {
        QMessageBox msg;
        msg.setWindowTitle(tr("提示"));
        msg.setStyleSheet("font:18pt '宋体'");
        msg.setText(info);
        msg.setIcon(QMessageBox::Critical);
        msg.addButton(tr("确定"),QMessageBox::ActionRole);
        msg.exec();
    }

    static void QMessageBoxOkCancel(const QString &info)
    {
        QMessageBox msg;
        msg.setWindowTitle(tr("提示"));
        msg.setText(info);
        msg.setIcon(QMessageBox::Information);
        msg.addButton(tr("确定"),QMessageBox::ActionRole);
        msg.addButton(tr("取消"),QMessageBox::ActionRole);
        msg.exec();
    }

    //显示错误框，仅确定按钮
    static void ShowMessageBoxError(QString info)
    {
        QMessageBox msg;
        msg.setStyleSheet("font:12pt '宋体'");
        msg.setWindowTitle(tr("错误"));
        msg.setText(info);
        msg.setIcon(QMessageBox::Critical);
        msg.addButton(tr("确定"),QMessageBox::ActionRole);
        msg.exec();
    }

    //窗体居中显示
    static void WidgetCenterShow(QWidget &frm)
    {
        QDesktopWidget desktop;
        int screenX = desktop.availableGeometry().width();
        int screenY = desktop.availableGeometry().height();
        int wndX = frm.width();
        int wndY = frm.height();
        QPoint movePoint(screenX/2-wndX/2,screenY/2-wndY/2);
        frm.move(movePoint);
    }

    //窗体没有最大化按钮
    static void FormNoMaxButton(QWidget &frm)
    {
        frm.setWindowFlags(Qt::WindowMinimizeButtonHint);
    }

    //窗体没有最大化和最小化按钮
    static void FormOnlyCloseButton(QWidget &frm)
    {
        frm.setWindowFlags(Qt::WindowMinMaxButtonsHint);
        frm.setWindowFlags(Qt::WindowCloseButtonHint);
    }

    //窗体不能改变大小
    static void FormNotResize(QWidget  &frm)
    {
        frm.setFixedSize(frm.width(),frm.height());
    }

    //获取桌面大小
    static QSize GetDesktopSize()
    {
        QDesktopWidget desktop;
        return
                QSize(desktop.availableGeometry().width(),desktop.availableGeometry().height());
    }

    //文件是否存在
    static bool FileExists(QString strFile)
    {
        QFile tempFile(strFile);
        if (tempFile.exists()){
            return true;
        }
        return false;
    }

    static QString ReadMacAddress()
    {
        QList<QNetworkInterface> list =
                QNetworkInterface::allInterfaces();
        foreach(const QNetworkInterface &interface,list){
            if(interface.name() == "eth0"){
                QString MacAddress =
                        interface.hardwareAddress();
                QStringList temp = MacAddress.split(":");
                return temp.join("");
            }
        }
    }

    static void SettingSystemDateTime(QString SystemDate)
    {
        //设置系统时间
        QString year = SystemDate.mid(0,4);
        QString month = SystemDate.mid(5,2);
        QString day = SystemDate.mid(8,2);
        QString hour = SystemDate.mid(11,2);
        QString minute = SystemDate.mid(14,2);
        QString second = SystemDate.mid(17,2);
        QProcess *process = new QProcess;
        process->start(tr("date %1%2%3%4%5.%6").arg(month).arg(day)
                       .arg(hour).arg(minute)
                       .arg(year).arg(second));
        process->waitForFinished(200);
        process->start("hwclock -w");
        process->waitForFinished(200);
        delete process;
    }

    static void Sleep(quint16 msec)
    {
        QTime dieTime = QTime::currentTime().addMSecs(msec);
        while(QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    static QString AddHeaderByte(QString MessageMerge)
    {
        //添加头20个字节:类似ICARD:00009871000000
        QString TotalLength =
                QString::number(MessageMerge.length() + 20);
        int len = TotalLength.length();
        for (int i = 0; i < 8 - len; i++)
        {
            TotalLength = "0" + TotalLength;
        }

        QString temp = QString("%1%2%3%4").arg("ICARD:")
                .arg(TotalLength)
                .arg("000000")
                .arg(MessageMerge);
        return temp;
    }

    //16进制字符串转字节数组
    static QByteArray HexStrToByteArray(QString str)
    {
        QByteArray senddata;
        int hexdata,lowhexdata;
        int hexdatalen = 0;
        int len = str.length();
        senddata.resize(len/2);
        char lstr,hstr;
        for(int i=0; i<len; )
        {
            hstr=str[i].toAscii();
            if(hstr == ' ')
            {
                i++;
                continue;
            }
            i++;
            if(i >= len)
                break;
            lstr = str[i].toAscii();
            hexdata = ConvertHexChar(hstr);
            lowhexdata = ConvertHexChar(lstr);
            if((hexdata == 16) || (lowhexdata == 16))
                break;
            else
                hexdata = hexdata*16+lowhexdata;
            i++;
            senddata[hexdatalen] = (char)hexdata;
            hexdatalen++;
        }
        senddata.resize(hexdatalen);
        return senddata;
    }

    static char ConvertHexChar(char ch)
    {
        if((ch >= '0') && (ch <= '9'))
            return ch-0x30;
        else if((ch >= 'A') && (ch <= 'F'))
            return ch-'A'+10;
        else if((ch >= 'a') && (ch <= 'f'))
            return ch-'a'+10;
        else return (-1);
    }

    //字节数组转16进制字符串
    static QString ByteArrayToHexStr(QByteArray data)
    {
        QString temp="";
        QString hex=data.toHex();
        for (int i=0;i<hex.length();i=i+2)
        {
            temp+=hex.mid(i,2)+" ";
        }
        return temp.trimmed().toUpper();
    }

    //16进制字符串转10进制
    static int StrHexToDecimal(QString strHex)
    {
        bool ok;
        return strHex.toInt(&ok,16);
    }

    //10进制字符串转10进制
    static int StrDecimalToDecimal(QString strDecimal)
    {
        bool ok;
        return strDecimal.toInt(&ok,10);
    }

    //2进制字符串转10进制
    static int StrBinToDecimal(QString strBin)
    {
        bool ok;
        return strBin.toInt(&ok,2);
    }

    //16进制字符串转2进制字符串
    static QString StrHexToStrBin(QString strHex)
    {
        uchar decimal=StrHexToDecimal(strHex);
        QString bin=QString::number(decimal,2);

        uchar len=bin.length();
        if (len<8)
        {
            for (int i=0;i<8-len;i++)
            {
                bin="0"+bin;
            }
        }

        return bin;
    }

    //10进制转2进制字符串一个字节
    static QString DecimalToStrBin1(int decimal)
    {
        QString bin=QString::number(decimal,2);

        uchar len=bin.length();
        if (len<=8)
        {
            for (int i=0;i<8-len;i++)
            {
                bin="0"+bin;
            }
        }

        return bin;
    }

    //10进制转2进制字符串两个字节
    static QString DecimalToStrBin2(int decimal)
    {
        QString bin=QString::number(decimal,2);

        uchar len=bin.length();
        if (len<=16)
        {
            for (int i=0;i<16-len;i++)
            {
                bin="0"+bin;
            }
        }

        return bin;
    }

    //计算校验码
    static uchar GetCheckCode(uchar data[],uchar len)
    {
        uchar temp=0;

        for (uchar i=0;i<len;i++)
        {
            temp+=data[i];
        }

        return temp%256;
    }

    //将溢出的char转为正确的uchar
    static uchar GetUChar(char data)
    {
        uchar temp;
        if (data>127)
        {
            temp=data+256;
        }
        else
        {
            temp=data;
        }
        return temp;
    }
};

#endif // COMMONSETTING_H
