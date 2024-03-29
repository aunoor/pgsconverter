#include <cstring>
#include "globaldefs.h"

app_settings_t app_settings;

void pntToRawPnt(safePoint_t &pnt, safeRecordV1_t *rawPnt)
{
    //преобразуем точку из нашего формата в формат ПГ
    std::memset((void*)rawPnt,0,sizeof(safeRecordV1_t));

    rawPnt->pos_y=qRound(pnt.lat*100000);
    rawPnt->pos_x=qRound(pnt.lon*100000);

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
    point.lat = safeRawPoint.pos_y*.00001;
    point.lon = safeRawPoint.pos_x*.00001;
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


quint8 txtType2PGType(quint8 txt_type, bool u2o)
{

/*
Расширенные iGO


201 ДИ "Children" "Дети" (без направления и скорости), обозначает места скопления пешеходов, детей и всего живого
203 ДИ "Tunnel" "Тоннель"
204 ДИ "Dangerous way" "Опасный участок" (соответствует знаку 1.33 "Прочие опасности ") озвучка: "Впереди опасный участок" Используется для предупреждения о ямах, плохой дороге, ремработах).
205 ДИ "POI" "Объект ПОИ" (без скорости и без направления) озвучка: "Впереди интересующий Вас объект ПОИ"
207 - Плохая дорога (Тоже не поддерживается, можно в прочие опасности занести)

  */

    switch (txt_type) {
        case 1: //01 (192) КД "Static" "Стационарная камера"
        case 2: //02 (194) КД "Redlight" "Камера, встроенная в светофор"
        case 3: //03 (68) КД "Redlight Only" "Контроль проезда на красный свет" (нет фиксации скорости)
        case 4: //04 (227) КД "Section" "Измерение скорости на участке"
        case 5: //05 (193) КД "Mobile" "Мобильная засада" (нет фиксации скорости)
        //ниже новые коды iGO для камер
        case 192:
        case 193:
        case 194:
        case 227:
        case 68:
        case 8: //08 КД "FixMobil" "Мобильная засада"
        case 199: //199 КД "FixMobil" "Мобильная засада" (с фиксацией скорости, по сути альтернатива КД 05 "Mobile")
                return CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM;
        //расширенные коды навител и iGO с комментариями
        case 101: return CFG_USER_SAFETY_INFO_TYPE_SPEEDLIMIT;

        case 102:
        case 200: //200 ДИ "Speed Breaker" "Лежкоп" (соответствует знаку 1.17 "Искусственная неровность") озвучка: "Впереди Лежачий коп"
                 return CFG_USER_SAFETY_INFO_TYPE_SPEEDBUMP;
        case 103:
        case 106: return CFG_USER_SAFETY_INFO_TYPE_DANGER;

        case 104:
        case 202: //202 ДИ "Dangerous turn" "Опасный поворот" (без скорости) (соответствует знаку 1.12.1 "Опасные повороты" озвучка: "Впереди опасный поворот"
                  return CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_TURN;
        case 198: //198 ДИ "Give Way" "Уступите дорогу" (без скорости) (соответствует знаку 2.4 "Уступите дорогу") озвучка: "Уступите дорогу"
        case 105: return CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_INTERSECTION;
        case 6: //06 ДИ "Railroad" "Переезд" (без скорости, всегда оба направления)
        case 197: return CFG_USER_SAFETY_INFO_TYPE_RAILWAY;

        case 15: //15 КД "RPS Post" "ДПС"
        case 206: //206 КД "RPS Post" "ДПС" озвучка: "Впереди пост ДПС"
                  return CFG_USER_SAFETY_INFO_TYPE_POLICE;
        case 9: //09 ДИ "Dangerous" "Опасность" (лежачие, опасные перекрестки, плохая дорога и тп)
                return CFG_USER_SAFETY_INFO_TYPE_DANGER;
        case 201: //201 ДИ "Children" "Дети" (без направления и скорости), обозначает места скопления пешеходов, детей и всего живого
                return CFG_USER_SAFETY_INFO_TYPE_SCHOOLZONE;
        default: if (u2o) return CFG_USER_SAFETY_INFO_TYPE_DANGER; else return CFG_USER_SAFETY_INFO_TYPE_NONE;
    }
    return 0;
}

quint8 PGType2txtType(quint8 pg_type)
{
    switch (pg_type) {
     case CFG_USER_SAFETY_INFO_TYPE_SPEEDLIMIT: return 101;
     case CFG_USER_SAFETY_INFO_TYPE_SPEEDBUMP: return 102;
     case CFG_USER_SAFETY_INFO_TYPE_DANGER:
     case CFG_USER_SAFETY_INFO_TYPE_WILDLIFE:
     case CFG_USER_SAFETY_INFO_TYPE_RAILWAY:
     case CFG_USER_SAFETY_INFO_TYPE_POLICE:
     case CFG_USER_SAFETY_INFO_TYPE_NONE: return 106;
     case CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM: return 1;
     case CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_INTERSECTION: return 105;
     case CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_TURN: return 104;
     case CFG_USER_SAFETY_INFO_TYPE_PASSWAY:
     case CFG_USER_SAFETY_INFO_TYPE_SCHOOLZONE: return 201;

        default: return 106;
    }
    return 106;
}

bool isCameraPointType(safePoint_t &point)
{
    if (point.pntType == CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM) return true;
    return false;
}

/*
 * param point_coords Координаты, проверяемые на попадание в квадрат
 * param area_center_coords Центр квадрата
 * param area_size Размер квадрата в метрах
 * return false - если точки разного типа или не сравниваемая точка на попадает в область или true если точка того же типа находиться рядом со сравниваемой точкой
 */
bool compareCoordsByArea(const safePoint_t &point_coords, const safePoint_t &area_center_coords, quint32 area_size, bool typeCheck) {

    if (typeCheck)
        if (point_coords.pntType != area_center_coords.pntType) return false;

//    qDebug() << "lat scale = " << area_size*LATSCALE;
//    qDebug() << "lon scale = " << area_size*LONGSCALE;
/*
    if ((point_coords.lat < area_center_coords.lat-area_size*LATSCALE) ||
        (point_coords.lat > area_center_coords.lat+area_size*LATSCALE)) return false;

    if ((point_coords.lon < area_center_coords.lon-area_size*LONGSCALE) ||
        (point_coords.lon > area_center_coords.lon+area_size*LONGSCALE)) return false;
*/
    double d = distanceBeetweenPoints(point_coords, area_center_coords);


    //if (d<100)
    //qDebug() << QString::number(area_center_coords.lat).left(5);
    //if (QString::number(area_center_coords.lat,'g',8).left(8) == "55.70117")
     //       qDebug() << "distance=" << d;

    if (d>area_size) return false;
#if 0
    qDebug() << "point:";
    point_coords.print();

    qDebug() << "sc_point:";
    area_center_coords.print();

    qDebug() << "distance=" << d;
#endif
//7603, 7540
//55,70109, 37,60762 - user
//55,70117, 37,60776 - sc

//7113 - камера контроля полосы
    return true;
}

double distanceBeetweenPoints(const safePoint_t &point_coords, const safePoint_t &area_center_coords) {

    double lat1 = point_coords.lat * (M_PI/180);
    double lat2 = area_center_coords.lat * (M_PI/180);

    double lon1 = point_coords.lon * (M_PI/180);
    double lon2 = area_center_coords.lon * (M_PI/180);

    double d=2*asin(sqrt(pow((sin((lat1-lat2)/2)),2) + cos(lat1)*cos(lat2)*pow((sin((lon1-lon2)/2)),2)));
    //double d = acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon1-lon2));
    d *= 6372.795 *1000;
    return d;

/*
cos(d) = sin(φА)·sin(φB) + cos(φА)·cos(φB)·cos(λА − λB),

где φА и φB — широты, λА, λB — долготы данных пунктов, d — расстояние между пунктами, измеряемое в радианах длиной дуги большого круга земного шара.
Расстояние между пунктами, измеряемое в километрах, определяется по формуле:

L = d·R,
  */

}
