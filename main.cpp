#include "mainwindow.h"
#include "cpatch.h"
#include "config.h"
#include "form_bank.h"
#include "form_dat.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QTranslator>
#include <QMainWindow>
#include <windows.h>

void Init();

int main(int argc, char *argv[])
{
    // app
    QApplication a(argc, argv);
    QTranslator translator;
    translator.load(":/resource/qtbase_zh_CN.qm");
    a.installTranslator(&translator);
    Init();

    // 解析传入参数
    int update = 0; // 0.null 1.更新成功 -1.更新失败
    QString updateInfo("000");
    int format = 0; // 0.null 1.jmp 2.bank 3.dat
    SDataInfo data;
    if (argc > 1) {
        const QString arg1 = QString::fromLocal8Bit(argv[1]);
        if (arg1.size() == 0);
        else if (arg1[0] == '-') {
            if (arg1 == "-update") { // 更新结果
                if (argc > 2) update = QString(argv[2]).toInt();
                if (argc > 3) updateInfo = argv[3];
            }
        }
        else {
            data.filePath = QString(arg1).replace('\\', '/');
            const int nPos = data.filePath.lastIndexOf('.');
            if (nPos >= 0) { // 判断类型
                const QString strFormat = data.filePath.mid(nPos);
                if      (strFormat == ".jmp")   format = 1;
                else if (strFormat == ".bank")  format = 2;
                else if (strFormat == ".dat")   format = 3;
            }
            if (format == 2 || format == 3) {
                data.type = 2;
                data.patchPath = data.filePath.split('/').back();
            }
        }
    }

    // 更新重启
    if (update) {
        (new MainWindow)->start(update, updateInfo);
        return a.exec();
    }
    // 打开主界面
    else if (format == 0) {
        (new MainWindow)->start();
        return a.exec();
    }
    // 打开jmp
    else if (format == 1) {
        (new MainWindow)->start(data.filePath);
        return a.exec();
    }
    // 打开bank
    else if (format == 2) {
        (new Form_bank)->load(nullptr, data);
        return a.exec();
    }
    // 打开dat
    else if (format == 3) {
        (new Form_dat)->load(nullptr, data);
        return a.exec();
    }
    else return -1;
}

// 初始化配置
void Init()
{
    auto config = qApp->applicationDirPath() + "/BConfig";
    if (!QDir(config).exists()) QDir().mkdir(config);
    SetDllDirectoryW(config.toStdWString().c_str());
    auto resource = [](const QString &qrc, const QString & file){ if (!QFile::exists(file)) Zlib::UncompGz(qrc, file); };
    resource(":/resource/readBank.dll.gz",  config + "/readBank.dll");
    resource(":/resource/fmod.dll.gz",      config + "/fmod.dll");
    resource(":/resource/update.exe.gz",    config + "/update.exe");
    resource(":/resource/help.html.gz",     config + "/help.html");
}
