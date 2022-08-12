#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "widget_hero.h"
#include "widget_data.h"
#include "widget_local.h"
#include "widget_net.h"
#include "dialog_set.h"
#include "cpatch.h"
#include "dialog_tip.h"
#include "form_reader.h"

#include <QMainWindow>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString m_strAppPath;           // Windows应用程序完整路径
    Widget_Hero m_wHero;            // 1.读取游戏目录资源
    Widget_Data m_wData;            // 2.读取单个data资源
    Widget_Local m_wLocal;          // 3.读取本地目录资源
    Widget_Net m_wNet;              // 4.浏览网络资源列表

    void Init();
    void OpenData(QString strDataPath);

private:
    Ui::MainWindow *ui;

    Dialog_Tip *m_pTip = NULL;

    void InitUI();
    void UpdateInfo(SDataInfo sDataInfo);
    void ReadEventFilter(int nEvent);
    void UpdateTip(QString strTip, int nTimeMS = 1000);
    virtual void resizeEvent(QResizeEvent *) override;

private slots:
    void on_Button_set_clicked();
    void on_Button_exportAll_clicked();
    void on_Button_lookup_clicked();
    void on_Button_clear_clicked();
    void on_logo_customContextMenuRequested(const QPoint &pos);
    void on_Button_log_clicked();
};
#endif // MAINWINDOW_H
