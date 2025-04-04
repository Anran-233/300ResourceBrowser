#include "datawidget.h"
#include "ui_datawidget.h"

DataWidget::DataWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataWidget)
{
    ui->setupUi(this);
}

DataWidget::~DataWidget()
{
    delete ui;
}

void DataWidget::init(const int &mode, const QString &strJmpFile)
{

}

void DataWidget::clear()
{

}

void DataWidget::lookup(const QString &str)
{

}
