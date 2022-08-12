#include "widget_hero.h"
#include "ui_widget_hero.h"
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
#include <QClipboard>

Widget_Hero::Widget_Hero(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget_Hero)
{
    ui->setupUi(this);

    m_pLabelInfo[0] = ui->label_info_0;
    m_pLabelInfo[1] = ui->label_info_1;
    m_pLabelInfo[2] = ui->label_info_2;
    m_pLabelInfo[3] = ui->label_info_3;
    m_pLabelInfo[4] = ui->label_info_4;
    m_pLabelInfo[5] = ui->label_info_5;

    m_pListWidget[0] = ui->list_dataList_0;
    m_pListWidget[1] = ui->list_dataList_1;
    m_pListWidget[2] = ui->list_dataList_2;
    m_pListWidget[3] = ui->list_dataList_3;
    m_pListWidget[4] = ui->list_dataList_4;
    m_pListWidget[5] = ui->list_dataList_5;

    connect(m_pListWidget[0], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(0, currentRow);});
    connect(m_pListWidget[1], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(1, currentRow);});
    connect(m_pListWidget[2], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(2, currentRow);});
    connect(m_pListWidget[3], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(3, currentRow);});
    connect(m_pListWidget[4], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(4, currentRow);});
    connect(m_pListWidget[5], &QListWidget::currentRowChanged, this, [=] (int currentRow) { this->CurrentRowChanged(5, currentRow);});

    connect(m_pListWidget[0], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 0); });
    connect(m_pListWidget[1], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 1); });
    connect(m_pListWidget[2], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 2); });
    connect(m_pListWidget[3], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 3); });
    connect(m_pListWidget[4], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 4); });
    connect(m_pListWidget[5], &QWidget::customContextMenuRequested, this, [=] (const QPoint &pos) { this->CustomContextMenuRequested(pos, 5); });

    for (auto i : m_pListWidget)
    {
        connect(i, &QListWidget::itemDoubleClicked, this, [&] (QListWidgetItem *item) {
            QApplication::clipboard()->setText(item->text().replace('/', '\\'));
            emit SendTip(QString::fromWCharArray(L"已复制 补丁路径：") + item->text());
        });
    }

    connect(ui->Button_5_proofread, &QPushButton::clicked, this, &Widget_Hero::ProofreadLackFile);
    connect(ui->Button_5_download, &QPushButton::clicked, this, &Widget_Hero::DownloadLackFile);
};

Widget_Hero::~Widget_Hero()
{
    delete ui;
}

// 初始化
void Widget_Hero::Init()
{
    Clear();

    // 读取游戏路径配置
    wchar_t lcGamePath[520];
    GetPrivateProfileString(L"配置", L"游戏路径", L"", lcGamePath, 520, L"BConfig/Config.ini");
    m_strGamePath = QString::fromWCharArray(lcGamePath);
    if (!QDir().exists(m_strGamePath))
    {
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"游戏路径不存在！"));
        return;
    }

    // 装载图片预览器
    m_pDialogPreview = new Dialog_Preview(this);
    m_pDialogPreview->move(1000, 600);
    m_pDialogPreview->setVisible(0);
    bPreviewPicture = GetPrivateProfileInt(L"配置", L"预览图片", 1, L"BConfig/Config.ini");

    // 读取资源数据
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 5) thPool.setMaxThreadCount(5);
    QString strPathList[4] = {"/Data.jmp", "/Data1.jmp", "/Data2.jmp", "/Data3.jmp"};
    QFuture<int> qFuture[5];
    m_pLabelInfo[5]->setText(QString::fromWCharArray(L"( 未校对 )"));
    // data数据
    for (int i = 0; i < 4; i++)
    {
        m_pLabelInfo[i]->setText(QString::fromWCharArray(L"( 正在读取文件... )"));
        QString strDataPath = m_strGamePath + strPathList[i];
        qFuture[i] = QtConcurrent::run(&thPool, CPatch::ReadDataFile, std::ref(m_vDataList[i]), strDataPath);
    }
    // asyncfile目录
    {
        m_pLabelInfo[4]->setText(QString::fromWCharArray(L"( 正在读取目录... )"));
        QString strDirPath = m_strGamePath + "/asyncfile";
        qFuture[4] = QtConcurrent::run(&thPool, CPatch::ReadDataDir, std::ref(m_vDataList[4]), strDirPath);
    }

    // 创建进度条
    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在读取资源..."), NULL, 0, 5, this, Qt::Dialog | Qt::FramelessWindowHint);
    qProgressBar.installEventFilter(&sg_cEventFilter);
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setCancelButton(0);
    qProgressBar.setMinimumDuration(1);
    qProgressBar.show();
    int nProgressNum = 0;
    while (thPool.activeThreadCount() > 0)
    {
        nProgressNum = 5 - thPool.activeThreadCount();
        qProgressBar.setValue(nProgressNum);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    qProgressBar.setValue(5);

    // 显示数据
    for (int i = 0; i < 5; i++) m_pLabelInfo[i]->setText(QString::fromWCharArray(L"( 读取完毕！正在整理数据... )"));
    for (int i = 0; i < 5; i++)
    {
        int ret = qFuture[i].result();
        if (ret == FILE_OK)
        {
            for (int j = 0; j < m_vDataList[i].size(); j++)
            {
                m_pListWidget[i]->addItem(m_vDataList[i][j].patchPath);
            }
            QString strInfo = QString::fromWCharArray(L"( 一共加载了 ") + QString::number(m_vDataList[i].size()) + QString::fromWCharArray(L" 个文件 )");
            m_pLabelInfo[i]->setText(strInfo);
            m_pListWidget[i]->setEnabled(1);
        }
        else if (ret == FILE_OPEN_FAEL)
        {
            m_pLabelInfo[i]->setText(QString::fromWCharArray(i == 4 ? L"( 目录不存在! )" : L"( 文件读取失败! )"));
        }
        else if (ret == FILE_DATA_NULL)
        {
            m_pLabelInfo[i]->setText(QString::fromWCharArray(i == 4 ? L"( 目录为空! )" : L"( 文件数据为空! )"));
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    ui->Button_5_proofread->setEnabled(1);
}

// 查找
void Widget_Hero::Lookup(QString strKeyword)
{
    for (int i = 0; i < 6; i++)
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
            for (int i = 0; i < 6; i++)
            {
                if (m_vDataList[i].isEmpty()) continue;
                for (int j = 0; j < m_vDataList[i].size(); j++)
                    m_pListWidget[i]->setEnabled(0);
            }
            for (int i = 0; i < 6; i++)
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

    ui->label_infoLookup->setText("(0/6)" + QString::fromWCharArray(L"正在查找中..."));
    for (int i = 0; i < 6; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        m_pListWidget[i]->setEnabled(0);
    }

    int nListNum = 0;
    int nLookupNum = 0;
    for (int i = 0; i < 6; i++)
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
        ui->label_infoLookup->setText("(" + QString::number(i + 1) + "/6)" + QString::fromWCharArray(L"正在查找中..."));
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    ui->label_infoLookup->setText(QString::fromWCharArray(L"查找完毕！遍历 ")
                                  + QString::number(nListNum)
                                  + QString::fromWCharArray(L" 项数据，一共查找到 ")
                                  + QString::number(nLookupNum)
                                  + QString::fromWCharArray(L" 个目标！"));
}

// 全部导出
void Widget_Hero::ExportAll()
{
    // 所有data列表为空时无法导出
    int nNull = 0;
    for (int i = 0; i < 5; i++)
    {
        if (m_pListWidget[i]->isEnabled() == 0 || m_pListWidget[i]->count() == 0)
            nNull += 1;
    }
    if (nNull == 5)
    {
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"所有data列表不可用！"));
        return;
    }

    // 再次询问是否确定导出全部
    if (QMessageBox::Yes != QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否要导出全部文件？"), QMessageBox::Yes|QMessageBox::No))
        return;

    // 初始化各项数据
    bool bTerminator = true;
    SExportConfig sExportConfig;
    QVector<SRealTimeData> sRtDataList(5);
    QVector<QString> strFileNameList{ "Data.jmp", "Data1.jmp", "Data2.jmp", "Data3.jmp", "asyncfile" };
    wchar_t lcExportPath[520];
    sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
    GetPrivateProfileString(L"配置", L"导出路径", L"", lcExportPath, 520, L"BConfig/Config.ini");
    sExportConfig.strExportPath = QString::fromWCharArray(lcExportPath);

    // 启动线程池
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 5) thPool.setMaxThreadCount(5);
    for (int i = 0; i < 4; i++)
        QtConcurrent::run(&thPool, CPatch::UnpackDataFile, i, QString(m_strGamePath + "/" + strFileNameList[i]), sExportConfig, std::ref(sRtDataList[i]), std::ref(bTerminator));
    QtConcurrent::run(&thPool, CPatch::UnpackDirFile, QString(m_strGamePath + "/" + strFileNameList[4]), sExportConfig, std::ref(sRtDataList[4]), std::ref(bTerminator));

    // 显示进度条
    Dialog_Progress dProgress(this);
    dProgress.Start(EProgressType::Unpack, strFileNameList, sRtDataList, bTerminator);
    dProgress.exec();
    thPool.waitForDone(100);
}

// 清除页面
void Widget_Hero::Clear()
{
    ui->label_infoLookup->setText("");
    for (int i = 0; i < 6; i++)
    {
        m_vDataList[i].clear();
        m_pListWidget[i]->clear();
        m_pListWidget[i]->setEnabled(0);
        m_pLabelInfo[i]->setText("");
    }
    m_strGamePath.clear();

    ui->Button_5_proofread->setEnabled(0);
    ui->Button_5_download->setEnabled(0);

    if (m_pDialogPreview != NULL)
    {
        m_pDialogPreview->Stop();
        delete m_pDialogPreview;
        m_pDialogPreview = NULL;
    }
}

// 校对缺失文件列表
void Widget_Hero::ProofreadLackFile()
{
    // 合并所有列表
    QVector<SDataInfo> sDataList;
    sDataList.reserve(m_vDataList[0].size() + m_vDataList[1].size() + m_vDataList[2].size() + m_vDataList[3].size() + m_vDataList[4].size() + 1);
    for (int i = 0; i < 5; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        sDataList.append(m_vDataList[i]);
    }

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
    QFuture<int> qFuture = QtConcurrent::run(&thPool, CPatch::ReadClientList, std::ref(strClientList), m_strGamePath, std::ref(strInfo), std::ref(bRun));
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
        QtConcurrent::run(&thPool, [&bRun, sDataList, strClientList] (QVector<int> nIndex, QVector<QString> &strPatchList, int &nProgress) {
            for (int i = nIndex[0]; (i < nIndex[1]) && bRun; i++)
            {
                int j = 0;
                while (j < sDataList.size() && bRun)
                {
                    if (sDataList[j].patchPath == strClientList[i]) break;
                    j += 1;
                }
                if ((j == sDataList.size()) && bRun) strPatchList.push_back(strClientList[i]);
                nProgress = (double)(i - nIndex[0] + 1) / (nIndex[1] - nIndex[0]) * 100;
            }
        }, nIndex, std::ref(strLackList[i]), std::ref(nProgressList[i]));
    }
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在校对数据..."));
    qProgressBar.setMaximum(nThPoolNum * (100 + 10));
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
    ui->Button_5_proofread->setEnabled(0);
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在合并校对数据..."));
    m_vDataList[5].clear();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    for (auto i : strLackList)
    {
        for (auto j : i)
        {
            SDataInfo tempDataInfo = { 3, "", "", 0, 0, 0 };
            tempDataInfo.patchPath = j;
            m_vDataList[5].push_back(tempDataInfo);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    qProgressBar.setValue(nThPoolNum * (100 + 5));

    // 装载数据
    qProgressBar.setLabelText(strInfo + QString::fromWCharArray(L" 正在装载数据..."));
    m_pListWidget[5]->clear();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    if (m_vDataList[5].isEmpty())
        m_pLabelInfo[5]->setText(strInfo + QString::fromWCharArray(L"\n( 没有缺失的文件! )"));
    else
    {
        m_pLabelInfo[5]->setText(strInfo + QString::fromWCharArray(L"\n( 正在装载数据... "));
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        for (auto i : m_vDataList[5]) m_pListWidget[5]->addItem(i.patchPath);
        m_pLabelInfo[5]->setText(strInfo + QString::fromWCharArray(L"\n( 一共加载了 ") + QString::number(m_vDataList[5].size()) + QString::fromWCharArray(L" 个文件 )"));
        m_pListWidget[5]->setEnabled(1);
        ui->Button_5_download->setEnabled(1);
    }
    ui->Button_5_proofread->setEnabled(1);
    qProgressBar.setValue(nThPoolNum * (100 + 10));
}

// 下载缺失文件
void Widget_Hero::DownloadLackFile()
{
    if (m_vDataList[5].isEmpty()) return;

    // 再次询问是否确定下载全部
    if (QMessageBox::Yes != QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否下载全部缺失文件？\n(下载文件保存在asyncfile目录中)"), QMessageBox::Yes|QMessageBox::No))
        return;

    // 初始化各项数据
    bool bTerminator = true;
    QVector<SRealTimeData> sRtDataList(1, {0, 0, 0, 0, 0, 0});

    // 启动线程池
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    QtConcurrent::run(&thPool, CPatch::DownloadDataFile, m_vDataList[5], m_strGamePath + "/asyncfile", std::ref(sRtDataList[0]), std::ref(bTerminator));

    // 显示进度条
    Dialog_Progress dProgress(this);
    dProgress.Start(EProgressType::Download, {QString::fromWCharArray(L"下载缺失文件")}, sRtDataList, bTerminator);
    dProgress.exec();

    // 重新整理asyncfile列表和缺失文件列表
    UpdateLackFile();
}

// 更新缺失文件列表
void Widget_Hero::UpdateLackFile()
{
    Lookup({127});  // 清除查找状态

    QProgressDialog qProgressBar( QString::fromWCharArray(L"正在重新整理asyncfile列表..."), NULL, 0, 5, this, Qt::Dialog | Qt::FramelessWindowHint);
    qProgressBar.installEventFilter(&sg_cEventFilter);
    qProgressBar.setWindowModality(Qt::WindowModal);
    qProgressBar.setMinimumDuration(0);
    qProgressBar.setCancelButton(0);
    qProgressBar.show();

    int nProgressNum = 0;
    QString strInfo = QString::fromWCharArray(L"正在重新整理asyncfile列表...");
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    QtConcurrent::run(&thPool, [&] {
        m_vDataList[4].clear();
        CPatch::ReadDataDir(m_vDataList[4], m_strGamePath + "/asyncfile");
        nProgressNum = 1;

        strInfo = QString::fromWCharArray(L"正在重新整理缺失文件列表...");
        QVector<SDataInfo> sDataList;
        for (auto i : m_vDataList[5])
        {
            int j = 0;
            while (j < m_vDataList[4].size())
            {
                if (i.patchPath == m_vDataList[4][j].patchPath) break;
                j += 1;
            }
            if (j == m_vDataList[4].size()) sDataList.push_back(i);
        }
        nProgressNum = 2;

        m_vDataList[5].clear();
        if (!sDataList.isEmpty()) m_vDataList[5] = sDataList;

        nProgressNum = 3;
        return;
    });

    while (thPool.activeThreadCount() > 0)
    {
        qProgressBar.setLabelText(strInfo);
        qProgressBar.setValue(nProgressNum);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    qProgressBar.setLabelText(QString::fromWCharArray(L"正在装载列表数据..."));

    m_pListWidget[4]->setEnabled(0);
    m_pListWidget[4]->clear();
    if (m_vDataList[4].isEmpty())
        m_pLabelInfo[4]->setText(QString::fromWCharArray(L"( 目录为空! )"));
    else
    {
        for (auto i : m_vDataList[4]) m_pListWidget[4]->addItem(i.patchPath);
        m_pLabelInfo[4]->setText(QString::fromWCharArray(L"( 一共加载了 ") + QString::number(m_vDataList[4].size()) + QString::fromWCharArray(L" 个文件 )"));
        m_pListWidget[4]->setEnabled(1);
    }
    qProgressBar.setValue(4);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    m_pListWidget[5]->setEnabled(0);
    ui->Button_5_download->setEnabled(0);
    m_pListWidget[5]->clear();
    QString strMode = m_pLabelInfo[5]->text().split('\n').at(0);
    if (m_vDataList[5].isEmpty())
        m_pLabelInfo[5]->setText(strMode + QString::fromWCharArray(L"\n( 没有缺失的文件! )"));
    else
    {
        for (auto i : m_vDataList[5]) m_pListWidget[5]->addItem(i.patchPath);
        QString strMode = m_pLabelInfo[5]->text().split('\n').at(0);
        m_pLabelInfo[5]->setText(strMode + QString::fromWCharArray(L"\n( 一共加载了 ") + QString::number(m_vDataList[5].size()) + QString::fromWCharArray(L" 个文件 )"));
        m_pListWidget[5]->setEnabled(1);
        ui->Button_5_download->setEnabled(1);
    }
    qProgressBar.setValue(5);
}

// 右键菜单事件
void Widget_Hero::RightMenuEvent(int nIndex, int currentRow, int nMenuButton)
{
    if (nMenuButton == 1)   //　预览全图
    {
        m_pDialogPreview->setVisible(0);
        QString strPathList[5] = {"/Data.jmp", "/Data1.jmp", "/Data2.jmp", "/Data3.jmp", "/asyncfile"};
        Dialog_Preview *pDialogPreview = new Dialog_Preview();
        pDialogPreview->setModal(1);
        pDialogPreview->setImageMax(m_vDataList[nIndex][currentRow], m_strGamePath + strPathList[nIndex]);
    }
    else if (nMenuButton == 2)  //　预览文本
    {
        QString strPathList[5] = {"/Data.jmp", "/Data1.jmp", "/Data2.jmp", "/Data3.jmp", "/asyncfile"};
        Form_reader *pReader = new Form_reader(&g_sets, m_vDataList[nIndex][currentRow].patchPath, QString::fromLocal8Bit(CPatch::ReadDataFileOfData(m_vDataList[nIndex][currentRow],  m_strGamePath + strPathList[nIndex])));
        pReader->show();
    }
    else
    {
        m_pDialogPreview->setVisible(0);

        // 获取选中列表
        QList<QListWidgetItem *> lsItemList(m_pListWidget[nIndex]->selectedItems());
        if (lsItemList.size() == 0) return;

        // 初始化数据
        QString strPathList[6] = {"/Data.jmp", "/Data1.jmp", "/Data2.jmp", "/Data3.jmp", "/asyncfile", ""};
        QVector<SDataInfo> sDataInfoList;
        for (int i = 0; i < lsItemList.size(); i++)
            sDataInfoList.push_back(m_vDataList[nIndex][m_pListWidget[nIndex]->row(lsItemList[i])]);
        SRealTimeData sRtData = {0, 0, 0, 0, 0, 0};
        SExportConfig sExportConfig;
        sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
        if (nMenuButton == 5)   // 下载到asyncfile
        {
            sExportConfig.strExportPath = m_strGamePath + "/asyncfile";
        }
        else if ((nMenuButton == 3) || (nMenuButton == 6))  // 使用导出路径
        {
            wchar_t lcExportPath[520];
            GetPrivateProfileString(L"配置", L"导出路径", L"BExport", lcExportPath, 520, L"BConfig/Config.ini");
            sExportConfig.strExportPath = QString::fromWCharArray(lcExportPath);
        }
        else if ((nMenuButton == 4) || (nMenuButton == 7))  // 使用单独路径
        {
            wchar_t lcAlonePath[520];
            GetPrivateProfileString(L"配置", L"单独导出", L"BAlone", lcAlonePath, 520, L"BConfig/Config.ini");
            sExportConfig.strExportPath = QString::fromWCharArray(lcAlonePath);
        }

        // 设置线程池
        bool bThread = true;
        QThreadPool thPool;
        QtConcurrent::run(&thPool, [nIndex, &bThread] (int nMenuButton, QVector<SDataInfo> sDataInfoList, QString strDataPath, SExportConfig sExportConfig, SRealTimeData &sRtData) {
            sRtData.nState = 1;
            sRtData.nDataNum = sDataInfoList.size();
            for (int i = 0; i < sDataInfoList.size(); i++)
            {
                if (!bThread) return;
                QString strFilePath = sExportConfig.strExportPath;
                if ((nMenuButton == 4) || (nMenuButton == 7))   // 单独路径只需要文件名
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
                    if (nIndex == 5)
                    {
                        if (0 == CPatch::DownloadTYGzFile(sDataInfoList[i].patchPath, strFilePath))
                            sRtData.nSuccessNum += 1;   // 成功数量 +1
                        else
                            sRtData.nFailNum += 1;      // 失败数量 +1
                    }
                    else if (nIndex == 4)
                    {
                        if (CPatch::CopyFilePath(sDataInfoList[i].filePath, strFilePath))
                            sRtData.nSuccessNum += 1;   // 成功数量 +1
                        else
                            sRtData.nFailNum += 1;      // 失败数量 +1
                    }
                    else
                    {
                        if (FILE_OK == CPatch::ExportDataFile(sDataInfoList[i], strDataPath, strFilePath))
                            sRtData.nSuccessNum += 1;   // 成功数量 +1
                        else
                            sRtData.nFailNum += 1;      // 失败数量 +1
                    }
                }
            }
        }, nMenuButton, sDataInfoList, m_strGamePath + strPathList[nIndex], sExportConfig, std::ref(sRtData));

        // 创建进度条
        QProgressDialog qProgressBar( QString::fromWCharArray(nIndex == 5 ? L"正在下载文件..." : L"正在导出文件..."), NULL, 0, sDataInfoList.size(), this, Qt::Dialog | Qt::FramelessWindowHint);
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
                               QString::fromWCharArray(nIndex == 5 ? L"文件下载完成！" : L"文件导出完成！\n")
                               + QString::fromWCharArray(L"成功:") + CPatch::StrIntL(sRtData.nSuccessNum, 3)
                               + QString::fromWCharArray(L"失败:") + CPatch::StrIntL(sRtData.nFailNum, 3)
                               + QString::fromWCharArray(L"忽略:") + CPatch::StrIntL(sRtData.nIgnoreNum, 3));
            thPool.waitForDone(100);
        }
        if (nMenuButton == 5) UpdateLackFile(); // 下载到asyncfile后需要更新列表
    }
}

// 弹出右键菜单
void Widget_Hero::CustomContextMenuRequested(const QPoint &pos, int nIndex)
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
    QAction* pMenuButton3 = new QAction(QString::fromWCharArray(L"路径导出"), this);
    QAction* pMenuButton4 = new QAction(QString::fromWCharArray(L"单独导出"), this);
    QAction* pMenuButton5 = new QAction(QString::fromWCharArray(L"下载到asyncfile"), this);
    QAction* pMenuButton6 = new QAction(QString::fromWCharArray(L"下载到导出路径"), this);
    QAction* pMenuButton7 = new QAction(QString::fromWCharArray(L"下载到单独路径"), this);
    connect(pMenuButton1, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 1); });
    connect(pMenuButton2, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 2); });
    connect(pMenuButton3, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 3); });
    connect(pMenuButton4, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 4); });
    connect(pMenuButton5, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 5); });
    connect(pMenuButton6, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 6); });
    connect(pMenuButton7, &QAction::triggered, this, [=] { RightMenuEvent(nIndex, currentRow, 7); });
    if (nIndex < 5)
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
        pMenu->addAction(pMenuButton7);
    }

    pMenu->exec(QCursor::pos());

    delete pMenuButton1;
    delete pMenuButton2;
    delete pMenuButton3;
    delete pMenuButton4;
    delete pMenuButton5;
    delete pMenuButton6;
    delete pMenuButton7;
    delete pMenu;
}

// 选择项发生变化时
void Widget_Hero::CurrentRowChanged(int nIndex, int currentRow)
{
    if (currentRow < 0) return;
    if (m_pDialogPreview != NULL) m_pDialogPreview->Stop();

    // 清除其他列表的选中项
    for (int i = 0; i < 6; i++)
    {
        if (m_vDataList[i].isEmpty()) continue;
        if (i == nIndex) continue;
        m_pListWidget[i]->setCurrentRow(-1);
        m_pListWidget[i]->clearSelection();
    }

    if (m_vDataList[nIndex].isEmpty()) return;
    emit SendInfo(m_vDataList[nIndex][currentRow]);

    // 是否启用了预览图片
    if ((bPreviewPicture) && (nIndex < 5))
    {
        QString strFormat = m_vDataList[nIndex][currentRow].patchPath.right(
                    m_vDataList[nIndex][currentRow].patchPath.size() - m_vDataList[nIndex][currentRow].patchPath.lastIndexOf('.'));
        // 是否为图片文件
        if (QString(".bmp.png.jpg.dds.tga").contains(strFormat, Qt::CaseInsensitive))
        {
            QString strPathList[5] = {"/Data.jmp", "/Data1.jmp", "/Data2.jmp", "/Data3.jmp", "/asyncfile"};
            if (bPreviewPicture == 1) m_pDialogPreview->setImageMin(m_vDataList[nIndex][currentRow],  m_strGamePath + strPathList[nIndex]);
            else if (bPreviewPicture == 2) m_pDialogPreview->setImageMinEX(m_vDataList[nIndex][currentRow],  m_strGamePath + strPathList[nIndex]);
        }
    }
}
