#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QMainWindow>
#include <QMenu>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>


#include "pointmodel.h"
#include "globaldefs.h"


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_treeView_doubleClicked(QModelIndex index);
    void on_treeView_customContextMenuRequested(QPoint pos);
    void on_action_del_from_list_triggered();
    void on_action_open_file_triggered();
    void on_action_append_from_file_triggered();
    void on_action_exit_triggered();
    void on_action_export_fav_triggered();
    void on_action_check_all_triggered();
    void on_action_uncheck_all_triggered();
    void on_action_about_Qt_triggered();
    void on_action_save_as_triggered();
    void on_action_save_triggered();
    void on_action_about_prog_triggered();
    void on_action_clone_point_triggered();
    void pointModel_dataChanged_slot(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void pointModel_rowChanged_slot(const QModelIndex &parent, int start, int end);
    void pointModel_modelReset_slot();

    void on_action_remove_twins_triggered();

    void on_actionConfigure_triggered();

    void on_action_remove_internal_twins_triggered();

private:
    Ui::MainWindow *ui;
    QMenu listMenu;
    QMenu iconMenu;
    QList<QAction*> iconActions;
    bool changed;
    QString openedFileName;
    PointModel pointModel;
    QCheckBox *odCheckBox;
    QLabel *ovCountLabel;
    QProgressBar *odCompareProgressBar;
    QSortFilterProxyModel proxyModel;
    QSortFilterProxyModel sc_proxyModel;
    QSettings settings;
    SafePointsList safe_cache_list;
    PointModel sc_pointModel;
    bool loadSafeRecords(QString fileName, SafePointsList &list);
    void showPointList(SafePointsList &list, bool append);
    bool loadCamTxt(QString fileName, SafePointsList &list, bool isUTF8);
    void chCheckItems(bool checked);
    bool storeInTxt(QString &fileName);
    bool storeInSafeDat(QString &fileName);
    int  countCheckedItems();
    void setChanged(bool ch);
    void updateCount();
    QMenu *createPopupMenu(); //заглушка, что бы не показывалось мену у тулбара
    bool loadSystemSafePoints(QString filePath);
    bool hasDupInList(safePoint_t point);
    void clearSafeCacheList();
    void showSCPointList(SafePointsList &list); //отображает загруженные точки из safety_cache.bin
    void updateSCCount(); //обновляем заголовок страницы с точками из safety_cache
    void doLoadPoints(bool append); //функция, открывает диалог для загрузки или добавления точек в список
    void loadSettings(); //загрузка переменных конфига
    void saveSettings(); //запись переменных конфига
protected:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *, QEvent *);

public slots:
    void showCompareProgress(unsigned int pos, unsigned int overal);
};

#endif // MAINWINDOW_H
