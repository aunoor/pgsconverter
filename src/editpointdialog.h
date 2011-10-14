#ifndef EDITPOINTDIALOG_H
#define EDITPOINTDIALOG_H

#include <QtGui>
#include "pointmodel.h"

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
        int exec(safePoint_t &point);

};

#endif // EDITPOINTDIALOG_H
