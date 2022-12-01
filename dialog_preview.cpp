#include "dialog_preview.h"
#include "ui_dialog_preview.h"
#include "ceventfilter.h"
#include "img_decode.h"

#include <QDebug>
#include <QDir>
#include <QMimeDatabase>
#include <QPainter>
#include <QWheelEvent>

#define MIN_SIZE_W  300 // 略缩图尺寸 宽
#define MIN_SIZE_H  300 // 略缩图尺寸 高
#define MIN_SIZE            QSize(MIN_SIZE_W, MIN_SIZE_H)
#define MIN_SIZE_IMG        QSize(MIN_SIZE_W - 20 * 2, MIN_SIZE_H - 20 * 2)

Dialog_Preview::Dialog_Preview(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Preview)
{
    ui->setupUi(this);
    if (parent != nullptr) m_pParent = parent;
    setAutoFillBackground(false);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // 如果设置了父级，则启用小图显示模式
    if (m_pParent != NULL)
    {
        setMinimumSize(MIN_SIZE);
        setMaximumSize(MIN_SIZE);

        setStyleSheet(QString::fromWCharArray(L"#Quit{font: 20px \"微软雅黑\";}"));
        ui->Quit->setMinimumSize(20, 20);
        ui->Quit->setMaximumSize(20, 20);
        ui->Quit->move(width() - 20, 0);

        ui->Image->setMinimumSize(MIN_SIZE_IMG);
        ui->Image->setMaximumSize(MIN_SIZE_IMG);
        ui->Image->move(20, 20);

        m_pThread = new CThreadImage(this);
        connect(m_pThread, &CThreadImage::SendImage, this, [=] (const QPixmap& pixImage, QString strImageType) {
            ui->Info->setText("(" + strImageType + ") " + QString::number(pixImage.width()) + "x" + QString::number(pixImage.height()));
            if ((pixImage.width() > (MIN_SIZE_W - 40)) || (pixImage.height() > (MIN_SIZE_H - 40)))
                ui->Image->setPixmap(pixImage.scaled(MIN_SIZE_W - 40, MIN_SIZE_H - 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            else
                ui->Image->setPixmap(pixImage);
            if (this->isVisible() == 0) this->setVisible(1);
            CheckImagePos();
        });

        installEventFilter(&sg_cEventFilter);
    }
}

Dialog_Preview::~Dialog_Preview()
{
    delete ui;
    if (m_pThread != NULL)
    {
        m_pThread->quit();
        m_pThread->wait();
        delete m_pThread;
    }
}

// 透明背景
void Dialog_Preview::paintEvent(QPaintEvent *)
{
    //Q_UNUSED(e);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 125));
}

void Dialog_Preview::setImageMax(SDataInfo sDataInfo, QString strDataPath)
{
    // 加载图片数据
    QByteArray&& img_data = CPatch::ReadDataFileOfData(sDataInfo, strDataPath);
    if (img_data.size() == 0) return;

    // 识别图片实际类型
    QString strImageType = QMimeDatabase().mimeTypeForData(img_data).name().split('/').at(1);

    // 读取图片
    QPixmap pixImage;
    if (strImageType.contains("dds", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(img_data, img_data.size(), CImgDecede::DDS);
        pixImage = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
    }
    else if (strImageType.contains("tga", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(img_data, img_data.size(), CImgDecede::TGA);
        pixImage = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
    }
    else pixImage.loadFromData(img_data);

    // 设置图片
    QString strInfo = "(" + strImageType + ") " + QString::number(pixImage.width()) + "x" + QString::number(pixImage.height());
    ui->Info->setText(strInfo);
    QSize szDialogSize(500, 500);
    if (pixImage.width() > (500 - 30 * 2)) szDialogSize.setWidth(pixImage.width() + 30 * 2);
    if (pixImage.height() > (500 - 30 * 2)) szDialogSize.setHeight(pixImage.height() + 30 * 2);
    setMinimumSize(szDialogSize);
    setMaximumSize(szDialogSize);

    setStyleSheet(QString::fromWCharArray(L"#Quit{font: 40px \"微软雅黑\";}"));
    ui->Quit->setMinimumSize(40, 40);
    ui->Quit->setMaximumSize(40, 40);
    ui->Quit->move(width() - 40, 0);

    ui->Image->setMinimumSize(szDialogSize - QSize(60, 60));
    ui->Image->setMaximumSize(szDialogSize - QSize(60, 60));
    ui->Image->move(30, 30);
    ui->Image->setPixmap(pixImage);

    if (this->isVisible() == 0) this->setVisible(1);
    CheckImagePos();
}

void Dialog_Preview::setImageMinEX(SDataInfo sDataInfo, QString strDataPath)
{
    // 加载图片数据
    QByteArray&& img_data = CPatch::ReadDataFileOfData(sDataInfo, strDataPath);
    if (img_data.size() == 0) return;

    // 识别图片实际类型
    QString strImageType = QMimeDatabase().mimeTypeForData(img_data).name().split('/').at(1);

    // 读取图片
    QPixmap pixImage;
    if (strImageType.contains("dds", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(img_data, img_data.size(), CImgDecede::DDS);
        pixImage = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
    }
    else if (strImageType.contains("tga", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(img_data, img_data.size(), CImgDecede::TGA);
        pixImage = QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888));
    }
    else pixImage.loadFromData(img_data);

    // 设置图片
    ui->Info->setText("(" + strImageType + ") " + QString::number(pixImage.width()) + "x" + QString::number(pixImage.height()));
    if ((pixImage.width() > (MIN_SIZE_W - 40)) || (pixImage.height() > (MIN_SIZE_H - 40)))
        ui->Image->setPixmap(pixImage.scaled(MIN_SIZE_W - 40, MIN_SIZE_H - 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        ui->Image->setPixmap(pixImage);
    if (this->isVisible() == 0) this->setVisible(1);
    CheckImagePos();
}

// 检查图片位置
void Dialog_Preview::CheckImagePos()
{
    // 是否被移出父级窗口边缘，未设置父级时不检查
    if (m_pParent != NULL)
    {
        if (x() < 0) move(0, y());
        if (y() < 0) move(x(), 0);
        if ((x() + width()) > m_pParent->width()) move(m_pParent->width() - width(), y());
        if ((y() + height()) > m_pParent->height()) move(x(), m_pParent->height() - height());
    }
}

// 点击【关闭按钮】
void Dialog_Preview::on_Quit_clicked()
{
    if (m_pParent != NULL)
        this->setVisible(0);
    else
        this->close();
}

// 鼠标【双击】事件
void Dialog_Preview::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        if (m_pParent != NULL)
            this->setVisible(0);
        else
            this->close();
    }
}

// 鼠标【按下】事件
void Dialog_Preview::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton)
        m_posStart = event->globalPos() - this->frameGeometry().topLeft();
}

// 鼠标【移动】事件
void Dialog_Preview::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons()==Qt::LeftButton)
    {
        this->move(event->globalPos() - m_posStart);
        CheckImagePos();
    }
}

void CThreadImage::ReadImage()
{
    // 加载图片数据
    QByteArray&& img_data = CPatch::ReadDataFileOfData(m_sDataInfo, m_strDataPath);
    if (img_data.size() == 0) return;

    // 识别图片实际类型
    QString strImageType = QMimeDatabase().mimeTypeForData(img_data).name().split('/').at(1);

    // 读取图片
    if (strImageType.contains("dds", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(img_data, img_data.size(), CImgDecede::DDS);
        emit SendImage(QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888)), strImageType);
    }
    else if (strImageType.contains("tga", Qt::CaseInsensitive)) {
        CImgDecede imgDecede(img_data, img_data.size(), CImgDecede::TGA);
        emit SendImage(QPixmap::fromImage(QImage(imgDecede.data(), imgDecede.w(), imgDecede.h(), QImage::Format_RGBA8888)), strImageType);
    }
    else {
        QPixmap pixImage;
        pixImage.loadFromData(img_data);
        emit SendImage(pixImage, strImageType);
    }
}
