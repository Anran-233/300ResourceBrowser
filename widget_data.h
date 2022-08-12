#ifndef WIDGET_DATA_H
#define WIDGET_DATA_H

#include "cpatch.h"
#include "dialog_preview.h"

#include <QWidget>

namespace Ui {
class Widget_Data;
}

class Widget_Data : public QWidget
{
    Q_OBJECT

public:
    explicit Widget_Data(QWidget *parent = nullptr);
    ~Widget_Data();

    int bPreviewPicture = 1;                    // 是否预览图片

    void Init(QString strDataPath);
    void Lookup(QString strKeyword);
    void ExportAll();
    void Clear();

private:
    Ui::Widget_Data *ui;

    QVector<SDataInfo> m_vDataList;             // 存放data数据信息的容器
    QString m_strDataPath;                      // data文件路径
    Dialog_Preview* m_pDialogPreview = NULL;    // 图片预览器

    void RightMenuEvent(int nSelectIndex, int nMenuButton);
    // 窗口尺寸变动时，检测图片预览器是否超出边界
    virtual void resizeEvent(QResizeEvent *) override {
        if (m_pDialogPreview == NULL) return;
        if (m_pDialogPreview->isVisible() == 0) return;
        m_pDialogPreview->CheckImagePos();
    }

signals:
    void SendInfo(SDataInfo sDataInfo);
    void SendTip(QString strTip, int nTimeMS = 1000);

private slots:
    void on_list_dataList_customContextMenuRequested(const QPoint &pos);
    void on_list_dataList_currentRowChanged(int currentRow);
};

#endif // WIDGET_DATA_H
