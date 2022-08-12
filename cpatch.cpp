#include "cpatch.h"
#include "dialog_progress.h"

#include "windows.h"
#include "string"
#include "fstream"
#include <QtZlib/zlib.h>
#include <QDir>
#include <QDirIterator>
#include <QThreadPool>
#include <QtConcurrent>

#pragma comment(lib, "URlmon.lib")

using namespace std;

QSet<void*> g_sets;

QString CPatch::StrIntL(int value, int nWidth)
{
    QString str(QString::number(value));
    str.resize(nWidth, ' ');
    return str;
}

int CPatch::CheckPathAndCreate(QString strPath)
{
    int nPos = strPath.lastIndexOf('/');
    if (nPos < 0) return -1;
    QString tempPath = strPath.left(nPos);
    if (QDir().exists(tempPath)) return 1;
    CheckPathAndCreate(tempPath);
    QDir().mkdir(tempPath);
    return 0;
}

QString CPatch::PathPatchToFile(QString strPatchPath)
{
    strPatchPath.replace('\\', '/');
    return strPatchPath.right(strPatchPath.size() - 2);
}

QString CPatch::PathFileToPatch(QString strFilesPath)
{
    strFilesPath.replace('/', '\\');
    return QString("..").append(strFilesPath);
}

bool CPatch::MoveFilePath(const QString oldFilePath, const QString newFilePath)
{
    CheckPathAndCreate(newFilePath);
    const wstring& str1 = oldFilePath.toStdWString();
    const wstring& str2 = newFilePath.toStdWString();
    if (0 == MoveFile(str1.data(), str2.data()))
    {
        SetFileAttributes(str2.data(), FILE_ATTRIBUTE_NORMAL);
        DeleteFile(str2.data());
        return MoveFile(str1.data(), str2.data());
    }
    return true;
}

bool CPatch::CopyFilePath(const QString oldFilePath, const QString newFilePath)
{
    CheckPathAndCreate(newFilePath);
    const wstring& str1 = oldFilePath.toStdWString();
    const wstring& str2 = newFilePath.toStdWString();
    if (0 == CopyFile(str1.data(), str2.data(), FALSE))
    {
        SetFileAttributes(str2.data(), FILE_ATTRIBUTE_NORMAL);
        DeleteFile(str2.data());
        return CopyFile(str1.data(), str2.data(), FALSE);
    }
    return true;
}

int CPatch::ReadDataFile(QVector<SDataInfo>& sDataList, QString strDataPath)
{
    ifstream inFile;
    inFile.open(strDataPath.toStdWString(), ios::binary);
    if (inFile.good())
    {
        int nDataNum = 0;   // 数据数量
        int nDataLen = 0;   // 数据长度
        inFile.seekg(50);
        inFile.read((char *)&nDataNum, 4);
        inFile.seekg(0, ios::end);
        nDataLen = inFile.tellg();
        if ((nDataNum < 1) && (nDataLen < 10485761))
        {
            inFile.close();
            return FILE_DATA_NULL;  // data数据为空
        }
        // 开始读取data数据列表
        for (int i = 0; i < nDataNum; i++)
        {
            SDataInfo tmpData = {1, "", "", 0, 0, 0};
            char cPatchpath[260] = {'\0'};
            int nIndex = 0;
            int nCompLen = 0;
            int nFileLen = 0;
            inFile.seekg(i * 304 + 54);
            inFile.read(cPatchpath, 260);
            inFile.read((char *)&nIndex, 4);
            inFile.read((char *)&nCompLen, 4);
            inFile.read((char *)&nFileLen, 4);

            tmpData.patchPath = cPatchpath;
            tmpData.index = nIndex;
            tmpData.compLen = nCompLen;
            tmpData.fileLen = nFileLen;

            sDataList.push_back(tmpData);
        }
        inFile.close();
        return FILE_OK;
    }
    else return FILE_OPEN_FAEL; // data打开失败
}

int CPatch::ReadDataDir(QVector<SDataInfo> &sDataList, QString strDirPath)
{
    if (!QDir().exists(strDirPath)) return DIR_PATH_FAEL;
    QDirIterator iterDir(strDirPath, QDirIterator::Subdirectories);
    while (iterDir.hasNext()) {
        iterDir.next();
        QFileInfo fileInfo = iterDir.fileInfo();
        if (fileInfo.isFile()) {
            SDataInfo tempDataInfo = {2, "", "", 0, 0, 0};
            QString tempFilePath = fileInfo.absoluteFilePath();
            tempDataInfo.filePath = tempFilePath;
            tempDataInfo.patchPath = PathFileToPatch(tempFilePath.right(tempFilePath.size() - strDirPath.size()));
            tempDataInfo.fileLen = fileInfo.size();
            sDataList.push_back(tempDataInfo);
        }
    }
    if (sDataList.size() == 0)
        return DIR_DATA_NULL;
    else
        return DIR_OK;
}

int CPatch::ReadClientList(QVector<QString> &strClientList, QString strGamePath, QString &strInfo, bool &bTerminator)
{
    bool bReadAsyncFile = true;
    QString strMode = QString::fromWCharArray(L"( 网络模式 )");

    // 加载客户端文件列表
    strInfo = QString::fromWCharArray(L"正在下载客户端列表...");
    QString strListPath = "BConfig/ClientFileList.xml";
    QString strXMLFile = "/LauncherCfg/ClientFileList.xml";
    QString strURL = "http://update.300hero.jumpwgame.com/xclient_unpack/bin/launchercfg/clientfilelist.xml";
    QString strTime = "?R=13&T=" + QString::number(time(0));
    if (!bTerminator) return 1;
    do {
        if (S_OK == URLDownloadToFile(NULL, QString(strURL).append(strTime).toStdWString().c_str(), L"BConfig/Temp/TEMP_XML", 0, NULL))
        {
            CopyFile(L"BConfig/Temp/TEMP_XML", strListPath.toStdWString().c_str(), FALSE);
            DeleteFile(L"BConfig/Temp/TEMP_XML"); // 删除临时文件
        }
        else
        {
            if (!bTerminator) return 1;
            if (S_OK == URLDownloadToFile(NULL,  QString(strURL).append(".gz").append(strTime).toStdWString().c_str(), L"BConfig/Temp/TEMP_XML.gz", 0, NULL))
            {
                CPatch::UncompTYGzFile("BConfig/Temp/TEMP_XML.gz", "BConfig/Temp/TEMP_XML");
                CopyFile(L"BConfig/Temp/TEMP_XML", strListPath.toStdWString().c_str(), FALSE);
                DeleteFile(L"BConfig/Temp/TEMP_XML"); // 删除临时文件
            }
            else
            {
                if (!bTerminator) return 1;
                strInfo = QString::fromWCharArray(L"列表下载失败，切换本地模式...");
                if (!strGamePath.isEmpty())
                {
                    if (QFile().exists(strGamePath + strXMLFile))
                        strListPath = strGamePath + strXMLFile;
                    else if (!QFile().exists(strListPath))
                    {
                        strInfo = QString::fromWCharArray(L"客户端列表加载失败!");
                        return FILE_OPEN_FAEL;
                    }
                    strMode = QString::fromWCharArray(L"( 本地模式 )");
                }
            }
        }

        // 读取客户端文件列表
        if (!bTerminator) return 1;
        strInfo = strMode + QString::fromWCharArray(L"列表文件加载成功，正在读取... ");
        strClientList.reserve(65536);
        QFile inFile(strListPath);
        if (inFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            while (!inFile.atEnd() && bTerminator)
            {
                QString strLine(inFile.readLine());
                int nPos1 = strLine.indexOf("\"..");
                if (nPos1 < 0) continue;
                int nPos2 = strLine.indexOf("\" M");
                if (nPos2 < 0) continue;
                QString strPatch(strLine.mid(nPos1 + 1, nPos2 - nPos1 - 1));
                int nPos3 = strPatch.indexOf('&');
                if (nPos3 > 0)
                {
                    int nPos4 = strPatch.indexOf(';');
                    strPatch.remove(nPos3 + 1, nPos4 - nPos3);
                }
                strClientList.push_back(strPatch);
            }
            inFile.close();
        }

        // 修订内容：TY更新了客户端列表，将异步文件列表从总文件列表中分离了，这里再读取一次异步文件列表
        if (!bTerminator) return 1;
        if (strXMLFile != "/asyncfile/asyncclientfilelist.xml")
        {
            for (auto i : strClientList)
            {
                if (i.contains("..\\ui\\hero\\233.bmp", Qt::CaseInsensitive))
                {
                    bReadAsyncFile = false;
                    break;
                }
            }
            strListPath = "BConfig/asyncclientfilelist.xml";
            strXMLFile = "/asyncfile/asyncclientfilelist.xml";
            strURL = "http://update.300hero.jumpwgame.com/xclient_unpack/bin/asyncfile/asyncclientfilelist.xml";
            strInfo = QString::fromWCharArray(L"正在下载异步资源列表...");
        }
        else bReadAsyncFile = false;
    } while (bReadAsyncFile && bTerminator);

    // 最后检查客户端列表是否为空
    if (!bTerminator) return 1;
    if (strClientList.isEmpty())
    {
        strInfo = strMode + QString::fromWCharArray(L"列表文件读取失败!");
        return FILE_DATA_NULL;
    }
    else
    {
        strInfo = strMode;
        return FILE_OK;
    }
}

int CPatch::ExportDataFile(SDataInfo sDataInfo, QString strDataPath, QString strFilePath)
{
    ifstream inFile(strDataPath.toStdWString(), ios::binary);
    if (inFile.good())
    {
        int nCompLen = sDataInfo.compLen;
        int nFileLen = sDataInfo.fileLen;
        unsigned char* pcCompData = new unsigned char[nCompLen];

        inFile.seekg(sDataInfo.index);
        inFile.read((char *)pcCompData, nCompLen);
        inFile.close();

        if (nFileLen > 0) {
            // zlib解压
            unsigned char* pcFileData = new unsigned char[nFileLen];
            if (Z_OK == z_uncompress(pcFileData, (unsigned long*)&nFileLen, pcCompData, nCompLen)) {
                CheckPathAndCreate(strFilePath);
                ofstream outFile(strFilePath.toStdWString(), ios::binary);
                outFile.write((char *)pcFileData, nFileLen);
                outFile.close();
                delete [] pcCompData;
                delete [] pcFileData;
                return FILE_OK; // 导出正常
            }
            else {
                delete [] pcCompData;
                delete [] pcFileData;
                return FILE_ZLIB_FAEL;  // zlib解压失败
            }
        }
        else {
            // 无压缩文件
            CheckPathAndCreate(strFilePath);
            ofstream outFile(strFilePath.toStdWString(), ios::binary);
            outFile.write((char *)pcCompData, nCompLen);
            outFile.close();
            delete [] pcCompData;
            return FILE_OK; // 导出正常
        }
    }
    else return FILE_OPEN_FAEL; // data打开失败
}

void CPatch::UnpackDataFile(int nID, QString strDataPath, SExportConfig sExportConfig, SRealTimeData &sRtData, bool& bTerminator)
{
    // 1 准备执行
    sRtData.nState = 1;
    QString strTempFile = "BConfig/Temp/TEMP_FEIL_" + QString::number(nID);    // 临时文件
    sRtData.nDataNum = 0;
    streampos nDataLen = 0;
    ifstream inFile;
    inFile.open(strDataPath.toStdWString(), ios::binary);
    if (inFile.is_open() == false)
    {
        sRtData.nState = -1;	// -1 打开失败
    }
    else
    {
        inFile.seekg(0, ios::end);
        nDataLen = inFile.tellg();
        if (nDataLen < 10485760)
        {
            sRtData.nState = -2;	// -2 数据为空
        }
        else
        {
            inFile.seekg(50);
            inFile.read((char *)&sRtData.nDataNum, 4);
            if (sRtData.nDataNum < 1)
            {
                sRtData.nState = -2;	// -2 数据为空
            }
            else
            {
                // 2 正在执行
                sRtData.nState = 2;
                for (int i = 0; (i < sRtData.nDataNum) && bTerminator; i++)
                {
                    char cPatchPath[260] = { '\0' };	// 补丁路径
                    unsigned long nIndex = 0;			// 索引
                    int nCompLen = 0;                   // 压缩数据长度
                    int nFileLen = 0;                   // 文件数据长度
                    inFile.seekg(i * 304 + 54);
                    inFile.read(cPatchPath, 260);
                    inFile.read((char *)&nIndex, 4);
                    inFile.read((char *)&nCompLen, 4);
                    inFile.read((char *)&nFileLen, 4);
                    // 转换文件路径
                    QString strNewFilePath = sExportConfig.strExportPath + PathPatchToFile(cPatchPath);

                    // 检查目标文件是否已存在（非覆盖模式下，将忽略已存在文件的重复导出）
                    if (QFile().exists(strNewFilePath) && (!sExportConfig.nExportMode))
                    {
                        sRtData.nIgnoreNum += 1;    // 忽略数据 +1
                    }
                    else if ((string(cPatchPath) == "_INVALID_BLOCK_") || (nCompLen == 0) || (nFileLen == 0))
                    {
                        sRtData.nNullNum += 1;	// 废弃数据 +1
                    }
                    else
                    {
                        unsigned char *pcCompData = new unsigned char[nCompLen];	// 压缩数据块
                        inFile.seekg(nIndex);
                        inFile.read((char *)pcCompData, nCompLen);
                        if (nFileLen > 0) {
                            // 解压zlib数据
                            unsigned char *pcFileData = new unsigned char[nFileLen];	// 文件数据块
                            if (Z_OK != z_uncompress(pcFileData, (unsigned long*)&nFileLen, pcCompData, nCompLen)) {
                                sRtData.nFailNum += 1;	// 失败数量 +1
                            }
                            else {
                                // 写入临时文件
                                ofstream outFile;
                                outFile.open(strTempFile.toStdWString(), ios::binary);
                                outFile.write((char *)pcFileData, nFileLen);
                                outFile.close();
                                // 移动到目标路径
                                MoveFilePath(strTempFile, strNewFilePath);
                                sRtData.nSuccessNum += 1;	// 成功数量 +1
                            }
                            delete[] pcCompData;
                            delete[] pcFileData;
                        }
                        else {
                            // 无压缩文件
                            ofstream outFile;
                            outFile.open(strTempFile.toStdWString(), ios::binary);
                            outFile.write((char *)pcCompData, nCompLen);
                            outFile.close();
                            delete[] pcCompData;
                            MoveFilePath(strTempFile, strNewFilePath);
                            sRtData.nSuccessNum += 1;	// 成功数量 +1
                        }
                    }
                }
                sRtData.nState = 3;		// 3 结束执行
            }
        }
        inFile.close();
    }
    return;
}

void CPatch::UnpackDirFile(QString strDirPath, SExportConfig sExportConfig, SRealTimeData &sRtData, bool &bTerminator)
{
    // 1 准备执行
    sRtData.nState = 1;
    sRtData.nDataNum = 0;
    QVector<SDataInfo> sDataList;
    int ret = ReadDataDir(sDataList, strDirPath);
    if (ret == FILE_OK)
    {
        // 2 正在执行
        sRtData.nState = 2;
        sRtData.nDataNum = sDataList.size();
        for (int i = 0; (i < sDataList.size()) && bTerminator; i++)
        {
            // 转换文件路径
            QString strNewFilePath = sExportConfig.strExportPath + PathPatchToFile(sDataList[i].patchPath);
            // 检查目标文件是否已存在（非覆盖模式下，将忽略已存在文件的重复导出）
            if (QFile().exists(strNewFilePath) && (!sExportConfig.nExportMode))
            {
                sRtData.nIgnoreNum += 1;    // 忽略数据 +1
            }
            else
            {
                // 复制到目标路径
                CheckPathAndCreate(strNewFilePath);
                if (0 == CopyFile(sDataList[i].filePath.toStdWString().c_str(), strNewFilePath.toStdWString().c_str(), FALSE))
                {
                    // 如果权限不够复制失败，将目标文件设置为可读写后再复制
                    SetFileAttributes(strNewFilePath.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (0 == CopyFile(sDataList[i].filePath.toStdWString().c_str(), strNewFilePath.toStdWString().c_str(), FALSE))
                        sRtData.nFailNum += 1;      // 失败数量 +1
                    else
                        sRtData.nSuccessNum += 1;	// 成功数量 +1
                }
                else
                    sRtData.nSuccessNum += 1;	// 成功数量 +1
            }
        }
        sRtData.nState = 3;		// 3 结束执行
    }
    else if (ret == FILE_OPEN_FAEL)
    {
        sRtData.nState = -1;	// -1 打开失败;
    }
    else if (ret == FILE_DATA_NULL)
    {
        sRtData.nState = -2;	// -2 数据为空
    }
}

int CPatch::UnpackToDataInfo(SRealTimeData sRtData, int &nProgress, QString &strInfo)
{
    if (sRtData.nState == 0)
    {
        nProgress = 0;
        strInfo = QString::fromWCharArray(L"正在等待...");
        return 0;
    }
    else if (sRtData.nState == 1)
    {
        nProgress = 0;
        strInfo = QString::fromWCharArray(L"正在准备读取...");
        return 0;
    }
    else if (sRtData.nState == 2)
    {
        int nFinishedNum = sRtData.nSuccessNum + sRtData.nFailNum + sRtData.nNullNum + sRtData.nIgnoreNum;
        nProgress = (double)nFinishedNum / sRtData.nDataNum * 100;
        strInfo = QString::fromWCharArray(L"总数：") + StrIntL(sRtData.nDataNum, 7)
                + QString::fromWCharArray(L"成功：") + StrIntL(sRtData.nSuccessNum, 7)
                + QString::fromWCharArray(L"失败：") + StrIntL(sRtData.nFailNum, 7)
                + QString::fromWCharArray(L"废弃：") + StrIntL(sRtData.nNullNum, 7)
                + QString::fromWCharArray(L"忽略：") + StrIntL(sRtData.nIgnoreNum, 7)
                + QString::fromWCharArray(L"剩余：") + StrIntL(sRtData.nDataNum - nFinishedNum, 6);
        return 0;
    }
    else if (sRtData.nState == 3)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"总数：") + StrIntL(sRtData.nDataNum, 7)
                + QString::fromWCharArray(L"成功：") + StrIntL(sRtData.nSuccessNum, 7)
                + QString::fromWCharArray(L"失败：") + StrIntL(sRtData.nFailNum, 7)
                + QString::fromWCharArray(L"废弃：") + StrIntL(sRtData.nNullNum, 7)
                + QString::fromWCharArray(L"忽略：") + StrIntL(sRtData.nIgnoreNum, 7)
                + QString::fromWCharArray(L"(已完成)");
        return 1;
    }
    else if (sRtData.nState == -1)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"错误：读取失败！");
        return 1;
    }
    else if (sRtData.nState == -2)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"错误：数据为空！");
        return 1;
    }
    return -1;
}

void CPatch::PackDataFile(QVector<SDataInfo> sDataList, SRealTimeData &sRtData, bool &bTerminator)
{
    const int& nCompDataPos = 10485760;         // 压缩数据存储起始位置 10MB = 10 * 1024 * 1024
    const int& nJmpDataMax = 0x7FFFFFFF;        // 文件数据存储上限大小 2^31 - 1
    const int& nIndexListMax = 34492;           // 索引列表存储上限大小 (nJmpDataMax - nCompDataPos - 54) / 304
    const int& nIndexAverage = 61956;           // 索引的存储数据平均值 nCompDataMax / nIndexListMax
    const char strNull[32]{ 0 };                // 空值

    // 1 准备执行
    sRtData.nState = 1;
    sRtData.nDataNum = 0;
    string strDataPath[4] = { "Data.jmp", "Data1.jmp", "Data2.jmp", "Data3.jmp" };
    int nDataIndex = 0;
    int nDataNum = 0;
    int nDataLen = nCompDataPos;

    // 创建4个空data文件
    for (int i = 0; i < 4; i++)
    {
        ofstream outFile(strDataPath[i]);
        outFile << "DATA" << to_string(i + 1) << ".0";
        outFile.close();
    }

    // 创建索引链表
    list<const SDataInfo*> infos;
    for(const auto& it : sDataList) infos.emplace_back(&it);
    infos.sort([](const SDataInfo* p1, const SDataInfo* p2) -> bool { return p1->fileLen > p2->fileLen; });

    // 2 开始执行
    sRtData.nState = 2;
    sRtData.nDataNum = sDataList.size();
    fstream outData;
    for (auto& strPath : strDataPath) {
        outData.open(strPath, ios::in | ios::out | ios::binary);
        if (outData.is_open()) break;
        nDataIndex++;
    }
    int nAverage = 0;   // 平均值
    while (!infos.empty()) {
        if (!bTerminator) break;
        // 选取合适文件
        const SDataInfo* pinfo = nullptr;
        if (nAverage < nIndexAverage) {
            pinfo = infos.front();
            infos.pop_front();
        }
        else {
            pinfo = infos.back();
            infos.pop_back();
        }
        // 获取压缩数据
        ifstream inFile;
        inFile.open(pinfo->filePath.toStdWString(), ios::binary);
        if (inFile.is_open() == false) {
            sRtData.nFailNum += 1;  // 失败数量 +1
            continue;
        }
        unsigned char *pcFileData = new unsigned char[pinfo->fileLen];
        inFile.read((char*)pcFileData, pinfo->fileLen);
        inFile.close();
        int nCompLen = z_compressBound(pinfo->fileLen);
        unsigned char *pcCompData = new unsigned char[nCompLen];
        if (Z_OK != z_compress(pcCompData, (unsigned long*)&nCompLen, pcFileData, pinfo->fileLen)) {
            sRtData.nFailNum += 1;  // 失败数量 +1
            delete [] pcFileData;
            delete [] pcCompData;
            continue;
        }
        delete [] pcFileData;
        // 验证data.jmp状态
        if (nDataNum + 1 > nIndexListMax || nDataLen + nCompLen > nJmpDataMax) {
            // 超出上限值，更换data
            outData.seekp(50);
            outData.write((char*)&nDataNum, 4);
            outData.close();
            nDataIndex++;
            nDataNum = 0;
            nDataLen = nCompDataPos;
            nAverage = 0;
            while (nDataIndex < 4) {
                outData.open(strDataPath[nDataIndex], ios::in | ios::out | ios::binary);
                if (outData.is_open()) break;
                nDataIndex++;
            }
            if (nDataIndex >= 4) {
                sRtData.nState = -3;    // -3 数据已满
                delete [] pcCompData;
                break;
            }
        }
        // 写入data.jmp
        if (nAverage) nAverage = (nAverage + nCompLen) * 0.5;
        else nAverage = nCompLen;
        string&& strPatch = pinfo->patchPath.toStdString();
        strPatch.resize(260, '\0');
        outData.seekp(54 + 304 * nDataNum);
        outData.write((char *)strPatch.c_str(), 260);
        outData.write((char *)&nDataLen, 4);            // 文件索引
        outData.write((char *)&nCompLen, 4);            // 压缩后大小
        outData.write((char *)&pinfo->fileLen, 4);      // 压缩前大小
        outData.write(strNull, 32);                     // 间隔空值
        outData.seekp(nDataLen);
        outData.write((char *)pcCompData, nCompLen);		// 压缩数据
        delete [] pcCompData;
        nDataNum++;
        nDataLen += nCompLen;
        sRtData.nSuccessNum += 1;   // 成功数量 +1
    }
    if (nDataIndex < 4) {
        outData.seekp(50);
        outData.write((char*)&nDataNum, 4);
        outData.close();
    }

    // 3 结束执行
    sRtData.nState = 3;
}

int CPatch::PackToDataInfo(SRealTimeData sRtData, int &nProgress, QString &strInfo)
{
    if (sRtData.nState == 0)
    {
        nProgress = 0;
        strInfo = QString::fromWCharArray(L"正在等待...");
        return 0;
    }
    else if (sRtData.nState == 1)
    {
        nProgress = 0;
        strInfo = QString::fromWCharArray(L"正在准备打包...");
        return 0;
    }
    else if (sRtData.nState == 2)
    {
        int nFinishedNum = sRtData.nSuccessNum + sRtData.nFailNum;
        nProgress = (double)nFinishedNum / sRtData.nDataNum * 100;
        strInfo = QString::fromWCharArray(L"总数：") + StrIntL(sRtData.nDataNum, 7)
                + QString::fromWCharArray(L"成功：") + StrIntL(sRtData.nSuccessNum, 7)
                + QString::fromWCharArray(L"失败：") + StrIntL(sRtData.nFailNum, 7)
                + QString::fromWCharArray(L"剩余：") + StrIntL(sRtData.nDataNum - nFinishedNum, 6);
        return 0;
    }
    else if (sRtData.nState == 3)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"总数：") + StrIntL(sRtData.nDataNum, 7)
                + QString::fromWCharArray(L"成功：") + StrIntL(sRtData.nSuccessNum, 7)
                + QString::fromWCharArray(L"失败：") + StrIntL(sRtData.nFailNum, 7)
                + QString::fromWCharArray(L"(已完成)");
        return 1;
    }
    else if (sRtData.nState == -3)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"错误：打包文件超出data容量上限！");
        return 1;
    }
    return -1;
}

int CPatch::DownloadTYGzFile(QString strPatchPath, QString strSavePath)
{
    QString strURL = "http://update.300hero.jumpwgame.com/xclient_unpack" + PathPatchToFile(strPatchPath) + ".gz?R=13&T=" + QString::number(time(0));
    if (QFile().exists("BConfig/Temp/TEMP_GZ")) DeleteFile(L"BConfig/Temp/TEMP_GZ");
    if (S_OK == URLDownloadToFile(NULL, strURL.toStdWString().c_str(), L"BConfig/Temp/TEMP_GZ", 0, NULL))
    {
        CheckPathAndCreate(strSavePath);
        if (FILE_OK == UncompTYGzFile("BConfig/Temp/TEMP_GZ", strSavePath))
            return 0;
        else
            return -2;
    }
    else
        return -1;
}

void CPatch::DownloadDataFile(QVector<SDataInfo> sDataList, QString strSavePath, SRealTimeData &sRtData, bool &bTerminator)
{
    // 1 准备执行
    sRtData.nState = 1;
    sRtData.nDataNum = 0;

    // 测试是否能够连接到服务器
    if (QFile().exists("BConfig/Temp/TEMP_GZ")) DeleteFile(L"BConfig/Temp/TEMP_GZ");
    if (S_OK == URLDownloadToFile(NULL, wstring(L"http://update.300hero.jumpwgame.com/xclient_unpack/bin/asyncfile/asyncclientfilelist.xml.gz?R=13&T=").append(to_wstring(time(0))).c_str(), L"BConfig/Temp/TEMP_GZ", 0, NULL))
    {
        DeleteFile(L"BConfig/Temp/TEMP_GZ");
        // 2 正在执行
        sRtData.nState = 2;
        sRtData.nDataNum = sDataList.size();
        for (int i = 0; i < sDataList.size(); i++)
        {
            if (!bTerminator) break;
            if (0 == DownloadTYGzFile(sDataList[i].patchPath, strSavePath + PathPatchToFile(sDataList[i].patchPath)))
                sRtData.nSuccessNum += 1;
            else
                sRtData.nFailNum += 1;
        }
        sRtData.nState = 3;		// 3 结束执行
    }
    else
    {
        sRtData.nState = -1;	// -1 连接服务器失败;
    }
}

int CPatch::DownloadToDataInfo(SRealTimeData sRtData, int &nProgress, QString &strInfo)
{
    if (sRtData.nState == 0)
    {
        nProgress = 0;
        strInfo = QString::fromWCharArray(L"正在等待...");
        return 0;
    }
    else if (sRtData.nState == 1)
    {
        nProgress = 0;
        strInfo = QString::fromWCharArray(L"正在准备下载...");
        return 0;
    }
    else if (sRtData.nState == 2)
    {
        int nFinishedNum = sRtData.nSuccessNum + sRtData.nFailNum;
        nProgress = (double)nFinishedNum / sRtData.nDataNum * 100;
        strInfo = QString::fromWCharArray(L"总数：") + StrIntL(sRtData.nDataNum, 7)
                + QString::fromWCharArray(L"成功：") + StrIntL(sRtData.nSuccessNum, 7)
                + QString::fromWCharArray(L"失败：") + StrIntL(sRtData.nFailNum, 7)
                + QString::fromWCharArray(L"剩余：") + StrIntL(sRtData.nDataNum - nFinishedNum, 6);
        return 0;
    }
    else if (sRtData.nState == 3)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"总数：") + StrIntL(sRtData.nDataNum, 7)
                + QString::fromWCharArray(L"成功：") + StrIntL(sRtData.nSuccessNum, 7)
                + QString::fromWCharArray(L"失败：") + StrIntL(sRtData.nFailNum, 7)
                + QString::fromWCharArray(L"(已完成)");
        return 1;
    }
    else if (sRtData.nState == -1)
    {
        nProgress = 100;
        strInfo = QString::fromWCharArray(L"错误：连接服务器失败！");
        return 1;
    }
    return -1;
}

int CPatch::UncompTYGzFile(QString strGzFilePath, QString strNewFilePath)
{
    ifstream inFile;
    inFile.open(strGzFilePath.toStdWString(), ios::binary);
    if (inFile.good())
    {
        int nCompLen = 0;
        int nFileLen = 0;
        inFile.read((char *)&nFileLen, 4);
        inFile.seekg(0, ios::end);
        nCompLen = inFile.tellg();
        nCompLen -= 4;
        unsigned char *pcCompData = new unsigned char[nCompLen];	// 压缩数据块
        inFile.seekg(4);
        inFile.read((char *)pcCompData, nCompLen);
        inFile.close();
        int ret = Z_OK;
        if (nFileLen > 0) {
            // 解压zlib数据
            unsigned char *pcFileData = new unsigned char[nFileLen];	// 文件数据块
            ret = z_uncompress(pcFileData, (unsigned long*)&nFileLen, pcCompData, nCompLen);
            if (ret == Z_OK) {
                // 写入临时文件
                ofstream outFile;
                outFile.open(strNewFilePath.toStdWString(), ios::binary);
                outFile.write((char *)pcFileData, nFileLen);
                outFile.close();
            }
            delete[] pcFileData;
        }
        else {
            // 无压缩文件
            ofstream outFile;
            outFile.open(strNewFilePath.toStdWString(), ios::binary);
            outFile.write((char *)pcCompData, nFileLen);
            outFile.close();
        }
        delete[] pcCompData;
        DeleteFile(strGzFilePath.toStdWString().c_str()); // 删除临时文件
        if (ret == Z_OK) return FILE_OK;
        else return FILE_ZLIB_FAEL;
    }
    else return FILE_OPEN_FAEL; // 文件打开失败
}

QByteArray CPatch::ReadDataFileOfData(SDataInfo sDataInfo, QString strDataPath)
{
    if (sDataInfo.type == 1) {
        QFile inFile(strDataPath);
        if (inFile.open(QIODevice::ReadOnly)) {
            inFile.seek(sDataInfo.index);
            QByteArray compData = inFile.read(sDataInfo.compLen);
            inFile.close();
            if (sDataInfo.fileLen > 0) {
                QByteArray fileData(sDataInfo.fileLen, '\0');
                if (Z_OK == z_uncompress((unsigned char*)fileData.data(), (unsigned long*)&sDataInfo.fileLen, (const unsigned char*)compData.data(), sDataInfo.compLen)) {
                    fileData.resize(sDataInfo.fileLen);
                    return fileData;
                }
                else QByteArray();
            }
            else return compData;
        }
    }
    else if (sDataInfo.type == 2) {
        QFile inFile(sDataInfo.filePath);
        if (inFile.open(QIODevice::ReadOnly)) {
            QByteArray data = inFile.readAll();
            inFile.close();
            return data;
        }
    }
    return QByteArray();
}
