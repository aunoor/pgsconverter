#include <QtXml>
#include <QProxyStyle>
#include <QWidgetAction>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <cstring>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editpointdialog.h"
#include "aboutdialog.h"
#include "safe_bin.h"
#include "configdialog.h"

/*
  Для наших целей должно быть достаточно что разница между N55,723510 и N55,723511 = 0.11 метра разницы,
  а между E37,382360 и E37,382361 = 0,06 метров разницы

  Широта: d0.0001 = 1.1 метра (1метр = 0.00009)
  Долгота: d0.002 = 1.2 метров (1метр = 0.00166)
  Подсчеты очень грубые и не учитывают реальное растояние в зависимости от широты и долготы
*/

class IconMenuStyle : public QProxyStyle
 {
   public:
    int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const
    {
        if (metric==QStyle::PM_SmallIconSize) return 24;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
 };

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ovCountLabel = new QLabel("");
    odCheckBox = new QCheckBox(tr("Загружать неизвестные типы точек как прочие опасности"));
    odCheckBox->setCheckState(Qt::Checked);

    odCompareProgressBar = new QProgressBar(this);

    this->ui->statusbar->addWidget(odCheckBox);
    this->ui->statusbar->addWidget(ovCountLabel);
    this->ui->statusbar->addWidget(odCompareProgressBar);

    odCompareProgressBar->setMaximum(1);
    odCompareProgressBar->setMinimum(0);
    odCompareProgressBar->setValue(0);
    odCompareProgressBar->setVisible(false);

    proxyModel.setSortRole(Qt::UserRole);
    proxyModel.setSourceModel(&this->pointModel);

    sc_proxyModel.setSortRole(Qt::UserRole);
    sc_proxyModel.setSourceModel(&this->sc_pointModel);

    this->ui->treeView->setModel(&this->proxyModel);
    this->ui->treeView->sortByColumn(0,Qt::AscendingOrder);

    this->ui->treeView->header()->resizeSection(0,80);
    this->ui->treeView->header()->resizeSection(1,50);
    this->ui->treeView->header()->resizeSection(2,60);
    this->ui->treeView->header()->resizeSection(3,200);
    this->ui->treeView->header()->resizeSection(4,200);
    this->ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);


    this->ui->treeView_sc->setModel(&this->sc_proxyModel);
    this->ui->treeView_sc->sortByColumn(0,Qt::AscendingOrder);

    this->ui->treeView_sc->header()->resizeSection(0,80);
    this->ui->treeView_sc->header()->resizeSection(1,50);
    this->ui->treeView_sc->header()->resizeSection(2,60);
    this->ui->treeView_sc->header()->resizeSection(3,200);
    this->ui->treeView_sc->header()->resizeSection(4,200);
    //this->ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);


    listMenu.addAction(this->ui->action_check_all);
    listMenu.addAction(this->ui->action_uncheck_all);
    listMenu.addSeparator();
    listMenu.addAction(this->ui->action_clone_point);
    listMenu.addSeparator();
    listMenu.addAction(this->ui->action_del_from_list);

    this->setWindowTitle(tr("Конвертер UserSafety точек"));
    this->setWindowIcon(QIcon(":/gui/icons/map_edit.png"));
    changed=false;

    ui->treeView->installEventFilter(this);

    iconMenu.setStyle(new IconMenuStyle);
    listMenu.setStyle(new IconMenuStyle);

    ui->menuBar->setVisible(false);

    QWidgetAction *wact = new QWidgetAction(ui->toolBar);
    QScrollArea *widget = new QScrollArea(ui->toolBar);
    widget->setFrameStyle(QFrame::NoFrame);
    widget->setMaximumHeight(ui->toolBar->height());
    wact->setDefaultWidget(widget);
    ui->toolBar->insertAction(ui->action_about_prog,wact);

    ui->toolBox->setCurrentIndex(0);
    ui->toolBox->setItemText(0,"");
    ui->toolBox->setItemText(1,"safety_cache");

    connect(&pointModel,SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),SLOT(pointModel_dataChanged_slot(const QModelIndex &, const QModelIndex &)));
    connect(&pointModel,SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(pointModel_rowChanged_slot(const QModelIndex &,int,int)));
    connect(&pointModel,SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(pointModel_rowChanged_slot(const QModelIndex &,int,int)));
    connect(&pointModel,SIGNAL(compareProgress(uint,uint)),SLOT(showCompareProgress(uint,uint)));

    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_exit_triggered()
{
    this->close();
}

void MainWindow::on_action_export_fav_triggered()
{
    if (!countCheckedItems()) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Не выбрано ни одной точки для сохранения."));
        return;
    }
    QString saveDir=app_settings.exportDir;
    QDir dir(saveDir);
    saveDir=dir.absoluteFilePath("UserSafety.dat");
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт UserSafety.dat"),saveDir,tr("Файл UserSafety точек ПроГород (UserSafety.dat *.dat)"));
    if (fileName.isEmpty()) return;
    QFileInfo fileInfo(fileName);
    app_settings.exportDir = fileInfo.absolutePath();
    saveSettings();
    if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".dat");
    bool res=storeInSafeDat(fileName);
    if (changed) setChanged(!res);
}

bool MainWindow::loadSafeRecords(QString fileName, SafePointsList &list)
{
    QFile file(fileName);
    if (!file.exists()) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Файл не найден."));
        return false;
    }

    bool res = file.open(QIODevice::ReadOnly);

    if (!res) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Ошибка при открытии файла."));
        return false;
    }

    list.clear();
    struct CFG_HEADER header;

    file.read((char *)&header,sizeof(header));

    for (uint i=0;i<header.nFieldCount;i++)
    {
        QByteArray ba = file.read(sizeof(SafeRecord_V1));
        if (ba.size()!=sizeof(SafeRecord_V1)) {
            QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Size of readed data != point data size!"));
            file.close();
            return false;
        }
        safeRecordV1_t safeRecord;
        std::memcpy(&safeRecord,ba.data(),sizeof(SafeRecord_V1));
        addRawPointToPointList(safeRecord, list);
    }

    file.close();
    return true;
}

void MainWindow::showPointList(SafePointsList &list, bool append) {
    if (!append) this->pointModel.clearModel();
    for (int i=0;i<list.size();i++) {
        //проверяем на дубликаты точек
//        if (hasDupInList(list.at(i))) continue;
        pointModel.appendPoint(list.at(i));
    }
    ui->treeView->setCurrentIndex(pointModel.index(0,0,QModelIndex()));
}

void MainWindow::showSCPointList(SafePointsList &list) {
    this->sc_pointModel.clearModel();
    for (int i=0;i<list.size();i++) {
        sc_pointModel.appendPoint(list.at(i));
    }
    ui->treeView_sc->setCurrentIndex(sc_pointModel.index(0,0,QModelIndex()));
}


bool MainWindow::loadCamTxt(QString fileName, SafePointsList &list, bool isUTF8) {

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

#if 0
   //Убеждаемся что мы открыли именно нужный файл
    bool hasHeader=false;
    while (true) {
        QString head=file.readLine();
        if (head.contains("IDX,X,Y,TYPE,SPEED,DIRTYPE,DIRECTION")) {
            hasHeader=true;
            break;
        }
    }
    if (!hasHeader) {
        file.close();
        qDebug() << "We don't have properly header.";
        return false;
    }
#endif

    while (!file.atEnd()) {
        QByteArray line=file.readLine();
        QTextCodec *codec = QTextCodec::codecForName(isUTF8?"UTF-8":"Windows-1251");
        QString str=codec->toUnicode(line);
        QString name;
        if (str.isEmpty()) continue;
        str=str.trimmed();
        if (str.at(0)==QChar('#')) continue;
        if (str.contains("//")) {
            int cmPos=str.indexOf("//");
            name=str.mid(cmPos+2);
            str.truncate(cmPos);
        }
        QStringList params=str.split(',');
        if (params.count()<7) continue; //количество параметров меньше допустимого.
        SafePoint spoint;
        spoint.idx = params.at(0);//IDX
        bool ok;
        spoint.lon= params.at(1).toDouble(&ok);//X
        if (!ok) continue;
        spoint.lat= params.at(2).toDouble(&ok);//Y
        if (!ok) continue;
        spoint.pntType = txtType2PGType(params.at(3).toInt(&ok), odCheckBox->checkState()==Qt::Checked);
        if (!ok) continue;
        spoint.speed = params.at(4).toInt(&ok);
        if (!ok) continue;
        spoint.dirtype = params.at(5).toInt(&ok);
        if (!ok) continue;
        spoint.direction = params.at(6).toInt(&ok);
        if (!ok) continue;
        spoint.checked=true;
        if (spoint.pntType<2) continue; //пропускаем неизвестные нам точки.

        if (!name.isEmpty()) {
            spoint.name=name.left(128);
        }

        spoint.uuid = QUuid::createUuid();
        list.append(spoint);
    }

    file.close();

    if (app_settings.auto_load_sc) {
        QFileInfo  fi(fileName);
        QString sc_fn=fi.absolutePath()+"/safety_cache.bin";
        loadSystemSafePoints(sc_fn);
    }

    return true;
}

void MainWindow::chCheckItems(bool checked) {
    int cnt=pointModel.rowCount(QModelIndex());
    for (int i=0; i<cnt;i++) {
        pointModel.setPointChecked(i,checked);
    }
    ui->treeView->repaint();
}

void MainWindow::on_action_check_all_triggered()
{
    chCheckItems(true);
}

void MainWindow::on_action_uncheck_all_triggered()
{
    chCheckItems(false);
}

int MainWindow::countCheckedItems() {
    return pointModel.getCheckedCount();
}

bool MainWindow::storeInTxt(QString &fileName){
    QFile file(fileName);
    bool res = file.open(QIODevice::WriteOnly);
    if (!res) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Ошибка при открытии файла %1.").arg(fileName));
        return res;
    }

    file.write(QByteArray("IDX, X, Y, TYPE, SPEED, DirType, Direction\r\n"));
    for (int i=0; i<pointModel.getPointsCount();i++) {
        if (!pointModel.getPoint(i).checked) continue;
        QByteArray string;
        string.append(QByteArray::number(i));
        string.append(',');
        safePoint_t point = pointModel.getPoint(i);
        string.append(QByteArray::number(point.lat,'g',7));
        string.append(',');
        string.append(QByteArray::number(point.lon,'g',7));
        string.append(',');
        string.append(QByteArray::number(PGType2txtType(point.pntType)));
        string.append(',');
        string.append(QByteArray::number(point.speed));
        string.append(',');
        string.append(QByteArray::number(point.dirtype));
        string.append(',');
        string.append(QByteArray::number(point.direction));
        if (!point.name.isEmpty()) {
            //QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
            QTextCodec *codec = QTextCodec::codecForName("UTF-8");
            string.append(" //");
            string.append(codec->fromUnicode(point.name));
        }
        string.append("\r\n");
        file.write(string);
    }//for
    file.close();
    return true;
}

bool MainWindow::storeInSafeDat(QString &fileName){
    QFile file(fileName);
    bool res = file.open(QIODevice::WriteOnly);
    if (!res) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Ошибка при открытии файла %1.").arg(fileName));
        return res;
    }//if !res

    quint16 cnt=countCheckedItems();

    //пишем заголовок

    struct CFG_HEADER header;

    header.nMajorVersion=1;
    header.nMinorVersion=0;
    header.nFieldCount=cnt;
    header.reserved[0]=0;
    header.reserved[1]=0;

    file.write((const char*)&header,sizeof(header));
    file.seek(sizeof(header));

    for (int i=0; i<pointModel.getPointsCount();i++) {
        if (!pointModel.getPoint(i).checked) continue;
        safePoint_t point = pointModel.getPoint(i);
        safeRecordV1_t *rawPnt = (safeRecordV1_t *)malloc(sizeof(safeRecordV1_t));
        pntToRawPnt(point, rawPnt);
        file.write((const char*)rawPnt, sizeof(safeRecordV1_t));
        free(rawPnt);
    }//for
    file.close();
    return true;

}

void MainWindow::doLoadPoints(bool append) {

    QString openFilter = tr("Файлы с точками [UTF8] (*.txt usersafety.dat *.dat);;Файлы с точками [Windows-1251] (*.txt);;Системный файл с точками (safety_cache.bin)");

    if (append) {
       openFilter = tr("Файлы с точками [UTF8] (*.txt usersafety.dat *.dat);;Файлы с точками [Windows-1251] (*.txt)");
    }

    QString openDir = append?app_settings.appendDir:app_settings.openDir;
    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this,append?tr("Добавить точки в список"):tr("Открыть список точек"),openDir, openFilter, &selectedFilter);
    if (fileName.isEmpty()) return;
    QFileInfo fileInfo(fileName);
    (append?app_settings.appendDir:app_settings.openDir)  = fileInfo.absolutePath();
    saveSettings();
    SafePointsList safeList;
    if (QFileInfo(fileName).suffix()=="txt") {
        bool isUTF=false;
        if (selectedFilter.contains("UTF8")) isUTF=true;
        if (!loadCamTxt(fileName, safeList, isUTF)) return;
    }
    else if (QFileInfo(fileName).suffix()=="dat"){
        if (!loadSafeRecords(fileName, safeList)) return;
        clearSafeCacheList();
    }//else
    else if (QFileInfo(fileName).baseName()=="safety_cache") {
        loadSystemSafePoints(fileName);
        return;
    }
    showPointList(safeList, append);
    setChanged(false);
}

void MainWindow::on_action_append_from_file_triggered()
{
    doLoadPoints(true);
}

void MainWindow::on_action_open_file_triggered()
{
    doLoadPoints(false);
}

void MainWindow::on_action_del_from_list_triggered()
{
    QModelIndex index=ui->treeView->currentIndex();
    if (!index.isValid()) return;
    pointModel.removeRow(index.row());
}

void MainWindow::on_treeView_customContextMenuRequested(QPoint pos)
{
    ui->action_del_from_list->setEnabled(ui->treeView->indexAt(pos).isValid());
    iconMenu.setEnabled(ui->treeView->indexAt(pos).isValid());
    QPoint locPos = mapToGlobal(pos);
    locPos.setY(locPos.y()+listMenu.sizeHint().height()-20);
    listMenu.popup(locPos,ui->action_check_all);
}

void MainWindow::on_action_about_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_action_save_as_triggered()
{
    if (!countCheckedItems()) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Не выбрано ни одной точки для сохранения."));
        return;
    }
    QString selFilt;
    QString saveDir=app_settings.saveDir;
    if (saveDir.isEmpty()) saveDir=".";
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт выбранных точек"),saveDir,tr("Файл избранных точек ПроГород (usersafety.dat *.dat);; Файл точек SpeedCam (*.txt)"),&selFilt);
    if (fileName.isEmpty()) return;
    QFileInfo fileInfo(fileName);
    app_settings.saveDir = fileInfo.absolutePath();
    saveSettings();
    bool res;
    if (selFilt.contains("txt")) {
        if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".txt");
        res = storeInTxt(fileName);
    } else {
        if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".dat");
        res = storeInSafeDat(fileName);
    }
    if (changed) setChanged(!res);
}

void MainWindow::setChanged(bool ch)
{
    changed = ch;
    QString wt=tr("Конвертер UserSafety точек");
    if (!openedFileName.isEmpty()) {
        wt.append(" ("+QFileInfo(openedFileName).fileName());
        wt.append(")");
    }
    if (changed) wt.append(" *");
    this->setWindowTitle(wt);
    updateCount();
}

void MainWindow::on_action_save_triggered()
{
    if (openedFileName.isEmpty()) {
        on_action_save_as_triggered();
        return;
    }

    bool res;
    if (QFileInfo(openedFileName).suffix()=="txt") {
        res = storeInTxt(openedFileName);
    } else
    {
        res = storeInSafeDat(openedFileName);
    }
    if (changed) setChanged(!res);
}

void MainWindow::on_action_about_prog_triggered()
{
    AboutDialog ad(this);
    ad.setVersion(VERSION);
    ad.exec();
}

void MainWindow::closeEvent(QCloseEvent *event)
 {
    if (!changed) {
        event->accept();
        return;
    }
     if (QMessageBox::question(this,tr("Выход"),tr("Есть незаписанные изменения. Действительно выйти?"),QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Ok)!=QMessageBox::Ok) {
        event->ignore();
        return;
     } else {
         event->accept();
     }
 }

void MainWindow::on_treeView_doubleClicked(QModelIndex index)
{
    if (index.column()==1) {
        QAbstractItemModel *model=ui->treeView->model();
        QModelIndex tmpIndex=model->index(index.row(),1);
        int currentType=model->data(tmpIndex,Qt::UserRole).toInt();

        for (int i=0;i<model->rowCount();i++) {
            QModelIndex tmpIndex=model->index(i,1);
            if (currentType==model->data(tmpIndex,Qt::UserRole).toInt()) {
                ui->treeView->selectionModel()->select(tmpIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            }//if
        }//for
        return;
    }

    EditPointDialog ed;
    safePoint_t point=pointModel.getPoint(index.row());
    int res=ed.exec(point);
    if (res==QDialog::Rejected) return;
    pointModel.setPoint(index.row(),point);
}

void MainWindow::pointModel_dataChanged_slot(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{

   if (topLeft.column()==0) {
       bool checked = pointModel.getPoint(topLeft.row()).checked;
       QModelIndexList selList=ui->treeView->selectionModel()->selectedRows(0);
       pointModel.massCheck(selList,checked);
   }
   setChanged(true);
}

void MainWindow::pointModel_rowChanged_slot(const QModelIndex &parent, int start, int end)
{
    setChanged(true);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object==this->ui->treeView && event->type()==QEvent::KeyPress) {
        if (!ui->treeView->currentIndex().isValid()) return false;
        QKeyEvent *kEvent = (QKeyEvent*)event;
        if (!(kEvent->modifiers() & Qt::ControlModifier)) return false;
        QModelIndex cIndex=ui->treeView->currentIndex();
        QModelIndex uIndex;
        if (kEvent->key()==Qt::Key_Up) {
            uIndex=pointModel.index(cIndex.row()-1,0,QModelIndex());
        } else if (kEvent->key()==Qt::Key_Down) {
            uIndex=pointModel.index(cIndex.row()+1,0,QModelIndex());
        }
        if (!uIndex.isValid()) return false;
        if (pointModel.swapRows(cIndex,uIndex)) {
            ui->treeView->setCurrentIndex(uIndex);
            kEvent->accept();
            return true;
        }
    }
    return false;
}

QMenu *MainWindow::createPopupMenu ()
{
 return 0;
}

void MainWindow::on_action_clone_point_triggered()
{
    QModelIndex index=ui->treeView->currentIndex();
    if (!index.isValid()) return;
    pointModel.clonePoint(index.row());
}

void MainWindow::updateCount()
{
    ovCountLabel->setText(" "+QString::number(pointModel.getPointsCount())+" ");
    ui->toolBox->setItemText(0,tr("Загруженные точки - ")+QString::number(pointModel.getPointsCount()));
}

void MainWindow::updateSCCount()
{
    ui->toolBox->setItemText(1,"safety_cache - "+QString::number(sc_pointModel.getPointsCount()));
}

void MainWindow::on_action_remove_twins_triggered()
{
    pointModel.delete_twins(&sc_pointModel);
    setChanged(true);
}

void MainWindow::clearSafeCacheList() {
    safe_cache_list.clear();
    sc_pointModel.clearModel();
}

bool MainWindow::loadSystemSafePoints(QString filePath) {
    if (sc_pointModel.getPointsCount()) return true; //не загружаем если уже загружено
    clearSafeCacheList();
    bool res=loadSystemSafeRecords(filePath,safe_cache_list);
    updateSCCount();
    if (!res) return false;
    showSCPointList(safe_cache_list);
    updateSCCount();
    safe_cache_list.clear();
    return true;
}

void MainWindow::loadSettings()
{
    app_settings.saveDir = settings.value("saveDir",QVariant(QString("."))).toString();
    app_settings.exportDir = settings.value("exportDir",QVariant(QString("."))).toString();
    app_settings.openDir = settings.value("openDir",QVariant(QString("."))).toString();
    app_settings.appendDir = settings.value("appendDir",QVariant(QString("."))).toString();
    app_settings.box_size = settings.value("boxSize",10).toInt();
    app_settings.auto_load_sc = settings.value("autoLoadSC",true).toBool();
}

void MainWindow::saveSettings()
{
    settings.setValue("openDir",app_settings.openDir);
    settings.setValue("saveDir",app_settings.saveDir);
    settings.setValue("exportDir",app_settings.exportDir);
    settings.setValue("appendDir",app_settings.appendDir);
    settings.setValue("boxSize",app_settings.box_size);
    settings.setValue("autoLoadSC",app_settings.auto_load_sc);
}

void MainWindow::on_actionConfigure_triggered()
{
    app_settings_t tmp_settings = app_settings;

    ConfigDialog configDialog(this);
    int res=configDialog.exec();
    if (res) saveSettings();
    else app_settings = tmp_settings;
}

void MainWindow::showCompareProgress(unsigned int pos, unsigned int overal)
{
//    qDebug() << "pos=" << pos <<" overal" << overal;

    odCompareProgressBar->setVisible(true);
    odCompareProgressBar->setMaximum(overal);
    odCompareProgressBar->setValue(pos);

    ui->statusbar->repaint();

    if (pos==overal) {
        odCompareProgressBar->setMaximum(1);
        odCompareProgressBar->setMinimum(0);
        odCompareProgressBar->setValue(0);
        odCompareProgressBar->setVisible(false);
        return;
    }


}
