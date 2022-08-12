#ifndef WIDGET_NET_H
#define WIDGET_NET_H

#include "cpatch.h"
#include "dialog_preview.h"

#include <QWidget>

namespace Ui {
class Widget_Net;
}

class Widget_Net : public QWidget
{
    Q_OBJECT

public:
    explicit Widget_Net(QWidget *parent = nullptr);
    ~Widget_Net();

    int bPreviewPicture = 1;                    // 是否预览图片

    void Init();
    void Lookup(QString strKeyword);
    void ExportAll();
    void Clear();

private:
    Ui::Widget_Net *ui;

    QVector<SDataInfo> m_vDataList;             // 存放data数据信息的容器
    Dialog_Preview* m_pDialogPreview = NULL;    // 图片预览器

    void RightMenuEvent(int nMenuButton);

signals:
    void SendInfo(SDataInfo sDataInfo);
    void SendTip(QString strTip, int nTimeMS = 1000);

private slots:
    void on_list_dataList_customContextMenuRequested(const QPoint &pos);
    void on_list_dataList_currentRowChanged(int currentRow);
};

#endif // WIDGET_NET_H
