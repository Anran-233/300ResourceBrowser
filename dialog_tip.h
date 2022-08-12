#ifndef DIALOG_TIP_H
#define DIALOG_TIP_H

#include <QDialog>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace Ui {
class Dialog_Tip;
}

class Dialog_Tip : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_Tip(QWidget *parent = nullptr);
    ~Dialog_Tip();

    void SetTip(QString strTip, int nTimeMS = 1000);

private:
    Ui::Dialog_Tip *ui;

    QTimer *m_pTimer = NULL;
    QGraphicsOpacityEffect *m_pOpacity = NULL;
    QPropertyAnimation *m_pAnimation = NULL;

protected:
    virtual void paintEvent(QPaintEvent *e) override;
};

#endif // DIALOG_TIP_H
