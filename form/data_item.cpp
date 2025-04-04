#include "data_item.h"
#include "ui_data_item.h"
#include "config.h"
#include "form_image.h"
#include "form_reader.h"
#include "form_dat.h"
#include "form_bank.h"
#include "tool.h"

#include <QClipboard>
#include <QElapsedTimer>
#include <QMenu>
#include <QTimer>

Data_item::Data_item(QWidget *parent, int parentType, int type, const QString &strPath) :
    QWidget(parent),
    ui(new Ui::Data_item),
    m_parentType(parentType),
    m_type(type),
    m_strPath(strPath)
{
    ui->setupUi(this);

    // 初始化ui
    setMinimumSize(minWidth(), minHeight());
    ui->button_proofread->setVisible(false);
    ui->button_download->setVisible(false);
    ui->listWidget->setEnabled(false);
    ui->listWidget->clear();
    ui->label_info1->clear();
    ui->label_info2->setText(u8"( 未加载 )");
    if      (m_type == 1) {
        QStringList&& list = m_strPath.split('/');
        if (list.size()) ui->label_name->setText(list.back());
    }
    else if (m_type == 2) {
        if (m_parentType == 1) ui->label_name->setText("asyncfile");
        else ui->label_name->setText(u8"本地资源目录");
    }
    else if (m_type == 3) {
        ui->label_name->setText(u8"300英雄资源列表");
    }
    else if (m_type == 4) {
        ui->label_info2->setText(u8"( 未校对 )");
        ui->label_name->setText(u8"缺失文件");
        ui->button_download->setVisible(true);
        ui->button_download->setEnabled(true);
        ui->button_proofread->setVisible(true);
        ui->button_proofread->setEnabled(true);
    }

    // 绑定基础事件
    connect(ui->button_download, &QPushButton::clicked, this, &Data_item::sigDownload);
    connect(ui->button_proofread, &QPushButton::clicked, this, &Data_item::sigProofread);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, [=] (QListWidgetItem *item) {
        QApplication::clipboard()->setText(item->text().replace('/', '\\'));
        emit tip(u8"已复制 补丁路径：" + item->text());
    });
    connect(ui->listWidget, &QListWidget::itemClicked, this, [=] (QListWidgetItem *item) {
        if (m_ignore || m_datas.isEmpty()) return;
        if (ui->listWidget->currentItem() != item) return;
        emit preview(&m_datas[ui->listWidget->row(item)]);
    });
    connect(ui->listWidget, &QListWidget::currentRowChanged, this, [=] (int currentRow) {
        if (m_ignore || m_datas.isEmpty() || !ui->listWidget->isEnabled()) return;
        emit sigClearOtherSelectItem(this);
        emit updateInfo(m_datas[currentRow]);
        emit preview(&m_datas[currentRow]);
    });
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, this, &Data_item::rightMenuCreate);
}

 Data_item::~Data_item()
{
    m_stop = true;
    delete ui;
}

void Data_item::load(QList<Data_item*> items)
{
    m_busy = true;
    m_loaded = false;
    clear();
    ui->button_download->setEnabled(false);
    ui->button_proofread->setEnabled(false);
    CThread* thread = new CThread;
    connect(thread, &CThread::started, thread, [thread, this, items]{
        ActionScope([=]{ emit thread->finished();});
        if      (m_type == 1) { // jmp文件
            emit thread->message(2, QString(u8"( 正在读取文件... )"));
            auto result = CPatch::ReadJmpFile(m_datas, m_strPath, &m_stop);
            if      (result == NoError)         emit thread->message(2, QString(u8"( 渲染列表 )"));
            else if (result == FileOpenError)   emit thread->message(2, QString(u8"( 打开失败 )"));
            else if (result == JmpDataNullError)emit thread->message(2, QString(u8"( 数据为空 )"));
            else if (result == TerminatorStop)  emit thread->message(2, QString(u8"( 中止操作 )"));
        }
        else if (m_type == 2) { // 文件夹
            emit thread->message(2, QString(u8"( 正在读取目录... )"));
            std::atomic_int progress;
            std::atomic_int *pprogress = &progress;
            QThread *progressThread = nullptr;
            if (m_parentType == 3) {
                progressThread = new QThread;
                QTimer *timer = new QTimer;
                timer->moveToThread(progressThread);
                connect(timer, &QTimer::timeout, this, [=]{ emit thread->message(2, QString(u8"( 正在读取目录 %1 )").arg((int)*pprogress, 10, 10)); });
                connect(progressThread, &QThread::finished, timer, [=]{ delete timer; });
                connect(progressThread, &QThread::started, timer, [=]{ timer->start(500); });
                progressThread->start();
            }
            auto result = CPatch::ReadLocalDir(this->m_datas, this->m_strPath, &m_stop, &progress);
            if      (result == NoError)         emit thread->message(2, QString(u8"( 渲染列表 )"));
            else if (result == DirNotExistError)emit thread->message(2, QString(u8"( 目录不存在 )"));
            else if (result == DirNullError)    emit thread->message(2, QString(u8"( 目录为空 )"));
            else if (result == TerminatorStop)  emit thread->message(2, QString(u8"( 中止操作 )"));
            if (progressThread) {
                progressThread->quit();
                progressThread->wait();
                delete progressThread;
            }
        }
        else if (m_type == 3 || m_type == 4) { // 网络列表 && 缺失文件列表
            emit thread->message(2, QString(u8"( 正在加载列表... )"));
            int mode = 0;
            QByteArray clientData, asyncData;
            PatchUniques patchs;
            // 1.网络模式
            if (!m_stop) {
                emit thread->message(2, QString(u8"( 下载服务器资源列表 )"));
                mode = 1;
                clientData = Http::DownloadData(CPatch::GetClientFileUrl(), true, &m_stop);
                asyncData = Http::DownloadData(CPatch::GetAsyncFileUrl(), true, &m_stop);
                if (m_stop) return;
                if (!clientData.isEmpty()) clientData = Zlib::UncompGz((uchar*)clientData.data(), clientData.size());
                if (m_stop) return;
                if (asyncData.isEmpty()) asyncData = Zlib::UncompGz((uchar*)asyncData.data(), asyncData.size());
                if (m_stop) return;
            }
            // 2.本地模式(游戏路径)
            if (!m_stop && (clientData.isEmpty() || asyncData.isEmpty()) && !qConfig.strGamePath.isEmpty()) {
                mode = 2;
                emit thread->message(2, QString(u8"( 尝试从游戏目录获取 )"));
                if (!m_stop && clientData.isEmpty()) Tool::ReadFileData(CPatch::GetClientFilePath(qConfig.strGamePath), clientData);
                if (!m_stop && asyncData.isEmpty()) Tool::ReadFileData(CPatch::GetAsyncFilePath(qConfig.strGamePath), asyncData);
            }
            // 3.本地模式(缓存文件)
            if (!m_stop && (clientData.isEmpty() || asyncData.isEmpty())) {
                mode = 3;
                emit thread->message(2, QString(u8"( 尝试从本地缓存获取 )"));
                if (!m_stop && clientData.isEmpty()) Tool::ReadFileData(qApp->applicationDirPath() + "/BConfig/ClientFileList.xml", clientData);
                if (!m_stop && asyncData.isEmpty()) Tool::ReadFileData(qApp->applicationDirPath() + "/BConfig/asyncclientfilelist.xml", asyncData);
            }
            // 保存到本地配置文件
            if (!m_stop && mode != 3) {
                if (!m_stop && !clientData.isEmpty()) Tool::WriteFileData(qApp->applicationDirPath() + "/BConfig/ClientFileList.xml", clientData);
                if (!m_stop && !asyncData.isEmpty()) Tool::WriteFileData(qApp->applicationDirPath() + "/BConfig/asyncclientfilelist.xml", asyncData);
            }
            // 数据是否为空
            if (!m_stop && (clientData.isEmpty() || asyncData.isEmpty())) {
                emit thread->message(2, QString(u8"( 列表加载失败 )"));
                return;
            }
            // 解析xml数据
            if (!m_stop) {
                emit thread->message(2, QString(u8"( 解析表单项 )"));
                if (!m_stop) CPatch::ReadXmlData(patchs, clientData, mode == 1, &m_stop);
                if (!m_stop) CPatch::ReadXmlData(patchs, asyncData, mode == 1, &m_stop);
                if (!m_stop) m_clients = patchs;
            }
            // 比对缺失列表
            if (!m_stop && m_type == 4) {
                emit thread->message(2, QString(u8"( 统计缺失项 )"));
                for (auto &item : items) for (auto &info : item->m_datas) {
                    if (m_stop) return;
                    else patchs.remove(info.patchPath);
                }
            }
            // 添加到列表
            if (m_stop) return;
            else for (auto it = patchs.cbegin(), itend = patchs.cend(); it != itend; ++it) {
                if (m_stop) return;
                SDataInfo d;
                d.type = 3;
                d.patchPath = it.key();
                d.md5 = it.value();
                m_datas.append(d);
            }
            emit thread->message(1, QString(mode == 1 ? u8"( 网络最新 )" : mode == 2 ? u8"( 游戏目录 )" : u8"( 本地缓存 )"));
            if (!patchs.isEmpty())          emit thread->message(2, QString(u8"( 渲染列表 )"));
            else if (m_clients.isEmpty())   emit thread->message(2, QString(u8"( 数据为空 )"));
            else                            emit thread->message(2, QString(u8"( 无缺失项 )"));
        }
    });
    connect(thread, &CThread::message, this, [this](int state, QVariant msg){
        if      (state == 0) emit sigProgressText(msg.toString());
        else if (state == 1) ui->label_info1->setText(msg.toString());
        else if (state == 2) ui->label_info2->setText(msg.toString());
    });
    connect(thread, &CThread::finished, this, [thread, this]{
        delete thread;
        if (!m_stop) { // 开始渲染列表
            if (m_datas.size()) ui->label_info2->setText(QString(u8"( %1项 )").arg(m_datas.size()));
            QStringList patchs;
            patchs.reserve(m_datas.size());
            for (auto &info : m_datas) patchs.append(info.patchPath);
            ui->listWidget->addItems(patchs);
            ui->listWidget->setEnabled(true);
            ui->button_download->setEnabled(true);
            ui->button_proofread->setEnabled(true);
        }
        m_loaded = true;
        m_busy = false;
        emit loaded();
    });
    thread->start();
}

void Data_item::updateLacks(QList<Data_item *> items)
{
    m_busy = true;
    m_loaded = false;
    if (m_type == 4) {
        ui->label_info2->setText(u8"( 正在更新列表... )");
        ui->button_download->setEnabled(false);
        ui->button_proofread->setEnabled(false);
        ui->listWidget->setEnabled(false);
        ui->listWidget->clear();
        m_datas.clear();
    }
    CThread* thread = new CThread;
    connect(thread, &CThread::started, thread, [thread, this, items]{
        ActionScope([=]{ emit thread->finished();});
        if (m_type != 4) return;
        PatchUniques patchs(m_clients);
        for (auto &item : items) for (auto &info : item->m_datas) {
            if (m_stop) return;
            else patchs.remove(info.patchPath);
        }
        for (auto it = patchs.cbegin(), itend = patchs.cend(); it != itend; ++it) {
            if (m_stop) return;
            SDataInfo d;
            d.type = 3;
            d.patchPath = it.key();
            d.md5 = it.value();
            m_datas.append(d);
        }
    });
    connect(thread, &CThread::finished, this, [thread, this]{
        delete thread;
        if (!m_stop && m_type == 4) { // 开始渲染列表
            if (m_datas.size()) {
                ui->label_info2->setText(QString(u8"( %1项 )").arg(m_datas.size()));
                QStringList patchs;
                patchs.reserve(m_datas.size());
                for (auto &info : m_datas) patchs.append(info.patchPath);
                ui->listWidget->addItems(patchs);
            }
            else ui->label_info2->setText(u8"( 无缺失项 )");
            ui->button_download->setEnabled(true);
            ui->button_proofread->setEnabled(true);
            ui->listWidget->setEnabled(true);
        }
        m_loaded = true;
        m_busy = false;
        emit loaded();
    });
    thread->start();
}

bool Data_item::isLoaded() const
{
    return m_loaded;
}

bool Data_item::isNull() const
{
    return m_datas.isEmpty();
}

bool Data_item::isProofread() const
{
    return !m_clients.isEmpty();
}

int Data_item::type() const
{
    return m_type;
}

QString Data_item::path() const
{
    return m_strPath;
}

int Data_item::lookup(const QString &key)
{
    if (m_datas.isEmpty()) return 0;
    // 恢复
    if (key.isEmpty()) {
        for (int i = 0, s = ui->listWidget->count(); i < s; ++i) ui->listWidget->setRowHidden(i, false);
        return 0;
    }
    // 查找
    else {
        clearSelectItem();
        auto items = ui->listWidget->findItems(key, Qt::MatchContains | Qt::MatchCaseSensitive);
        QSet<QListWidgetItem*> sets(items.cbegin(), items.cend());
        for (int i = 0, s = ui->listWidget->count(); i < s; ++i)
            ui->listWidget->setRowHidden(i, !sets.contains(ui->listWidget->item(i)));
        return items.size();
    }
}

void Data_item::clear()
{
    ui->label_info1->clear();
    ui->label_info2->setText(m_type == 4 ? u8"( 未校对 )" : u8"");
    ui->listWidget->setEnabled(false);
    ui->listWidget->clear();
    m_datas.clear();
}

int Data_item::minWidth()
{
    return 200;
}

int Data_item::minHeight()
{
    return 80;
}

void Data_item::clearSelectItem()
{
    m_ignore = true;
    ui->listWidget->setCurrentRow(-1);
    m_ignore = false;
}

QVector<SDataInfo> *Data_item::datas()
{
    return &m_datas;
}

void Data_item::rightMenuCreate(const QPoint &pos)
{
    if (m_busy) return;
    int nSelectIndex = -1;
    if (ui->listWidget->itemAt(pos)) nSelectIndex = ui->listWidget->currentRow();
    if (nSelectIndex < 0) return;

    // 创建右键菜单
    QList<QAction*> pButtons;
    QMenu* pMenu = new QMenu(ui->listWidget);
    ActionScope([pMenu, &pButtons]{
        qDeleteAll(pButtons);
        delete pMenu;
    });
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
#define MENUITEM(i, str) \
    QAction* pButton##i = new QAction(str); \
    connect(pButton##i, &QAction::triggered, this, [=]{ rightMenuEvent(i, nSelectIndex); }); \
    pButtons.push_back(pButton##i);
    if (m_type == 1 || m_type == 2) {
        MENUITEM(101, u8"预览全图");
        MENUITEM(102, u8"预览文本");
        MENUITEM(103, u8"预览数据");
        MENUITEM(104, u8"试听语音");
        MENUITEM(105, u8"路径导出");
        MENUITEM(106, u8"单独导出");
        const QString& strPatch = m_datas[nSelectIndex].patchPath;
        const QString& strFormat = strPatch.right(strPatch.size() - strPatch.lastIndexOf('.')) + ".";
        // 是否为图片
        if (QString(".bmp.png.jpg.dds.tga.").contains(strFormat, Qt::CaseInsensitive)) pMenu->addAction(pButton101);
        // 是否为文本
        else if (QString(".txt.ini.lua.xml.json.fx.psh.string.").contains(strFormat, Qt::CaseInsensitive)) pMenu->addAction(pButton102);
        // 是否为数据库
        else if (QString(".dat.").contains(strFormat, Qt::CaseInsensitive)) pMenu->addAction(pButton103);
        // 是否为语音
        else if (QString(".bank.").contains(strFormat, Qt::CaseInsensitive)) pMenu->addAction(pButton104);
        // 路径导出
        if (m_type == 1 || m_parentType == 1) pMenu->addAction(pButton105);
        // 单独导出
        pMenu->addAction(pButton106);
    }
    else if (m_type == 3 || m_type == 4) {
        MENUITEM(201, u8"复制下载链接");
        MENUITEM(202, u8"下载到资源包里");
        MENUITEM(203, u8"下载到导出路径");
        MENUITEM(204, u8"下载到单独路径");
        pMenu->addAction(pButton201);
        if (m_type == 4 && m_parentType == 1) pMenu->addAction(pButton202);
        pMenu->addAction(pButton203);
        pMenu->addAction(pButton204);
    }
    else return;
#undef MENUITEM
    pMenu->exec(QCursor::pos());
}

void Data_item::rightMenuEvent(int type, int nIndex)
{
    static const auto getSelectedDatas = [](QListWidget *listWidget, QVector<SDataInfo> &datas)->QList<SDataInfo*>{
        auto items = listWidget->selectedItems();
        QList<SDataInfo*> list;
        for (auto &item : items) list.append(&datas[listWidget->row(item)]);
        return list;
    };
    if      (type == 101) { // 预览全图
        (new Form_Image(this, m_datas[nIndex]))->show();
    }
    else if (type == 102) { // 预览文本
        (new Form_reader)->load(this, m_datas[nIndex]);
    }
    else if (type == 103) { // 预览数据
        (new Form_dat)->load(this, m_datas[nIndex]);
    }
    else if (type == 104) { // 试听语音
        (new Form_bank)->load(this, m_datas[nIndex]);
    }
    else if (type == 105) { // 路径导出
        emit sigExportSelected(false, getSelectedDatas(ui->listWidget, m_datas), this);
    }
    else if (type == 106) { // 单独导出
        emit sigExportSelected(true, getSelectedDatas(ui->listWidget, m_datas), this);
    }
    else if (type == 201) { // 复制下载链接
        QString strURL = "http://update.300hero.jumpwgame.com/xclient_unpack" + m_datas[nIndex].patchPath.mid(2).replace('\\', '/') + ".gz";
        QApplication::clipboard()->setText(strURL);
        emit tip(u8"已复制 下载链接：" + strURL);
    }
    else if (type == 202) { // 下载到资源包里
        emit sigDownloadSelected(1, getSelectedDatas(ui->listWidget, m_datas));
    }
    else if (type == 203) { // 下载到导出路径
        emit sigDownloadSelected(2, getSelectedDatas(ui->listWidget, m_datas));
    }
    else if (type == 204) { // 下载到单独路径
        emit sigDownloadSelected(3, getSelectedDatas(ui->listWidget, m_datas));
    }
}

