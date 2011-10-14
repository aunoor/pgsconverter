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

    QString coords=QString(tr("N %2, E %1")).arg(QString::number(point.lon,'g',8).leftJustified(8,'0',true)).arg(QString::number(point.lat,'g',8).leftJustified(8,'0',true));

    this->m_ui->coordsLabel->setText(coords);
    this->m_ui->speedSpinBox->setValue(point.speed);

    int res=QDialog::exec();
    if (res!=QDialog::Accepted) return res;
    point.name = this->m_ui->nameLineEdit->text();
    return res;
}
