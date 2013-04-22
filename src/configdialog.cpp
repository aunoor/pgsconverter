#include "src/configdialog.h"
#include "ui_configdialog.h"
#include "src/globaldefs.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    ui->spinBox_BoxSize->setValue(app_settings.box_size);
    ui->checkBox->setChecked(app_settings.auto_load_sc);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::accept() {
    app_settings.box_size = ui->spinBox_BoxSize->value();
    app_settings.auto_load_sc = ui->checkBox->isChecked();
    done(QDialog::Accepted);
}
