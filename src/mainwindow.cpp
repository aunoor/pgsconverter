#include <QtXml>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editpointdialog.h"
#include "aboutdialog.h"

/*
  Для наших целей должно быть достаточно что разница между N55,723510 и N55,723511 = 0.11 метра разницы,
  а между E37,382360 и E37,382361 = 0,06 метров разницы

  Широта: d0.0001 = 1.1 метра
  Долгота: d0.002 = 1.2 метров
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

    this->ui->treeView->setModel(&this->pointModel);

    this->ui->treeView->header()->resizeSection(0,110);
    this->ui->treeView->header()->resizeSection(1,200);
    this->ui->treeView->header()->resizeSection(2,200);
    this->ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    listMenu.addAction(this->ui->action_check_all);
    listMenu.addAction(this->ui->action_uncheck_all);
    listMenu.addSeparator();
    listMenu.addAction(this->ui->action_clone_point);
    listMenu.addSeparator();
    listMenu.addAction(this->ui->action_del_from_list);

    this->setWindowTitle(tr("Конвертер UserSafety точек"));
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
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт UserSafety.dat"),".",tr("Файл UserSafety точек ПроГород (UserSafety.dat *.dat)"));
    if (fileName.isEmpty()) return;
    if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".dat");
    bool res=storeInSafeDat(fileName);
    if (changed) setChanged(!res);
}

void MainWindow::on_action_save_gpx_triggered()
{
    if (!countCheckedItems()) {
        QMessageBox::critical(this,QObject::tr("Ошибка"), tr("Не выбрано ни одной точки для сохранения."));
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт SpeedCam.txt"),".",tr("Файл точек SpeedCam (*.txt)"));
    if (fileName.isEmpty()) return;
    if (QFileInfo(fileName).suffix().isEmpty()) fileName.append(".txt");
    bool res=storeInTxt(fileName);
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
        qMemCopy(&safeRecord,ba.data(),sizeof(SafeRecord_V1));

        addRawPointToPointList(safeRecord, list);
    }

    file.close();
    return true;
}

void MainWindow::showPointList(SafePointsList &list, bool append) {
    if (!append) this->pointModel.clearModel();
    for (int i=0;i<list.size();i++) {
        pointModel.appendPoint(list.at(i));
    }
    ui->treeView->setCurrentIndex(pointModel.index(0,0,QModelIndex()));
}

bool MainWindow::loadCamTxt(QString fileName, SafePointsList &list) {

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
        QString str=file.readLine();
        if (str.isEmpty()) continue;
        str=str.trimmed();
        if (str.at(0)==QChar('#')) continue;
        QStringList params=str.split(',');
        if (params.count()<7) continue; //количество параметров меньше допустимого.
        SafePoint spoint;
        spoint.idx = params.at(0);//IDX
        bool ok;
        spoint.lat= params.at(1).toDouble(&ok);//X
        if (!ok) continue;
        spoint.lon= params.at(2).toDouble(&ok);//Y
        if (!ok) continue;
        spoint.pntType = txtType2PGType(params.at(3).toInt(&ok));
        if (!ok) continue;
        spoint.speed = params.at(4).toInt(&ok);
        if (!ok) continue;
        spoint.dirtype = params.at(5).toInt(&ok);
        if (!ok) continue;
        spoint.direction = params.at(6).toInt(&ok);
        if (!ok) continue;
        spoint.checked=true;
        spoint.uuid = QUuid::createUuid();
        list.append(spoint);
    }

    file.close();
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
        string.append(QByteArray::number(point.lat));
        string.append(',');
        string.append(QByteArray::number(point.lon));
        string.append(',');
        string.append(QByteArray::number(PGType2txtType(point.pntType)));
        string.append(',');
        string.append(QByteArray::number(point.speed));
        string.append(',');
        string.append(QByteArray::number(point.dirtype));
        string.append(',');
        string.append(QByteArray::number(point.direction));
        if (!point.name.isEmpty()) {
            QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
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

void MainWindow::on_action_append_from_file_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Добавить точки в список"),".",tr("Файлы с точками (*.txt usersafety.dat *.dat)"));
    if (fileName.isEmpty()) return;
    SafePointsList safeList;
    if (QFileInfo(fileName).suffix()=="txt") {if (!loadCamTxt(fileName, safeList)) return;}
    else {if (!loadSafeRecords(fileName, safeList)) return; }
    showPointList(safeList, true);
    if (!openedFileName.isEmpty()) setChanged(true);
    else { openedFileName = fileName;
        setChanged(false);
    }
}

void MainWindow::on_action_open_file_triggered()
{

    QString fileName = QFileDialog::getOpenFileName(this,tr("Открыть список точек"),".",tr("Файлы с путевыми точками (*.txt usersafety.dat *.dat)"));
    if (fileName.isEmpty()) return;
    SafePointsList safeList;
    if (QFileInfo(fileName).suffix()=="txt") {if (!loadCamTxt(fileName, safeList)) return;}
    else {if (!loadSafeRecords(fileName, safeList)) return; }
    openedFileName = fileName;
    showPointList(safeList, false);
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
    QString fileName = QFileDialog::getSaveFileName(this,tr("Экспорт выбранных точек"),".",tr("Файл избранных точек ПроГород (usersafety.dat *.dat);; Файл точек SpeedCam (*.txt)"),&selFilt);
    if (fileName.isEmpty()) return;
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
    EditPointDialog ed;
    QString name, coords;
    safePoint_t point=pointModel.getPoint(index.row());
    int pntType = point.pntType;


    name = point.name;
    coords = index.model()->data(pointModel.index(index.row(),3,QModelIndex()),Qt::DisplayRole).toString();
    int res=ed.exec(point);
    if (res==QDialog::Rejected) return;
#if 0
    point.desc = desc;
    point.name = name;

    pointModel.setPoint(index.row(),point);
#endif
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
    safePoint_t point=pointModel.getPoint(index.row());
/*
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
    */
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
