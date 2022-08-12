#ifndef FORM_READER_H
#define FORM_READER_H

#include <QWidget>

namespace Ui {
class Form_reader;
}

class Form_reader : public QWidget
{
    Q_OBJECT

public:
    explicit Form_reader(QSet<void*>* psets, const QString& strTitle, const QString& strText, QWidget *parent = nullptr);
    ~Form_reader();

private:
    Ui::Form_reader *ui;
    QSet<void*>* m_psets = nullptr;
};

#endif // FORM_READER_H
