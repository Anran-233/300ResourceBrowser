#include "form_image.h"
#include "ui_form_image.h"
#include "img_decode.h"

#include <QMimeDatabase>
#include <QPainter>
#include <QThread>
#include <QTimer>
#include <QWheelEvent>

Form_Image::Form_Image(QWidget *parent, bool enable, int size, int mode) :
    QDialog(parent),
    ui(new Ui::Form_Image),
    m_mini(true),
    m_enable(enable),
    m_mode(mode),
    m_size(size),
    m_thread(new QThread)
{
    ui->setupUi(this);
    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    size = qBound(300, size, qMin(parent->width(), parent->height()));
    resize(size, size);
    ui->image->resize(size - 40, size - 40);
    ui->close->setVisible(false);
    connect(this, &Form_Image::sigUpdateImage, this, &Form_Image::onUpdateImage);

    QObject *obj = new QObject;
    connect(m_thread, &QThread::destroyed, obj, [=]{delete obj;}, Qt::DirectConnection);
    obj->moveToThread(m_thread);
    m_thread->start();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, obj, [=]{
        size_t dateinfo = m_datainfo;
        if (!dateinfo) return;
        // 读取数据
        SDataInfo sDataInfo = *(SDataInfo*)dateinfo;
        const QByteArray &data = CPatch::ReadData(sDataInfo);
        if (dateinfo != m_datainfo) return;
        // 识别图片实际类型
        QString strImageType = QMimeDatabase().mimeTypeForData(data).name().split('/').at(1);
        if (dateinfo != m_datainfo) return;
        // 读取图片
        QPixmap img;
        if (strImageType.contains("dds", Qt::CaseInsensitive)) {
            CImgDecede imgDecede(data, data.size(), CImgDecede::DDS);
            img = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
        }
        else if (strImageType.contains("tga", Qt::CaseInsensitive)) {
            CImgDecede imgDecede(data, data.size(), CImgDecede::TGA);
            img = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
        }
        else img.loadFromData(data);
        if (dateinfo != m_datainfo) return;
        // 返回结果
        emit sigUpdateImage(QString("(%1) %2x%3").arg(strImageType).arg(img.width()).arg(img.height()), img);
    });
}

Form_Image::Form_Image(QWidget *parent, const SDataInfo &sDataInfo) : ui(new Ui::Form_Image)
{
    ui->setupUi(this);
    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    if (parent) connect(parent, &QObject::destroyed, this, [=]{ delete this; });
    connect(ui->close, &QPushButton::clicked, this, &QWidget::close);

    // 读取数据
    const QByteArray &data = CPatch::ReadData(sDataInfo);
    // 识别图片实际类型
    QString strImageType = QMimeDatabase().mimeTypeForData(data).name().split('/').at(1);
    // 读取图片
    QPixmap img;
    if (strImageType.contains("dds", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(data, data.size(), CImgDecede::DDS);
        img = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
    }
    else if (strImageType.contains("tga", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(data, data.size(), CImgDecede::TGA);
        img = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
    }
    else img.loadFromData(data);
    // 设置图片
    ui->info->setText(QString("(%1) %2x%3").arg(strImageType).arg(img.width()).arg(img.height()));
    this->resize(qMax(500, img.width() + 40), qMax(500, img.height() + 40));
    ui->close->move(this->width() - ui->close->width(), 0);
    ui->image->resize(img.size());
    ui->image->move((this->width() - img.width()) / 2, (this->height() - img.height()) / 2);
    ui->image->setPixmap(img);
}

Form_Image::~Form_Image()
{
    if (m_timer) m_timer->stop();
    if (m_datainfo) m_datainfo = 0;
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
    }
    delete ui;
}

void Form_Image::init()
{
    if (m_first) checkBounds();
}

void Form_Image::updateImage(SDataInfo *pDataInfo)
{
    if (!m_mini) return;
    if (m_mode) { // 1.直接加载
        if (!pDataInfo) return;
        if (m_datainfo == (size_t)pDataInfo) {
            if (!isVisible()) show();
            return;
        }
        else m_datainfo = (size_t)pDataInfo;
        // 读取数据
        const QByteArray &data = CPatch::ReadData(*pDataInfo);
        // 识别图片实际类型
        QString strImageType = QMimeDatabase().mimeTypeForData(data).name().split('/').at(1);
        // 读取图片
        QPixmap img;
        if (strImageType.contains("dds", Qt::CaseInsensitive)) {
            CImgDecede imgDecede(data, data.size(), CImgDecede::DDS);
            img = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
        }
        else if (strImageType.contains("tga", Qt::CaseInsensitive)) {
            CImgDecede imgDecede(data, data.size(), CImgDecede::TGA);
            img = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
        }
        else img.loadFromData(data);
        // 返回结果
        emit sigUpdateImage(QString("(%1) %2x%3").arg(strImageType).arg(img.width()).arg(img.height()), img);
    }
    else { // 0.缓冲加载
        if (pDataInfo) {
            if (m_datainfo == (size_t)pDataInfo) {
                if (!isVisible()) show();
            }
            else {
                m_datainfo = (size_t)pDataInfo;
                if (m_timer->isActive()) m_timer->stop();
                m_timer->start();
            }
        }
        else {
            m_datainfo = (size_t)pDataInfo;
            if (m_timer->isActive()) m_timer->stop();
        }
    }
}

void Form_Image::updateEnable(bool enable)
{
    m_enable = enable;
    if (!m_enable && isVisible()) clear();
}

void Form_Image::updateSize(int size)
{
    m_size = size;
    checkBounds();
}

void Form_Image::updateMode(int mode)
{
    m_mode = mode;
}

void Form_Image::clear()
{
    if (!m_mini) return;
    m_datainfo = 0;
    m_timer->stop();
    close();
}

void Form_Image::checkBounds()
{
    if (m_mini) {
        if (m_size != width()) {
            int size = qBound(300, m_size, qMin(parentWidget()->width(), parentWidget()->height()));
            resize(size, size);
            ui->image->resize(size - 40, size - 40);
            if (m_img.width() <= ui->image->width() && m_img.height() <= ui->image->height()) ui->image->setPixmap(m_img);
            else if (m_img.width() >= m_img.height()) ui->image->setPixmap(m_img.scaledToWidth(ui->image->width(), Qt::SmoothTransformation));
            else ui->image->setPixmap(m_img.scaledToHeight(ui->image->height(), Qt::SmoothTransformation));
        }
        QPoint pos = m_first
                ? QPoint(parentWidget()->width() - width(), parentWidget()->height() - height())
                : QPoint(qBound(0, x(), parentWidget()->width() - width()), qBound(0, y(), parentWidget()->height() - height()));
        if (pos != this->pos()) move(pos);
    }
}

void Form_Image::paintEvent(QPaintEvent *)
{
    QPainter(this).fillRect(rect(), QColor(0, 0, 0, 125));
}

void Form_Image::mouseDoubleClickEvent(QMouseEvent *)
{
    close();
}

void Form_Image::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) m_movebeg = event->globalPos() - this->frameGeometry().topLeft();
}

void Form_Image::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->move(event->globalPos() - m_movebeg);
        checkBounds();
    }
}

void Form_Image::onUpdateImage(QString info, QPixmap img)
{
    if (!m_mode && !m_datainfo) return;
    if (m_first) m_first = false;
    ui->info->setText(info);
    std::swap(m_img, img);
    if (m_img.width() <= ui->image->width() && m_img.height() <= ui->image->height()) ui->image->setPixmap(m_img);
    else if (m_img.width() >= m_img.height()) ui->image->setPixmap(m_img.scaledToWidth(ui->image->width(), Qt::SmoothTransformation));
    else ui->image->setPixmap(m_img.scaledToHeight(ui->image->height(), Qt::SmoothTransformation));
    if (!isVisible()) {
        show();
        checkBounds();
    }
}
