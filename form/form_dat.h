#ifndef FORM_DAT_H
#define FORM_DAT_H

#include "cpatch.h"
#include <QWidget>

// DAT数据
struct SDatData {
    int         type    { 0 };      // 类型: 0.为空值; 1.整数型; 2.浮点型; 3.字符型;
    long long   Int     { 0 };      // 整数型
    float       Float   { 0.0f };   // 浮点型
    QString     Str     { "" };     // 字符型
};
typedef QVector<QVector<SDatData> > DATLIST;    // DAT数据表

namespace Ui {
class Form_dat;
}

class Form_dat : public QWidget
{
    Q_OBJECT

public:
    explicit Form_dat(QWidget *parent = nullptr);
    ~Form_dat();
    void load(QWidget *parent, const SDataInfo& sDataInfo);

signals:
    void sigTabelHead();
    void sigClose();
    void sigRender();
    void sigLookup(int row, int column);

private slots:
    void onTableHead();
    void onRender();
    void onLookup(int row, int column);

private:
    Ui::Form_dat *ui;
    DATLIST m_data;
};

#endif // FORM_DAT_H
