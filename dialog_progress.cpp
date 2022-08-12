#include "dialog_progress.h"
#include "ui_dialog_progress.h"
#include "ceventfilter.h"

#include "windows.h"
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QReadWriteLock>
#include <QWheelEvent>

Dialog_Progress::Dialog_Progress(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Progress)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    m_pFrame[0] = ui->frame_0;
    m_pFrame[1] = ui->frame_1;
    m_pFrame[2] = ui->frame_2;
    m_pFrame[3] = ui->frame_3;
    m_pFrame[4] = ui->frame_4;
    m_pLabelName[0] = ui->label_name_0;
    m_pLabelName[1] = ui->label_name_1;
    m_pLabelName[2] = ui->label_name_2;
    m_pLabelName[3] = ui->label_name_3;
    m_pLabelName[4] = ui->label_name_4;
    m_pLabelInfo[0] = ui->label_info_0;
    m_pLabelInfo[1] = ui->label_info_1;
    m_pLabelInfo[2] = ui->label_info_2;
    m_pLabelInfo[3] = ui->label_info_3;
    m_pLabelInfo[4] = ui->label_info_4;
    m_pProgressBar[0] = ui->progressBar_0;
    m_pProgressBar[1] = ui->progressBar_1;
    m_pProgressBar[2] = ui->progressBar_2;
    m_pProgressBar[3] = ui->progressBar_3;
    m_pProgressBar[4] = ui->progressBar_4;

    connect(ui->Button_close, &QPushButton::clicked, this, [=] {this->close();});

    installEventFilter(&sg_cEventFilter);
}

Dialog_Progress::~Dialog_Progress()
{
    delete ui;

    m_pThread->quit();
    m_pThread->wait();
    delete [] m_pThread;
}

// 进度开始
void Dialog_Progress::Start(EProgressType eProgressType, QVector<QString> strFileNameList, QVector<SRealTimeData> &sRtDataList, bool &bTerminator)
{
    SetName(strFileNameList);
    // 初始化数据处理线程
    m_pThread = new CThreadProgress();
    connect(m_pThread, &CThreadProgress::SendUpdate, this, [&] (int nID, int nProgress, QString strInfo) {
        m_pProgressBar[nID]->setValue(nProgress);
        m_pLabelInfo[nID]->setText(strInfo);
    });
    connect(m_pThread, &CThreadProgress::SendEnd, this, [&] (QString strMessage, QString strText) {
        if (m_nProgressState == 1) QMessageBox::about(this, QString::fromWCharArray(L"提示"), strMessage);
        ui->Button_close->setText(strText);
    });

    // 设置线程参数
    m_pThread->Set(eProgressType, m_nProgressNum, m_nProgressState, sRtDataList, bTerminator);

    // 启动线程
    m_pThread->start();
}

// 设置启用的进度条数量（1~5）以及名字
void Dialog_Progress::SetName(QVector<QString> strFileNameList)
{
    int nProgressNum = strFileNameList.size();
    for (int i = 0; i < 5; i++)
    {
        if (i < nProgressNum)
        {
            m_pLabelName[i]->setText(strFileNameList[i]);
            m_pLabelInfo[i]->setText(QString::fromWCharArray(L"正在等待..."));
            m_pProgressBar[i]->setValue(0);
            m_pFrame[i]->setVisible(1);
        }
        else
        {
            m_pLabelName[i]->setText("");
            m_pLabelInfo[i]->setText("");
            m_pProgressBar[i]->setValue(0);
            m_pFrame[i]->setVisible(0);
        }
    }
    this->setMinimumSize(600, nProgressNum * 60 + 40);
    this->setMaximumSize(600, nProgressNum * 60 + 40);
    ui->Button_close->setText(QString::fromWCharArray(L"取消"));
    m_nProgressNum = nProgressNum;
}

// 重写【退出】事件
void Dialog_Progress::closeEvent(QCloseEvent *event)
{
    if (m_nProgressState == 1)
        event->accept();    // 正常退出
    else if (m_nProgressState == -1)
        event->accept();    // 中途强制退出
    else if (m_nProgressState == 0)
    {
        if (QMessageBox::Yes == QMessageBox::information(this, QString::fromWCharArray(L"警告"), QString::fromWCharArray(L"当前正在处理数据中，\n是否要中止操作？"), QMessageBox::Yes|QMessageBox::No))
            m_nProgressState = -1;  // 置为中途强制退出状态
        event->ignore();        // 取消本次退出操作
    }
}

void CThreadProgress::PrintfData()
{
    QReadWriteLock qReadWriteLock;
    int nThOverNum = 0;
    bool bRun = true;
    while (bRun)
    {
        // 检测所有线程是否结束
        if (nThOverNum < m_nNum)
        {
            if (*m_pnState == -1)
            {
                // 操作中止，置零终止符，终止线程和刷新信息
                qReadWriteLock.lockForWrite();
                *m_pbTerminator = false;
                qReadWriteLock.unlock();
                bRun = false;
            }
            else
            {
                // 控制打印信息频率
                for (int i = 0; (i < 10) && (*m_pnState == 0) && (*m_pbTerminator); i++)
                    Sleep(100);
            }
        }
        else
        {
            // 进度已完成，正常退出
            *m_pnState = 1;
            bRun = false;
        }

        // 刷新信息
        qReadWriteLock.lockForRead();
        QVector<SRealTimeData> tempRtDataList(*m_psRtDataList);
        qReadWriteLock.unlock();
        nThOverNum = 0;
        for (int i = 0; i < m_nNum; i++)
        {
            int nProgress = 0;
            QString strInfo("");
            if (m_eProgressType == EProgressType::Pack)
                nThOverNum += CPatch::PackToDataInfo(tempRtDataList[i], nProgress, strInfo);
            else if (m_eProgressType == EProgressType::Unpack)
                nThOverNum += CPatch::UnpackToDataInfo(tempRtDataList[i], nProgress, strInfo);
            else if (m_eProgressType == EProgressType::Download)
                nThOverNum += CPatch::DownloadToDataInfo(tempRtDataList[i], nProgress, strInfo);
            emit SendUpdate(i, nProgress, strInfo);
        }
    }
    if (*m_pnState == 1)
    {
        if (m_eProgressType == EProgressType::Pack)
            emit SendEnd(QString::fromWCharArray(L"全部打包完毕！"), QString::fromWCharArray(L"打包已完成,关闭进度条"));
        else if (m_eProgressType == EProgressType::Unpack)
            emit SendEnd(QString::fromWCharArray(L"全部导出完毕！"), QString::fromWCharArray(L"导出已完成,关闭进度条"));
        else if (m_eProgressType == EProgressType::Download)
            emit SendEnd(QString::fromWCharArray(L"全部下载完毕！"), QString::fromWCharArray(L"下载已完成,关闭进度条"));
    }
    else if (*m_pnState == -1)
        emit SendEnd(NULL, QString::fromWCharArray(L"操作已中止,关闭进度条"));
}

