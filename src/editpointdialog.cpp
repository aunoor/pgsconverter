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

int EditPointDialog::exec(safePoint_t &point)
{
    this->m_ui->nameLineEdit->setText(point.name);
    this->m_ui->speedSpinBox->setValue(point.speed);
    this->m_ui->latSpinBox->setValue(point.lat);
    this->m_ui->lonSpinBox->setValue(point.lon);
    this->m_ui->typeComboBox->setCurrentIndex(point.pntType-1);

    int res=QDialog::exec();
    if (res!=QDialog::Accepted) return res;
    point.name = this->m_ui->nameLineEdit->text();
    point.lat = this->m_ui->latSpinBox->value();
    point.lon = this->m_ui->lonSpinBox->value();
    point.pntType = this->m_ui->typeComboBox->currentIndex()+1;
    point.speed = this->m_ui->speedSpinBox->value();
    return res;
}
