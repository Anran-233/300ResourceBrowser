#include "widget_local.h"
#include "ui_widget_local.h"
#include "dialog_progress.h"
#include "ceventfilter.h"
#include "form_reader.h"

#include "windows.h"
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThreadPool>
#include <QtConcurrent>
#include <QMenu>
#include <QFileDialog>
#include <QClipboard>

Widget_Local::Widget_Local(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget_Local)
{
    ui->setupUi(this);

    m_pLabelInfo[0] = ui->label_info_0;
    m_pLabelInfo[1] = ui->label_info_1;

    m_pListWidget[0] = ui->list_dataList_0;
    m_pListWidget[1] = ui->list_dataList_1;

    connect(m_pListWidget[0], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(0, currentRow);});
    connect(m_pListWidget[1], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(1, currentRow);});

    connect(m_pListWidget[0], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 0); });
    connect(m_pListWidget[1], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 1); });

    for (auto i : m_pListWidget)
    {
        connect(i, &QListWidget::itemDoubleClicked, this, [&] (QListWidgetItem *item) {
            QApplication::clipboard()->setText(item->text().replace('/', '\\'));
            emit SendTip(QString::fromWCharArray(L"已复制 补丁路径：") + item->text());
        });
    }

    connect(ui->Button_lackList, &QPushButton::clicked, this, &Widget_Local::SetLackListVisible);
    connect(ui->Button_1_proofread, &QPushButton::clicked, this, &Widget_Local::ProofreadLackFile);
    connect(ui->Button_1_download, &QPushButton::clicked, this, &Widget_Local::DownloadLackFile);
}

Widget_Local::~Widget_Local()
{
    delete ui;
}

// 初始化
void Widget_Local::Init()
{
    Clear();

    // 读取游戏路径配置
    wchar_t lcLocalPath[520];
    GetPrivateProfileString(L"配置", L"导出路径", L"", lcLocalPath, 520, L"BConfig/Config.ini");
    m_strLocalPath = QString::fromWCharArray(lcLocalPath);
    if (!QDir().exists(m_strLocalPath))
    {
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"本地导出资源路径不存在！"));
        return;
    }

    // 装载图片预览器
    m_pDialogPreview = new Dialog_Preview(this);
    m_pDialogPreview->move(1000, 600);
    m_pDialogPreview->setVisible(0);
    bPreviewPicture = GetPrivateProfileInt(L"配置", L"预览图片", 1, L"BConfig/Config.ini");

    // 读取资源数据
    m_pLabelInfo[1]->setText(QString::fromWCharArray(L"( 未校对 )"));
    m_pLabelInfo[0]->setText(QString::fromWCharArray(L"( 正在读取目录... )"));
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    QFuture<int> qFuture = QtConcurrent::run(&thPool, CPatch::ReadDataDir, std::ref(m_vDataList[0]), m_strLocalPath);

    // 创建进度条
    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在读取资源..."), NULL, 0, 0, this, Qt::Dialog | Qt::FramelessWindowHint);
    qProgressBar.installEventFilter(&sg_cEventFilter);
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setCancelButton(0);
    qProgressBar.setMinimumDuration(1);
    qProgressBar.show();
    while (thPool.activeThreadCount() > 0)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    qProgressBar.close();

    // 显示数据
    m_pLabelInfo[0]->setText(QString::fromWCharArray(L"( 读取完毕！正在整理数据... )"));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    int ret = qFuture.result();
    if (ret == FILE_OK)
    {
        for (int i = 0; i < m_vDataList[0].size(); i++)
        {
            m_pListWidget[0]->addItem(m_vDataList[0][i].patchPath);
        }
        QString strInfo = QString::fromWCharArray(L"( 一共加载了 ") + QString::number(m_vDataList[0].size()) + QString::fromWCharArray(L" 个文件 )");
        m_pLabelInfo[0]->setText(strInfo);
        m_pListWidget[0]->setEnabled(1);
    }
    else if (ret == FILE_OPEN_FAEL)
    {
        m_pLabelInfo[0]->setText(QString::fromWCharArray(L"( 目录不存在! )"));
    }
    else if (ret == FILE_DATA_NULL)
    {
        m_pLabelInfo[0]->setText(QString::fromWCharArray(L"( 目录为空! )"));
    }
    ui->Button_1_proofread->setEnabled(1);
}

// 查找
void Widget_Local::Lookup(QString strKeyword)
{
    for (int i = 0; i < 2; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        m_pListWidget[i]->setCurrentRow(-1);
        m_pListWidget[i]->clearSelection();
    }
    if (strKeyword.size() < 2)
    {
        if (strKeyword == 127)
        {
            ui->label_infoLookup->setText(QString::fromWCharArray(L"正在清除查找结果..."));
            for (int i = 0; i < 2; i++)
            {
                if (m_vDataList[i].isEmpty()) continue;
                for (int j = 0; j < m_vDataList[i].size(); j++)
                    m_pListWidget[i]->setEnabled(0);
            }
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            for (int i = 0; i < 2; i++)
            {
                if (m_vDataList[i].isEmpty()) continue;
                for (int j = 0; j < m_vDataList[i].size(); j++)
                {
                    if (m_pListWidget[i]->isRowHidden(j))
                        m_pListWidget[i]->setRowHidden(j, 0);
                }
                m_pListWidget[i]->setEnabled(1);
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
            ui->label_infoLookup->setText("");
        }
        else
            ui->label_infoLookup->setText(QString::fromWCharArray(L"请至少输入两个字符再进行查找！"));
        return;
    }

    ui->label_infoLookup->setText(QString::fromWCharArray(L"正在查找中..."));
    // 如果缺失文件列表不为空，那么显示出来一起查找
    if (!m_vDataList[1].isEmpty())
    {
        ui->frame_1_lackfile->setVisible(1);
        ui->Button_lackList->setText(QString::fromWCharArray(L"折叠缺失文件列表"));
    }
    for (int i = 0; i < 2; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        m_pListWidget[i]->setEnabled(0);
    }
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    int nListNum = 0;
    int nLookupNum = 0;
    for (int i = 0; i < 2; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        nListNum += m_vDataList[i].size();
        for (int j = 0; j < m_vDataList[i].size(); j++)
        {
            if (m_vDataList[i][j].patchPath.contains(strKeyword, Qt::CaseInsensitive))
            {
                m_pListWidget[i]->setRowHidden(j, 0);
                nLookupNum += 1;
            }
            else
                m_pListWidget[i]->setRowHidden(j, 1);
        }
        m_pListWidget[i]->setEnabled(1);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    ui->label_infoLookup->setText(QString::fromWCharArray(L"查找完毕！遍历 ")
                                  + QString::number(nListNum)
                                  + QString::fromWCharArray(L" 项数据，一共查找到 ")
                                  + QString::number(nLookupNum)
                                  + QString::fromWCharArray(L" 个目标！"));
}

// 全部导出（全部打包）
void Widget_Local::ExportAll()
{
    if (m_vDataList[0].isEmpty()) return;

    // 再次询问是否确定打包全部
    if (QMessageBox::Yes != QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否打包全部文件？\n(输出文件在应用程序目录下)"), QMessageBox::Yes|QMessageBox::No))
        return;

    // 初始化各项数据
    bool bTerminator = true;
    QVector<SRealTimeData> sRtDataList(1, {0, 0, 0, 0, 0, 0});

    // 启动线程池
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    QtConcurrent::run(&thPool, CPatch::PackDataFile, m_vDataList[0], std::ref(sRtDataList[0]), std::ref(bTerminator));

    // 显示进度条
    Dialog_Progress dProgress(this);
    dProgress.Start(EProgressType::Pack, {QString::fromWCharArray(L"打包为data文件")}, sRtDataList, bTerminator);
    dProgress.exec();
}

// 清除页面
void Widget_Local::Clear()
{
    ui->label_infoLookup->setText("");
    for (int i = 0; i < 2; i++)
    {
        m_vDataList[i].clear();
        m_pListWidget[i]->clear();
        m_pListWidget[i]->setEnabled(0);
        m_pLabelInfo[i]->setText("");
    }
    m_strLocalPath.clear();

    ui->frame_1_lackfile->setVisible(0);
    ui->Button_lackList->setText(QString::fromWCharArray(L"展开缺失文件列表"));
    ui->Button_1_proofread->setEnabled(0);
    ui->Button_1_download->setEnabled(0);

    if (m_pDialogPreview != NULL)
    {
        m_pDialogPreview->Stop();
        delete m_pDialogPreview;
        m_pDialogPreview = NULL;
    }
}

// 展开/折叠 缺失文件列表
void Widget_Local::SetLackListVisible()
{
    bool bVisible = ui->frame_1_lackfile->isVisible();
    ui->frame_1_lackfile->setVisible(!bVisible);
    ui->Button_lackList->setText(QString::fromWCharArray(bVisible ? L"展开缺失文件列表" : L"折叠缺失文件列表"));
}

// 校对缺失文件列表
void Widget_Local::ProofreadLackFile()
{
    // 创建进度条
    bool bRun = true;
    QThreadPool thPool;
    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在加载客户端列表..."), QString::fromWCharArray(L"取消"), 0, 0, this, Qt::Dialog | Qt::FramelessWindowHint);
    qProgressBar.installEventFilter(&sg_cEventFilter);
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setMinimumDuration(0);
    connect(&qProgressBar, &QProgressDialog::canceled, this, [&] {
        bRun = false;
        QProgressDialog qProgressBar_cance(QString::fromWCharArray(L"正在取消中..."), NULL, 0, 0, this, Qt::Dialog | Qt::FramelessWindowHint);
        qProgressBar_cance.installEventFilter(&sg_cEventFilter);
        qProgressBar_cance.setCancelButton(0);
        qProgressBar_cance.setMinimumDuration(0);
        qProgressBar_cance.show();
        while (thPool.activeThreadCount() > 0)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        qProgressBar_cance.close();
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"校对失败！\n校对过程被中止！"));
        });
    qProgressBar.show();

    // 读取客户端列表
    QVector<QString> strClientList;
    QString strInfo;
    wchar_t lcGamePath[520];
    GetPrivateProfileString(L"配置", L"游戏路径", L"", lcGamePath, 520, L"BConfig/Config.ini");
    QFuture<int> qFuture = QtConcurrent::run(&thPool, CPatch::ReadClientList, std::ref(strClientList), QString::fromWCharArray(lcGamePath), std::ref(strInfo), std::ref(bRun));
    while (thPool.activeThreadCount() > 0)
    {
        if (qProgressBar.labelText() != strInfo)
            qProgressBar.setLabelText(strInfo);
        if(qProgressBar.wasCanceled()) break;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    if (!bRun) return;
    if (qFuture.result() == FILE_OPEN_FAEL)
    {
        qProgressBar.close();
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"校对失败！\n客户端列表加载失败！"));
        return;
    }
    else if (qFuture.result() == FILE_DATA_NULL)
    {
        qProgressBar.close();
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"校对失败！\n客户端列表数据为空！"));
        return;
    }

    // 校对数据
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 准备校对数据..."));
    int nThPoolNum = thPool.maxThreadCount();
    int nLen = strClientList.size() / nThPoolNum + 1;
    QVector< QVector < QString > > strLackList(nThPoolNum);
    QVector<int> nProgressList(nThPoolNum);
    for (int i = 0; i < nThPoolNum; i++)
    {
        QVector<int> nIndex(2);
        nIndex[0] = i * nLen;
        nIndex[1] = (i + 1) * nLen;
        if ((i + 1) == nThPoolNum) nIndex[1] = strClientList.size();
        QtConcurrent::run(&thPool, [&] (QVector<int> nIndex, QVector<QString> &strPatchList, int &nProgress) {
            for (int i = nIndex[0]; (i < nIndex[1]) && bRun; i++)
            {
                int j = 0;
                while (j < m_vDataList[0].size() && bRun)
                {
                    if (m_vDataList[0][j].patchPath == strClientList[i]) break;
                    j += 1;
                }
                if ((j == m_vDataList[0].size()) && bRun) strPatchList.push_back(strClientList[i]);
                nProgress = (double)(i - nIndex[0] + 1) / (nIndex[1] - nIndex[0]) * 100;
            }
        }, nIndex, std::ref(strLackList[i]), std::ref(nProgressList[i]));
    }
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在校对数据..."));
    qProgressBar.setMaximum(nThPoolNum * (100 + 2));
    qProgressBar.setValue(0);
    while (thPool.activeThreadCount() > 0)
    {
        QVector<int> tempProgressList(nProgressList);
        int nProgressNum = 0;
        for (auto i : tempProgressList) nProgressNum += i;
        qProgressBar.setValue(nProgressNum);
        if(qProgressBar.wasCanceled()) break;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    if (!bRun) return;

    // 合并数据
    ui->Button_1_proofread->setEnabled(0);
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在合并校对数据..."));
    m_vDataList[1].clear();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    for (auto i : strLackList)
    {
        for (auto j : i)
        {
            SDataInfo tempDataInfo = { 3, "", "", 0, 0, 0 };
            tempDataInfo.patchPath = j;
            m_vDataList[1].push_back(tempDataInfo);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    qProgressBar.setValue(nThPoolNum * (100 + 1));

    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在装载数据..."));
    m_pListWidget[1]->clear();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    if (m_vDataList[1].isEmpty())
        m_pLabelInfo[1]->setText(strInfo + QString::fromWCharArray(L"\n( 没有缺失的文件! )"));
    else
    {
        m_pLabelInfo[1]->setText(strInfo + QString::fromWCharArray(L"\n( 正在装载数据... )"));
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        for (auto i : m_vDataList[1]) m_pListWidget[1]->addItem(i.patchPath);
        m_pLabelInfo[1]->setText(strInfo + QString::fromWCharArray(L"\n( 一共加载了 ") + QString::number(m_vDataList[1].size()) + QString::fromWCharArray(L" 个文件 )"));
        m_pListWidget[1]->setEnabled(1);
        ui->Button_1_download->setEnabled(1);
    }
    ui->Button_1_proofread->setEnabled(1);
    qProgressBar.setValue(nThPoolNum * (100 + 2));
}

// 下载缺失文件
void Widget_Local::DownloadLackFile()
{
    if (m_vDataList[1].isEmpty()) return;

    // 再次询问是否确定下载全部
    if (QMessageBox::Yes != QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否下载全部缺失文件？\n(下载文件保存在导出资源目录中)"), QMessageBox::Yes|QMessageBox::No))
        return;

    // 初始化各项数据
    bool bTerminator = true;
    QVector<SRealTimeData> sRtDataList(1, {0, 0, 0, 0, 0, 0});

    // 启动线程池
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    QtConcurrent::run(&thPool, CPatch::DownloadDataFile, m_vDataList[1], m_strLocalPath, std::ref(sRtDataList[0]), std::ref(bTerminator));

    // 显示进度条
    Dialog_Progress dProgress(this);
    dProgress.Start(EProgressType::Download, {QString::fromWCharArray(L"下载缺失文件")}, sRtDataList, bTerminator);
    dProgress.exec();

    // 询问是否重新整理本地导出资源列表和缺失文件列表
    if (QMessageBox::Yes == QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"资源列表已变动，是否重新进行整理？"), QMessageBox::Yes|QMessageBox::No))
        UpdateLackFile();
}

// 更新缺失文件列表
void Widget_Local::UpdateLackFile()
{
    Lookup({127});  // 清除查找状态

    // 创建进度条
    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在重新读取本地导出资源列表..."), NULL, 0, 0, this, Qt::Dialog | Qt::FramelessWindowHint);
    qProgressBar.installEventFilter(&sg_cEventFilter);
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setMinimumDuration(0);
    qProgressBar.setCancelButton(0);
    qProgressBar.show();

    // 重新读取本地资源
    QThreadPool thPool;
    qProgressBar.setLabelText(QString::fromWCharArray(L"正在重新读取本地导出资源列表..."));
    QtConcurrent::run(&thPool, CPatch::ReadDataDir, std::ref(m_vDataList[0]), m_strLocalPath);
    while (thPool.activeThreadCount() > 0)
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // 重新校对数据
    qProgressBar.setLabelText(QString::fromWCharArray(L"正在重新校对数据..."));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    int nThPoolNum = thPool.maxThreadCount();
    int nLen = m_vDataList[1].size() / nThPoolNum + 1;
    QVector< QVector < QString > > strLackList(nThPoolNum);
    QVector<int> nProgressList(nThPoolNum);
    for (int i = 0; i < nThPoolNum; i++)
    {
        QVector<int> nIndex(2);
        nIndex[0] = i * nLen;
        nIndex[1] = (i + 1) * nLen;
        if ((i + 1) == nThPoolNum) nIndex[1] = m_vDataList[1].size();
        QtConcurrent::run(&thPool, [&] (QVector<int> nIndex, QVector<QString> &strPatchList, int &nProgress) {
            for (int i = nIndex[0]; (i < nIndex[1]); i++)
            {
                int j = 0;
                while (j < m_vDataList[0].size())
                {
                    if (m_vDataList[0][j].patchPath == m_vDataList[1][i].patchPath) break;
                    j += 1;
                }
                if (j == m_vDataList[0].size()) strPatchList.push_back(m_vDataList[1][i].patchPath);
                nProgress = (double)(i - nIndex[0] + 1) / (nIndex[1] - nIndex[0]) * 100;
            }
        }, nIndex, std::ref(strLackList[i]), std::ref(nProgressList[i]));
    }
    qProgressBar.setMaximum(nThPoolNum * (100 + 10));
    qProgressBar.setValue(0);
    while (thPool.activeThreadCount() > 0)
    {
        QVector<int> tempProgressList(nProgressList);
        int nProgressNum = 0;
        for (auto i : tempProgressList) nProgressNum += i;
        qProgressBar.setValue(nProgressNum);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);
    }

    // 合并数据
    ui->Button_1_proofread->setEnabled(0);
    qProgressBar.setLabelText(QString::fromWCharArray(L" 正在合并校对数据..."));
    m_vDataList[1].clear();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    for (auto i : strLackList)
    {
        for (auto j : i)
        {
            SDataInfo tempDataInfo = { 3, "", "", 0, 0, 0 };
            tempDataInfo.patchPath = j;
            m_vDataList[1].push_back(tempDataInfo);
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
    }
    qProgressBar.setValue(nThPoolNum * (100 + 3));

    qProgressBar.setLabelText(QString::fromWCharArray(L"正在装载列表数据..."));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    m_pListWidget[0]->setEnabled(0);
    m_pListWidget[0]->clear();
    if (m_vDataList[0].isEmpty())
        m_pLabelInfo[0]->setText(QString::fromWCharArray(L"( 目录为空! )"));
    else
    {
        for (auto i : m_vDataList[0]) m_pListWidget[0]->addItem(i.patchPath);
        m_pLabelInfo[0]->setText(QString::fromWCharArray(L"( 一共加载了 ") + QString::number(m_vDataList[0].size()) + QString::fromWCharArray(L" 个文件 )"));
        m_pListWidget[0]->setEnabled(1);
    }
    qProgressBar.setValue(nThPoolNum * (100 + 7));

    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    m_pListWidget[1]->setEnabled(0);
    ui->Button_1_download->setEnabled(0);
    m_pListWidget[1]->clear();
    QString strMode = m_pLabelInfo[1]->text().split('\n').at(0);
    if (m_vDataList[1].isEmpty())
        m_pLabelInfo[1]->setText(strMode + QString::fromWCharArray(L"\n( 没有缺失的文件! )"));
    else
    {
        for (auto i : m_vDataList[1]) m_pListWidget[1]->addItem(i.patchPath);
        QString strMode = m_pLabelInfo[1]->text().split('\n').at(0);
        m_pLabelInfo[1]->setText(strMode + QString::fromWCharArray(L"\n( 一共加载了 ") + QString::number(m_vDataList[1].size()) + QString::fromWCharArray(L" 个文件 )"));
        m_pListWidget[1]->setEnabled(1);
        ui->Button_1_download->setEnabled(1);
    }
    qProgressBar.setValue(nThPoolNum * (100 + 10));
}

// 右键菜单事件
void Widget_Local::RightMenuEvent(int nIndex, int currentRow, int nMenuButton)
{
    if (nMenuButton == 1)   //　预览全图
    {
        m_pDialogPreview->setVisible(0);
        Dialog_Preview *pDialogPreview = new Dialog_Preview();
        pDialogPreview->setModal(1);
        pDialogPreview->setImageMax(m_vDataList[nIndex][currentRow], m_strLocalPath);
    }
    else if (nMenuButton == 2)  // 预览文本
    {
        Form_reader *pReader = new Form_reader(&g_sets, m_vDataList[nIndex][currentRow].patchPath, QString::fromLocal8Bit(CPatch::ReadDataFileOfData(m_vDataList[nIndex][currentRow], m_strLocalPath)));
        pReader->show();
    }
    else if (nMenuButton == 3)  // 替换文件
    {
        m_pDialogPreview->setVisible(0);
        QString strFilePath = QFileDialog::getOpenFileName(this, QString::fromWCharArray(L"请选择要替换的文件"));
        if (!strFilePath.isEmpty())
        {
            QFile::remove(m_vDataList[nIndex][currentRow].filePath);
            QFile::copy(strFilePath, m_vDataList[nIndex][currentRow].filePath);
            m_vDataList[nIndex][currentRow].fileLen = QFile(strFilePath).size();
            emit SendInfo(m_vDataList[nIndex][currentRow]);
        }
    }
    else if(nMenuButton == 4)   // 复制到单独目录
    {
        m_pDialogPreview->setVisible(0);

        // 获取选中列表
        QList<QListWidgetItem *> lsItemList(m_pListWidget[nIndex]->selectedItems());
        if (lsItemList.size() == 0) return;

        // 初始化数据
        QVector<SDataInfo> sDataInfoList;
        for (int i = 0; i < lsItemList.size(); i++)
            sDataInfoList.push_back(m_vDataList[nIndex][m_pListWidget[nIndex]->row(lsItemList[i])]);
        SRealTimeData sRtData = {0, 0, 0, 0, 0, 0};
        SExportConfig sExportConfig;
        sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
        wchar_t lcAlonePath[520];
        GetPrivateProfileString(L"配置", L"单独导出", L"BAlone", lcAlonePath, 520, L"BConfig/Config.ini");
        sExportConfig.strExportPath = QString::fromWCharArray(lcAlonePath);

        // 设置线程池
        bool bThread = true;
        QThreadPool thPool;
        QtConcurrent::run(&thPool, [&bThread] (QVector<SDataInfo> sDataInfoList, SExportConfig sExportConfig, SRealTimeData &sRtData) {
            sRtData.nState = 1;
            sRtData.nDataNum = sDataInfoList.size();
            for (int i = 0; i < sDataInfoList.size(); i++)
            {
                if (!bThread) return;
                QString strFilePath = sExportConfig.strExportPath + "/" + sDataInfoList[i].patchPath.split('\\').last();
                // 检查目标文件是否已存在（非覆盖模式下，将忽略已存在文件的重复导出）
                if (QFile().exists(strFilePath) && (!sExportConfig.nExportMode))
                {
                   sRtData.nIgnoreNum += 1; // 忽略数量 +1
                }
                else
                {
                    if (CPatch::CopyFilePath(sDataInfoList[i].filePath, strFilePath))
                        sRtData.nSuccessNum += 1;   // 成功数量 +1
                    else
                        sRtData.nFailNum += 1;      // 失败数量 +1
                }
            }
        }, sDataInfoList, sExportConfig, std::ref(sRtData));

        // 创建进度条
        QProgressDialog qProgressBar( QString::fromWCharArray(L"正在复制文件..."), NULL, 0, sDataInfoList.size(), this, Qt::Dialog | Qt::FramelessWindowHint);
        connect(&qProgressBar, &QProgressDialog::rejected, this, [&bThread, &qProgressBar]{
            if (QMessageBox::Yes == QMessageBox::information(&qProgressBar, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否中止当前操作？"), QMessageBox::Yes|QMessageBox::No))
                bThread = false;
            else qProgressBar.show();
        });
        qProgressBar.setWindowModality(Qt::WindowModal);
        qProgressBar.setCancelButton(0);
        qProgressBar.setMinimumDuration(0);
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
                               QString::fromWCharArray(L"文件复制完成！")
                               + QString::fromWCharArray(L"成功:") + CPatch::StrIntL(sRtData.nSuccessNum, 3)
                               + QString::fromWCharArray(L"失败:") + CPatch::StrIntL(sRtData.nFailNum, 3)
                               + QString::fromWCharArray(L"忽略:") + CPatch::StrIntL(sRtData.nIgnoreNum, 3));
            thPool.waitForDone(100);
        }
    }
    else
    {
        m_pDialogPreview->setVisible(0);

        // 获取选中列表
        QList<QListWidgetItem *> lsItemList(m_pListWidget[nIndex]->selectedItems());
        if (lsItemList.size() == 0) return;

        // 初始化数据
        QVector<SDataInfo> sDataInfoList;
        for (int i = 0; i < lsItemList.size(); i++)
            sDataInfoList.push_back(m_vDataList[nIndex][m_pListWidget[nIndex]->row(lsItemList[i])]);
        SRealTimeData sRtData = {0, 0, 0, 0, 0, 0};
        SExportConfig sExportConfig;
        sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
        if (nMenuButton == 5)  // 使用导出路径
        {
            sExportConfig.strExportPath = m_strLocalPath;
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
                if (!bThread) return;
                QString strFilePath = sExportConfig.strExportPath;
                if (nMenuButton == 6)   // 单独路径只需要文件名
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
        if (nMenuButton == 5)
        {
            // 询问是否重新整理本地导出资源列表和缺失文件列表
            if (QMessageBox::Yes == QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"资源列表已变动，是否重新进行整理？"), QMessageBox::Yes|QMessageBox::No))
                UpdateLackFile();
        }
    }
}

// 弹出右键菜单
void Widget_Local::CustomContextMenuRequested(const QPoint &pos, int nIndex)
{
    QListWidgetItem* pCurItem = m_pListWidget[nIndex]->itemAt(pos);
    if (pCurItem == NULL) return;
    int currentRow = m_pListWidget[nIndex]->currentRow();
    if (currentRow < 0) return;

    // 创建右键菜单
    QMenu* pMenu = new QMenu(this);
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
    QAction* pMenuButton1 = new QAction(QString::fromWCharArray(L"预览全图"), this);
    QAction* pMenuButton2 = new QAction(QString::fromWCharArray(L"预览文本"), this);
    QAction* pMenuButton3 = new QAction(QString::fromWCharArray(L"替换文件"), this);
    QAction* pMenuButton4 = new QAction(QString::fromWCharArray(L"复制到单独目录"), this);
    QAction* pMenuButton5 = new QAction(QString::fromWCharArray(L"下载到导出路径"), this);
    QAction* pMenuButton6 = new QAction(QString::fromWCharArray(L"下载到单独路径"), this);
    connect(pMenuButton1, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 1); });
    connect(pMenuButton2, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 2); });
    connect(pMenuButton3, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 3); });
    connect(pMenuButton4, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 4); });
    connect(pMenuButton5, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 5); });
    connect(pMenuButton6, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 6); });
    if (nIndex == 0)
    {
        pMenu->addAction(pMenuButton1);
        pMenu->addAction(pMenuButton2);
        pMenu->addAction(pMenuButton3);
        pMenu->addAction(pMenuButton4);
        QString strFormat = m_vDataList[nIndex][currentRow].patchPath.right(
                    m_vDataList[nIndex][currentRow].patchPath.size() - m_vDataList[nIndex][currentRow].patchPath.lastIndexOf('.'));
        // 是否为图片
        if (!QString(".bmp.png.jpg.dds.tga").contains(strFormat, Qt::CaseInsensitive)) pMenuButton1->setEnabled(0);
        // 是否为文本
        if (!QString(".txt.ini.lua.xml.json.fx.psh.string").contains(strFormat, Qt::CaseInsensitive)) pMenuButton2->setEnabled(0);
    }
    else
    {
        pMenu->addAction(pMenuButton5);
        pMenu->addAction(pMenuButton6);
    }

    pMenu->exec(QCursor::pos());

    delete pMenuButton1;
    delete pMenuButton2;
    delete pMenuButton3;
    delete pMenuButton4;
    delete pMenuButton5;
    delete pMenuButton6;
    delete pMenu;
}

// 选择项发生变化时
void Widget_Local::CurrentRowChanged(int nIndex, int currentRow)
{
    if (currentRow < 0) return;
    if (m_pDialogPreview != NULL) m_pDialogPreview->Stop();

    // 清除其他列表的选中项
    for (int i = 0; i < 2; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        if (i == nIndex) continue;
        m_pListWidget[i]->setCurrentRow(-1);
        m_pListWidget[i]->clearSelection();
    }

    if (m_vDataList[nIndex].isEmpty()) return;
    emit SendInfo(m_vDataList[nIndex][currentRow]);

    // 是否启用了预览图片
    if ((bPreviewPicture) && (nIndex == 0))
    {
        QString strFormat = m_vDataList[nIndex][currentRow].patchPath.right(
                    m_vDataList[nIndex][currentRow].patchPath.size() - m_vDataList[nIndex][currentRow].patchPath.lastIndexOf('.'));
        // 是否为图片文件
        if (QString(".bmp.png.jpg.dds.tga").contains(strFormat, Qt::CaseInsensitive))
        {
            if (bPreviewPicture == 1) m_pDialogPreview->setImageMin(m_vDataList[nIndex][currentRow],  m_strLocalPath);
            else if (bPreviewPicture == 2) m_pDialogPreview->setImageMinEX(m_vDataList[nIndex][currentRow],  m_strLocalPath);
        }
    }
}
