#include "readidentifiercardinfo.h"
#include "TcpThread/tcpcommunicate.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    CommonSetting::SetUTF8Code();
    CommonSetting::OpenDataBase();

    ReadIdentifierCardInfo read_serial;
    read_serial.show();

    //创建子线程2,用来与服务器进行tcp通信
    TcpCommunicate *tcp_communicate = new TcpCommunicate;
    QThread tcp_work_thread;
    tcp_communicate->moveToThread(&tcp_work_thread);
    tcp_work_thread.start();

    return app.exec();
}
