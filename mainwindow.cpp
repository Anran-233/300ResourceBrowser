#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "windows.h"

#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    m_pTip = new Dialog_Tip(this);
}

MainWindow::~MainWindow()
{
    // 清理其他窗口
    while (g_sets.isEmpty()) ((Form_reader*)(*g_sets.begin()))->close();

    delete ui;

    if (m_pTip != NULL) delete m_pTip;

    // 清理临时文件夹
    if (QDir().exists("BConfig/Temp"))
    {
        QDir tempDir("BConfig/Temp");
        tempDir.removeRecursively();
    }
}

// 初始化
void MainWindow::Init()
{
    ui->label_home->setVisible(1);
    ui->textBrowser_log->setVisible(0);

    ui->content->addWidget(&m_wHero);
    ui->content->addWidget(&m_wData);
    ui->content->addWidget(&m_wLocal);
    ui->content->addWidget(&m_wNet);

    connect(&m_wHero, &Widget_Hero::SendInfo, this, &MainWindow::UpdateInfo);
    connect(&m_wData, &Widget_Data::SendInfo, this, &MainWindow::UpdateInfo);
    connect(&m_wLocal, &Widget_Local::SendInfo, this, &MainWindow::UpdateInfo);
    connect(&m_wNet, &Widget_Net::SendInfo, this, &MainWindow::UpdateInfo);

    connect(&m_wHero, &Widget_Hero::SendTip, this, &MainWindow::UpdateTip);
    connect(&m_wData, &Widget_Data::SendTip, this, &MainWindow::UpdateTip);
    connect(&m_wLocal, &Widget_Local::SendTip, this, &MainWindow::UpdateTip);
    connect(&m_wNet, &Widget_Net::SendTip, this, &MainWindow::UpdateTip);

    connect(ui->Button_viewHero, &QPushButton::clicked, this, [=]{ReadEventFilter(1);});
    connect(ui->Button_viewData, &QPushButton::clicked, this, [=]{ReadEventFilter(2);});
    connect(ui->Button_viewLocal, &QPushButton::clicked, this, [=]{ReadEventFilter(3);});
    connect(ui->Button_viewNet, &QPushButton::clicked, this, [=]{ReadEventFilter(4);});

    InitUI();
    ui->line_lookup->setEnabled(0);
    ui->Button_lookup->setEnabled(0);
    ui->Button_clear->setEnabled(0);
    ui->Button_exportAll->setEnabled(0);

    connect(ui->line_lookup, &QLineEdit::returnPressed, this, &MainWindow::on_Button_lookup_clicked);

    connect(ui->Button_openFilePath, &QPushButton::clicked, this, [&]{
        WinExec(QString("explorer.exe /select,\"" + ui->line_filePath->text().replace('/','\\') + "\"").toLocal8Bit(), SW_SHOWDEFAULT);
        // QProcess调用中文参数有BUG，已弃用
        // QProcess::startDetached("explorer.exe /select,\"" + ui->line_filePath->text().replace('/','\\') + "\"");
    });
    connect(ui->Button_copyFilePath, &QPushButton::clicked, this, [&]{
        QApplication::clipboard()->setText(ui->line_filePath->text().replace('/', '\\'));
        UpdateTip(QString::fromWCharArray(L"已复制 文件路径：") + ui->line_filePath->text().replace('/', '\\'));
    });
    connect(ui->Button_copyFileName, &QPushButton::clicked, this, [&]{
        QApplication::clipboard()->setText(ui->line_patchPath->text().split('\\').back());
        UpdateTip(QString::fromWCharArray(L"已复制 文件名称：") + ui->line_patchPath->text().split('\\').back());
    });
    connect(ui->Button_copyPatchPath, &QPushButton::clicked, this, [&]{
        QApplication::clipboard()->setText(ui->line_patchPath->text());
        UpdateTip(QString::fromWCharArray(L"已复制 补丁路径：") + ui->line_patchPath->text());
    });

    UpdateTip(QString::fromWCharArray(L"欢迎使用 300资源浏览器 简易版！"), 3000);
}

// 直接打开单个Data文件
void MainWindow::OpenData(QString strDataPath)
{
    InitUI();
    ui->Button_exportAll->setText(QString::fromWCharArray(L"全部导出"));
    ui->content->setCurrentIndex(2);
    m_wData.Init(strDataPath.replace('\\', '/'));
}

// 初始化界面
void MainWindow::InitUI()
{
    ui->line_lookup->setText("");
    ui->line_filePath->setText("");
    ui->line_patchPath->setText("");
    ui->line_index->setText("");
    ui->line_compLen->setText("");
    ui->line_fileLen->setText("");

    ui->line_filePath->setEnabled(0);
    ui->line_patchPath->setEnabled(0);
    ui->line_index->setEnabled(0);
    ui->line_compLen->setEnabled(0);
    ui->line_fileLen->setEnabled(0);

    ui->line_lookup->setEnabled(1);
    ui->Button_lookup->setEnabled(1);
    ui->Button_clear->setEnabled(1);
    ui->Button_exportAll->setEnabled(1);

    ui->Button_openFilePath->setEnabled(0);
    ui->Button_copyFilePath->setEnabled(0);
    ui->Button_copyFileName->setEnabled(0);
    ui->Button_copyPatchPath->setEnabled(0);
}

// 更新信息显示
void MainWindow::UpdateInfo(SDataInfo sDataInfo)
{
    ui->line_filePath->setText(sDataInfo.filePath);
    ui->line_patchPath->setText(sDataInfo.patchPath);
    ui->line_index->setText(sDataInfo.index == 0 ? "" : QString::number(sDataInfo.index));
    ui->line_compLen->setText(sDataInfo.compLen == 0 ? "" : QString::number(sDataInfo.compLen));
    ui->line_fileLen->setText(sDataInfo.fileLen == 0 ? "" : QString::number(sDataInfo.fileLen));

    if (sDataInfo.type == 1)         // 1.正常data数据(01111)
    {
        ui->line_filePath->setEnabled(0);
        ui->line_patchPath->setEnabled(1);
        ui->line_index->setEnabled(1);
        ui->line_compLen->setEnabled(1);
        ui->line_fileLen->setEnabled(1);
        ui->Button_openFilePath->setEnabled(0);
        ui->Button_copyFilePath->setEnabled(0);
        ui->Button_copyFileName->setEnabled(1);
        ui->Button_copyPatchPath->setEnabled(1);
    }
    else if (sDataInfo.type == 2)    // 2.本地文件(11001)
    {
        ui->line_filePath->setEnabled(1);
        ui->line_patchPath->setEnabled(1);
        ui->line_index->setEnabled(0);
        ui->line_compLen->setEnabled(0);
        ui->line_fileLen->setEnabled(1);
        ui->Button_openFilePath->setEnabled(1);
        ui->Button_copyFilePath->setEnabled(1);
        ui->Button_copyFileName->setEnabled(1);
        ui->Button_copyPatchPath->setEnabled(1);
    }
    else if (sDataInfo.type == 3)    // 3.网络文件(01000)
    {
        ui->line_filePath->setEnabled(0);
        ui->line_patchPath->setEnabled(1);
        ui->line_index->setEnabled(0);
        ui->line_compLen->setEnabled(0);
        ui->line_fileLen->setEnabled(0);
        ui->Button_openFilePath->setEnabled(0);
        ui->Button_copyFilePath->setEnabled(0);
        ui->Button_copyFileName->setEnabled(1);
        ui->Button_copyPatchPath->setEnabled(1);
    }
}

// 【读取】按钮事件过滤器
void MainWindow::ReadEventFilter(int nEvent)
{
    // 读取单个data数据时弹出文件选择对话框
    QString strDataPath;
    if (nEvent == 2)
    {
        strDataPath = QFileDialog::getOpenFileName(this, QString::fromWCharArray(L"请选择要读取的数据包"), nullptr,"JMP File(*.jmp)");
        if (strDataPath.isNull()) return;
    }

    // 清除当前页面
    int nLowIndex = ui->content->currentIndex();
    switch (nLowIndex) {
    case 0: break;
    case 1: m_wHero.Clear();break;
    case 2: m_wData.Clear();break;
    case 3: m_wLocal.Clear();break;
    case 4: m_wNet.Clear();break;
    }

    // 初始化顶部界面
    InitUI();

    // 设置导出按钮文本
    switch (nEvent) {
    case 0: ui->Button_exportAll->setText(QString::fromWCharArray(L"全部导出"));break;
    case 1: ui->Button_exportAll->setText(QString::fromWCharArray(L"全部导出"));break;
    case 2: ui->Button_exportAll->setText(QString::fromWCharArray(L"全部导出"));break;
    case 3: ui->Button_exportAll->setText(QString::fromWCharArray(L"全部打包"));break;
    case 4: ui->Button_exportAll->setText(QString::fromWCharArray(L"全部下载"));break;
    }

    // 初始化新页面
    ui->content->setCurrentIndex(nEvent);
    switch (nEvent) {
    case 0: break;
    case 1: m_wHero.Init();break;
    case 2: m_wData.Init(strDataPath);break;
    case 3: m_wLocal.Init();break;
    case 4: m_wNet.Init();break;
    }
}

// 更新提示气泡
void MainWindow::UpdateTip(QString strTip, int nTimeMS)
{
    m_pTip->SetTip(strTip, nTimeMS);
}

// 更新提示气泡框大小
void MainWindow::resizeEvent(QResizeEvent *)
{
    if (m_pTip != NULL)
    {
        m_pTip->setMinimumWidth(this->width());
        m_pTip->setMaximumWidth(this->width());
    }
}

// 点击【设置】按钮
void MainWindow::on_Button_set_clicked()
{
    Dialog_Set *pDSet = new Dialog_Set(m_strAppPath, this);
    connect(pDSet, &Dialog_Set::SendUpdateSet, this, [=] {
        int nPreviewMode = GetPrivateProfileInt(L"配置", L"预览图片", 1, L"BConfig/Config.ini");
        int nIndex = ui->content->currentIndex();
        switch (nIndex) {
        case 0: break;
        case 1: m_wHero.bPreviewPicture = nPreviewMode;break;
        case 2: m_wData.bPreviewPicture = nPreviewMode;break;
        case 3: m_wLocal.bPreviewPicture = nPreviewMode;break;
        case 4: m_wNet.bPreviewPicture = nPreviewMode;break;
        }
    });
    pDSet->setModal(true);
    pDSet->show();
}

// 点击【全部导出】按钮
void MainWindow::on_Button_exportAll_clicked()
{
    int nIndex = ui->content->currentIndex();
    switch (nIndex) {
    case 0: break;
    case 1: m_wHero.ExportAll();break;
    case 2: m_wData.ExportAll();break;
    case 3: m_wLocal.ExportAll();break;
    case 4: m_wNet.ExportAll();break;
    }
}

// 点击【查找】按钮
void MainWindow::on_Button_lookup_clicked()
{
    int nIndex = ui->content->currentIndex();
    switch (nIndex) {
    case 0: break;
    case 1: m_wHero.Lookup(ui->line_lookup->text());break;
    case 2: m_wData.Lookup(ui->line_lookup->text());break;
    case 3: m_wLocal.Lookup(ui->line_lookup->text());break;
    case 4: m_wNet.Lookup(ui->line_lookup->text());break;
    }
}

// 点击【清除】按钮
void MainWindow::on_Button_clear_clicked()
{
    int nIndex = ui->content->currentIndex();
    switch (nIndex) {
    case 0: break;
    case 1: m_wHero.Lookup({127});break;
    case 2: m_wData.Lookup({127});break;
    case 3: m_wLocal.Lookup({127});break;
    case 4: m_wNet.Lookup({127});break;
    }
    ui->line_lookup->setText("");
}

// logo的右键菜单
void MainWindow::on_logo_customContextMenuRequested(const QPoint &)
{
    if (!ui->logo->geometry().contains(this->mapFromGlobal(QCursor::pos()))) return;
    // 创建右键菜单
    QMenu* pMenu = new QMenu(this);
    pMenu->setStyleSheet("QMenu::item {padding:3px 7px;} QMenu::item:selected {background-color: #bbb;}");
    QAction* pMenuButton = new QAction(QString::fromWCharArray(L"获取开源代码"), this);
    connect(pMenuButton, &QAction::triggered, this, [&] {
        if (QFile::exists("300ResourceBrowser.zip"))
        {
            if (!QFile::remove("300ResourceBrowser.zip"))
            {
                QFile::setPermissions("300ResourceBrowser.zip", QFileDevice::ReadOther | QFileDevice::WriteOther);
                QFile::remove("300ResourceBrowser.zip");
            }
        }
        if (QFile::copy(":/300ResourceBrowser.zip", "300ResourceBrowser.zip"))
            QMessageBox::about(this, QString::fromWCharArray(L"提示"), QString::fromWCharArray(L"开源代码 (300ResourceBrowser.zip)\n文件已放置在应用程序目录中。"));
        else
            QMessageBox::warning(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"复制文件失败 (300ResourceBrowser.zip)！\n请手动删除应用程序目录下同名文件！"));
    });
    pMenu->addAction(pMenuButton);
    pMenu->exec(QCursor::pos());

    delete pMenuButton;
    delete pMenu;
}

// 单击【更新日志】按钮
void MainWindow::on_Button_log_clicked()
{
    ui->label_home->setVisible(!ui->label_home->isVisible());
    ui->textBrowser_log->setVisible(!ui->textBrowser_log->isVisible());
}
