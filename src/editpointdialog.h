#ifndef EDITPOINTDIALOG_H
#define EDITPOINTDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
    class EditPointDialog;
}

class EditPointDialog : public QDialog {
    Q_OBJECT
public:
    EditPointDialog(QWidget *parent = 0);
    ~EditPointDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::EditPointDialog *m_ui;

public slots:
        int exec(QString &name, QString &desc, QString coords);

};

#endif // EDITPOINTDIALOG_H
