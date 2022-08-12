#include "widget_net.h"
#include "ui_widget_net.h"
#include "dialog_progress.h"
#include "ceventfilter.h"

#include "windows.h"
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThreadPool>
#include <QtConcurrent>
#include <QClipboard>

Widget_Net::Widget_Net(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget_Net)
{
    ui->setupUi(this);

    connect(ui->list_dataList, &QListWidget::itemDoubleClicked, this, [&] (QListWidgetItem *item) {
        QApplication::clipboard()->setText(item->text().replace('/', '\\'));
        emit SendTip(QString::fromWCharArray(L"已复制 补丁路径：") + item->text());
    });
}

Widget_Net::~Widget_Net()
{
    delete ui;
}

// 初始化
void Widget_Net::Init()
{
    Clear();

    // 创建进度条
    bool bRun = true;
    QThreadPool thPool;
    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在加载资源列表..."), NULL, 0, 0, this, Qt::Dialog | Qt::FramelessWindowHint);
    qProgressBar.installEventFilter(&sg_cEventFilter);
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setMinimumDuration(0);
    qProgressBar.setCancelButton(0);
    qProgressBar.show();

    // 读取客户端列表
    ui->label_info->setText(QString::fromWCharArray(L"( 正在读取资源列表... )"));
    QVector<QString> strClientList;
    QString strInfo;
    wchar_t lcGamePath[520];
    GetPrivateProfileString(L"配置", L"游戏路径", L"", lcGamePath, 520, L"BConfig/Config.ini");
    QFuture<int> qFuture = QtConcurrent::run(&thPool, CPatch::ReadClientList, std::ref(strClientList), QString::fromWCharArray(lcGamePath), std::ref(strInfo), std::ref(bRun));
    while (thPool.activeThreadCount() > 0)
    {
        if (qProgressBar.labelText() != strInfo)
            qProgressBar.setLabelText(strInfo);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    // 装载数据
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在装载数据..."));
    if (qFuture.result() == FILE_OK)
    {
        ui->label_info->setText(strInfo + QString::fromWCharArray(L"\n( 正在装载数据... )"));
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        for (auto i : strClientList) m_vDataList.push_back(SDataInfo{ 3, "", i, 0, 0, 0});
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        for (auto i : m_vDataList) ui->list_dataList->addItem(i.patchPath);
        ui->label_info->setText(strInfo + QString::fromWCharArray(L"\n( 一共加载了 ") + QString::number(m_vDataList.size()) + QString::fromWCharArray(L" 个文件 )"));
        ui->list_dataList->setEnabled(1);
    }
    if (qFuture.result() == FILE_OPEN_FAEL)
    {
        ui->label_info->setText(strInfo + QString::fromWCharArray(L"\n( 资源列表加载失败！ )"));
    }
    else if (qFuture.result() == FILE_DATA_NULL)
    {
        ui->label_info->setText(strInfo + QString::fromWCharArray(L"\n( 资源列表数据为空！ )"));
    }
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    qProgressBar.close();
}

// 查找
void Widget_Net::Lookup(QString strKeyword)
{
    ui->list_dataList->clearSelection();
    const int nListNum = ui->list_dataList->count();
    if (strKeyword.size() < 2)
    {
        if (strKeyword == 127)
        {
            ui->list_dataList->setEnabled(0);
            ui->label_infoLookup->setText(QString::fromWCharArray(L"正在清除查找结果..."));
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            for (int i = 0; i < nListNum; i++)
            {
                if (ui->list_dataList->isRowHidden(i))
                    ui->list_dataList->setRowHidden(i, 0);
            }
            ui->list_dataList->setEnabled(1);
            ui->label_infoLookup->setText("");
        }
        else
            ui->label_infoLookup->setText(QString::fromWCharArray(L"请至少输入两个字符再进行查找！"));
        return;
    }

    int nLookupNum = 0;
    ui->list_dataList->setEnabled(0);
    ui->label_infoLookup->setText(QString::fromWCharArray(L"正在查找中..."));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    for (int i = 0; i < nListNum; i++)
    {
        if (m_vDataList[i].patchPath.contains(strKeyword, Qt::CaseInsensitive))
        {
            ui->list_dataList->setRowHidden(i, 0);
            nLookupNum += 1;
        }
        else
            ui->list_dataList->setRowHidden(i, 1);
    }
    ui->label_infoLookup->setText(QString::fromWCharArray(L"查找完毕！遍历 ")
                                  + QString::number(nListNum)
                                  + QString::fromWCharArray(L" 项数据，一共查找到 ")
                                  + QString::number(nLookupNum)
                                  + QString::fromWCharArray(L" 个目标！"));
    ui->list_dataList->setEnabled(1);
}

// 全部导出（全部下载）
void Widget_Net::ExportAll()
{
    // 资源列表为空时无法下载
    if (ui->list_dataList->isEnabled() == 0 || ui->list_dataList->count() == 0)
    {
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"当前资源列表不可用！"));
        return;
    }

    // 再次询问是否确定下载全部
    if (QMessageBox::Yes != QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否下载全部资源文件？\n(下载文件保存在导出资源目录中)"), QMessageBox::Yes|QMessageBox::No))
        return;

    // 初始化各项数据
    bool bTerminator = true;
    QVector<SRealTimeData> sRtDataList(1, {0, 0, 0, 0, 0, 0});

    // 启动线程池
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    wchar_t lcExportPath[520];
    GetPrivateProfileString(L"配置", L"导出路径", L"BExport", lcExportPath, 520, L"BConfig/Config.ini");
    QtConcurrent::run(&thPool, CPatch::DownloadDataFile, m_vDataList, QString::fromWCharArray(lcExportPath), std::ref(sRtDataList[0]), std::ref(bTerminator));

    // 显示进度条
    Dialog_Progress dProgress(this);
    dProgress.Start(EProgressType::Download, {QString::fromWCharArray(L"下载全部资源")}, sRtDataList, bTerminator);
    dProgress.exec();
}

// 清除页面
void Widget_Net::Clear()
{
    m_vDataList.clear();

    ui->list_dataList->clear();
    ui->list_dataList->setEnabled(0);

    ui->label_infoLookup->setText("");
    ui->label_info->setText("");

    if (m_pDialogPreview != NULL)
    {
        m_pDialogPreview->Stop();
        delete m_pDialogPreview;
        m_pDialogPreview = NULL;
    }
}

// 右键菜单事件
void Widget_Net::RightMenuEvent(int nMenuButton)
{
    // 获取选中列表
    QList<QListWidgetItem *> lsItemList(ui->list_dataList->selectedItems());
    if (lsItemList.size() == 0) return;

    // 初始化数据
    QVector<SDataInfo> sDataInfoList;
    for (int i = 0; i < lsItemList.size(); i++)
        sDataInfoList.push_back(m_vDataList[ui->list_dataList->row(lsItemList[i])]);
    SRealTimeData sRtData = {0, 0, 0, 0, 0, 0};
    SExportConfig sExportConfig;
    sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
    if (nMenuButton == 1)  // 使用导出路径
    {
        wchar_t lcExportPath[520];
        GetPrivateProfileString(L"配置", L"导出路径", L"BExport", lcExportPath, 520, L"BConfig/Config.ini");
        sExportConfig.strExportPath = QString::fromWCharArray(lcExportPath);
    }
    else    // 使用单独路径
    {
        wchar_t lcAlonePath[520];
        GetPrivateProfileString(L"配置", L"单独导出", L"BAlone", lcAlonePath, 520, L"BConfig/Config.ini");
        sExportConfig.strExportPath = QString::fromWCharArray(lcAlonePath);
    }

    // 设置线程池
    bool bThread = true;
    QThreadPool thPool;
    QtConcurrent::run(&thPool, [&bThread] (int nMenuButton, QVector<SDataInfo> sDataInfoList, SExportConfig sExportConfig, SRealTimeData &sRtData) {
        sRtData.nState = 1;
        sRtData.nDataNum = sDataInfoList.size();
        for (int i = 0; i < sDataInfoList.size(); i++)
        {
            if(!bThread) return;
            QString strFilePath = sExportConfig.strExportPath;
            if (nMenuButton == 2)   // 单独路径只需要文件名
                strFilePath += "/" + sDataInfoList[i].patchPath.split('\\').last();
            else
                strFilePath += CPatch::PathPatchToFile(sDataInfoList[i].patchPath);
            // 检查目标文件是否已存在（非覆盖模式下，将忽略已存在文件的重复导出）
            if (QFile().exists(strFilePath) && (!sExportConfig.nExportMode))
            {
               sRtData.nIgnoreNum += 1; // 忽略数量 +1
            }
            else
            {
                if (0 == CPatch::DownloadTYGzFile(sDataInfoList[i].patchPath, strFilePath))
                    sRtData.nSuccessNum += 1;   // 成功数量 +1
                else
                    sRtData.nFailNum += 1;      // 失败数量 +1
            }
        }
    }, nMenuButton, sDataInfoList, sExportConfig, std::ref(sRtData));

    // 创建进度条
    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在下载文件..."), NULL, 0, sDataInfoList.size(), this, Qt::Dialog | Qt::FramelessWindowHint);
    connect(&qProgressBar, &QProgressDialog::rejected, this, [&bThread, &qProgressBar]{
        if (QMessageBox::Yes == QMessageBox::information(&qProgressBar, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否中止当前操作？"), QMessageBox::Yes|QMessageBox::No))
            bThread = false;
        else qProgressBar.show();
    });
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setCancelButton(0);
    qProgressBar.setMinimumDuration(1);
    qProgressBar.show();
    int nProgressNum = 0;
    while (nProgressNum < sDataInfoList.size())
    {
        if (!bThread) {
            thPool.waitForDone(1000);
            break;
        }
        nProgressNum = sRtData.nSuccessNum + sRtData.nFailNum + sRtData.nIgnoreNum;
        qProgressBar.setValue(nProgressNum);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    if (bThread){
        qProgressBar.setValue(sDataInfoList.size());
        QMessageBox::about(this, QString::fromWCharArray(L"提示"),
                           QString::fromWCharArray(L"文件下载完成！")
                           + QString::fromWCharArray(L"成功:") + CPatch::StrIntL(sRtData.nSuccessNum, 3)
                           + QString::fromWCharArray(L"失败:") + CPatch::StrIntL(sRtData.nFailNum, 3)
                           + QString::fromWCharArray(L"忽略:") + CPatch::StrIntL(sRtData.nIgnoreNum, 3));
        thPool.waitForDone(100);
    }
}

// 弹出右键菜单
void Widget_Net::on_list_dataList_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* pCurItem = ui->list_dataList->itemAt(pos);
    if (pCurItem == NULL) return;
    int currentRow = ui->list_dataList->currentRow();
    if (currentRow < 0) return;

    // 创建右键菜单
    QMenu* pMenu = new QMenu(this);
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
    QAction* pMenuButton1 = new QAction(QString::fromWCharArray(L"下载到导出路径"), this);
    QAction* pMenuButton2 = new QAction(QString::fromWCharArray(L"下载到单独路径"), this);
    connect(pMenuButton1, &QAction::triggered, this, [=] { RightMenuEvent(1); });
    connect(pMenuButton2, &QAction::triggered, this, [=] { RightMenuEvent(2); });

    pMenu->addAction(pMenuButton1);
    pMenu->addAction(pMenuButton2);

    pMenu->exec(QCursor::pos());

    delete pMenuButton1;
    delete pMenuButton2;
    delete pMenu;
}

// 选择项发生变化时
void Widget_Net::on_list_dataList_currentRowChanged(int currentRow)
{
    if (m_vDataList.isEmpty()) return;
    emit SendInfo(m_vDataList[currentRow]);
}
