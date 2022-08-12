#include "mainwindow.h"
#include "dialog_set.h"

#include "windows.h"
#include <QApplication>
#include <QDir>
#include <QTranslator>

void InitConfig(MainWindow* w, QString strAppPath);

int main(int argc, char *argv[])
{
    // 设置工作路径为程序所在目录
    QDir::setCurrent(QString::fromLocal8Bit(argv[0]).left(QString::fromLocal8Bit(argv[0]).lastIndexOf('\\')));
    QApplication a(argc, argv);
    QTranslator translator;
    translator.load(":/qt_zh_CN.qm");
    a.installTranslator(&translator);
    MainWindow w;
    InitConfig(&w, QString::fromLocal8Bit(argv[0]));
    if (argc > 1)
    {
        QString strDataPath(QString::fromLocal8Bit(argv[1]));
        if (strDataPath.right(4) == ".jmp")
            w.OpenData(strDataPath);
    }
    return a.exec();
}

// 初始化配置
void InitConfig(MainWindow* w, QString strAppPath)
{
    w->m_strAppPath = strAppPath;
    w->show();
    w->Init();

    QDir isDir;
    if (!isDir.exists("BConfig")) isDir.mkdir("BConfig");
    if (!isDir.exists("BConfig/Temp")) isDir.mkdir("BConfig/Temp");
//    if (!isDir.exists("BExport")) isDir.mkdir("BExport");
//    if (!isDir.exists("BAlone")) isDir.mkdir("BAlone");

    QFile inFile;
    if (!inFile.exists("BConfig/ico_jmpFile.ico")) QFile::copy(":/ico_jmpFile.ico", "BConfig/ico_jmpFile.ico");
    if (!inFile.exists("BConfig/Config.ini"))
    {
        QString strCurrentPath(QDir::currentPath());
        if (strCurrentPath.right(1) != "/") strCurrentPath.append("/");
        WritePrivateProfileString(L"配置", L"游戏路径", L"", L"BConfig/Config.ini");
        WritePrivateProfileString(L"配置", L"导出路径", QString(strCurrentPath).append("BExport").toStdWString().c_str(), L"BConfig/Config.ini");
        WritePrivateProfileString(L"配置", L"单独导出", QString(strCurrentPath).append("BAlone").toStdWString().c_str(), L"BConfig/Config.ini");
        WritePrivateProfileString(L"配置", L"导出模式", L"1", L"BConfig/Config.ini");
        WritePrivateProfileString(L"配置", L"预览图片", L"1", L"BConfig/Config.ini");

        Dialog_Set *pDSet = new Dialog_Set(strAppPath, w);
        pDSet->setModal(true);
        pDSet->show();
    }
}
