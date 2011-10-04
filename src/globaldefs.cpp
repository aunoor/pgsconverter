#include "globaldefs.h"

void pntToRawPnt(favPoints_t &pnt, favRecord_t *rawPnt)
{
    qMemSet((void*)rawPnt->head,0,sizeof(favRecord_t));
    rawPnt->pntType=0L;
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    rawPnt->lat=pnt.lat*100000;
    rawPnt->lon=pnt.lon*100000;
    QByteArray ba = codec->fromUnicode(QString(pnt.name));
    rawPnt->pntType = pnt.pntType;
    rawPnt->iconNum = pnt.iconNum;

    //Из-за каких то глюков Qt, либо я что-то не осилил, но приходиться копировать байты руками...
    for (int i=0;i<(ba.size()>0x100?0x100:ba.size()-2);i++) {
        rawPnt->name[i] = ba.at(i+2);
    }

    ba =  codec->fromUnicode(QString(pnt.desc));

    for (int i=0;i<(ba.size()>0x100?0x100:ba.size()-2);i++) {
        rawPnt->desc[i] = ba.at(i+2);
    }
}

void addRawPointToPointList(favRecord_t &favRawPoint, FavPointsList &list) {
    list.append(trRawPointToPoint(favRawPoint));
}

favPoints_t trRawPointToPoint(favRecord_t &favRawPoint) {
    favPoints_t point;
    point.lat = favRawPoint.lat*.00001;
    point.lon = favRawPoint.lon*.00001;
    point.pntType = favRawPoint.pntType;

    point.desc=QString::fromUtf16((unsigned short*)favRawPoint.desc,0x100);
    point.desc.truncate(point.desc.indexOf(QChar('\0')));

    point.name=QString::fromUtf16((unsigned short*)favRawPoint.name,0x100);
    point.name.truncate(point.name.indexOf(QChar('\0')));
    point.checked = true;
    point.uuid = QUuid::createUuid();
    point.iconNum = favRawPoint.iconNum;
    return point;
}

