#include "configdialog.h"
#include "ui_configdialog.h"
#include "globaldefs.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    ui->spinBox_Int_BoxSize->setValue(app_settings.int_box_size);
    ui->spinBox_SC_BoxSize->setValue(app_settings.sc_box_size);
    ui->checkBox_scByAuto->setChecked(app_settings.auto_load_sc);
    ui->checkBox_UnkAsOthers->setChecked(app_settings.unk_as_others);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::accept() {
    app_settings.sc_box_size = ui->spinBox_SC_BoxSize->value();
    app_settings.int_box_size = ui->spinBox_Int_BoxSize->value();
    app_settings.auto_load_sc = ui->checkBox_scByAuto->isChecked();
    app_settings.unk_as_others = ui->checkBox_UnkAsOthers->isChecked();
    done(QDialog::Accepted);
}
