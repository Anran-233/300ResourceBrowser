#include "config.h"

#include "windows.h"
#include <QDir>
#include <QDebug>

Config::Config(QObject *parent) : QObject(parent)
{
    QDir(qApp->applicationDirPath()).mkdir("./BConfig");
#define INI(app,key,value) if (!GetValue(u8##app, u8##key, value)) SetValue(u8##app, u8##key, value)
    INI("配置", "游戏路径", strGamePath);
    INI("配置", "导出路径", strExportPath);
    INI("配置", "单独路径", strAlonePath);
    INI("配置", "导出模式", nExportMode);
    INI("略缩图预览器", "是否启用", bPreview);
    INI("略缩图预览器", "预览尺寸", nPreviewSize);
    INI("略缩图预览器", "读取模式", nPreviewMode);
    INI("语音", "音量", nBankVolume);
    INI("自动更新", "上次检测", strUpdateLast);
    INI("自动更新", "检测频率", nUpdateInterval);
    INI("自动更新", "更新源", nUpdateSource);
    INI("界面", "显示列数", nMainColumn);
#undef INI
}

Config &Config::Instance()
{
    static Config config;
    return config;
}

bool Config::GetValue(const QString &app, const QString &key, int &value)
{
    static const std::wstring &configPath = qApp->applicationDirPath().append("/BConfig/config.ini").toStdWString();
    int n = GetPrivateProfileIntW(app.toStdWString().c_str(), key.toStdWString().c_str(), INT_MAX, configPath.c_str());
    if (n == INT_MAX) return false;
    value = n;
    return true;
}

bool Config::GetValue(const QString &app, const QString &key, QString &value)
{
    static const std::wstring &configPath = qApp->applicationDirPath().append("/BConfig/config.ini").toStdWString();
    wchar_t temp[260]{0};
    GetPrivateProfileStringW(app.toStdWString().c_str(), key.toStdWString().c_str(), L"NULL", temp, 260, configPath.c_str());
    if (!lstrcmpW(temp, L"NULL")) return false;
    value = QString::fromWCharArray(temp);
    return true;
}

void Config::SetValue(const QString &app, const QString &key, const int &value)
{
    SetValue(app, key, QString::number(value));
}

void Config::SetValue(const QString &app, const QString &key, const QString &value)
{
    static const std::wstring &configPath = qApp->applicationDirPath().append("/BConfig/config.ini").toStdWString();
    WritePrivateProfileString(app.toStdWString().c_str(), key.toStdWString().c_str(), value.toStdWString().c_str(), configPath.c_str());
}

QString Config::getExportPath() const
{
    if (QDir::isAbsolutePath(strExportPath)) return strExportPath;
    else return QDir(qApp->applicationDirPath() + "/" + strExportPath).absolutePath();
}

QString Config::getAlonePath() const
{
    if (QDir::isAbsolutePath(strAlonePath)) return strAlonePath;
    else return QDir(qApp->applicationDirPath() + "/" + strAlonePath).absolutePath();
}
