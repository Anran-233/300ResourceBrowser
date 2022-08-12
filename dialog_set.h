#ifndef DIALOG_SET_H
#define DIALOG_SET_H

#include <QDialog>

namespace Ui {
class Dialog_Set;
}

class Dialog_Set : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_Set(QString strAppPath, QWidget *parent = nullptr);
    ~Dialog_Set();

private:
    Ui::Dialog_Set *ui;

    QString m_strAppPath;

    void Init();

private slots:
    void on_Button_OUT_clicked();
    void on_Button_OK_clicked();
    void on_Button_find_clicked();
    void on_Button_gamePath_clicked();
    void on_Button_exportPath_clicked();
    void on_Button_alone_clicked();

    void on_Button_fileOpen_clicked();

signals:
    void SendUpdateSet();
};

#endif // DIALOG_SET_H
