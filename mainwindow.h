#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cpatch.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CTip;
class Data_main;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static MainWindow* Instance();
    void start();
    void start(const QString& strJmpFile);
    void start(int update, const QString& value);

signals:
    void started();
    void sigResize(const QSize& size);

public slots:
    void tip(const QString& strTip, int nTimeout = 1000);
    void updateInfo(const SDataInfo& sDataInfo);

private:
    static MainWindow* m_pClass;
    Ui::MainWindow *ui;
    CTip* m_tip = nullptr;
    Data_main* m_datamain = nullptr;

    void init();
    void initUI();
    void checkUpdate();
    void readResource(int mode, QString strJmpPath = {});
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // MAINWINDOW_H
