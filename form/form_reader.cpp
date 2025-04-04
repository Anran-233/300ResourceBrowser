#include "form_reader.h"
#include "ui_form_reader.h"

#include "tool.h"
#include <QProgressDialog>
#include <qtextcodec.h>

Form_reader::Form_reader(QObject *parent) :
    ui(new Ui::Form_reader)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    if (parent) connect(parent, &QObject::destroyed, this, [=]{ delete this; }, Qt::DirectConnection);
}

Form_reader::~Form_reader()
{
    delete ui;
}

void Form_reader::load(QWidget *parent, const SDataInfo& sDataInfo)
{
    // 创建进度条
    QProgressDialog* progressBar = new QProgressDialog(u8"正在加载...", nullptr, 0, 0, parent, Qt::Dialog | Qt::FramelessWindowHint);
    progressBar->installEventFilter(CEventFilter::Instance());
    progressBar->setWindowModality(Qt::WindowModal);
    progressBar->setMinimumDuration(0);
    progressBar->show();

    // 创建线程
    CThread* thread = new CThread(this);
    connect(thread, &CThread::started, thread, [=]{ // 读取数据
        m_data = CPatch::ReadData(sDataInfo);
        emit thread->finished();
    });
    connect(thread, &CThread::finished, this, [=]{ // 加载文本
        this->setWindowTitle(sDataInfo.patchPath);
        ui->text->setPlainText(Tool::FromText(m_data));
        show();
        delete progressBar;
        delete thread;
    });
    thread->start();
}
