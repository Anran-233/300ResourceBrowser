#include "form_reader.h"
#include "ui_form_reader.h"

Form_reader::Form_reader(QSet<void*>* psets, const QString& strTitle, const QString& strText, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form_reader)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle(strTitle);
    ui->text->setPlainText(strText);
    psets->insert(this);
    m_psets = psets;
}

Form_reader::~Form_reader()
{
    if (m_psets) if (m_psets->contains(this)) m_psets->remove(this);
    delete ui;
}
