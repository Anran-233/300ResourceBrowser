#include "loading.h"
#include "ui_loading.h"

#include <QMovie>
#include <QTimer>

Loading *qLoading = nullptr;

Loading::Loading(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Loading)
{
    ui->setupUi(this);

    QMovie *movie = new QMovie(":/icon/loading.gif");
    ui->img->setMovie(movie);
    movie->setSpeed(125);
    movie->start();

    m_labelValue = new QLabel(ui->img);
    m_labelValue->setAlignment(Qt::AlignCenter);
    m_labelValue->setStyleSheet(u8"font: 48px \"微软雅黑\"; font-weight: bold; color: #444;");
    QVBoxLayout *layout = new QVBoxLayout(ui->img);
    layout->addWidget(m_labelValue);
    ui->img->setLayout(layout);

    connect(ui->cancel, &QPushButton::clicked, this, &Loading::cancel);

    m_timer = new QTimer(this);
    m_timer->setInterval(200);
    connect(m_timer, &QTimer::timeout, this, &Loading::updateValue);

    resize(parent->size());
    setVisible(false);
}

Loading::~Loading()
{
    delete ui;
}

QObject *Loading::start(QString msg, bool bCancel, int nValue)
{
    setText(msg);
    setCancel(bCancel);
    setValue(nValue);
    updateValue();
    m_timer->start();
    if (!object) object = new QObject(this);
    setVisible(true);
    emit started();
    return object;
}

void Loading::stop()
{
    setVisible(false);
    m_timer->stop();
    if (object) {
        delete object;
        object = nullptr;
    }
    emit stoped();
}

void Loading::onResize(const QSize &size)
{
    resize(size);
}

void Loading::setCancel(const bool &bEnable)
{
    ui->cancel->setVisible(bEnable);
}

void Loading::setText(const QString &text)
{
    ui->info->setText(text);
    ui->info->setVisible(text.size());
}

void Loading::setValue(const int &value)
{
    if (this->value == value) return;
    this->value = value;
}

void Loading::updateValue()
{
    if (m_value == value) return;
    m_value = value;
    m_labelValue->setText(m_value < 0 ? "" : QString::number(m_value) + "%");
}
