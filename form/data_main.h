#ifndef DATA_MAIN_H
#define DATA_MAIN_H

#include <QWidget>
#include "cpatch.h"

class Config;
class Data_item;
class Form_Image;

namespace Ui {
class Data_main;
}

class Data_main : public QWidget
{
    Q_OBJECT

public:
    explicit Data_main(QWidget *parent = nullptr);
    ~Data_main();
    void load(const int &mode, const QVariant& data = QVariant());
    void reload(const int &count = 0);
    void clear();
    void lookup(const QString &key = "");
    void restoreInfoBar();

signals:
    void tip(const QString &strTip, int nTimeout = 1000);
    void updateInfo(const SDataInfo &sDataInfo);

public slots:
    /// 窗口大小更新
    void onResize(QSize);
    /// 更新视图列状态
    void columnMaxChanged();
    /// 更新略缩图
    void onUpdateImage(SDataInfo *pDataInfo);
    /// 更新列数
    void onUpdateColumnNum();
    /// 显示/折叠 空视图
    void onButtonFoldNull();
    /// 检查MD5
    void onButtonMd5();
    /// 整理资源 [1.游戏资源] [2.单个资源]
    void onButtonOrganizeData();
    /// 导出全部 [1.游戏资源] [2.单个资源]
    void onButtonExportAll();
    /// 打包全部 [3.本地资源]
    void onButtonPackAll();
    /// 下载全部 [4.网络资源]
    void onButtonDownloadAll();
    /// 显示/隐藏 缺失文件列表
    void onButtonLackList();
    /// 导出选中项 (bAlone 是否单独导出)
    void onExportSelected(bool bAlone, const QList<SDataInfo*> &datas, Data_item *self);
    /// 下载选中项 (type 1.资源包 2.导出路径 3.单独路径)
    void onDownloadSelected(int type, const QList<SDataInfo*> &datas);
    /// 下载缺失文件 [1.游戏资源] [3.本地资源]
    void onDownload();
    /// 校对缺失文件 [1.游戏资源] [3.本地资源]
    void onProofread();

private:
    /// 创建新视图 (type 视图类型: 1.jmp文件; 2.资源文件夹; 3.网络文件列表; 4.缺失文件列表)
    Data_item *newItem(int type, const QString &strPath = {});
    /// 导出资源 (type 目标位置: 1.路径导出目录 2.单独导出目录)
    void exportData(int type, QStringList paths, const QVector<QList<SDataInfo*>> &totalDatas, bool bShowMessage = false);
    /// 导入资源 (path mode=1.游戏目录 mode=2.单个资源路径 mode=3.打包目录)
    void importData(QString path, const QVector<SDataInfoList *> &datas);
    /// 下载资源 (type 目标位置: 1.资源包 2.导出路径 3.单独路径)
    void downloadData(int type, const QList<SDataInfo*> &datas, bool bShowMessage = false);

private:
    Ui::Data_main *ui;
    Config *m_config = nullptr;
    int m_mode = 0;                     ///< 显示模式：0.无 1.游戏资源 2.单个资源 3.本地资源 4.网络资源
    int m_columns = 1;                  ///< 视图当前列数
    bool m_bFoldNull = false;           ///< 是否折叠空视图
    QList<Data_item*> m_items;          ///< 资源视图列表
    Data_item *m_lack = nullptr;        ///< 缺失资源列表
    Form_Image *m_image = nullptr;      ///< 略缩图预览器
};

#endif // DATA_MAIN_H
