#include "dialog_tip.h"
#include "ui_dialog_tip.h"
#include "ceventfilter.h"

#include <QPainter>
#include <QTimer>

Dialog_Tip::Dialog_Tip(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Tip)
{
    ui->setupUi(this);
    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    move(0, 0);
    setMinimumWidth(parent->width());
    setMaximumWidth(parent->width());
    installEventFilter(&sg_cEventFilter);

    m_pOpacity = new QGraphicsOpacityEffect(this);
    m_pOpacity->setOpacity(1);
    ui->label_info->setGraphicsEffect(m_pOpacity);

    m_pAnimation = new QPropertyAnimation(m_pOpacity, "opacity");
    m_pAnimation->setDuration(500);
    m_pAnimation->setStartValue(1);
    m_pAnimation->setEndValue(0);
    connect(m_pAnimation, &QPropertyAnimation::finished, this, [&] {this->setVisible(0);});

    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, [&] {m_pAnimation->start(); m_pTimer->stop();});
}

Dialog_Tip::~Dialog_Tip()
{
    delete ui;
    if (m_pTimer != NULL)
    {
        m_pTimer->stop();
        delete m_pTimer;
    }
    if (m_pAnimation != NULL)
        delete m_pAnimation;
    if (m_pOpacity != NULL)
        delete m_pOpacity;
}

void Dialog_Tip::SetTip(QString strTip, int nTimeMS)
{
    ui->label_info->setText("   " + strTip + "   ");
    m_pAnimation->stop();
    m_pOpacity->setOpacity(1);
    this->show();
    m_pTimer->start(nTimeMS);
}

// 透明背景
void Dialog_Tip::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 0));
}
