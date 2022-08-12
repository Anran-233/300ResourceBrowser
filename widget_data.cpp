#include "widget_data.h"
#include "ui_widget_data.h"
#include "dialog_progress.h"
#include "ceventfilter.h"
#include "form_reader.h"

#include "windows.h"
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThreadPool>
#include <QtConcurrent>
#include <QClipboard>

Widget_Data::Widget_Data(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget_Data)
{
    ui->setupUi(this);

    connect(ui->list_dataList, &QListWidget::itemDoubleClicked, this, [&] (QListWidgetItem *item) {
        QApplication::clipboard()->setText(item->text().replace('/', '\\'));
        emit SendTip(QString::fromWCharArray(L"已复制 补丁路径：") + item->text());
    });
}

Widget_Data::~Widget_Data()
{
    delete ui;
}

// 初始化
void Widget_Data::Init(QString strDataPath)
{
    Clear();

    int nPos = strDataPath.lastIndexOf('/');
    if (nPos == -1) nPos = strDataPath.lastIndexOf('\\');
    ui->label_nameData->setText(strDataPath.right(strDataPath.size() - nPos -1));

    // 装载图片预览器
    m_pDialogPreview = new Dialog_Preview(this);
    m_pDialogPreview->move(1000, 600);
    m_pDialogPreview->setVisible(0);
    bPreviewPicture = GetPrivateProfileInt(L"配置", L"预览图片", 1, L"BConfig/Config.ini");

    // 读取data数据
    ui->label_infoData->setText(QString::fromWCharArray(L"( 正在读取文件... )"));
    int ret = CPatch::ReadDataFile(m_vDataList, strDataPath);
    if (ret != FILE_OK)
    {
        if (ret == FILE_OPEN_FAEL)
        {
            ui->label_infoData->setText(QString::fromWCharArray(L"( 文件读取失败! )"));
            return;
        }
        if (ret == FILE_DATA_NULL)
        {
            ui->label_infoData->setText(QString::fromWCharArray(L"( 文件数据为空! )"));
            return;
        }
    }

    // 显示data数据
    for (int i = 0; i < m_vDataList.size(); i++)
    {
        ui->list_dataList->addItem(m_vDataList[i].patchPath);
    }
    QString strInfo = QString::fromWCharArray(L"( 一共加载了 ") + QString::number(m_vDataList.size()) + QString::fromWCharArray(L" 个文件 )");
    ui->label_infoData->setText(strInfo);
    ui->list_dataList->setEnabled(1);
    m_strDataPath = strDataPath;
}

// 查找
void Widget_Data::Lookup(QString strKeyword)
{
    ui->list_dataList->clearSelection();
    const int nListNum = ui->list_dataList->count();
    if (strKeyword.size() < 2)
    {
        if (strKeyword == 127)
        {
            ui->label_infoLookup->setText(QString::fromWCharArray(L"正在清除查找结果..."));
            ui->list_dataList->setEnabled(0);
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

// 全部导出
void Widget_Data::ExportAll()
{
    // data列表为空时无法导出
    if (ui->list_dataList->isEnabled() == 0 || ui->list_dataList->count() == 0)
    {
        QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"当前data列表不可用！"));
        return;
    }

    // 再次询问是否确定导出全部
    if (QMessageBox::Yes != QMessageBox::information(this, QString::fromWCharArray(L"询问"), QString::fromWCharArray(L"是否要导出全部文件？"), QMessageBox::Yes|QMessageBox::No))
        return;

    // 初始化各项数据
    int nID = 0;
    bool bTerminator = true;
    SExportConfig sExportConfig;
    QVector<SRealTimeData> sRtDataList(1);
    QVector<QString> strFileNameList(1, ui->label_nameData->text());
    wchar_t lcExportPath[520];
    sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
    GetPrivateProfileString(L"配置", L"导出路径", L"", lcExportPath, 520, L"BConfig/Config.ini");
    sExportConfig.strExportPath = QString::fromWCharArray(lcExportPath);

    // 启动线程池
    QThreadPool thPool;
    if (thPool.maxThreadCount() < 1) thPool.setMaxThreadCount(1);
    QtConcurrent::run(&thPool, CPatch::UnpackDataFile, nID, m_strDataPath, sExportConfig, std::ref(sRtDataList[0]), std::ref(bTerminator));

    // 显示进度条
    Dialog_Progress dProgress(this);
    dProgress.Start(EProgressType::Unpack, strFileNameList, sRtDataList, bTerminator);
    dProgress.exec();
    thPool.waitForDone(100);
}

// 清除页面
void Widget_Data::Clear()
{
    m_strDataPath.clear();
    m_vDataList.clear();

    ui->list_dataList->clear();
    ui->list_dataList->setEnabled(0);

    ui->label_infoLookup->setText("");
    ui->label_infoData->setText("");
    ui->label_nameData->setText("");

    if (m_pDialogPreview != NULL)
    {
        m_pDialogPreview->Stop();
        delete m_pDialogPreview;
        m_pDialogPreview = NULL;
    }
}

// 右键菜单事件
void Widget_Data::RightMenuEvent(int nSelectIndex, int nMenuButton)
{
    if (nMenuButton == 1)   //　预览全图
    {
        m_pDialogPreview->setVisible(0);
        Dialog_Preview *pDialogPreview = new Dialog_Preview();
        pDialogPreview->setModal(1);
        pDialogPreview->setImageMax(m_vDataList[nSelectIndex], m_strDataPath);
    }
    else if (nMenuButton == 2)   //　预览文本
    {
        Form_reader *pReader = new Form_reader(&g_sets, m_vDataList[nSelectIndex].patchPath, QString::fromLocal8Bit(CPatch::ReadDataFileOfData(m_vDataList[nSelectIndex], m_strDataPath)));
        pReader->show();
    }
    else if ((nMenuButton == 3) || (nMenuButton == 4))  // 导出文件
    {
        // 获取选中列表
        QList<QListWidgetItem *> lsItemList(ui->list_dataList->selectedItems());
        if (lsItemList.size() == 0) return;
        ui->list_dataList->setEnabled(0);

        // 初始化数据
        QVector<SDataInfo> sDataInfoList;
        for (int i = 0; i < lsItemList.size(); i++)
            sDataInfoList.push_back(m_vDataList[ui->list_dataList->row(lsItemList[i])]);
        SRealTimeData sRtData = {0, 0, 0, 0, 0, 0};
        SExportConfig sExportConfig;
        sExportConfig.nExportMode = GetPrivateProfileInt(L"配置", L"导出模式", 1, L"BConfig/Config.ini");
        if (nMenuButton == 3)       // 路径导出
        {
            wchar_t lcExportPath[520];
            GetPrivateProfileString(L"配置", L"导出路径", L"BExport", lcExportPath, 520, L"BConfig/Config.ini");
            sExportConfig.strExportPath = QString::fromWCharArray(lcExportPath);
        }
        else if (nMenuButton == 4)  // 单独导出
        {
            wchar_t lcAlonePath[520];
            GetPrivateProfileString(L"配置", L"单独导出", L"BAlone", lcAlonePath, 520, L"BConfig/Config.ini");
            sExportConfig.strExportPath = QString::fromWCharArray(lcAlonePath);
        }

        // 设置线程池
        bool bThread = true;
        QThreadPool thPool;
        QtConcurrent::run(&thPool, [&bThread] (int nMenuButton, QVector<SDataInfo> sDataInfoList, QString strDataPath, SExportConfig sExportConfig, SRealTimeData &sRtData) {
            sRtData.nState = 1;
            sRtData.nDataNum = sDataInfoList.size();
            for (int i = 0; i < sDataInfoList.size(); i++)
            {
                if (!bThread) return;
                QString strFilePath = sExportConfig.strExportPath;
                if (nMenuButton == 3)       // 路径导出
                    strFilePath += CPatch::PathPatchToFile(sDataInfoList[i].patchPath);
                else if (nMenuButton == 4)  // 单独导出
                    strFilePath += "/" + sDataInfoList[i].patchPath.split('\\').last();
                // 检查目标文件是否已存在（非覆盖模式下，将忽略已存在文件的重复导出）
                if (QFile().exists(strFilePath) && (!sExportConfig.nExportMode))
                {
                   sRtData.nIgnoreNum += 1; // 忽略数量 +1
                }
                else
                {
                    if (FILE_OK == CPatch::ExportDataFile(sDataInfoList[i], strDataPath, strFilePath))
                        sRtData.nSuccessNum += 1;   // 成功数量 +1
                    else
                        sRtData.nFailNum += 1;      // 失败数量 +1
                }
            }
        }, nMenuButton, sDataInfoList, m_strDataPath, sExportConfig, std::ref(sRtData));

        // 创建进度条
        QProgressDialog qProgressBar( QString::fromWCharArray(L"正在导出文件..."), QString::fromWCharArray(L"取消"), 0, sDataInfoList.size(), this, Qt::Dialog | Qt::FramelessWindowHint);
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
                               QString::fromWCharArray(L"文件导出完成！\n")
                               + QString::fromWCharArray(L"成功:") + CPatch::StrIntL(sRtData.nSuccessNum, 3)
                               + QString::fromWCharArray(L"失败:") + CPatch::StrIntL(sRtData.nFailNum, 3)
                               + QString::fromWCharArray(L"忽略:") + CPatch::StrIntL(sRtData.nIgnoreNum, 3));
            thPool.waitForDone(100);
        }
        ui->list_dataList->setEnabled(1);
    }
}

// 弹出右键菜单
void Widget_Data::on_list_dataList_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* pCurItem = ui->list_dataList->itemAt(pos);
    if (pCurItem == NULL) return;
    int nSelectIndex = ui->list_dataList->currentRow();
    if (nSelectIndex < 0) return;

    // 创建右键菜单
    QMenu* pMenu = new QMenu(this);
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
    QAction* pMenuButton1 = new QAction(QString::fromWCharArray(L"预览全图"), this);
    QAction* pMenuButton2 = new QAction(QString::fromWCharArray(L"预览文本"), this);
    QAction* pMenuButton3 = new QAction(QString::fromWCharArray(L"路径导出"), this);
    QAction* pMenuButton4 = new QAction(QString::fromWCharArray(L"单独导出"), this);
    connect(pMenuButton1, &QAction::triggered, this, [=]{RightMenuEvent(nSelectIndex, 1);});
    connect(pMenuButton2, &QAction::triggered, this, [=]{RightMenuEvent(nSelectIndex, 2);});
    connect(pMenuButton3, &QAction::triggered, this, [=]{RightMenuEvent(nSelectIndex, 3);});
    connect(pMenuButton4, &QAction::triggered, this, [=]{RightMenuEvent(nSelectIndex, 4);});
    pMenu->addAction(pMenuButton1);
    pMenu->addAction(pMenuButton2);
    pMenu->addAction(pMenuButton3);
    pMenu->addAction(pMenuButton4);


    QString strFormat = m_vDataList[nSelectIndex].patchPath.right(
                m_vDataList[nSelectIndex].patchPath.size() - m_vDataList[nSelectIndex].patchPath.lastIndexOf('.'));
    // 是否为图片
    if (!QString(".bmp.png.jpg.dds.tga").contains(strFormat, Qt::CaseInsensitive)) pMenuButton1->setEnabled(0);
    // 是否为文本
    if (!QString(".txt.ini.lua.xml.json.fx.psh.string").contains(strFormat, Qt::CaseInsensitive)) pMenuButton2->setEnabled(0);

    pMenu->exec(QCursor::pos());

    delete pMenuButton1;
    delete pMenuButton2;
    delete pMenuButton3;
    delete pMenuButton4;
    delete pMenu;
}

// 选择项发生变化时
void Widget_Data::on_list_dataList_currentRowChanged(int currentRow)
{
    if (m_pDialogPreview != NULL) m_pDialogPreview->Stop();
    if (m_vDataList.isEmpty()) return;
    emit SendInfo(m_vDataList[currentRow]);
    // 是否启用了预览图片
    if (bPreviewPicture)
    {
        QString strFormat = m_vDataList[currentRow].patchPath.right(m_vDataList[currentRow].patchPath.size() - m_vDataList[currentRow].patchPath.lastIndexOf('.'));
        // 是否为图片文件
        if (QString(".bmp.png.jpg.dds.tga").contains(strFormat, Qt::CaseInsensitive))
        {
            if (bPreviewPicture == 1) m_pDialogPreview->setImageMin(m_vDataList[currentRow], m_strDataPath);
            else if (bPreviewPicture == 2) m_pDialogPreview->setImageMinEX(m_vDataList[currentRow], m_strDataPath);
        }
    }
}
