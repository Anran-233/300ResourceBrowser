#ifndef DIALOG_PROGRESS_H
#define DIALOG_PROGRESS_H

#include "cpatch.h"
#include <QDialog>
#include <QThread>

class QFrame;
class QLabel;
class QProgressBar;

enum EProgressType { Pack, Unpack, Download };

// 处理进度条数据的线程
class CThreadProgress : public QThread
{
    Q_OBJECT
public:
    explicit CThreadProgress(QObject *parent = nullptr) : QThread(parent) { }
    ~CThreadProgress() { }

    void run(){
        PrintfData();
        return;
    }
    void Set(EProgressType eProgressType,int nProgressNum, int& nProgressState, QVector<SRealTimeData>& sRtDataList, bool& bTerminator){
        m_eProgressType = eProgressType;
        m_nNum = nProgressNum;
        m_pnState = &nProgressState;
        m_psRtDataList = &sRtDataList;
        m_pbTerminator = &bTerminator;
    }

private:
    EProgressType m_eProgressType;
    int m_nNum;
    int* m_pnState;
    QVector<SRealTimeData>* m_psRtDataList;
    bool* m_pbTerminator;

    void PrintfData();

signals:
    void SendUpdate(int nID, int nProgress, QString strInfo);
    void SendEnd(QString strMessage, QString strText);
};

// 进度条窗口UI的主线程
namespace Ui {
class Dialog_Progress;
}

class Dialog_Progress : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_Progress(QWidget *parent = nullptr);
    ~Dialog_Progress();

    int m_nProgressState = 0;   // 进度条状态：0 正常状态; 1 进度完成; -1 中途退出;
    void Start(EProgressType eProgressType, QVector<QString> strFileNameList, QVector<SRealTimeData>& sRtDataList, bool& bTerminator);

private:
    Ui::Dialog_Progress *ui;

    int m_nProgressNum = 5;
    QFrame* m_pFrame[5];
    QProgressBar* m_pProgressBar[5];
    QLabel* m_pLabelName[5];
    QLabel* m_pLabelInfo[5];
    CThreadProgress* m_pThread = NULL;

    void SetName(QVector<QString> strFileNameList);
    void UnpackMode(QVector<SRealTimeData>& sRtDataList, bool& bTerminator);
    void closeEvent(QCloseEvent *event) override;
};

#endif // DIALOG_PROGRESS_H
