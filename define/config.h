#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QCoreApplication>

#define qConfig Config::Instance()

#define INIVAR(type,app,key,name) \
public: \
    void set_##name(const type& value) { \
        if (name == value) return; \
        SetValue(u8##app, u8##key, name = value); \
        Q_EMIT name##_Changed(); \
    } \
    Q_SIGNAL void name##_Changed(); \
    type

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = nullptr);
    static Config& Instance();
    static bool GetValue(const QString& app, const QString& key, int& value);
    static bool GetValue(const QString& app, const QString& key, QString& value);
    static void SetValue(const QString& app, const QString& key, const int& value);
    static void SetValue(const QString& app, const QString& key, const QString& value);

    /** INI配置 **/
    INIVAR(QString, "配置", "游戏路径", strGamePath) strGamePath; ///< 游戏路径
    INIVAR(QString, "配置", "导出路径", strExportPath) strExportPath = "BExport"; ///< 导出路径
    INIVAR(QString, "配置", "单独路径", strAlonePath) strAlonePath = "BAlone"; ///< 单独路径
    INIVAR(int, "配置", "导出模式", nExportMode) nExportMode = 3; ///< 导出模式 0.仅限新增; 1.快速更新; 2.完整更新; 3.完全覆盖
    INIVAR(int, "略缩图预览器", "是否启用", bPreview) bPreview = 1; ///< 略缩图预览器 是否启用 0.禁用; 1.启用;
    INIVAR(int, "略缩图预览器", "预览尺寸", nPreviewSize) nPreviewSize = 0; ///< 略缩图预览器 预览尺寸 0.300px; 1.480px; 2.720px;
    INIVAR(int, "略缩图预览器", "读取模式", nPreviewMode) nPreviewMode = 1; ///< 略缩图预览器 读取模式 0.缓冲加载; 1.直接加载;
    INIVAR(int, "语音", "音量", nBankVolume) nBankVolume = 100; ///< 语音 音量 0~100%
    INIVAR(QString, "自动更新", "上次检测", strUpdateLast) strUpdateLast = "1000000000"; ///< 自动更新 上次检测时间
    INIVAR(int, "自动更新", "检测频率", nUpdateInterval) nUpdateInterval = 0; ///< 自动更新 检测频率 0.关闭; 1.每天; 2.每周; 3.每月;
    INIVAR(int, "自动更新", "更新源", nUpdateSource) nUpdateSource = 0; ///< 自动更新 更新源 0.gitee; 1.github; 2.备用
    INIVAR(int, "界面", "显示列数", nMainColumn) nMainColumn = 3; ///< 主界面最大显示列数

    /// 获取导出路径(绝对路径)
    QString getExportPath() const;
    /// 获取导出路径(绝对路径)
    QString getAlonePath() const;
};

#undef INIVAR
#endif // CONFIG_H
