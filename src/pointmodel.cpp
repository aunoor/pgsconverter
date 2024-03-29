#include <cstring>
#include "pointmodel.h"

//IDX, X,       Y,       TYPE,SPEED,DIRTYPE,DIRECTION,
//2332,35.61041,55.46594,4,   60,   1,      285,,,#Московская обл., Минское ш., Цуканово, в Москву - Стрелка - М1-4

PointModel::PointModel()
{
}

int PointModel::columnCount(const QModelIndex & parent) const
{
    return 4; //N,тип,Ограничение,Координаты
}

QVariant PointModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) return QVariant();

    if (index.row()>pointList.count()-1) return QVariant();
    switch (role) {
       case Qt::UserRole:
        if (index.column()==1) return QVariant((int)pointList.at(index.row()).pntType);
       case Qt::DisplayRole:
        if (index.column()==0) return QVariant(index.row()+1);
        else if (index.column()==1) return QVariant();
        else if (index.column()==2) return QVariant(pointList.at(index.row()).speed);
        else if (index.column()==3) return QString(tr("N %1, E %2")).arg(QString::number(pointList.at(index.row()).lat,'g',8).leftJustified(8,'0',true)).arg(QString::number(pointList.at(index.row()).lon,'g',8).leftJustified(8,'0',true));
        //else if (index.column()==4) return ((safePoint_t)pointList.at(index.row())).name;
        break;
       case Qt::CheckStateRole:
        if (index.column()==0) return ((safePoint_t)pointList.at(index.row())).checked?Qt::Checked:Qt::Unchecked;
        else return QVariant();
        break;
#if 0
       case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
        break;
#endif
       case Qt::DecorationRole:
        if (index.column()==1)
        {
            QString iconName;
            switch ((int)pointList.at(index.row()).pntType)
            {
              case CFG_USER_SAFETY_INFO_TYPE_NONE: iconName=":/gui/icons/pg_icons/qu.png"; break;
            //пока на ограничение скорости отрисуем 50
              case CFG_USER_SAFETY_INFO_TYPE_SPEEDLIMIT: iconName=":/gui/icons/pg_icons/50.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_SCHOOLZONE: iconName=":/gui/icons/pg_icons/children.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_DANGER: iconName=":/gui/icons/pg_icons/att.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_SPEEDBUMP: iconName=":/gui/icons/pg_icons/sb.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_WILDLIFE: iconName=":/gui/icons/pg_icons/animals.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_RAILWAY: iconName=":/gui/icons/pg_icons/rc.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_POLICE: iconName=":/gui/icons/pg_icons/dps.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM: iconName=":/gui/icons/pg_icons/camera.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_INTERSECTION: iconName=":/gui/icons/pg_icons/intersection.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_TURN: iconName=":/gui/icons/pg_icons/danger_turn.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_DEDICATED_LANE_CAM: iconName=":/gui/icons/pg_icons/dedic_line.png"; break;
              case CFG_USER_SAFETY_INFO_TYPE_PASSWAY: iconName=":/gui/icons/pg_icons/school_zone.png"; break;
            default: {
                iconName=":/gui/icons/pg_icons/qu.png";
                qDebug() << "PointModel::data(): unknown point type: " << (int)pointList.at(index.row()).pntType;
                }//default
            }
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
            case 1: return tr("Тип");
            case 2: return tr("Ограничение");
            case 3: return tr("Координаты");
            case 4: return tr("Наименование");
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
    //return createIndex(row, column, (int)QString(QString::number(row)+QString::number(column)+QString::number(row*column)).toLongLong());
    return createIndex(row, column, pointList.at(row).uuid.data1);
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

    std::memcpy(&aPid, ba.constData(),sizeof(aPid));
    ba.remove(0,sizeof(aPid));

    std::memcpy(&cnt,ba.constData(),sizeof(cnt));
    ba.remove(0,sizeof(cnt));

    bool moveMode=true;
    if (aPid!=qApp->applicationPid()) moveMode=false;

    SafePointsList tmpPL;

    for (int i=0;i<cnt;i++) {
        safeRecordV1_t safeRawPoint;
        std::memcpy(&safeRawPoint,ba.constData(), sizeof(safeRecordV1_t));
        addRawPointToPointList(safeRawPoint, tmpPL);
        ba.remove(0, sizeof(safeRecordV1_t));
        int uuidLen;
        std::memcpy(&uuidLen,ba.constData(),sizeof(uuidLen));
        ba.remove(0,sizeof(uuidLen));

        QByteArray dsBa = ba.left(uuidLen);
        QUuid uuid = QUuid(QString(dsBa));
        safePoint_t * fp = &tmpPL[tmpPL.count()-1];
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
    emit dataChanged(index(0,0,QModelIndex()),index(pointList.count(),columnCount(QModelIndex())-1));
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
        safeRecordV1_t *rawPnt = (safeRecordV1_t *)malloc(sizeof(safeRecordV1_t));
        safePoint_t pnt = pointList.at(((QModelIndex)indexes.at(i)).row());
        pntToRawPnt(pnt,rawPnt);
        ba.append((const char*)rawPnt,sizeof(safeRecordV1_t));
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
    return true;
}

void PointModel::appendPoint(const safePoint_t &point)
{
    unsigned int rNum=0;
    if (pointList.count()>0) rNum=pointList.count();
    beginInsertRows(QModelIndex(),rNum,rNum);
    pointList.append(point);
    endInsertRows();
}

void PointModel::insertPoint(const int row, const safePoint_t &point)
{
    beginInsertRows(QModelIndex(),row,row);
    pointList.insert(row, point);
    endInsertRows();
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
    emit dataChanged(index(row,0,QModelIndex()),index(row,columnCount(QModelIndex())-1));
}

int PointModel::getCheckedCount()
{
    uint cnt=0;
    for (int i=0;i<pointList.count();i++) {
        if (pointList.at(i).checked) cnt++;
    }
    return cnt;
}

safePoint_t PointModel::getPoint(int row)
{
    return pointList.at(row);
}

int PointModel::getPointsCount() const
{
    return pointList.count();
}

void PointModel::setPoint(int row, safePoint_t &point)
{
    if (row>pointList.count()-1) return;
    pointList[row] = point;
    emit dataChanged(index(row,0,QModelIndex()),index(row,columnCount(QModelIndex())-1));
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
                safePoint_t point =  pointList[i];
                point.pntType=0;
                setPoint(i,point);
            }//if ==
            }//for
        }//!=0
    safePoint_t point = pointList.at(row);
    point.pntType = type;
    setPoint(row, point);
    return true;
}

void PointModel::clonePoint(int row)
{
    if (row<0 || row> pointList.count()-1) return;
    safePoint_t src_point = getPoint(row);
    insertPoint(row+1,src_point);
}

void PointModel::massCheck(QModelIndexList &list, bool checked)
{
    if (list.size()==0) return;
    for (int i=0;i<list.size();i++) {
        pointList[list.at(i).row()].checked=checked;
    }//for
}

void PointModel::delete_sc_twins(PointModel *point_model)
{
    unsigned int ovrl = pointList.count();
    unsigned int pos=0;
    bool res;

    emit compareProgress(pos,ovrl);

    SafePointsList::iterator curr=pointList.begin();
    while (curr!=pointList.end()) {
        emit compareProgress(pos,ovrl);
        res=false;
        for (int i=0;i<point_model->getPointsCount();i++) {
            safePoint_t tmp_pnt = point_model->getPoint(i);
            res=compareCoordsByArea((*curr), tmp_pnt, app_settings.sc_box_size, true);
            if (res) break;
        }//for
        pos++;
        if (res) curr=pointList.erase(curr);
        else curr++;
    }//while

    emit compareProgress(pos,pos);

    endResetModel();
}


void PointModel::delete_internal_twins()
{
    unsigned int ovrl = pointList.count()*2;
    unsigned int pos=0;

    beginResetModel();

    emit compareProgress(pos,ovrl);

    QStringList ids;
    for (int i=0;i<pointList.count();i++) {
        pos=i;
        emit compareProgress(pos,ovrl);
        for (int i2=i+1;i2<pointList.count();i2++) {
            bool res=compareCoordsByArea(pointList.at(i), pointList.at(i2), app_settings.int_box_size, true);
            if (res) ids.append(pointList.at(i2).idx);
        }//for i2
    }//for i

    ovrl = pointList.count()+ids.count();

    for (int i=0;i<ids.count();i++) {
        pos+=i;
        emit compareProgress(pos,ovrl);
        SafePointsList::iterator curr=pointList.begin();
        while (curr!=pointList.end()) {
            if ((*curr).idx==ids.at(i)) curr=pointList.erase(curr);
            else curr++;
        }//while
    }//for i

    emit compareProgress(pos,pos);

    endResetModel();
}

void PointModel::selectByType(int pointType) {
    for (int i=0;i<pointList.count();i++) {
        if (pointList.at(i).pntType==pointType) pointList[i].checked=true;
        else pointList[i].checked=false;
    }//for

    emit dataChanged(index(0,0,QModelIndex()),index(rowCount(),columnCount(QModelIndex())));
}
