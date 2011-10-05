#include "globaldefs.h"

void pntToRawPnt(safePoints_t &pnt, safeRecordV1_t *rawPnt)
{
    qMemSet((void*)rawPnt->pos_x,0,sizeof(safeRecordV1_t));
    rawPnt->type=1L;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    rawPnt->pos_x=pnt.lat*100000;
    rawPnt->pos_y=pnt.lon*100000;
    QByteArray ba = codec->fromUnicode(QString(pnt.name));
    rawPnt->type = pnt.pntType;
    rawPnt->speed = pnt.speed;

    //Из-за каких то глюков Qt, либо я что-то не осилил, но приходиться копировать байты руками...
    for (int i=0;i<(ba.size()>128?128:ba.size()-2);i++) {
        rawPnt->name[i] = ba.at(i+2);
    }
}

void addRawPointToPointList(safeRecordV1_t &safeRawPoint, SafePointsList &list) {
    list.append(trRawPointToPoint(safeRawPoint));
}

safePoints_t trRawPointToPoint(safeRecordV1_t &safeRawPoint) {
    safePoints_t point;
    point.lat = safeRawPoint.pos_x*.00001;
    point.lon = safeRawPoint.pos_y*.00001;
    point.pntType = safeRawPoint.type;

    //point.name=QString::fromUtf16((unsigned short*)safeRawPoint.name,0x100);
    //point.name.truncate(point.name.indexOf(QChar('\0')));
    point.checked = true;
    point.idx = QUuid::createUuid().toString().left(6);
    point.speed = safeRawPoint.speed;
    return point;
}

