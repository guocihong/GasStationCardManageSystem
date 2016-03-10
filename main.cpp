#include "readidentifiercardinfo.h"
#include "TcpThread/tcpcommunicate.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CommonSetting::SetUTF8Code();
    CommonSetting::OpenDataBase();

    system("/bin/UdpMulticastClient -qws &");
    CommonSetting::Sleep(1000);
    system("/bin/CheckMainProgramState -qws &");
    CommonSetting::Sleep(1000);

    //创建子线程1,用来与服务器进行tcp通信,上传刷卡记录
    TcpCommunicate *tcp_communicate = new TcpCommunicate;
    QThread tcp_work_thread;
    tcp_communicate->moveToThread(&tcp_work_thread);
    tcp_work_thread.start();

    //读身份证信息、拍照、解析、联动
    ReadIdentifierCardInfo read_serial;
//    read_serial.show();

    CommonSetting::WriteCommonFileTruncate("/bin/MainProgramState",QString("OK"));

    return app.exec();
}
