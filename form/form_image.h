#ifndef FORM_IMAGE_H
#define FORM_IMAGE_H

#include "cpatch.h"
#include <QDialog>

class ImageThread;

namespace Ui {
class Form_Image;
}

class Form_Image : public QDialog
{
    Q_OBJECT
public:
    /// minsize
    Form_Image(QWidget *parent, bool enable, int size, int mode);
    /// maxsize
    Form_Image(QWidget *parent, const SDataInfo& sDataInfo);
    ~Form_Image();
    void init();
    void updateImage(SDataInfo* pDataInfo);
    void updateEnable(bool enable);
    void updateSize(int size);
    void updateMode(int mode);
    void clear();

signals:
    void sigUpdateImage(QString info, QPixmap img);

public slots:
    void checkBounds();

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void mouseDoubleClickEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent *) override;
    virtual void mouseMoveEvent(QMouseEvent *) override;

private slots:
    void onUpdateImage(QString info, QPixmap img);

private:
    Ui::Form_Image *ui;
    QPoint m_movebeg{0,0};
    bool m_mini = false;
    bool m_enable = true;
    bool m_first = true;
    int m_mode = 0; // 0.缓冲加载; 1.直接加载;
    int m_size = 300;
    std::atomic_size_t m_datainfo{0};
    QThread *m_thread{nullptr};
    QTimer *m_timer{nullptr};
    QPixmap m_img;
};

#endif // FORM_IMAGE_H
