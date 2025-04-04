#ifndef CPATCH_H
#define CPATCH_H

#include <QFile>
#include <QVariant>

enum CPatchResult : int {
    NoError             = 0,    ///< 返回正常
    TerminatorStop      = 1,    ///< 线程终止
    FileOpenError       = -99,  ///< 文件打开失败
    FileNullError,              ///< 文件数据为空
    FileRemoveError,            ///< 文件无法删除
    FileRenameError,            ///< 文件无法移动
    JmpOpenError,               ///< jmp打开失败
    JmpDataNullError,           ///< jmp数据为空
    JmpWriteError,              ///< jmp无法写入
    JmpReplaceError,            ///< jmp替换失败
    JmpTypeError,               ///< jmp异常类型
    DirNotExistError,           ///< 目录不存在
    DirNullError,               ///< 目录为空
    ZlibCompError,              ///< zlib压缩失败
    ZlibUncompError,            ///< zlib解压失败
    DownloadError,              ///< 下载失败
};

/**
 * @brief 数据信息
 */
struct SDataInfo {
    int type{0};        ///< 数据类型: 1.内部数据; 2.本地文件; 3.网络文件;
    int num{0};         ///< 文件信息索引
    int index{0};       ///< 压缩数据索引
    int compLen{0};     ///< 压缩数据长度
    int fileLen{0};     ///< 文件数据长度(type=1且fileLen=-1时，表示为无压缩资源，压缩长度便是文件长度)
    QString filePath;   ///< [type=1:jmp文件路径] [type=2:文件路径]
    QString patchPath;  ///< 补丁路径
    QByteArray md5;     ///< MD5码[32]
};
typedef QVector<SDataInfo> SDataInfoList;
typedef QMap<QString,QByteArray> PatchUniques;
Q_DECLARE_METATYPE(SDataInfo);
Q_DECLARE_METATYPE(SDataInfoList);

/**
 * @brief 结果数据
 */
struct SResultData {
    int total{0};       ///< 总数量
    int success{0};     ///< 成功
    int fail{0};        ///< 失败
    int ignore{0};      ///< 忽略
    int discard{0};     ///< 丢弃
    int count{0};       ///< 计数
};
Q_DECLARE_METATYPE(SResultData);

/**
 * @brief jmp文件句柄
 */
struct SJmpFile {
    QFile *file{nullptr};
    QString strGamePath;
    int index{0};
    int count{0};
};

/**
 * @brief jmp导入项
 */
struct SJmpImportItem {
    int i{0};
    SDataInfo *d{nullptr};
};

namespace CPatch {
    /// 获取 客户端资源列表 下载地址
    const QString &GetClientFileUrl();
    /// 获取 异步资源列表 下载地址
    const QString &GetAsyncFileUrl();
    /// 获取 单个资源 下载地址 (patch 补丁路径)
    QString GetPatchFileUrl(const QString &patch);
    /// 获取 客户端资源列表 本地路径 (strGamePath 游戏目录)
    QString GetClientFilePath(const QString &strGamePath);
    /// 获取 异步资源列表 本地路径 (strGamePath 游戏目录)
    QString GetAsyncFilePath(const QString &strGamePath);
    /// 获取 异步资源目录 本地路径 (strGamePath 游戏目录)
    QString GetAsyncDirPath(const QString &strGamePath);
    /// 获取 游戏多个jmp文件列表 (strGamePath 游戏目录)
    QStringList GetGameJmpFiles(const QString&strGamePath);

    /// 补丁路径 转换为 文件路径 (patch 补丁路径, prefix 前缀目录)
    QString PatchToPath(const QString &patch, const QString &prefix);

    /// 读取数据 (返回值: 数据块) (info 数据信息)
    QByteArray ReadData(const SDataInfo& info, QFile *file = nullptr);
    CPatchResult ReadJmpFile(SDataInfoList &sDataList, const QString &strJmpPath, std::atomic_bool *pTerminator = nullptr);
    CPatchResult ReadLocalDir(SDataInfoList &sDataList, const QString &strDirPath, std::atomic_bool *pTerminator = nullptr, std::atomic_int *progress = nullptr);
    CPatchResult ReadXmlData(PatchUniques &patchs, const QByteArray &xmldata, bool bMD5, std::atomic_bool *pTerminator = nullptr);

    /// 导入数据 (返回值: 0成功) (jmp 数据包, info 数据信息, data 导入数据)
    CPatchResult ImportData(SJmpFile &jmp, SDataInfo *info, const QByteArray &data);

    /// 整理资源 (返回值: 0成功) (jmpPaths jmp路径列表, datas jmp索引数据, pStop 停止符, pPause 暂停符, pProgress 进度[100%])
    QVector<QList<SJmpImportItem>> OrganizeData(QList<SJmpImportItem> &totalItems);

    /// 更新资源MD5 (返回值: 数据块) (info 数据信息, jmp 数据包文件)
    CPatchResult UpdateDataMd5(SDataInfo& info, QFile *jmp);
};

namespace Zlib {
    /// 压缩zlib数据 (返回值: 0成功) (sourceData 源数据地址, sourceLen 源数据长度, destData 目标数据)
    CPatchResult Comp(const uchar* sourceData, const ulong& sourceLen, QByteArray& destData);
    /// 压缩zlib数据 (返回值: 压缩数据) (sourceData 源数据地址, sourceLen 源数据长度)
    QByteArray Comp(const uchar* sourceData, const ulong& sourceLen);
    /// 压缩zlib数据 (返回值: 压缩数据) (sourceData 源数据)
    QByteArray Comp(const QByteArray& sourceData);
    /// 压缩zlib数据 (返回值: 压缩数据) (strSourceFile 源文件)
    QByteArray Comp(const QString& strSourceFile);

    /// 解压zlib数据 (返回值: 0成功) (sourceData 源数据地址, sourceLen 源数据长度, destData 目标数据, destLen 目标数据长度)
    CPatchResult Uncomp(const uchar* sourceData, const ulong& sourceLen, QByteArray& destData, const ulong &destLen);
    /// 解压zlib数据 (返回值: 解压数据) (sourceData 源数据地址, sourceLen 源数据长度, destLen 目标数据长度)
    QByteArray Uncomp(const uchar* sourceData, const ulong& sourceLen, const ulong &destLen);
    /// 解压zlib数据 (返回值: 解压数据) (sourceData 源数据, destLen 目标数据长度)
    QByteArray Uncomp(const QByteArray& sourceData, const ulong &destLen);

    /// 解压gz文件 (返回值: 0成功) (sourceData 源数据地址, sourceLen 源数据长度, destData 目标数据)
    CPatchResult UncompGz(const uchar* sourceData, const ulong& sourceLen, QByteArray& destData);
    /// 解压gz文件 (返回值: 0成功) (sourceData 源数据地址, sourceLen 源数据长度, strDestFile 目标文件)
    CPatchResult UncompGz(const uchar* sourceData, const ulong& sourceLen, const QString& strDestFile);
    /// 解压gz文件 (返回值: 0成功) (strSourceFile 源文件, strDestFile 目标文件)
    CPatchResult UncompGz(const QString& strSourceFile, const QString& strDestFile);
    /// 解压gz文件 (返回值: 解压数据) (sourceData 源数据地址, sourceLen 源数据长度)
    QByteArray UncompGz(const uchar* sourceData, const ulong& sourceLen);
    /// 解压gz文件 (返回值: 解压数据) (strSourceFile 源文件)
    QByteArray UncompGz(const QString& strSourceFile);
}

#endif // CPATCH_H
