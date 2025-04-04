#ifndef DATA_ITEM_H
#define DATA_ITEM_H

#include <QWidget>
#include "cpatch.h"

namespace Ui {
class Data_item;
}

class Data_item : public QWidget
{
    Q_OBJECT

public:
    explicit Data_item(QWidget *parent, int parentType, int type, const QString& strPath);
    ~Data_item();
    /// 加载数据
    void load(QList<Data_item*> items = {});
    /// 更新缺失文件列表
    void updateLacks(QList<Data_item*> items);
    /// 状态
    bool isLoaded() const;
    /// 是否为空
    bool isNull() const;
    /// 是否已校对
    bool isProofread() const;
    /// 类型 1.jmp文件; 2.资源文件夹; 3.网络文件列表; 4.缺失文件列表
    int type() const;
    /// 路径
    QString path() const;
    /// 查找
    int lookup(const QString &key);
    /// 清理
    void clear();
    /// 最小宽度
    static int minWidth();
    /// 最小高度
    static int minHeight();
    /// 清除选中项
    void clearSelectItem();
    /// 获取所有列表
    QVector<SDataInfo> *datas();

signals:
    /// 加载结束
    void loaded();
    /// 略缩图预览器
    void preview(SDataInfo *pDataInfo);
    /// 调用悬浮提示
    void tip(QString strTip, int nTimeMS = 1000);
    /// 更新信息显示
    void updateInfo(const SDataInfo& sDataInfo);
    /// 更新进度文本
    void sigProgressText(QString text);
    /// 清除其他视图选中项
    void sigClearOtherSelectItem(Data_item *self);
    /// 导出资源(选中项)
    void sigExportSelected(bool bAlone, const QList<SDataInfo*> &datas, Data_item *self);
    /// 下载资源(选中项)[1.资源包 2.导出路径 3.单独路径]
    void sigDownloadSelected(int type, const QList<SDataInfo*> &datas);
    /// 下载资源(全部)
    void sigDownload();
    /// 校对缺失列表
    void sigProofread();

private slots:
    /// 创建右键菜单
    void rightMenuCreate(const QPoint &pos);
    /// 处理右键菜单事件
    void rightMenuEvent(int type, int nIndex);

private:
    Ui::Data_item *ui;
    std::atomic_bool m_stop{false};         ///< 是否终止执行任务
    std::atomic_bool m_busy{false};         ///< 是否正在执行任务
    std::atomic_bool m_loaded{false};       ///< 是否已经加载完毕
    std::atomic_bool m_ignore{false};       ///< 是否忽略事件
    int m_parentType = 0;                   ///< 父级类型: 1.游戏目录; 2.单个资源; 3.本地目录; 4.网络资源
    int m_type;                             ///< 显示类型: 1.jmp文件; 2.资源文件夹; 3.网络文件列表; 4.缺失文件列表
    QString m_strPath;                      ///< 文件路径
    QVector<SDataInfo> m_datas;             ///< 资源数据列表
    PatchUniques m_clients;                 ///< 客户端文件列表
};

#endif // DATA_ITEM_H
