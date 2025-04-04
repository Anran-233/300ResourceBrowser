#include "form_bank.h"
#include "ui_form_bank.h"
#include "tool.h"
#include "config.h"

#include <QLibrary>
#include <QProgressDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>

#define qBank CReadBankDll::Instance()
class CReadBankDll
{
private:
    QLibrary m_dll;
public:
    CReadBankDll(){ m_dll.setFileName("readBank"); }
    ~CReadBankDll(){ if (m_dll.isLoaded()) m_dll.unload(); }
    static CReadBankDll& Instance(){
        static CReadBankDll bank;
        return bank;
    }
    bool init(){
        if (m_bValid) return true;
        if (m_dll.isLoaded()) m_dll.unload();
        if (m_dll.load()){
#define RESOLVE(f) if(!(f = (decltype(f))m_dll.resolve(#f))) return false;
            RESOLVE(SetBankVolume);
            RESOLVE(GetBankVolume);
            RESOLVE(StopBankSound);
            RESOLVE(IsBankPlaying);
            RESOLVE(ReadBankData);
            RESOLVE(ReleaseBank);
            RESOLVE(GetBankSoundSize);
            RESOLVE(GetBankSubSoundNameU);
            RESOLVE(GetBankSubSoundLength);
            RESOLVE(PlayBankSubSound);
            RESOLVE(OutBankSubSound);
#undef RESOLVE
            SetBankVolume(qConfig.nBankVolume);
            m_bValid = true;
            return true;
        }
        qDebug() << "bank error:" << m_dll.errorString();
        return false;
    }
    bool m_bValid = false;
    void(*SetBankVolume)(int)                   = nullptr;
    int (*GetBankVolume)()                      = nullptr;
    void(*StopBankSound)()                      = nullptr;
    int (*IsBankPlaying)()                      = nullptr;
    int (*ReadBankData)(const char*,int)        = nullptr;
    void(*ReleaseBank)(int)                     = nullptr;
    int (*GetBankSoundSize)(int)                = nullptr;
    const char*(*GetBankSubSoundNameU)(int,int) = nullptr;
    int (*GetBankSubSoundLength)(int,int)       = nullptr;
    void(*PlayBankSubSound)(int,int)            = nullptr;
    void(*OutBankSubSound)(int,int,const char*) = nullptr;
};

Form_bank::Form_bank(QWidget *parent) :
    ui(new Ui::Form_bank)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    ui->listWidget->installEventFilter(this);
    ui->spinBox->installEventFilter(this);

    connect(ui->button_stop, &QPushButton::clicked, this, []{ qBank.StopBankSound(); });
    connect(ui->button_play, &QPushButton::clicked, this, [this]{
        qBank.PlayBankSubSound(m_bank, ui->listWidget->currentRow()); // dll内部有越界检测
    });
    connect(ui->button_out, &QPushButton::clicked, this, [this]{
        int nIndex = ui->listWidget->currentRow();
        if (nIndex < 0) return;
        QString strFileName = QFileDialog::getSaveFileName(this,
                                                           u8"选择导出目录",
                                                           QString("%1/%2.wav")
                                                           .arg(qConfig.getAlonePath(), qBank.GetBankSubSoundNameU(m_bank, nIndex)),
                                                           "music file(*.wav)");
        if (strFileName.isEmpty()) return;
        // 创建进度条
        QProgressDialog *qProgressBar = new QProgressDialog(u8"正在导出音频...", nullptr, 0, 0, this, Qt::Dialog | Qt::FramelessWindowHint);
        qProgressBar->installEventFilter(CEventFilter::Instance());
        qProgressBar->setWindowModality(Qt::WindowModal);
        qProgressBar->setMinimumDuration(0);
        qProgressBar->show();
        // 创建线程
        CThread* thread = new CThread(this);
        connect(thread, &CThread::started, thread, [=]{
            qBank.OutBankSubSound(m_bank, nIndex, strFileName.toLocal8Bit().data());
            emit thread->finished();
        });
        connect(thread, &CThread::finished, this, [=]{ delete qProgressBar; });
        thread->start();
    });
    connect(ui->button_set, &QPushButton::clicked, this, [this]{
        if (ui->button_set->minimumWidth() == 100) {
            ui->button_set->setText(u8"确定");
            ui->button_set->setMinimumWidth(36);
            ui->spinBox->setValue(qBank.GetBankVolume());
            ui->spinBox->setVisible(true);
        }
        else {
            qConfig.set_nBankVolume(ui->spinBox->value());
            qBank.SetBankVolume(ui->spinBox->value());
            ui->spinBox->setVisible(false);
            ui->button_set->setText(u8"设置音量");
            ui->button_set->setMinimumWidth(100);
        }
    });
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){
        qBank.PlayBankSubSound(m_bank, ui->listWidget->row(item)); // dll内部有越界检测
    });
    if (parent) connect(parent, &QObject::destroyed, this, [=]{ delete this; });
}

Form_bank::~Form_bank()
{
    if (m_bank) qBank.ReleaseBank(m_bank);
    delete ui;
}

void Form_bank::load(QWidget *parent, const SDataInfo& sDataInfo)
{
    // 初始化dll
    if (!qBank.init()) {
        QMessageBox::critical(parent, u8"错误", u8"无法加载readBank.dll!");
        this->deleteLater();
        return;
    }

    // 创建进度条
    QProgressDialog* progressBar = new QProgressDialog(u8"正在加载...", nullptr, 0, 0, parent, Qt::Dialog | Qt::FramelessWindowHint);
    progressBar->installEventFilter(CEventFilter::Instance());
    progressBar->setWindowModality(Qt::WindowModal);
    progressBar->setMinimumDuration(0);
    progressBar->show();

    // 创建线程
    CThread* thread = new CThread(this);
    connect(thread, &CThread::started, thread, [=]{
        const QByteArray& data = CPatch::ReadData(sDataInfo); // 读取数据
        m_bank = qBank.ReadBankData(data.data(), data.size()); // 读取bank
        emit thread->finished();
    });
    connect(thread, &CThread::finished, this, [=]{
        ActionScope([=]{ delete progressBar; delete thread; });
        // 是否失败
        if (!m_bank) {
            progressBar->close();
            QMessageBox::critical(parent, u8"错误", QString(u8"无法读取%1!").arg(sDataInfo.patchPath.split('\\').back()));
            this->deleteLater();
            return;
        }
        // 初始化UI
        this->setWindowTitle(sDataInfo.patchPath.split('\\').back());
        ui->listWidget->clear();
        ui->spinBox->setVisible(false);
        ui->button_set->setMinimumWidth(100);
        ui->button_set->setText(u8"设置音量");
        int nSoundSize = qBank.GetBankSoundSize(m_bank);
        for (int i = 0; i < nSoundSize; i++) {
            QString strSoundName = qBank.GetBankSubSoundNameU(m_bank, i);
            int nSoundLength = qBank.GetBankSubSoundLength(m_bank, i);
            ui->listWidget->addItem(QDateTime::fromMSecsSinceEpoch(nSoundLength).toString("[mm:ss:zzz] ") + strSoundName);
        }
        // 显示界面
        show();
    });
    thread->start();
}

void Form_bank::StopBank()
{
    if (qBank.StopBankSound) qBank.StopBankSound();
}

bool Form_bank::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        if (((QKeyEvent *)event)->key() == Qt::Key_Enter) {
            if (obj == ui->spinBox) {
                ui->button_set->clicked();
                return true;
            }
            else if (obj == ui->listWidget) {
                ui->button_play->clicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
