#include "globaldefs.h"

void pntToRawPnt(safePoint_t &pnt, safeRecordV1_t *rawPnt)
{
    //преобразуем точку из нашего формата в формат ПГ
    qMemSet((void*)rawPnt,0,sizeof(safeRecordV1_t));

    rawPnt->pos_x=pnt.lat*100000;
    rawPnt->pos_y=pnt.lon*100000;

    switch (pnt.pntType) {
        case SPEED_CAM_MOBILE:
        case SPEED_CAM_SPEEDCAM:
        case SPEED_CAM_SPEEDCAM_TWIN:
        case SPEED_CAM_REDLIGHT_CAM:
            rawPnt->type = CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM; break;
        case SPEED_CAM_REDLIGHT:
            rawPnt->type = CFG_USER_SAFETY_INFO_TYPE_DANGER; break;
        default: rawPnt->type = CFG_USER_SAFETY_INFO_TYPE_DANGER;
    }
    rawPnt->speed = pnt.speed;

    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    QByteArray ba = codec->fromUnicode(QString(pnt.name));
    //Из-за каких то глюков Qt, либо я что-то не осилил, но приходиться копировать байты руками...
    for (int i=0;i<(ba.size()>128?128:ba.size()-2);i++) {
        rawPnt->name[i] = ba.at(i+2);
    }
}

void addRawPointToPointList(safeRecordV1_t &safeRawPoint, SafePointsList &list) {
    list.append(trRawPointToPoint(safeRawPoint));
}

safePoint_t trRawPointToPoint(safeRecordV1_t &safeRawPoint) {
    safePoint_t point;
    point.lat = safeRawPoint.pos_x*.00001;
    point.lon = safeRawPoint.pos_y*.00001;

    switch (safeRawPoint.type) {
        case CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM: point.pntType = SPEED_CAM_SPEEDCAM; break;
        default: point.pntType = 0;
    }

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

