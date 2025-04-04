#include "cpatch.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QScopeGuard>
#include <QTextStream>
#include <QThread>
#include <QtZlib/zlib.h>

#pragma comment(lib, "URlmon.lib")

#define __RandomName(name,num) name##num
#define _RandomName(name,num) __RandomName(name,num)
#define RandomName _RandomName(var,__LINE__)

#define ScopeGuard auto RandomName = qScopeGuard

const qint64 nJmpFileSizeMax    = INT_MAX;                          ///< 文件数据存储上限大小 有符号32位整数最大值
const qint32 nJmpCompbegin      = 10 * 1024 * 1024;                 ///< 压缩数据存储起始位置
const qint32 nJmpCompMax        = nJmpFileSizeMax - nJmpCompbegin;  ///< 压缩数据存储上限大小
const qint32 nJmpIndexMax       = (nJmpCompbegin - 54) / 304;       ///< 索引列表存储上限大小
const qint32 nJmpCompAverage    = nJmpCompMax / nJmpIndexMax;       ///< 压缩数据平均值
const char patchNull[]          = "_INVALID_BLOCK_";                ///< 废弃资源路径

const QString &CPatch::GetClientFileUrl()
{
    static const QString url("https://update.300hero.jumpwgame.com/xclient_unpack/bin/launchercfg/clientfilelist.xml.gz");
    return url;
}

const QString &CPatch::GetAsyncFileUrl()
{
    static const QString url("https://update.300hero.jumpwgame.com/xclient_unpack/bin/asyncfile/asyncclientfilelist.xml.gz");
    return url;
}

QString CPatch::GetPatchFileUrl(const QString &patch)
{
    static const QString url("https://update.300hero.jumpwgame.com/xclient_unpack");
    return url + patch.mid(2).replace('\\', '/') + ".gz";
}

QString CPatch::GetClientFilePath(const QString &strGamePath)
{
    return strGamePath + "/LauncherCfg/ClientFileList.xml";
}

QString CPatch::GetAsyncFilePath(const QString &strGamePath)
{
    return strGamePath + "/asyncfile/asyncclientfilelist.xml";
}

QString CPatch::GetAsyncDirPath(const QString &strGamePath)
{
    return strGamePath + "/asyncfile";
}

QStringList CPatch::GetGameJmpFiles(const QString &strGamePath)
{
    QStringList files;
    for (int i = 0; i < 99; ++i) {
        QFileInfo file(QString("%1/Data%2.jmp").arg(strGamePath, i ? QString::number(i) : ""));
        if (file.exists()) files.append(file.absoluteFilePath());
        else break;
    }
    return files;
}

QString CPatch::PatchToPath(const QString &patch, const QString &prefix)
{
    return prefix + patch.mid(2).replace('\\', '/');
}

QByteArray CPatch::ReadData(const SDataInfo &info, QFile *file)
{
    if (info.type == 1) { // jmp数据
        QFile *inFile = file;
        // 是否需要打开jmp文件
        if (!inFile) {
            inFile = new QFile(info.filePath);
            if (!inFile->open(QIODevice::ReadOnly)) {
                delete inFile;
                inFile = nullptr;
            }
        }
        if (!inFile) return {};
        // 读取数据
        inFile->seek(info.index);
        QByteArray compData = inFile->read(info.compLen);
        if (!file) {
            inFile->close();
            delete inFile;
        }
        // zlib解压
        if (info.fileLen > 0) return Zlib::Uncomp(compData, info.fileLen);
        else return compData;
    }
    else if (info.type == 2) { // 单独文件
        QFile inFile(info.filePath);
        if (inFile.open(QIODevice::ReadOnly)) {
            ScopeGuard([&]{inFile.close();});
            return inFile.readAll();
        }
    }
    return {};
}

CPatchResult CPatch::ReadJmpFile(SDataInfoList &sDataList, const QString &strJmpPath, std::atomic_bool *pStop)
{
    QFile inFile(strJmpPath);
    if (!inFile.open(QIODevice::ReadOnly)) return FileOpenError;
    ScopeGuard([&inFile]{inFile.close();});
    qint32 nDataNum = 0;
    inFile.seek(50);
    inFile.read((char *)&nDataNum, 4);
    if ((nDataNum < 1) || (nDataNum > nJmpIndexMax) || (inFile.size() > nJmpFileSizeMax)) return JmpDataNullError;
    for (int i = 0; i < nDataNum; i++) {
        if(pStop) if (*pStop) return TerminatorStop;
        SDataInfo info{1, i, 0, 0, 0, strJmpPath, "", ""};
        char patch[260]{0};
        char md5[32]{0};
        inFile.seek(i * 304 + 54);
        inFile.read(patch, 260);
        inFile.read((char *)&info.index, 4);
        inFile.read((char *)&info.compLen, 4);
        inFile.read((char *)&info.fileLen, 4);
        inFile.read(md5, 32);
        info.patchPath = QString::fromLocal8Bit(patch);
        if (info.patchPath.isEmpty() || info.patchPath == patchNull) continue;
        if (md5[0]) info.md5 = QByteArray(md5, 32);
        sDataList.append(std::move(info));
    }
    return NoError;
}

CPatchResult CPatch::ReadLocalDir(SDataInfoList &sDataList, const QString &strDirPath, std::atomic_bool *pStop, std::atomic_int *progress)
{
    if (!QDir(strDirPath).exists()) return DirNotExistError;
    QDirIterator iter(strDirPath, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        if(pStop) if (*pStop) return TerminatorStop;
        iter.next();
        QFileInfo fileInfo = iter.fileInfo();
        if (fileInfo.isFile()) {
            SDataInfo info{2, 0, 0, 0, (int)fileInfo.size(), fileInfo.absoluteFilePath(), "", ""};
            info.patchPath = ".." + info.filePath.mid(strDirPath.size()).replace('/', '\\');
            sDataList.append(std::move(info));
            if (progress) *progress = sDataList.size();
        }
    }
    if (sDataList.isEmpty()) return DirNullError;
    else return NoError;
}

CPatchResult CPatch::ReadXmlData(PatchUniques &patchs, const QByteArray &xmldata, bool bMD5, std::atomic_bool *pStop)
{
    if (xmldata.isEmpty()) return FileNullError;
    QTextStream stream(xmldata);
    while (!stream.atEnd()) {
        if(pStop) if (*pStop) return TerminatorStop;
        QString line = stream.readLine();
        // patch
        int nPos1 = line.indexOf("\"..");
        if (nPos1 < 0) continue;
        int nPos2 = line.indexOf("\" M");
        if (nPos2 < 0) continue;
        QString strPatch(line.mid(nPos1 + 1, nPos2 - nPos1 - 1));
        if (strPatch.indexOf('&') > 0) strPatch.replace("&lt;", "<").replace("&gt;", ">").replace("&amp;", "&").replace("&apos;", "'").replace("&quot;", "\"");
        // md5
        QByteArray md5;
        if (bMD5) {
            int nPos3 = line.indexOf("\" S");
            if (nPos3 < 0) continue;
            md5 = line.mid(nPos2 + 7, nPos3 - nPos2 - 7).toLocal8Bit();
        }
        patchs.insert(strPatch, md5);
    }
    if (patchs.isEmpty()) return DirNullError;
    else return NoError;
}

CPatchResult CPatch::ImportData(SJmpFile &jmp, SDataInfo *info, const QByteArray &data)
{
    /// 导入源 info.type
    /// 1.内部资源 整理 不能中断 data(jmpData)
    /// 2.文件资源 打包 不能中断 data(fileData)
    /// 3.网络资源 下载 可以断点 data(gzData)
    /// 导入目标 序列jmp 单独jmp(必须提前准备好)

    const char *compData = nullptr;     ///< 压缩数据
    int fileLen = 0;                    ///< 文件数据长度
    int compLen = 0;                    ///< 压缩数据长度
    QByteArray temp;

    // 1.内部资源
    if (info->type == 1) {
        compData = data.data();
        compLen = data.size();
        fileLen = info->fileLen;
    }
    // 2.文件资源
    else if (info->type == 2) {
        compData = data.data();
        compLen = data.size();
        fileLen = -1;
    }
    // 3.网络资源
    else if (info->type == 3) {
        compData = data.data() + 4;
        compLen = data.size();
        fileLen = *((int*)data.data());
    }
    // 未知类型
    else return JmpTypeError;

    // MD5
    if (info->type == 2) info->md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    else if (info->md5.isEmpty()) {
        if (fileLen < 0) { // 数据未压缩，直接计算MD5
            info->md5 = QCryptographicHash::hash(info->type == 1 ? data : data.mid(4), QCryptographicHash::Md5);
        }
        else { // 数据已压缩，先解压出来再计算MD5
            if (NoError == Zlib::Uncomp((uchar*)compData, compLen, temp, fileLen)) {
                info->md5 = QCryptographicHash::hash(temp, QCryptographicHash::Md5);
            }
        }
    }

    // 尝试压缩
    if (fileLen < 0) {
        if (NoError == Zlib::Comp((uchar*)compData, compLen, temp)) {
            fileLen = compLen;
            compLen = temp.size();
            compData = temp.data();
        }
    }

    // 检查jmp文件
    while (true) {
        if (!jmp.file) jmp.file = new QFile(QString("%1/Data%2.jmp").arg(jmp.strGamePath, jmp.index ? QString::number(jmp.index) : ""));
        if (!jmp.file->isOpen()) {
            if (!jmp.file->open(QIODevice::ReadWrite)) return JmpOpenError;
            jmp.file->write("DATA" + QByteArray::number(jmp.index + 1) + ".0");
            jmp.file->seek(50);
            jmp.file->read((char *)&jmp.count, 4);
        }
        if (jmp.count >= nJmpIndexMax || compLen > nJmpFileSizeMax - jmp.file->size()) {
            jmp.file->close();
            delete jmp.file;
            jmp.file = nullptr;
            jmp.index++;
            continue;
        }
        break;
    }

    // 导入数据
    int compIndex = qMax((qint64)nJmpCompbegin, jmp.file->size());
    jmp.file->seek(54 + 304 * jmp.count);
    jmp.file->write(info->patchPath.toLocal8Bit());
    jmp.file->seek(54 + 304 * jmp.count + 260);
    jmp.file->write((char*)&compIndex, 4);
    jmp.file->write((char*)&compLen, 4);
    jmp.file->write((char*)&fileLen, 4);
    jmp.file->write(info->md5);
    jmp.file->seek(compIndex);
    jmp.file->write(compData, compLen);
    jmp.file->seek(50);
    jmp.file->write((char*)&(++jmp.count), 4);

    // 导入完成
    return NoError;
}

QVector<QList<SJmpImportItem> > CPatch::OrganizeData(QList<SJmpImportItem> &totalItems)
{
    // 按大小排序
    std::sort(totalItems.begin(), totalItems.end(), [](const SJmpImportItem &d1, const SJmpImportItem &d2)->bool {
        return (d1.d->type == 1 ? d1.d->compLen : d1.d->fileLen) < (d2.d->type == 1 ? d2.d->compLen : d2.d->fileLen);
    });

    // 整理
    int count = 0, length = 0, average = 0;
    QVector<QList<SJmpImportItem> > packItems;
    QList<SJmpImportItem> *pitems = nullptr;
    while(!totalItems.empty()) {
        if (!pitems) {
            packItems.push_back({});
            pitems = &packItems.back();
            count = 0, length = 0, average = 0;
        }
        bool beg = average < nJmpCompAverage;
        auto &item = beg ? totalItems.first() : totalItems.last();
        if (count + 1 <= nJmpIndexMax && length + item.d->compLen <= nJmpCompMax) {
            count++;
            length += item.d->compLen;
            average += beg ? item.d->compLen : -item.d->compLen;
            pitems->append(beg ?  totalItems.takeFirst() : totalItems.takeLast());
        }
        else pitems = nullptr; // jmp文件已满
    }

    // 按补丁路径排序
    for (auto &items : packItems) {
        std::sort(items.begin(), items.end(), [](const SJmpImportItem &d1, const SJmpImportItem &d2)->bool {
            return d1.d->patchPath < d2.d->patchPath;
        });
    }

    return packItems;
}

CPatchResult CPatch::UpdateDataMd5(SDataInfo &info, QFile *jmp)
{
    if (info.type != 1) return JmpTypeError;
    if (!jmp || !jmp->isOpen()) return JmpOpenError;
    jmp->seek(info.index);
    auto compData = jmp->read(info.compLen);
    QByteArray md5;
    if (info.fileLen > 0) {
        auto data = Zlib::Uncomp(compData, info.fileLen);
        if (data.isEmpty()) return ZlibUncompError;
        md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    }
    else md5 = QCryptographicHash::hash(compData, QCryptographicHash::Md5).toHex();
    if (info.md5.compare(md5, Qt::CaseInsensitive)) {
        jmp->seek(54 + 304 * info.num + 260 + 12);
        jmp->write(info.md5 = md5);
    }
    return NoError;
}

CPatchResult Zlib::Comp(const uchar *sourceData, const ulong &sourceLen, QByteArray &destData)
{
    ulong compLen = z_compressBound(sourceLen);
    destData = QByteArray(compLen, '\0');
    const int& bOk = z_compress((uchar*)destData.data(), &compLen, sourceData, sourceLen);
    if (bOk != Z_OK) return ZlibCompError;
    if (compLen != (ulong)destData.size()) destData.resize(compLen);
    return NoError;
}

QByteArray Zlib::Comp(const uchar *sourceData, const ulong &sourceLen)
{
    QByteArray destData;
    if (Comp(sourceData, sourceLen, destData)) return {};
    else return destData;
}

QByteArray Zlib::Comp(const QByteArray& sourceData)
{
    QByteArray destData;
    if (Comp((uchar*)sourceData.data(), sourceData.size(), destData)) return {};
    else return destData;
}

QByteArray Zlib::Comp(const QString &strSourceFile)
{
    // 读取文件
    QFile inFile(strSourceFile);
    if (!inFile.open(QIODevice::ReadOnly)) return {};
    QByteArray&& sourceData = inFile.readAll();
    inFile.close();
    // 解压数据
    QByteArray destData;
    if (Comp((uchar*)sourceData.data(), sourceData.size(), destData)) return {};
    else return destData;
}

CPatchResult Zlib::Uncomp(const uchar *sourceData, const ulong &sourceLen, QByteArray &destData, const ulong &destLen)
{
    ulong fileLen = destLen;
    destData = QByteArray(fileLen, '\0');
    const int& bOk = z_uncompress((uchar*)destData.data(), &fileLen, sourceData, sourceLen);
    if (bOk != Z_OK) return ZlibUncompError;
    if (fileLen != destLen) destData.resize(fileLen);
    return NoError;
}

QByteArray Zlib::Uncomp(const uchar *sourceData, const ulong &sourceLen, const ulong &destLen)
{
    QByteArray destData;
    if (Uncomp(sourceData, sourceLen, destData, destLen)) return {};
    else return destData;
}

QByteArray Zlib::Uncomp(const QByteArray& sourceData, const ulong &destLen)
{
    QByteArray destData;
    if (Uncomp((uchar*)sourceData.data(), sourceData.size(), destData, destLen)) return {};
    else return destData;
}

CPatchResult Zlib::UncompGz(const uchar *sourceData, const ulong &sourceLen, QByteArray &destData)
{
    qint32 head = *(qint32*)(sourceData);
    if (head > 0) {
        ulong destLen = head;
        destData = QByteArray(destLen, '\0');
        const int& bOk = z_uncompress((uchar*)destData.data(), &destLen, sourceData + 4, sourceLen - 4);
        if (bOk != Z_OK) return ZlibUncompError;
        if (destLen != (ulong)destData.size()) destData.resize(destLen);
    }
    else { // 无压缩文件
        destData = QByteArray((const char*)(sourceData + 4), destData.size() - 4);
    }
    return NoError;
}

CPatchResult Zlib::UncompGz(const uchar *sourceData, const ulong &sourceLen, const QString &strDestFile)
{
    // 解压数据
    QByteArray destData;
    if (auto error = UncompGz(sourceData, sourceLen, destData)) return error;
    // 写入文件
    QDir().mkpath(QFileInfo(strDestFile).absolutePath());
    QFile outFile(strDestFile);
    if (!outFile.open(QIODevice::WriteOnly)) return FileOpenError;
    outFile.write(destData);
    outFile.close();
    return NoError;
}

CPatchResult Zlib::UncompGz(const QString &strSourceFile, const QString &strDestFile)
{
    // 读取文件
    QFile inFile(strSourceFile);
    if (!inFile.open(QIODevice::ReadOnly)) return FileOpenError;
    QByteArray&& sourceData = inFile.readAll();
    inFile.close();
    // 解压数据
    QByteArray destData;
    if (auto error = UncompGz((uchar*)sourceData.data(), sourceData.size(), destData)) return error;
    // 写入文件
    QDir().mkpath(QFileInfo(strDestFile).absolutePath());
    QFile outFile(strDestFile);
    if (!outFile.open(QIODevice::WriteOnly)) return FileOpenError;
    outFile.write(destData);
    outFile.close();
    return NoError;
}

QByteArray Zlib::UncompGz(const uchar *sourceData, const ulong &sourceLen)
{
    QByteArray destData;
    if (UncompGz(sourceData, sourceLen, destData)) return {};
    else return destData;
}

QByteArray Zlib::UncompGz(const QString &strSourceFile)
{
    // 读取文件
    QFile inFile(strSourceFile);
    if (!inFile.open(QIODevice::ReadOnly)) return {};
    QByteArray&& sourceData = inFile.readAll();
    inFile.close();
    // 解压数据
    QByteArray destData;
    if (UncompGz((uchar*)sourceData.data(), sourceData.size(), destData)) return {};
    else return destData;
}
