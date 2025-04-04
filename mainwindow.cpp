#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dialog_set.h"
#include "tool.h"
#include "config.h"
#include "data_main.h"
#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <loading.h>

MainWindow* MainWindow::m_pClass = nullptr;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    if (!m_pClass) m_pClass = this;
    setAttribute(Qt::WA_DeleteOnClose);
    qLoading = new Loading(this);
    connect(this, &MainWindow::sigResize, qLoading, &Loading::onResize);
    installEventFilter(this);
    init();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_pClass == this) m_pClass = nullptr;
    if (QDir().exists("BConfig/Temp")) QDir("BConfig/Temp").removeRecursively(); // 清理临时文件夹
}

MainWindow *MainWindow::Instance()
{
    return m_pClass;
}

// 启动主界面(正常启动)
void MainWindow::start()
{
    show();
    checkUpdate();
    tip(u8"欢迎使用 300资源浏览器 简易版！", 3000);
}

// 启动主界面(打开jmp文件)
void MainWindow::start(const QString &strJmpFile)
{
    if (!QFile().exists(strJmpFile)) {

    }
}

// 启动主界面(更新重启)
void MainWindow::start(int update, const QString &value)
{
    if (update == 1) {
        show();
        UpdateApplicationAssociation(value);
        QMessageBox::information(this, u8"提示", u8"程序更新成功！\n版本：v" APP_VERSION);
        tip(u8"欢迎使用 300资源浏览器 简易版！", 3000);
    }
    else if (update == -1) {
        show();
        QMessageBox::critical(this, u8"错误",
                              u8"程序更新失败！请手动覆盖程序文件进行更新！\n新版本程序所在目录：\n"
                              + QCoreApplication::applicationDirPath()
                              + "/BConfig/300ResourceBrowser.exe");
        tip(u8"欢迎使用 300资源浏览器 简易版！", 3000);
    }
    else start();
}

// 悬浮提示
void MainWindow::tip(const QString &strTip, int nTimeout)
{
    m_tip->tip(strTip, nTimeout);
}

// 更新信息显示
void MainWindow::updateInfo(const SDataInfo& sDataInfo)
{
    const bool list[4][5]{  // {文件路径,补丁路径,索引位置,压缩大小,文件大小}
        {0,0,0,0,0},    // 1.空数据(00000)
        {0,1,1,1,1},    // 1.jmp数据(01111)
        {1,1,0,0,1},    // 2.本地文件(11001)
        {0,1,0,0,0},    // 3.网络文件(01000)
    };
    if (sDataInfo.type < 0 || sDataInfo.type > 3) return;   // 未知数据
    const auto& enabled = list[sDataInfo.type];
    ui->line_filePath->setText(enabled[0] ? sDataInfo.filePath : "");
    ui->line_patchPath->setText(enabled[1] ? sDataInfo.patchPath : "");
    ui->line_index->setText(enabled[2] ? QString::number(sDataInfo.index) : "");
    ui->line_compLen->setText(enabled[3] ? QString::number(sDataInfo.compLen) : "");
    ui->line_fileLen->setText(enabled[4] ? QString::number(sDataInfo.fileLen) : "");
    ui->line_filePath->setEnabled(enabled[0]);
    ui->line_patchPath->setEnabled(enabled[1]);
    ui->line_index->setEnabled(enabled[2]);
    ui->line_compLen->setEnabled(enabled[3]);
    ui->line_fileLen->setEnabled(enabled[4]);
    ui->Button_openFilePath->setEnabled(enabled[0]);
    ui->Button_copyFilePath->setEnabled(enabled[0]);
    ui->Button_copyFileName->setEnabled(enabled[1]);
    ui->Button_copyPatchPath->setEnabled(enabled[1]);
}

// 初始化
void MainWindow::init()
{
    initUI();
    ui->label_home->setVisible(1);
    ui->textBrowser_log->setVisible(0);
    ui->content->addWidget(m_datamain = new Data_main(this));
    connect(m_datamain, &Data_main::tip, this, &MainWindow::tip);
    connect(m_datamain, &Data_main::updateInfo, this, &MainWindow::updateInfo);
    connect(this, &MainWindow::sigResize, m_datamain, &Data_main::onResize);

    // 读取资源按钮
    connect(ui->Button_viewHero,    &QPushButton::clicked, this, [=]{ readResource(1); });
    connect(ui->Button_viewData,    &QPushButton::clicked, this, [=]{ readResource(2); });
    connect(ui->Button_viewLocal,   &QPushButton::clicked, this, [=]{ readResource(3); });
    connect(ui->Button_viewNet,     &QPushButton::clicked, this, [=]{ readResource(4); });

    // 帮助按钮
    connect(ui->Button_help, &QPushButton::clicked, this, [=]{ QDesktopServices::openUrl(QUrl("file:BConfig/help.html")); });

    // 设置按钮
    connect(ui->Button_set, &QPushButton::clicked, this, [=]{ (new Dialog_Set(this))->show(); });

    // 资源信息快捷按钮
    connect(ui->Button_openFilePath, &QPushButton::clicked, this, [=]{
        QProcess::startDetached("explorer", QStringList() << "/select," << ui->line_filePath->text().replace('/','\\'));
    });
    connect(ui->Button_copyFilePath, &QPushButton::clicked, this, [=]{
        QApplication::clipboard()->setText(ui->line_filePath->text().replace('/', '\\'));
        tip(u8"已复制 文件路径：" + ui->line_filePath->text().replace('/', '\\'));
    });
    connect(ui->Button_copyFileName, &QPushButton::clicked, this, [=]{
        QApplication::clipboard()->setText(ui->line_patchPath->text().split('\\').back());
        tip(u8"已复制 文件名称：" + ui->line_patchPath->text().split('\\').back());
    });
    connect(ui->Button_copyPatchPath, &QPushButton::clicked, this, [=]{
        QApplication::clipboard()->setText(ui->line_patchPath->text());
        tip(u8"已复制 补丁路径：" + ui->line_patchPath->text());
    });

    // 查找
    ui->line_lookup->setEnabled(0);
    ui->Button_lookup->setEnabled(0);
    ui->Button_clear->setEnabled(0);
    connect(ui->line_lookup, &QLineEdit::returnPressed, ui->Button_lookup, [=]{ emit ui->Button_lookup->clicked(); });
    connect(ui->Button_lookup, &QPushButton::clicked, this, [=]{ m_datamain->lookup(ui->line_lookup->text()); });
    connect(ui->Button_clear, &QPushButton::clicked, this, [=]{ ui->line_lookup->clear(), m_datamain->lookup(); });

    // 右键单击logo显示标语
    connect(ui->logo, &QLabel::customContextMenuRequested, this, [=]{
        QApplication::clipboard()->setText("Lovely is justice!");
        tip("Lovely is justice!");
    });

    // 展开/关闭日志
    connect(ui->Button_log, &QPushButton::clicked, this, [=]{
        if (ui->label_home->isVisible()) ui->label_home->setVisible(false), ui->textBrowser_log->setVisible(true);
        else ui->label_home->setVisible(true), ui->textBrowser_log->setVisible(false);
    });

    // 悬浮提示
    m_tip = new CTip(this);
    connect(this, &MainWindow::sigResize, m_tip, &CTip::updateSize);
}

// 初始化界面
void MainWindow::initUI()
{
    ui->line_filePath->clear();
    ui->line_patchPath->clear();
    ui->line_index->clear();
    ui->line_compLen->clear();
    ui->line_fileLen->clear();

    ui->line_filePath->setEnabled(0);
    ui->line_patchPath->setEnabled(0);
    ui->line_index->setEnabled(0);
    ui->line_compLen->setEnabled(0);
    ui->line_fileLen->setEnabled(0);

    ui->line_lookup->clear();
    ui->line_lookup->setEnabled(1);
    ui->Button_lookup->setEnabled(1);
    ui->Button_clear->setEnabled(1);

    ui->Button_openFilePath->setEnabled(0);
    ui->Button_copyFilePath->setEnabled(0);
    ui->Button_copyFileName->setEnabled(0);
    ui->Button_copyPatchPath->setEnabled(0);
}

// 检查更新
void MainWindow::checkUpdate()
{
    static const qint64 nInterval[4]{ 0, 1 * 24 * 3600, 7 * 24 * 3600, 30 * 24 * 3600 };
    if (!qConfig.nUpdateInterval) return; // 不检查更新
    if ((time(0) - qConfig.strUpdateLast.toLongLong()) < (nInterval[qBound(0, qConfig.nUpdateInterval, 3)] * 24 * 3600)) return;
    CheckUpdate(this, qConfig.nUpdateSource, false);
}

// 读取资源
void MainWindow::readResource(int mode)
{
    // 1.读取游戏目录资源时检查目录内是否有数据包
    QStringList gameJmpFiles;
    if (mode == 1) {
        gameJmpFiles = CPatch::GetGameJmpFiles(qConfig.strGamePath);
        if (gameJmpFiles.isEmpty()) {
            QMessageBox::critical(this, u8"错误", u8"未检测到资源数据文件！");
            return;
        }
    }

    // 2.读取单个data数据时弹出文件选择对话框
    QString strJmpPath;
    if (mode == 2) {
        strJmpPath = QFileDialog::getOpenFileName(this, u8"请选择要读取的数据包", nullptr, "JMP File(*.jmp)");
        if (strJmpPath.isEmpty()) return;
    }

    // 初始化顶部界面
    initUI();

    // 初始化新界面
    ui->content->setCurrentIndex(1);
    if      (mode == 1) m_datamain->load(mode, gameJmpFiles);
    else if (mode == 2) m_datamain->load(mode, strJmpPath);
    else if (mode == 3) m_datamain->load(mode);
    else if (mode == 4) m_datamain->load(mode);
}

// 窗体大小变更
void MainWindow::resizeEvent(QResizeEvent *event)
{
    emit sigResize(event->size());
}

// 事件过滤器
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (qLoading->object) { // Loading界面禁用键盘事件
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) return false;
    }
    if (obj == this) { // MainWindow
        if (event->type() == QEvent::KeyRelease) { // 键盘事件 按键释放
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->modifiers() == Qt::ControlModifier) { // 按住 Ctrl
                // Ctrl + F 查找
                if (ui->line_lookup->isEnabled() && keyEvent->key() == Qt::Key_F) {
                    ui->line_lookup->setFocus();
                    return false;
                }
                // Ctrl + I 略缩图预览器 启用/禁用
                if (ui->content->currentIndex() && keyEvent->key() == Qt::Key_I) {
                    qConfig.set_bPreview(!qConfig.bPreview);
                    tip(qConfig.bPreview ? u8"启用 略缩图预览" : u8"禁用 略缩图预览");
                    return false;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
