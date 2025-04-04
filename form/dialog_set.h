#ifndef DIALOG_SET_H
#define DIALOG_SET_H

#include <QDialog>

/// 更新文件关联 (value 文件关联值[000])
void UpdateApplicationAssociation(const QString& value);

/// 检测更新 (parent 窗口指针, type 更新源, bSuccessMsg 成功是否弹窗)
void CheckUpdate(QWidget* parent, const int& type, bool bSuccessMsg);

namespace Ui {
class Dialog_Set;
}

class Dialog_Set : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_Set(QWidget *parent = nullptr);
    ~Dialog_Set();

private:
    Ui::Dialog_Set *ui;
    /// 初始化
    void Init();

private slots:
    /// 确定设置
    void onOK();
    /// 取消设置
    void onOUT();
    /// 自动搜索300路径
    void onAutoFind();
    /// 设置路径 (index: 0.游戏路径 1.导出路径 2.单独路径)
    void onSetPath(int index);
    /// 设置关联状态 (index: 0.jmp 1.bank 2.dat)
    void onSetLink(int index);
    /// 检测更新
    void onUpdate();
};

#endif // DIALOG_SET_H
