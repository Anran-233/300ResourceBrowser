#ifndef FORM_READER_H
#define FORM_READER_H

#include "cpatch.h"
#include <QWidget>

namespace Ui {
class Form_reader;
}

class Form_reader : public QWidget
{
    Q_OBJECT

public:
    explicit Form_reader(QObject *parent = nullptr);
    ~Form_reader();
    void load(QWidget *parent, const SDataInfo& sDataInfo);

private:
    Ui::Form_reader *ui;
    QByteArray m_data;
};

#endif // FORM_READER_H
