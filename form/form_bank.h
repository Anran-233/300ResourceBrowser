#ifndef FORM_BANK_H
#define FORM_BANK_H

#include "cpatch.h"
#include <QWidget>

namespace Ui {
class Form_bank;
}

class Form_bank : public QWidget
{
    Q_OBJECT

public:
    explicit Form_bank(QWidget *parent = nullptr);
    ~Form_bank();
    void load(QWidget *parent, const SDataInfo& sDataInfo);
    static void StopBank();

private:
    Ui::Form_bank *ui;
    int m_bank = 0;

    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // FORM_BANK_H
