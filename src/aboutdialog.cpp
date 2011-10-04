#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("О программе"));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::setVersion(QString version)
{
    ui->label->setText(ui->label->text().replace("VERSION", version));
}
