#ifndef DIALOG_PREVIEW_H
#define DIALOG_PREVIEW_H

#include "cpatch.h"

#include <QDialog>
#include <QThread>
#include <QProcess>
#include <QWheelEvent>

#define DISPLAY_MAX     1   // 按原图尺寸显示图片
#define DISPLAY_MIN     0   // 按小图尺寸显示图片
#define POS_NULL        QPoint(0, 0)    // 原图尺寸时可用此空值

enum EDisplayMode { MIN, MAX }; // 图片显示模式：MIN 小图模式; MAX 原图模式;

// 处理线程
class CThreadImage : public QThread
{
    Q_OBJECT
public:
    explicit CThreadImage(QObject *parent = nullptr) : QThread(parent) { }
    ~CThreadImage() { }

    void run(){
        for (int i = 0; (i < 50) && (m_bRun); i++) msleep(1);
        if (!m_bRun) return;
        ReadImage();
        return;
    }
    void Stop(){
        m_bRun = false;
        if (m_pExe != NULL) m_pExe->kill();
    }
    void Set(SDataInfo sDataInfo, QString strDataPath){
        m_bRun = true;
        m_sDataInfo = sDataInfo;
        m_strDataPath = strDataPath;
    }

private:
    bool m_bRun = false;
    SDataInfo m_sDataInfo;
    QString m_strDataPath;
    QProcess* m_pExe = NULL;

    void ReadImage();

signals:
    void SendImage(const QPixmap& imgImage, QString strImageType);
};

namespace Ui {
class Dialog_Preview;
}

class Dialog_Preview : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_Preview(QWidget *parent = nullptr);
    ~Dialog_Preview();

    void setImageMax(SDataInfo sDataInfo, QString strDataPath);
    void setImageMinEX(SDataInfo sDataInfo, QString strDataPath);
    void setImageMin(SDataInfo sDataInfo, QString strDataPath){
        m_pThread->Set(sDataInfo, strDataPath);
        m_pThread->start();
    }
    void Stop(){
        if (m_pThread != NULL)
        {
            m_pThread->Stop();
            m_pThread->quit();
            m_pThread->wait();
        }
    }
    void CheckImagePos();

private slots:
    void on_Quit_clicked();

private:
    Ui::Dialog_Preview *ui;
    QWidget* m_pParent = NULL;
    QPoint m_posStart = POS_NULL;
    CThreadImage* m_pThread = NULL;

protected:
    virtual void paintEvent(QPaintEvent *e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent *) override;
    virtual void mouseMoveEvent(QMouseEvent *) override;
};

#endif // DIALOG_PREVIEW_H
