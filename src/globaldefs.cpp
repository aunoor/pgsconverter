#include "globaldefs.h"

void pntToRawPnt(safePoint_t &pnt, safeRecordV1_t *rawPnt)
{
    //преобразуем точку из нашего формата в формат ПГ
    qMemSet((void*)rawPnt,0,sizeof(safeRecordV1_t));

    rawPnt->pos_x=qRound(pnt.lat*100000);
    rawPnt->pos_y=qRound(pnt.lon*100000);

    rawPnt->type = pnt.pntType;
    rawPnt->speed = pnt.speed;

    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    QByteArray ba = codec->fromUnicode(QString(pnt.name));
    //Из-за каких то глюков Qt, либо я что-то не осилил, но приходиться копировать байты руками...
    for (int i=0;i<(ba.size()>128?128:ba.size());i++) {
        rawPnt->name[i] = ba.at(i);
    }
}

void addRawPointToPointList(safeRecordV1_t &safeRawPoint, SafePointsList &list) {
    list.append(trRawPointToPoint(safeRawPoint));
}

safePoint_t trRawPointToPoint(safeRecordV1_t &safeRawPoint) {
    safePoint_t point;
    point.lat = safeRawPoint.pos_x*.00001;
    point.lon = safeRawPoint.pos_y*.00001;
    point.pntType = safeRawPoint.type;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    point.name=codec->toUnicode(safeRawPoint.name,128);
    point.name.truncate(point.name.indexOf(QChar('\0')));
    point.checked = true;
    QUuid uuid = QUuid::createUuid();
    point.idx = uuid.toString().left(6);
    point.uuid = uuid;
    point.speed = safeRawPoint.speed;
    return point;
}


quint8 txtType2PGType(quint8 txt_type)
{
    switch (txt_type) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5: return CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM;
        case 101: return CFG_USER_SAFETY_INFO_TYPE_SPEEDLIMIT;
        case 102: return CFG_USER_SAFETY_INFO_TYPE_SPEEDBUMP;
        case 103:
        case 106: return CFG_USER_SAFETY_INFO_TYPE_DANGER;
        case 104: return CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_TURN;
        case 105: return CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_INTERSECTION;
        default: return CFG_USER_SAFETY_INFO_TYPE_NONE;
    }
    return 0;
}

quint8 PGType2txtType(quint8 pg_type)
{
    switch (pg_type) {
     case CFG_USER_SAFETY_INFO_TYPE_SPEEDLIMIT: return 101;
     case CFG_USER_SAFETY_INFO_TYPE_SPEEDBUMP: return 102;
     case CFG_USER_SAFETY_INFO_TYPE_DANGER:
     case CFG_USER_SAFETY_INFO_TYPE_SCHOOLZONE:
     case CFG_USER_SAFETY_INFO_TYPE_WILDLIFE:
     case CFG_USER_SAFETY_INFO_TYPE_RAILWAY:
     case CFG_USER_SAFETY_INFO_TYPE_POLICE:
     case CFG_USER_SAFETY_INFO_TYPE_NONE: return 106;
     case CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM: return 1;
     case CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_INTERSECTION: return 105;
     case CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_TURN: return 104;

        default: return 106;
    }
    return 106;
}
