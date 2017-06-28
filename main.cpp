#include "mainform.h"
#include "globalconfig.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //全局变量初始化
    GlobalConfig::init();

    //设置编码和打开数据库
    CommonSetting::SetUTF8Code();
    CommonSetting::OpenDataBase();

//    system("/bin/UdpMulticastClient -qws &");
//    CommonSetting::Sleep(1000);
    system("/bin/CheckMainProgramState -qws &");
    CommonSetting::Sleep(1000);

    //读身份证信息、拍照、解析、联动
    MainForm form;
    form.showFullScreen();

    CommonSetting::WriteCommonFileTruncate("/bin/MainProgramState",QString("OK"));

    return app.exec();
}
