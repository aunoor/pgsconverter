#include <QtXml>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editpointdialog.h"
#include "aboutdialog.h"

/*
 Координаты храняться в little endian uint32.

 ПроГород поддерживает 5 значимых знаков после запятой в сохраняемых координатах.
 Из-за этого, преобразование из gpx может приводить к потерям точности.
 Но т.к. погрешность GPS до 10 метров, несколько секунд большой разницы не играют.
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

    this->ui->treeView->setModel(&this->pointModel);

    this->ui->treeView->header()->resizeSection(0,110);
    this->ui->treeView->header()->resizeSection(1,200);
    this->ui->treeView->header()->resizeSection(2,200);
    this->ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    initIconMenu();

    listMenu.addAction(this->ui->action_check_all);
    listMenu.addAction(this->ui->action_uncheck_all);
    listMenu.addSeparator();
    listMenu.addMenu(&iconMenu);
    listMenu.addSeparator();
    listMenu.addAction(this->ui->action_del_from_list);

    this->setWindowTitle(tr("Конвертер избранных точек"));
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

    connect(&pointModel,SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),SLOT(pointModel_dataChanged_slot(const QModelIndex &, const QModelIndex &)));
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
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт favorites.dat"),".",tr("Файл избранных точек ПроГород (favorites.dat *.dat)"));
    if (fileName.isEmpty()) return;
    if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".dat");
    bool res=storeInFavDat(fileName);
    if (changed) setChanged(!res);
}

void MainWindow::on_action_save_gpx_triggered()
{
    if (!countCheckedItems()) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Не выбрано ни одной точки для сохранения."));
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт gpx waypoints"),".",tr("Файл точек GPX (*.gpx)"));
    if (fileName.isEmpty()) return;
    if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".gpx");
    bool res=storeInGpx(fileName);
    if (changed) setChanged(!res);
}

bool MainWindow::loadFavRecords(QString fileName, FavPointsList &list)
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
    file.seek(6); //пропускаем заголовок
    while (!file.atEnd()) {
        QByteArray ba = file.read(0x414);
        if (ba.size()!=0x414) {
            QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Size of readed data != point data size!"));
            file.close();
            return false;
        }
        favRecord_t favRawPoint;
        qMemCopy(&favRawPoint,ba.data(),0x414);
        addRawPointToPointList(favRawPoint, list);
    }
    file.close();
    return true;
}

void MainWindow::showPointList(FavPointsList &list, bool append) {
    if (!append) this->pointModel.clearModel();
    for (int i=0;i<list.size();i++) {
        pointModel.appendPoint(list.at(i));
    }
    ui->treeView->setCurrentIndex(pointModel.index(0,0,QModelIndex()));
}

bool MainWindow::loadGpx(QString fileName, FavPointsList &list) {
    QDomDocument doc("gpx");
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    if (!doc.setContent(&file)) {
        file.close();
        return false;
    }
    file.close();

    list.clear();
    QDomElement docElem = doc.documentElement();

    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {
            if (e.tagName().toLower()=="wpt") {
                favPoints_t point;
                point.lon = e.attributeNode("lon").value().toDouble();
                point.lat = e.attributeNode("lat").value().toDouble();
                point.checked = true;
                point.uuid = QUuid::createUuid();
                point.iconNum = 0;
                point.pntType = 0;
                QDomNodeList nl=e.childNodes();
                for (int i=0;i<nl.size();i++) {
                    if (nl.at(i).nodeName().toLower()=="name") point.name=nl.at(i).toElement().text();
                    if (nl.at(i).nodeName().toLower()=="desc") point.desc=nl.at(i).toElement().text();
                }//for nl.size
                list.append(point);
            }//if =="wpt"
        }//!e.isNull()
        n = n.nextSibling();
    }//while(!n.isNull())
    return true;
}

void MainWindow::chCheckItems(bool checked) {
    int cnt=pointModel.rowCount(QModelIndex());
    for (int i=0; i<cnt;i++) {
        pointModel.setPointChecked(i,checked);
    }
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

bool MainWindow::storeInGpx(QString &fileName){
    QFile file(fileName);
    bool res = file.open(QIODevice::WriteOnly);
    if (!res) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Ошибка при открытии файла %1.").arg(fileName));
        return res;
    }

    file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    file.write("<gpx version=\"1.0\" ");
    file.write("creator=\"pgfconverter ");
    file.write(VERSION);
    file.write("\">\n");
    int cnt=pointModel.getPointsCount();
    for (int i=0; i<cnt;i++) {
        if (!pointModel.getPoint(i).checked) continue;
        favPoints_t point = pointModel.getPoint(i);
        file.write("<wpt lat=\"");
        file.write(QString::number(point.lat,'g',9).toLatin1());
        file.write("\" lon=\"");
        file.write(QString::number(point.lon,'g',9).toLatin1());
        file.write("\">\n");
        if (!point.name.isEmpty()){
            file.write("\t<name>");
            file.write(point.name.toUtf8());
            file.write("</name>\n");
        }
        if (!point.desc.isEmpty()){
            file.write("\t<desc>");
            file.write(point.desc.toUtf8());
            file.write("</desc>\n");
        }
        file.write("</wpt>\n");
    }//for
    file.write("</gpx>\n");
    return true;
}

bool MainWindow::storeInFavDat(QString &fileName){
    QFile file(fileName);
    bool res = file.open(QIODevice::WriteOnly);
    if (!res) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Ошибка при открытии файла %1.").arg(fileName));
        return res;
    }//if !res

    quint32 cnt=countCheckedItems();

    //пишем заголовок
    char a[2]={1,0};
    file.write(&a[0],2);
    file.write((char*)&cnt,4);
    file.seek(6);
    //пишем данные
    for (int i=0; i<pointModel.getPointsCount();i++) {
        if (!pointModel.getPoint(i).checked) continue;
        favPoints_t point = pointModel.getPoint(i);
        favRecord_t *rawPnt = (favRecord_t *)malloc(sizeof(favRecord_t));
        pntToRawPnt(point, rawPnt);
        file.write((const char*)rawPnt, sizeof(favRecord_t));
        free(rawPnt);
    }//for
    file.close();
    return true;

}

void MainWindow::on_action_append_from_file_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Добавить точки в список"),".",tr("Файлы с путевыми точками (*.gpx favorites.dat *.dat)"));
    if (fileName.isEmpty()) return;
    FavPointsList favList;
    if (QFileInfo(fileName).suffix()=="gpx") {if (!loadGpx(fileName, favList)) return;}
    else {if (!loadFavRecords(fileName, favList)) return; }
    showPointList(favList, true);
    if (!openedFileName.isEmpty()) setChanged(true);
    else { openedFileName = fileName;
        setChanged(false);
    }
}

void MainWindow::on_action_open_file_triggered()
{

    QString fileName = QFileDialog::getOpenFileName(this,tr("Открыть список точек"),".",tr("Файлы с путевыми точками (*.gpx favorites.dat *.dat)"));
    if (fileName.isEmpty()) return;
    FavPointsList favList;
    if (QFileInfo(fileName).suffix()=="gpx") {if (!loadGpx(fileName, favList)) return;}
    else {if (!loadFavRecords(fileName, favList)) return; }
    openedFileName = fileName;
    showPointList(favList, false);
    setChanged(false);
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
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт выбранных точек"),".",tr("Файл точек GPX (*.gpx);; Файл избранных точек ПроГород (favorites.dat *.dat)"),&selFilt);
    if (fileName.isEmpty()) return;
    bool res;
    if (selFilt.contains("gpx")) {
        if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".gpx");
        res = storeInGpx(fileName);
    } else {
        if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".dat");
        res = storeInFavDat(fileName);
    }
    if (changed) setChanged(!res);
}

void MainWindow::setChanged(bool ch)
{
    changed = ch;
    QString wt=tr("Конвертер избранных точек");
    if (!openedFileName.isEmpty()) {
        wt.append(" ("+QFileInfo(openedFileName).fileName());
        wt.append(")");
    }
    if (changed) wt.append(" *");
    this->setWindowTitle(wt);
}

void MainWindow::on_action_save_triggered()
{
    if (openedFileName.isEmpty()) {
        on_action_save_as_triggered();
        return;
    }

    bool res;
    if (QFileInfo(openedFileName).suffix()=="gpx") {
        res = storeInGpx(openedFileName);
    } else
    {
        res = storeInFavDat(openedFileName);
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

    EditPointDialog ed;
    QString name, desc, coords;
    favPoints_t point=pointModel.getPoint(index.row());
    name = point.name;
    desc = point.desc;
    coords = index.model()->data(pointModel.index(index.row(),3,QModelIndex()),Qt::DisplayRole).toString();
    int res=ed.exec(name, desc, coords);
    if (res==QDialog::Rejected) return;

    point.desc = desc;
    point.name = name;

    pointModel.setPoint(index.row(),point);
}

void MainWindow::pointModel_dataChanged_slot(const QModelIndex &topLeft, const QModelIndex &bottomRight)
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

void MainWindow::setIcon_Type()
{
    QModelIndex index=ui->treeView->currentIndex();
    if (!index.isValid()) return;

    int icn_t = ((QAction*)this->sender())->data().toInt();
    favPoints_t point=pointModel.getPoint(index.row());

    if (icn_t==99) {
        pointModel.setPointType(index.row(), ptNone);
    } else
    if (icn_t==98) {
        pointModel.setPointType(index.row(), ptHome);
    } else
    if (icn_t==97) {
        pointModel.setPointType(index.row(), ptOffice);
    } else
    if (icn_t==96) {
        point.iconNum=0;
        pointModel.setPoint(index.row(),point);
    } else
    if (icn_t>-1 && icn_t<21) {
        point.iconNum = icn_t;
        pointModel.setPoint(index.row(),point);
    }
}

void MainWindow::initIconMenu()
{
    iconMenu.setTitle(tr("Иконки..."));
    QAction *action;
    action = iconMenu.addAction(tr("Сбросить тип"),this,SLOT(setIcon_Type()));
    action->setData(99);
    QIcon icon = QIcon(":/gui/icons/bt_home_n.2.png");
    action = iconMenu.addAction(icon,tr("Дом"),this,SLOT(setIcon_Type()));
    action->setData(98);
    icon = QIcon(":/gui/icons/bt_office_n.2.png");
    action = iconMenu.addAction(icon,tr("Офис"),this,SLOT(setIcon_Type()));
    action->setData(97);
    iconMenu.addSeparator();
    for (int i=0;i<20;i++) {
        icon = QIcon(":/gui/icons/p_icons/"+QString::number(i+1)+".png");
        action = iconMenu.addAction(icon,QString::number(i+1),this,SLOT(setIcon_Type()));
        action->setData(i);
    }
}

QMenu *MainWindow::createPopupMenu ()
{
 return 0;
}
