#include "data_main.h"
#include "ui_data_main.h"

#include "config.h"
#include "data_item.h"
#include "form_image.h"
#include "loading.h"
#include "tool.h"

#include <QCryptographicHash>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

std::atomic_bool g_stop;
std::atomic_bool g_pause;
#define ThreadInit g_stop = false, g_pause = false
#define IsThreadStop while(g_pause) QThread::msleep(200); if (g_stop) return

/* 设置进度(线程安全) */
void SetProgress(const int &i, const int &value, std::atomic_int *progress = nullptr) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    static QVector<int> values;
    static std::atomic_int *p = nullptr;
    if (progress) values = QVector<int>(i, 0), p = progress;
    else {
        values[i] = value;
        *p = std::accumulate(values.begin(), values.end(), 0);
    }
};


Data_main::Data_main(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Data_main),
    m_config(&qConfig)
{
    ui->setupUi(this);

    // 功能组
    ui->spinBox_num->setValue(m_config->nMainColumn);
    connect(ui->spinBox_num, &QSpinBox::editingFinished, this, &Data_main::onUpdateColumnNum);
    connect(ui->button_foldNull, &QPushButton::clicked, this, &Data_main::onButtonFoldNull);
    connect(ui->button_md5, &QPushButton::clicked, this, &Data_main::onButtonMd5);
    connect(ui->button_organizeData, &QPushButton::clicked, this, &Data_main::onButtonOrganizeData);
    connect(ui->button_exportAll, &QPushButton::clicked, this, &Data_main::onButtonExportAll);
    connect(ui->button_packAll, &QPushButton::clicked, this, &Data_main::onButtonPackAll);
    connect(ui->button_downloadAll, &QPushButton::clicked, this, &Data_main::onButtonDownloadAll);
    connect(ui->button_lackList, &QPushButton::clicked, this, &Data_main::onButtonLackList);

    // 略缩图预览器
    static const QVector<int> imgsize{300,480,720};
    m_image = new Form_Image(this, m_config->bPreview, imgsize[qBound(0, m_config->nPreviewSize, 2)], m_config->nPreviewMode);
    connect(m_config, &Config::bPreview_Changed, m_image, [=]{m_image->updateEnable(m_config->bPreview);});
    connect(m_config, &Config::nPreviewMode_Changed, m_image, [=]{m_image->updateMode(m_config->nPreviewMode);});
    connect(m_config, &Config::nPreviewSize_Changed, m_image, [=]{m_image->updateSize(imgsize[qBound(0, m_config->nPreviewSize, 2)]);});
}

Data_main::~Data_main()
{
    delete ui;
}

void Data_main::load(const int &mode, const QVariant &data)
{
    clear();
    m_mode = mode;
    if      (mode == 1) { // 读取游戏资源目录
        ui->widget_num->setVisible(1);
        ui->button_foldNull->setVisible(1);
        ui->button_md5->setVisible(1);
        ui->button_organizeData->setVisible(1);
        ui->button_exportAll->setVisible(1);
        ui->button_lackList->setVisible(1);
        for (const auto &file : data.toStringList()) newItem(1, file);
        newItem(2, CPatch::GetAsyncDirPath(m_config->strGamePath));
        m_lack = newItem(4);
        columnMaxChanged();
        qLoading->start(u8"正在读取游戏数据");
        for (auto &item : m_items) connect(item, &Data_item::loaded, qLoading->object, [=]{
            for (auto &item : m_items) if (!item->isLoaded()) return;
            restoreInfoBar();
            qLoading->stop();
        });
        for (auto &item : m_items) item->load();
    }
    else if (mode == 2) { // 读取单个data资源
        ui->button_md5->setVisible(1);
        ui->button_organizeData->setVisible(1);
        ui->button_exportAll->setVisible(1);
        auto item = newItem(1, data.toString());
        columnMaxChanged();
        qLoading->start(u8"正在读取文件数据");
        connect(item, &Data_item::loaded, qLoading->object, [=]{ restoreInfoBar(), qLoading->stop(); });
        item->load();
    }
    else if (mode == 3) { // 读取本地资源目录
        ui->button_packAll->setVisible(1);
        ui->button_lackList->setVisible(1);
        auto item = newItem(2, m_config->getExportPath());
        m_lack = newItem(4);
        columnMaxChanged();
        qLoading->start(u8"正在读取本地资源目录\n首次遍历目录可能较慢");
        connect(item, &Data_item::loaded, qLoading->object, [=]{ restoreInfoBar(), qLoading->stop(); });
        item->load();
    }
    else if (mode == 4) { // 读取网络资源列表
        ui->button_downloadAll->setVisible(1);
        auto item = newItem(3);
        columnMaxChanged();
        qLoading->start(u8"正在获取网络资源列表");
        connect(item, &Data_item::loaded, qLoading->object, [=]{ restoreInfoBar(), qLoading->stop(); });
        item->load();
    }
    m_image->init();
}

void Data_main::reload(const int &count)
{
    if      (m_mode == 1) { // 游戏资源目录
        qLoading->start(u8"正在更新视图");
        int oldsize = m_items.size() - 1;
        if (oldsize < count) {
            for (int i = m_items.size() - 1; i < count; ++i) {
                newItem(1, QString("%1/Data%2.jmp").arg(m_config->strGamePath).arg(i));
                std::swap(m_items[i], m_items[i+1]);
            }
            columnMaxChanged();
        }
        else if (oldsize > count) {
            QList<Data_item*> items;
            auto itemBack = m_items.takeLast();
            for (int i = oldsize - count; i > 0; --i) delete m_items.takeLast();
            m_items.append(itemBack);
            columnMaxChanged();
        }
        for (auto &item : m_items) connect(item, &Data_item::loaded, qLoading->object, [=]{
            for (auto &item : m_items) if (!item->isLoaded()) return;
            columnMaxChanged();
            restoreInfoBar();
            if (m_lack) {
                connect(m_lack, &Data_item::loaded, qLoading->object, [=]{ qLoading->stop(); });
                m_lack->updateLacks(m_items);
            }
            else qLoading->stop();
        });
        for (auto &item : m_items) item->load();
    }
    else if (m_mode == 2) { // 单个data资源
        if (m_items.size() <= 0) return;
        qLoading->start(u8"正在更新视图");
        connect(m_items[0], &Data_item::loaded, qLoading->object, [=]{ restoreInfoBar(), qLoading->stop(); });
        m_items[0]->load();
    }
    else if (m_mode == 3) { // 本地资源目录
        if (m_items.size() <= 0) return;
        qLoading->start(u8"正在更新视图");
        connect(m_items[0], &Data_item::loaded, qLoading->object, [=]{
            restoreInfoBar();
            if (m_lack) {
                connect(m_lack, &Data_item::loaded, qLoading->object, [=]{ qLoading->stop(); });
                m_lack->updateLacks(m_items);
            }
            else qLoading->stop();
        });
        m_items[0]->load();
    }
}

void Data_main::clear()
{
    m_mode = 0;

    // 信息栏
    m_bFoldNull = false;
    ui->label_info->clear();
    ui->widget_num->setVisible(0);
    ui->button_foldNull->setVisible(0);
    ui->button_foldNull->setText(u8"折叠空视图");
    ui->button_md5->setVisible(0);
    ui->button_organizeData->setVisible(0);
    ui->button_exportAll->setVisible(0);
    ui->button_packAll->setVisible(0);
    ui->button_downloadAll->setVisible(0);
    ui->button_lackList->setVisible(0);
    ui->button_lackList->setText(u8"展开缺失列表");

    // 主视窗
    m_columns = 1;
    while (auto child = ui->layout_itemList->takeAt(0)) delete child;
    qDeleteAll(m_items);
    m_items.clear();

    // 缺失资源列表
    if (auto child = ui->layout_lackList->takeAt(0)) delete child;
    if (m_lack) delete m_lack;
    m_lack = nullptr;
    ui->frame_lackList->setVisible(0);
    ui->layout_main->setStretch(0, 2);
    ui->layout_main->setStretch(1, 0);

    // 略缩图预览器
    m_image->clear();
}

void Data_main::lookup(const QString &key)
{
    int count = 0;
    for (auto &item : m_items) count += item->lookup(key);
    if (m_lack) count += m_lack->lookup(key);
    if (key.isEmpty()) restoreInfoBar();
    else {
        ui->label_info->setText(QString(u8"一共查找到 %1 项资源。").arg(count));
    }
}

void Data_main::restoreInfoBar()
{
    if (m_mode >= 1 && m_mode <= 4) {
        int n = 0;
        for (auto &item : m_items) n += item->datas()->size();
        ui->label_info->setText(QString(u8"一共 %1 个视图，%2 项资源。").arg(m_items.size()).arg(n));
    }
    else ui->label_info->clear();
}

void Data_main::onResize(QSize)
{
    m_image->checkBounds();
    columnMaxChanged();
}

void Data_main::columnMaxChanged()
{
    const bool bLackList = ui->frame_lackList->isVisible();
    int columnMax = m_config->nMainColumn;
    int itemSize = m_items.size();
    if (itemSize) {
        if (m_bFoldNull) for (auto &item : m_items) if (item->isNull()) itemSize -= 1;
        int size = itemSize;
        if (size < columnMax) columnMax = size;
        int n = width() / Data_item::minWidth();
        if (bLackList) n -= 1;
        if (n < columnMax) columnMax = n;
    }
    else columnMax = 1;
    if (columnMax == m_columns && itemSize == ui->layout_itemList->count()) return;
    m_columns = columnMax;
    while (auto child = ui->layout_itemList->takeAt(0)) delete child;
    for (auto &item : m_items) {
        if (m_bFoldNull && item->isNull()) item->setVisible(0);
        else {
            item->setVisible(1);
            int index = ui->layout_itemList->count();
            ui->layout_itemList->addWidget(item, index / columnMax, index % columnMax);
        }
    }
    ui->layout_main->setStretch(0, qMax(2, columnMax));
}

void Data_main::onUpdateImage(SDataInfo *pDataInfo)
{
    m_image->updateImage(pDataInfo);
}

void Data_main::onUpdateColumnNum()
{
    m_config->set_nMainColumn(ui->spinBox_num->value());
    columnMaxChanged();
}

void Data_main::onButtonFoldNull()
{
    m_bFoldNull = !m_bFoldNull;
    ui->button_foldNull->setText(m_bFoldNull ? u8"显示空视图" : u8"折叠空视图");
    columnMaxChanged();
}

void Data_main::onButtonMd5()
{
    if (m_items.isEmpty()) return;
    if (QMessageBox::question(this, u8"提示", u8"是否更新所有资源的MD5特征码？") != QMessageBox::Yes) return;
    ThreadInit;
    qLoading->start(u8"正在更新MD5特征码", true, 0);
    CThread* thread = new CThread;
    connect(thread, &CThread::started, thread, [=]{
        SResultData result;
        ActionScope([&]{ emit thread->finished(QVariant::fromValue(result)); });

        QList<Data_item*> totalItems(m_items);
        if (m_mode == 1) totalItems.pop_back();

        // 多线程
        const int totalSize = totalItems.size();
        const int nProgressMax = 100 / totalSize;
        int oldProgress = 0;
        std::atomic_int progress{0};
        QVector<QThread*> ths(totalSize);
        QVector<SResultData> results(totalSize);
        SetProgress(totalItems.size(), 0, &progress); // 初始化进度
        for (int i = 0; i < totalSize; ++i) {
            result.total += (results[i].total = totalItems[i]->datas()->size());
            ths[i] = QThread::create([i, nProgressMax](QString jmpPath, QVector<SDataInfo> *datas, SResultData *r) {
                // 读取jmp文件
                QFile jmp(jmpPath);
                if (!jmp.open(QIODevice::ReadWrite)) {
                    r->fail = r->total;
                    SetProgress(i, nProgressMax);
                    return;
                }
                ActionScope([&]{ if (jmp.isOpen()) jmp.close(); });
                // 更新MD5
                LoadingProgress progress;
                progress.max = datas->size();
                for (auto &d : *datas) {
                    IsThreadStop;
                    if (CPatch::UpdateDataMd5(d, &jmp)) r->fail++;
                    else r->success++;
                    if (progress.add(nProgressMax)) SetProgress(i, progress.p); // 更新进度
                }
                IsThreadStop;
            }, totalItems[i]->path(), totalItems[i]->datas(), &results[i]);
        }
        for (auto &th : ths) th->start();

        // 等待线程完成
        bool bRunning = true;
        while (bRunning) {
            QThread::msleep(200);
            bRunning = false;
            for (auto &th : ths) {
                if (th->isFinished()) continue;
                bRunning = true;
                break;
            }
            if (oldProgress != progress) {
                oldProgress = progress;
                emit thread->message(oldProgress, {});
            }
        }
        for (auto &th : ths) delete th;

        // 更新数据
        for (auto &r : results) {
            result.success += r.success;
            result.fail += r.fail;
        }
        emit thread->message(100, {});
    });
    connect(thread, &CThread::message, this, [=](int value, QVariant){
        qLoading->setValue(value);
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        delete thread;
        auto r = result.value<SResultData>();
        QMessageBox::information(this, u8"提示", QString(u8"成功：%1  失败：%2  剩余：%3")
                                 .arg(r.success).arg(r.fail).arg(r.total - r.success - r.fail));
        qLoading->stop();
    });
    connect(qLoading, &Loading::cancel, qLoading->object, [=]{
        ActionScope([]{g_pause = true;},[]{g_pause = false;});
        if (QMessageBox::question(this, u8"提示", u8"是否取消操作？") == QMessageBox::Yes) qLoading->start(u8"正在取消操作"), g_stop = true;
    });
    thread->start();
}

void Data_main::onButtonOrganizeData()
{
    if (m_items.isEmpty()) return;
    if (QMessageBox::question(this, u8"提示", u8"是否开始整理资源？") != QMessageBox::Yes) return;
    QString path = m_mode == 1 ? m_config->strGamePath : m_items[0]->path();
    QVector<SDataInfoList*> list;
    for (auto &item : m_items) list.append(item->datas());
    if (m_mode == 1) list.pop_back(); // 游戏目录资源去掉最后一个异步文件夹
    importData(path, list);
}

void Data_main::onButtonExportAll()
{
    if (m_items.isEmpty()) return;
    if (QMessageBox::question(this, u8"提示", u8"是否导出全部资源？") != QMessageBox::Yes) return;
    QStringList paths;
    QVector<QList<SDataInfo*>> totalList;
    for (auto &item : m_items) {
        paths.push_back(item->type() == 1 ? item->path() : "");
        totalList.push_back({});
        auto &list = totalList.back();
        auto &datas = *item->datas();
        for (auto &d : datas) list.append(&d);
    }
    exportData(1, paths, totalList, true);
}

void Data_main::onButtonPackAll()
{
    if (m_items.isEmpty()) return;
    if (QMessageBox::question(this, u8"提示", u8"是否打包全部资源？") != QMessageBox::Yes) return;
    importData(qApp->applicationDirPath(), {m_items[0]->datas()});
}

void Data_main::onButtonDownloadAll()
{
    if (m_items.isEmpty()) return;
    if (QMessageBox::question(this, u8"提示", u8"是否下载全部资源？") != QMessageBox::Yes) return;
    QList<SDataInfo*> list;
    auto &datas = *m_items[0]->datas();
    for (auto &d : datas) list.append(&d);
    downloadData(2, list, true);
}

void Data_main::onButtonLackList()
{
    if (ui->frame_lackList->isVisible()) {
        ui->frame_lackList->setVisible(0);
        ui->button_lackList->setText(u8"展开缺失列表");
        ui->layout_main->setStretch(1, 0);
    }
    else {
        ui->frame_lackList->setVisible(1);
        ui->button_lackList->setText(u8"隐藏缺失列表");
        ui->layout_main->setStretch(1, 1);
    }
    columnMaxChanged();
}

void Data_main::onExportSelected(bool bAlone, const QList<SDataInfo*> &datas, Data_item *self)
{
    QString path = self->type() == 1 ? self->path() : "";
    exportData(bAlone ? 2 : 1, {path}, {datas});
}

void Data_main::onDownloadSelected(int type, const QList<SDataInfo *> &datas)
{
    downloadData(type, datas);
}

void Data_main::onDownload()
{
    if (!m_lack->isProofread() || m_lack->isNull()) return;
    if (QMessageBox::question(this, u8"提示", u8"是否下载全部缺失资源？") != QMessageBox::Yes) return;
    QList<SDataInfo*> datas;
    for (auto &d : *m_lack->datas()) datas.append(&d);
    downloadData(m_mode == 1 ? 1 : 2, datas, true);
}

void Data_main::onProofread()
{
    qLoading->start(u8"正在校对缺失资源");
    connect(m_lack, &Data_item::sigProgressText, qLoading->object, [=](QString text){ qLoading->setText(text); });
    connect(m_lack, &Data_item::loaded, qLoading->object, [=]{ qLoading->stop(); });
    if (m_lack->isProofread()) m_lack->updateLacks(m_items);
    else m_lack->load(m_items);
}

Data_item *Data_main::newItem(int type, const QString &strPath)
{
    // 类型: 1.jmp文件; 2.资源文件夹; 3.网络文件列表; 4.缺失文件列表
    auto item = new Data_item(this, m_mode, type, strPath);
    connect(item, &Data_item::tip, this, &Data_main::tip);
    connect(item, &Data_item::updateInfo, this, &Data_main::updateInfo);
    connect(item, &Data_item::sigClearOtherSelectItem, this, [=](Data_item *self){
        for (auto &item : m_items) if (item != self) item->clearSelectItem();
    });
    if (type <= 2) connect(item, &Data_item::preview, m_image, [=](SDataInfo *pDataInfo){
        if (!m_config->bPreview) return;
        CActionScope scope([=]{m_image->updateImage(nullptr);});
        if (!pDataInfo) return;
        if (pDataInfo->patchPath.size() < 4) return;
        if (!QString(".bmp.png.jpg.dds.tga.").contains(pDataInfo->patchPath.right(4), Qt::CaseInsensitive)) return;
        scope.clear();
        m_image->updateImage(pDataInfo);
    });
    if (type <= 2) connect(item, &Data_item::sigExportSelected, this, &Data_main::onExportSelected);
    if (type >= 3) connect(item, &Data_item::sigDownloadSelected, this, &Data_main::onDownloadSelected);
    if (type >= 3) connect(item, &Data_item::sigDownload, this, &Data_main::onDownload);
    if (type >= 4) connect(item, &Data_item::sigProofread, this, &Data_main::onProofread);
    if (type <= 3) m_items.append(item);
    else ui->layout_lackList->addWidget(item);
    return item;
}

void Data_main::exportData(int type, QStringList paths, const QVector<QList<SDataInfo *>> &totalDatas, bool bShowMessage)
{
    /* 是否为重复资源 */ static const auto IsIgnore = [](const bool &bExport, const int &mode, const SDataInfo *d, const QFileInfo &fileInfo) {
        if (!bExport) return false; // 非路径导出
        if (mode > 2) return false; // 不检查重复资源
        if (!fileInfo.exists()) return false; // 文件不存在
        if (mode != 0) {
            const int &fileLen = d->type == 1 && d->fileLen < 0 ? d->compLen : d->fileLen;
            if (fileLen != fileInfo.size()) return false; // 大小不一样
            if (mode != 1) {
                if (d->md5.isEmpty()) return false; // MD5为空，留到后面检验
                if (d->md5.compare(Tool::GetFileMD5(fileInfo.absoluteFilePath()), Qt::CaseInsensitive)) return false; // MD5不一样
            }
        }
        return true; // 忽略导出
    };
    /* 是否为重复资源(检测MD5) */ static const auto IsIgnoreMD5 = [](const bool &bExport, const int &mode, SDataInfo *d, const QFileInfo &fileInfo, const QByteArray &fileData) {
        if (!bExport) return false;             // 非路径导出
        if (mode != 2) return false;            // 不检查MD5
        if (!d->md5.isEmpty()) return false;    // MD5已经检测
        d->md5 = QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex();
        if (d->md5.compare(Tool::GetFileMD5(fileInfo.absoluteFilePath()), Qt::CaseInsensitive)) return false;
        return true;
    };

    ThreadInit;
    qLoading->start(u8"正在导出资源", true, 0);
    CThread* thread = new CThread;
    connect(thread, &CThread::started, thread, [=]{
        SResultData result;
        ActionScope([&]{ emit thread->finished(QVariant::fromValue(result)); });

        // 查重
        QSet<QString> patchs;
        QVector<QList<SDataInfo *>> newDatas(totalDatas.size());
        QVector<SResultData> rs(totalDatas.size());
        for (int i = 0, s = totalDatas.size(); i < s; ++i) {
            auto &source = totalDatas[i];
            auto &dest = newDatas[i];
            for (auto &d : source) if (!patchs.contains(d->patchPath)) patchs.insert(d->patchPath), dest.append(d);
            result.total += source.size();
            rs[i].total = dest.size();
        }
        result.ignore = result.total - patchs.size();
        emit thread->message(5, {});
        IsThreadStop;

        // 多线程导出
        bool bExport = type == 1;
        const int &nProgressMax = 90 / newDatas.size();
        const int &nExportMode = m_config->nExportMode;
        const QString &strExportPath = m_config->getExportPath();
        const QString &strAlonePath = m_config->getAlonePath();
        int oldProgress = 0;
        std::atomic_int progress{0};
        QVector<QThread*> ths(newDatas.size());
        SetProgress(newDatas.size(), 0, &progress); // 初始化进度
        for (int i = 0, s = newDatas.size(); i < s; ++i) {
            ths[i] = QThread::create([i, bExport, nProgressMax, nExportMode, strExportPath, strAlonePath](QString jmpPath, QList<SDataInfo*> *datas, SResultData *r) {
                // 读取jmp文件
                QFile jmp(jmpPath);
                ActionScope([&]{ if (jmp.isOpen()) jmp.close(); });
                if (jmpPath.size()) {
                    if (!jmp.open(QIODevice::ReadOnly)) {
                        r->fail = r->total;
                        SetProgress(i, nProgressMax);
                        return;
                    }
                }
                // 导出数据
                LoadingProgress progress;
                progress.max = datas->size();
                for (auto &d : *datas) {
                    IsThreadStop;
                    QFileInfo fileInfo(bExport ? CPatch::PatchToPath(d->patchPath, strExportPath) : QString("%1/%2").arg(strAlonePath, d->patchPath.split('\\').back()));
                    if (IsIgnore(bExport, nExportMode, d, fileInfo)) r->ignore++; // 是否跳过重复资源
                    else {
                        IsThreadStop;
                        auto fileData = CPatch::ReadData(*d, &jmp); // 读取数据
                        IsThreadStop;
                        if (fileData.isEmpty()) r->fail++; // 读取失败
                        else if (IsIgnoreMD5(bExport, nExportMode, d, fileInfo, fileData)) r->ignore++; // 是否跳过重复资源
                        else { // 写入文件
                            if (Tool::WriteFileData(fileInfo.absoluteFilePath(), fileData)) r->success++;
                            else r->fail++; // 写入失败
                        }
                    }
                    // 更新进度
                    if (progress.add(nProgressMax)) SetProgress(i, progress.p);
                }
                IsThreadStop;
            }, paths[i], &newDatas[i], &rs[i]);
        }
        for (auto &th : ths) th->start();

        // 等待线程完成
        bool bRunning = true;
        while (bRunning) {
            QThread::msleep(200);
            bRunning = false;
            for (auto &th : ths) {
                if (th->isFinished()) continue;
                bRunning = true;
                break;
            }
            if (oldProgress != progress) {
                oldProgress = progress;
                emit thread->message(5 + oldProgress, {});
            }
        }
        for (auto &th : ths) delete th;
        emit thread->message(95, {});

        // 更新数据
        for (auto &r : rs) {
            result.success += r.success;
            result.fail += r.fail;
            result.ignore += r.ignore;
        }
        emit thread->message(100, {});
    });
    connect(thread, &CThread::message, this, [=](int value, QVariant){
        qLoading->setValue(value);
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        delete thread;
        auto r = result.value<SResultData>();
        if (r.total != r.success || bShowMessage) { // 没有全部成功时弹出信息
            QMessageBox::information(this, u8"提示", QString(u8"成功：%1  失败：%2  忽略：%3  剩余：%4")
                                     .arg(r.success).arg(r.fail).arg(r.ignore)
                                     .arg(r.total - r.success - r.fail - r.ignore));
        }
        qLoading->stop();
    });
    connect(qLoading, &Loading::cancel, qLoading->object, [=]{
        ActionScope([]{g_pause = true;},[]{g_pause = false;});
        if (QMessageBox::question(this, u8"提示", u8"是否取消操作？") == QMessageBox::Yes) qLoading->start(u8"正在取消操作"), g_stop = true;
    });
    thread->start();
}

void Data_main::importData(QString path, const QVector<SDataInfoList *> &datas)
{
    ThreadInit;
    qLoading->start(m_mode != 3 ? u8"正在整理资源" : u8"正在打包资源", true, 0);
    CThread* thread = new CThread;
    connect(thread, &CThread::started, thread, [=]{
        SResultData result;
        ActionScope([&]{ emit thread->finished(QVariant::fromValue(result)); });
        const bool &bSingle = m_mode == 2;  ///< 是否为单个资源
        const bool &bJmpData = m_mode != 3; ///< 是否为jmp数据
        QStringList jmpPaths;

        // 合并去重
        QSet<QString> patchs;
        QList<SJmpImportItem> totalItems;
        for (int i = 0, s = datas.size(); i < s; ++i) {
            auto &source = *datas[i];
            for (auto &d : source) if (!patchs.contains(d.patchPath)) patchs.insert(d.patchPath), totalItems.append({i, &d});
            result.total += source.size();
            jmpPaths.append(bSingle ? path : QString("%1/Data%2.jmp").arg(path, i ? QString::number(i) : ""));
        }
        result.discard = result.total - patchs.size();
        emit thread->message(4, {});
        IsThreadStop;

        // 整理规划资源
        auto packItems = CPatch::OrganizeData(totalItems);
        emit thread->message(8, {});
        IsThreadStop;

        // 写入临时文件(.temp)
        const int &packSize = packItems.size();
        const int &nProgressMax = 90 / packSize;
        int oldProgress = 0;
        std::atomic_int progress{0};
        std::atomic_int error{0};
        QStringList outPaths;
        QVector<QThread*> ths(packSize);
        SetProgress(packSize, 0, &progress); // 初始化进度
        for (int i = 0; i < packSize; ++i) {
            outPaths.append(jmpPaths[i] + ".temp");
            ths[i] = QThread::create([bSingle, jmpPaths, i, nProgressMax](QString outPath, QList<SJmpImportItem> *pitems, std::atomic_int *perror) {
                static const auto isStop = [](std::atomic_int &error)->bool { IsThreadStop true; return error; };
                static const auto readData = [](const SJmpImportItem &item, QVector<QFile*> &jmps, const QStringList &paths, std::atomic_int &error)->QByteArray {
                    if (item.d->type == 1) { // 1.内部资源
                        auto &jmp = jmps[item.i];
                        if (!jmp) jmp = new QFile(paths[item.i]);
                        if (!jmp->isOpen()) {
                            if (!jmp->open(QIODevice::ReadOnly)) {
                                if (!error) error = JmpOpenError; // 无法读取jmp
                                return {};
                            }
                        }
                        jmp->seek(item.d->index);
                        return jmp->read(item.d->compLen);
                    }
                    else { // 2.本地文件
                        QByteArray data;
                        if (Tool::ReadFileData(item.d->filePath, data)) return data;
                        if (!error) error = FileOpenError; // 无法读取文件
                        return {};
                    }
                };
                auto &items = *pitems;
                auto &error = *perror;
                LoadingProgress progress;
                progress.max = pitems->size();
                SJmpFile out;
                out.file = new QFile(outPath);
                ActionScope([&]{ out.file->close(), delete out.file; });
                if (out.file->open(QIODevice::WriteOnly)) {
                    QVector<QFile*> jmps(jmpPaths.size(), nullptr);
                    ActionScope([&]{ for (auto &jmp : jmps) if (jmp) jmp->close(), delete jmp; });
                    // 文件头部
                    if (bSingle) {
                        jmps[0] = new QFile(jmpPaths[0]);
                        if (!jmps[0]->open(QIODevice::ReadOnly)) if (!error) error = JmpOpenError; // 无法读取jmp
                        if (isStop(error)) return;
                        out.file->write(jmps[0]->read(8));
                    }
                    else out.file->write("DATA" + QByteArray::number(i + 1) + ".0");
                    // 导入资源
                    for (auto &item : items) {
                        if (isStop(error)) return;
                        // 读取数据
                        auto data = readData(item, jmps, jmpPaths, error);
                        if (isStop(error)) return;
                        // 导入数据
                        if (int r = CPatch::ImportData(out, item.d, data)) if (!error) error = r; // 无法导入资源
                        if (isStop(error)) return;
                        // 更新进度
                        if (progress.add(nProgressMax)) SetProgress(i, progress.p);
                    }
                }
                else if (!error) error = JmpWriteError; // 无法写入jmp
                if (isStop(error)) return;
            }, outPaths[i], &packItems[i], &error);
        }
        for (auto &th : ths) th->start();

        // 等待线程完成
        bool bRunning = true;
        while (bRunning) {
            QThread::msleep(200);
            bRunning = false;
            for (auto &th : ths) {
                if (th->isFinished()) continue;
                bRunning = true;
                break;
            }
            if (oldProgress != progress) {
                oldProgress = progress;
                emit thread->message(8 + oldProgress, {});
            }
        }
        for (auto &th : ths) delete th;
        CActionScope removeTemp([outPaths]{ for (auto &path : outPaths) QFile::remove(path); });
        if ((result.fail = error)) return;
        IsThreadStop;
        removeTemp.clear();
        emit thread->message(98, {});

        // 删除原文件
        if (bJmpData) for (auto &path : jmpPaths) QFile::remove(path);
        else for (auto &path : outPaths) QFile::remove(path.left(path.size() - 5));
        // 重命名临时文件
        for (auto &path : outPaths) if (!QFile::rename(path, path.left(path.size() - 5))) result.fail = FileRenameError;
        if (result.fail) return;

        // 更新数据
        result.success = result.total - result.discard;
        result.count = packSize;
        emit thread->message(100, {});
    });
    connect(thread, &CThread::message, this, [=](int value, QVariant){
        qLoading->setValue(value);
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        delete thread;
        auto r = result.value<SResultData>();
        if (r.fail) { // 失败
            QString info;
            if (r.fail == JmpOpenError) QMessageBox::critical(this, u8"错误", u8"无法读取资源数据！");
            else if (r.fail == FileRemoveError || r.fail == FileRenameError) {
                QMessageBox::warning(this, u8"警告", u8"替换文件失败！请手动替换剩下的文件！\n临时文件(.jmp.temp)改名为资源文件(.jmp)");
                QStringList programs;
                if (m_mode == 2) programs << "/select,";
                programs << QString(path).replace('/', '\\');
                QProcess::startDetached("explorer", programs);
                qApp->quit(); // 退出程序
            }
            else QMessageBox::critical(this, u8"错误", u8"无法添加资源！");
            qLoading->stop();
        }
        else { // 成功
            QMessageBox::information(this, u8"提示", QString(u8"成功：%1  废弃：%2").arg(r.success).arg(r.discard));
            if (m_mode == 1 || m_mode == 2) reload(r.count); // 重新加载视图
            else qLoading->stop();
        }
    });
    connect(qLoading, &Loading::cancel, qLoading->object, [=]{
        ActionScope([]{g_pause = true;},[]{g_pause = false;});
        if (QMessageBox::question(this, u8"提示", u8"是否取消操作？") == QMessageBox::Yes) qLoading->start(u8"正在取消操作"), g_stop = true;
    });
    thread->start();
}

void Data_main::downloadData(int type, const QList<SDataInfo *> &datas, bool bShowMessage)
{
    ThreadInit;
    qLoading->start(u8"正在下载资源", true, 0);
    CThread* thread = new CThread;
    connect(thread, &CThread::started, thread, [=]{
        SJmpFile jmp;
        SResultData result;
        LoadingProgress progress;
        ActionScope([&]{
            if (jmp.file) jmp.file->close(), delete jmp.file;
            result.count = jmp.index + 1;
            emit thread->finished(QVariant::fromValue(result));
        });
        const QString &strExportPath = m_config->getExportPath();
        const QString &strAlonePath = m_config->getAlonePath();
        if (type == 1) jmp.strGamePath = m_config->strGamePath;
        progress.max = result.total =  datas.size();
        for (auto &d : datas) {
            IsThreadStop;
            auto gzData = Http::DownloadData(CPatch::GetPatchFileUrl(d->patchPath), true, &g_stop); // 下载文件
            IsThreadStop;
            if (gzData.isEmpty()) result.fail++; // 下载失败
            else {
                // 下载到jmp文件
                if (type == 1) {
                    auto r = CPatch::ImportData(jmp, d, gzData);
                    if (r == JmpOpenError) { // jmp文件无法打开
                        result.fail = result.total - result.success;
                        return;
                    }
                }
                // 下载到导出目录
                else if (type == 2) {
                    int r = Zlib::UncompGz((uchar*)gzData.data(), gzData.size(), CPatch::PatchToPath(d->patchPath, strExportPath));
                    if (r) result.fail++;
                    else result.success++;
                }
                // 下载到单独目录
                else if (type == 3) {
                    int r = Zlib::UncompGz((uchar*)gzData.data(), gzData.size(), QString("%1/%2").arg(strAlonePath, d->patchPath.split('\\').back()));
                    if (r) result.fail++;
                    else result.success++;
                }
                else result.fail++;
            }
            if (progress.add()) emit thread->message(progress.p, {}); // 更新进度
        }
        IsThreadStop;
    });
    connect(thread, &CThread::message, this, [=](int value, QVariant){
        qLoading->setValue(value);
    });
    connect(thread, &CThread::finished, this, [=](QVariant result){
        delete thread;
        auto r = result.value<SResultData>();
        if (r.total != r.success || bShowMessage) { // 没有全部成功时弹出信息
            QMessageBox::information(this, u8"提示", QString(u8"成功：%1  失败：%2  剩余：%3").arg(r.success).arg(r.fail).arg(r.total - r.success - r.fail));
        }
        if (r.success && ((m_mode == 1 && type == 1) || (m_mode == 3 && type == 2))) reload(r.count); // 重新加载视图
        else qLoading->stop();
    });
    connect(qLoading, &Loading::cancel, qLoading->object, [=]{
        ActionScope([]{g_pause = true;},[]{g_pause = false;});
        if (QMessageBox::question(this, u8"提示", u8"是否取消操作？") == QMessageBox::Yes) qLoading->start(u8"正在取消操作"), g_stop = true;
    });
    thread->start();
}
