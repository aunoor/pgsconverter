#include "editpointdialog.h"
#include "ui_editpointdialog.h"

EditPointDialog::EditPointDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::EditPointDialog)
{
    m_ui->setupUi(this);
    this->setWindowTitle(tr("Изменение описания точки"));
}

EditPointDialog::~EditPointDialog()
{
    delete m_ui;
}

void EditPointDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

int EditPointDialog::exec(QString &name, QString &desc, QString coords)
{
    this->m_ui->nameLineEdit->setText(name);
    this->m_ui->descLineEdit->setText(desc);
    this->m_ui->coordsLabel->setText(coords);

    int res=QDialog::exec();
    if (res!=QDialog::Accepted) return res;
    name = this->m_ui->nameLineEdit->text();
    desc = this->m_ui->descLineEdit->text();
    return res;
}
