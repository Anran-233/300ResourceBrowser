#ifndef CPATCH_H
#define CPATCH_H

#define FILE_OK         0   // 正常
#define FILE_OPEN_FAEL  -1  // 文件打开失败
#define FILE_DATA_NULL  -2  // 文件数据为空
#define FILE_ZLIB_FAEL  -3  // zlib解压失败

#define DIR_OK          0   // 正常
#define DIR_PATH_FAEL   -1  // 目录不存在
#define DIR_DATA_NULL   -2  // 目录为空

#include <QVector>
#include <QWidget>
#include <QDebug>
#include <QThread>

extern QSet<void*> g_sets;

struct SDataInfo        // data数据信息
{
    int type;           // 数据类型: 1 正常data数据; 2 本地文件; 3 网络文件;
    QString filePath;   // 文件路径
    QString patchPath;  // 补丁路径
    int index;          // 索引位置
    int compLen;        // 压缩数据长度
    int fileLen;        // 文件数据长度
};

struct SRealTimeData    // 实时数据信息
{
    int nState;			// 执行状态：0_等待执行；1_准备执行；2_正在执行；3_结束执行；-1_打开失败；-2_数据为空；-3_数据已满
    int nDataNum;		// 数据总数
    int nSuccessNum;	// 成功数量
    int nFailNum;		// 失败数量
    int nNullNum;		// 废弃数量
    int nIgnoreNum;		// 忽略数量
};

struct SExportConfig    // 导出配置
{
    int nExportMode;        // 导出模式：0 非覆盖模式; 1 覆盖模式
    QString strExportPath;  // 导出路径
};

class CPatch
{
public:
    /// int转换QString（固定字节宽度，向左对齐，删除多余部分，不足时右边补充空格）
    static QString StrIntL(int value, int nWidth);
    /// 检查目标路径是否存在，若不存在则创建路径（返回1为路径存在，0则不存在）
    static int CheckPathAndCreate(QString strPath);
    /// 补丁路径转换为文件路径
    static QString PathPatchToFile(QString strPatchPath);
    /// 文件路径转换为补丁路径
    static QString PathFileToPatch(QString strFilesPath);
    /// 移动文件
    static bool MoveFilePath(const QString oldFilePath, const QString newFilePath);
    /// 复制文件
    static bool CopyFilePath(const QString oldFilePath, const QString newFilePath);
    /// 读取data文件（返回0为读取正常）（sDataList 存放data数据列表的容器; strDataPath 需要读取的文件路径）
    static int ReadDataFile(QVector<SDataInfo>& sDataList, QString strDataPath);
    /// 读取资源目录文件（返回0为读取正常）（sDataList 存放data数据列表的容器; strDirPath 需要读取的目录路径）
    static int ReadDataDir(QVector<SDataInfo>& sDataList, QString strDirPath);
    /// 读取客户端列表（返回0为读取正常）（strClientList 存放文件列表的容器; strGamePath 游戏路径(不需要时留空); strInfo 回调的信息; bTerminator 线程终止符）
    static int ReadClientList(QVector<QString>& strClientList, QString strGamePath, QString& strInfo, bool& bTerminator);
    /// 导出data单个文件（返回0为导出正常）（sDataInfo 导出目标的数据信息; strDataPath 读取data文件路径; strFilePath 导出文件路径）
    static int ExportDataFile(SDataInfo sDataInfo, QString strDataPath, QString strFilePath);
    /// 解包data文件（）（nID 线程编号; strDataPath 读取data文件路径; sExportConfig 导出配置; sRtData 回调的实时数据; bTerminator 线程终止符）
    static void UnpackDataFile(int nID, QString strDataPath, SExportConfig sExportConfig, SRealTimeData& sRtData, bool& bTerminator);
    /// 解包目录资源（）（strDirPath 读取目录路径; sExportConfig 导出配置; sRtData 回调的实时数据; bTerminator 线程终止符）
    static void UnpackDirFile(QString strDirPath, SExportConfig sExportConfig, SRealTimeData& sRtData, bool& bTerminator);
    /// 解包信息解析（返回线程状态: 0 正在运行; 1 已经结束）（sRtData 实时数据; nProgress 回调的进度; strInfo 回调的信息文本）
    static int UnpackToDataInfo(SRealTimeData sRtData, int& nProgress, QString& strInfo);
    /// 打包data文件（）（sDataList 文件列表; sRtData 回调的实时数据; bTerminator 线程终止符）
    static void PackDataFile(QVector<SDataInfo> sDataList, SRealTimeData& sRtData, bool& bTerminator);
    /// 打包信息解析（返回线程状态: 0 正在运行; 1 已经结束）（sRtData 实时数据; nProgress 回调的进度; strInfo 回调的信息文本）
    static int PackToDataInfo(SRealTimeData sRtData, int& nProgress, QString& strInfo);
    /// 下载单个TY文件（返回下载结果: 0 成功; -1 下载失败; -2 解压失败）（strPatchPath 补丁路径; strSavePath 下载保存路径）
    static int DownloadTYGzFile(QString strPatchPath, QString strSavePath);
    /// 下载缺失文件（sDataList 存放data数据列表的容器; strSavePath 下载保存路径; sRtData 回调的实时数据; bTerminator 线程终止符）
    static void DownloadDataFile(QVector<SDataInfo> sDataList, QString strSavePath, SRealTimeData& sRtData, bool& bTerminator);
    /// 下载信息解析（返回线程状态: 0 正在运行; 1 已经结束）（sRtData 实时数据; nProgress 回调的进度; strInfo 回调的信息文本）
    static int DownloadToDataInfo(SRealTimeData sRtData, int& nProgress, QString& strInfo);
    /// 解压TY的Gz文件（返回0为解压正常）（strGzFilePath Gz文件路径; strNewFilePath 解压后的文件路径）
    static int UncompTYGzFile(QString strGzFilePath, QString strNewFilePath);
    /// 返回data文件数据（返回文件数据）（sDataInfo 导出目标的数据信息; strDataPath 读取data文件路径）
    static QByteArray ReadDataFileOfData(SDataInfo sDataInfo, QString strDataPath);
};

#endif // CPATCH_H
