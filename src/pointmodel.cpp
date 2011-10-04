#include "pointmodel.h"

//IDX, X,       Y,       TYPE,SPEED,DIRTYPE,DIRECTION,
//2332,35.61041,55.46594,4,   60,   1,      285,,,#Московская обл., Минское ш., Цуканово, в Москву - Стрелка - М1-4

PointModel::PointModel()
{
}

int PointModel::columnCount(const QModelIndex & parent) const
{
    return 4; //будет 6 по большому счету...
}

QVariant PointModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) return QVariant();

    if (index.row()>pointList.count()-1) return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        //qDebug() << index.row() << index.column();
        //if (index.column()==0) return QString::number(index.row()+1);
        if (index.column()==0) return QVariant(index.row()+1);
//        else if (index.column()==1) return ((favPoints_t)pointList.at(index.row())).name;
//        else if (index.column()==2) return ((favPoints_t)pointList.at(index.row())).desc;
        else if (index.column()==1) return QString(tr("N %2, E %1")).arg(QString::number(pointList.at(index.row()).lon,'g',8).leftJustified(8,'0',true)).arg(QString::number(pointList.at(index.row()).lat,'g',8).leftJustified(8,'0',true));
        break;
    case Qt::CheckStateRole:
        if (index.column()==0) return ((favPoints_t)pointList.at(index.row())).checked?Qt::Checked:Qt::Unchecked;
        else return QVariant();
        break;
    case Qt::DecorationRole:
        if (index.column()==0)
        {
            QString iconName=":/gui/icons/p_icons/"+QString::number(pointList.at(index.row()).iconNum+1)+".png";
            QIcon icon(iconName);
            return icon;
        } else
            if (index.column()==1)
            {
                if (!pointList.at(index.row()).pntType) return QVariant();
                QString iconName;
                if (pointList.at(index.row()).pntType==1) iconName=":/gui/icons/bt_home_n.2.png";
                if (pointList.at(index.row()).pntType==2) iconName=":/gui/icons/bt_office_n.2.png";
                QIcon icon(iconName);
                return icon;
            }
        break;
    default:
        QVariant();
    }//switch
    return QVariant();
}

bool PointModel::setData (const QModelIndex & index, const QVariant & value, int role)
{
    if (index.column()==0)
    {
        if (role==Qt::CheckStateRole) pointList[(index.row())].checked=(value.toInt()==Qt::Checked)?true:false;
        dataChanged(index,index);
        return true;
    }
    return false;
}

Qt::ItemFlags PointModel::flags (const QModelIndex & index) const
{
    Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;

    if (index.isValid())
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

QVariant PointModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    {
        if (orientation==Qt::Horizontal) {
            switch (section) {
            case 0: return tr("N");
            case 1: return tr("Координаты");
            default:
                return QVariant();
            }//switch
        } else return QVariant();
        break;
    }//case Qt::DisplayRole:
    default:
        QVariant();
    }//switch
    return QVariant();

}

QModelIndex PointModel::index(int row, int column, const QModelIndex & parent) const
{
    if (row>pointList.count()-1) return QModelIndex();
    if (row<-1) return QModelIndex();
    return createIndex(row, column, (int)QString(QString::number(row)+QString::number(column)+QString::number(row*column)).toLongLong());
}

int PointModel::rowCount ( const QModelIndex & parent) const
{
    if (!parent.isValid()) return pointList.count();
    else return 0;
}

QModelIndex PointModel::parent( const QModelIndex & index) const
{
    return QModelIndex();
}

bool PointModel::hasChildren( const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        return true;
    }else return false;
}

Qt::DropActions PointModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool PointModel::dropMimeData(const QMimeData *data,
                              Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    //qDebug() << action << row << column << parent << data->formats() << data->data(MIME_RAW_POINT_TYPE);
    if (!data->formats().contains(MIME_RAW_POINT_TYPE)) return false;
    quint16 cnt=0;
    qint64 aPid=0;
    QByteArray ba=data->data(MIME_RAW_POINT_TYPE);

    qMemCopy(&aPid, ba.constData(),sizeof(aPid));
    ba.remove(0,sizeof(aPid));

    qMemCopy(&cnt,ba.constData(),sizeof(cnt));
    ba.remove(0,sizeof(cnt));

    bool moveMode=true;
    if (aPid!=qApp->applicationPid()) moveMode=false;

    FavPointsList tmpPL;

    for (int i=0;i<cnt;i++) {
        favRecord_t favRawPoint;
        qMemCopy(&favRawPoint,ba.constData(), sizeof(favRecord_t));
        addRawPointToPointList(favRawPoint, tmpPL);
        ba.remove(0, sizeof(favRecord_t));
        int uuidLen;
        qMemCopy(&uuidLen,ba.constData(),sizeof(uuidLen));
        ba.remove(0,sizeof(uuidLen));

        QByteArray dsBa = ba.left(uuidLen);
        QUuid uuid = QUuid(QString(dsBa));
        favPoints_t * fp = &tmpPL[tmpPL.count()-1];
        fp->uuid = uuid;
        ba.remove(0,uuidLen);
    }

    if (moveMode) {
        //режим, когда записи перемещаются внутри одного приложения...
        if (!parent.isValid()) return false;
        else {
            QUuid parUUID = pointList.at(parent.row()).uuid;
            for (int i=0;i<tmpPL.count();i++) {
                for (int i2=0;i2<pointList.count();i2++) {
                    if (pointList.at(i2).uuid == tmpPL.at(i).uuid) {
                        removeRow(i2);
                        break;
                    }//if
                }//for i2
            }//for
            int pntInd=0;
            for (int i=0;i<pointList.count();i++) {
                if (parUUID==pointList.at(i).uuid) { pntInd = i; break; }
            }
            for (int i=0;i<tmpPL.count(); i++) {
                beginInsertRows(QModelIndex(), pntInd, pntInd);
                pointList.insert(pntInd,tmpPL.at(i));
                endInsertRows();
                pntInd+=1;
            }
        }//else if (!parent.isValid())
    } else {
        int pntInd=0;
        if (parent.isValid()) {
            QUuid parUUID = pointList.at(parent.row()).uuid;
            for (int i=0;i<pointList.count();i++) {
                if (parUUID==pointList.at(i).uuid) { pntInd = i; break; }
            }
        }//parent.isValid
        else pntInd = pointList.count();

        for (int i=0;i<tmpPL.count(); i++) {
            beginInsertRows(QModelIndex(), pntInd, pntInd);
            pointList.insert(pntInd,tmpPL.at(i));
            endInsertRows();
            pntInd+=1;
        }
    }//else moveMode
    emit dataChanged(index(0,0,QModelIndex()),index(pointList.count(),columnCount(QModelIndex())));
    return true;
}

QStringList PointModel::mimeTypes() const
{
    return QStringList() << MIME_RAW_POINT_TYPE;
}

QMimeData * PointModel::mimeData ( const QModelIndexList & indexes ) const
{
    QMimeData * mimeData = new QMimeData();
    QByteArray ba, baR;
    quint16 cnt=0;
    for (int i=0;i<indexes.count();i++) {
        if (!((QModelIndex)indexes.at(i)).isValid() || ((QModelIndex)indexes.at(i)).column()!=0) continue;
        favRecord_t *rawPnt = (favRecord_t *)malloc(sizeof(favRecord_t));
        favPoints_t pnt = pointList.at(((QModelIndex)indexes.at(i)).row());
        pntToRawPnt(pnt,rawPnt);
        ba.append((const char*)rawPnt,sizeof(favRecord_t));
        free(rawPnt);
        QByteArray dsBa=pnt.uuid.toString().toLatin1();
        int cntBa=dsBa.count();
        ba.append((const char*)&cntBa,sizeof(cntBa));
        ba.append(dsBa);
        cnt++;
    }
    qint64 aP = qApp->applicationPid();
    baR.append((const char*)&aP,sizeof(aP));
    baR.append((const char*)&cnt,sizeof(cnt));
    baR.append(ba);
    mimeData->setData(MIME_RAW_POINT_TYPE,baR);
    return mimeData;
}

bool PointModel::removeRow (int row, const QModelIndex & parent)
{
    if (row>pointList.count()-1) return false;
    beginRemoveRows(QModelIndex(),row, row);
    pointList.removeAt(row);
    endRemoveRows();
    emit dataChanged(index(row,0,QModelIndex()),index(row,columnCount(QModelIndex())));
    return true;
}

void PointModel::appendPoint(const favPoints_t &point)
{
    pointList.append(point);
    beginInsertRows(QModelIndex(),pointList.count()-1,pointList.count()-1);
    endInsertRows();
    emit dataChanged(index(pointList.count(),0,QModelIndex()),index(pointList.count(),columnCount(QModelIndex())));
}

void PointModel::clearModel()
{
    beginRemoveRows(QModelIndex(),0,pointList.count());
    pointList.clear();
    endRemoveRows();
}

void PointModel::setPointChecked(int row, bool checked)
{
    if (row>pointList.count()) return;
    pointList[row].checked=checked;
}

int PointModel::getCheckedCount()
{
    uint cnt=0;
    for (int i=0;i<pointList.count();i++) {
        if (pointList.at(i).checked) cnt++;
    }
    return cnt;
}

favPoints_t PointModel::getPoint(int row)
{
    return pointList.at(row);
}

int PointModel::getPointsCount()
{
    return pointList.count();
}

void PointModel::setPoint(int row, favPoints_t &point)
{
    if (row>pointList.count()-1) return;
    pointList[row] = point;
    emit dataChanged(index(row,0,QModelIndex()),index(row,columnCount(QModelIndex())));
}

bool PointModel::swapRows(int oldRow, int newRow)
{
    QModelIndex oldIndex=index(oldRow,0,QModelIndex());
    QModelIndex newIndex=index(newRow,0,QModelIndex());
    return swapRows(oldIndex, newIndex);
}

bool PointModel::swapRows(QModelIndex &oldRow, QModelIndex &newRow)
{
    if (!oldRow.isValid() || !newRow.isValid()) return false;
    pointList.swap(oldRow.row(), newRow.row());
    emit dataChanged(index(oldRow.row(),0,QModelIndex()),index(newRow.row(),columnCount(QModelIndex())));
    return true;
}

bool PointModel::setPointType(int row, uint type)
{
    if (row<0 || row> pointList.count()-1) return false;
    if (type!=0) {
        for (int i=0;i<pointList.count()-1;i++) {
            if (pointList.at(i).pntType==type) {
                favPoints_t point =  pointList[i];
                point.pntType=0;
                setPoint(i,point);
            }//if ==
            }//for
        }//!=0
    favPoints_t point = pointList.at(row);
    point.pntType = type;
    setPoint(row, point);
    return true;
}
