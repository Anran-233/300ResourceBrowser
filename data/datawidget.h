#ifndef DATAWIDGET_H
#define DATAWIDGET_H

#include <QWidget>

namespace Ui {
class DataWidget;
}

class DataWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataWidget(QWidget *parent = nullptr);
    ~DataWidget();
    void init(const int& mode, const QString& strJmpFile = "");
    void clear();
    void lookup(const QString& str = "");

private:
    Ui::DataWidget *ui;
};

#endif // DATAWIDGET_H
