#ifndef WIDGET_LOCAL_H
#define WIDGET_LOCAL_H

#include "cpatch.h"
#include "dialog_preview.h"

#include <QWidget>

class QLabel;
class QListWidget;

namespace Ui {
class Widget_Local;
}

class Widget_Local : public QWidget
{
    Q_OBJECT

public:
    explicit Widget_Local(QWidget *parent = nullptr);
    ~Widget_Local();

    int bPreviewPicture = 1;                    // 是否预览图片

    void Init();
    void Lookup(QString strKeyword);
    void ExportAll();
    void Clear();

private:
    Ui::Widget_Local *ui;

    QVector<SDataInfo> m_vDataList[2];          // 存放data数据信息的容器
    QString m_strLocalPath;                     // 本地导出资源目录
    Dialog_Preview* m_pDialogPreview = NULL;    // 图片预览器
    QLabel* m_pLabelInfo[2];
    QListWidget* m_pListWidget[2];

    void SetLackListVisible();
    void ProofreadLackFile();
    void DownloadLackFile();
    void UpdateLackFile();
    void RightMenuEvent(int nIndex, int currentRow, int nMenuButton);
    void CustomContextMenuRequested(const QPoint &pos, int nIndex);
    void CurrentRowChanged(int nIndex, int currentRow);
    // 窗口尺寸变动时，检测图片预览器是否超出边界
    virtual void resizeEvent(QResizeEvent *) override {
        if (m_pDialogPreview == NULL) return;
        if (m_pDialogPreview->isVisible() == 0) return;
        m_pDialogPreview->CheckImagePos();
    }

signals:
    void SendInfo(SDataInfo sDataInfo);
    void SendTip(QString strTip, int nTimeMS = 1000);
};

#endif // WIDGET_LOCAL_H
