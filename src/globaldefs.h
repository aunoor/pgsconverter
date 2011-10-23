#ifndef GLOBALDEFS_H
#define GLOBALDEFS_H

#include <QtCore>

#define VERSION "v1.0.5"

#define MIME_RAW_POINT_TYPE "application/x-rawsafepointlist"

enum CFG_USER_SAFETY_INFO
{
   CFG_USER_SAFETY_INFO_TYPE_NONE               = 1,
   CFG_USER_SAFETY_INFO_TYPE_SPEEDLIMIT,
   CFG_USER_SAFETY_INFO_TYPE_SCHOOLZONE,
   CFG_USER_SAFETY_INFO_TYPE_DANGER,
   CFG_USER_SAFETY_INFO_TYPE_SPEEDBUMP,
   CFG_USER_SAFETY_INFO_TYPE_WILDLIFE,
   CFG_USER_SAFETY_INFO_TYPE_RAILWAY,
   CFG_USER_SAFETY_INFO_TYPE_POLICE,
   CFG_USER_SAFETY_INFO_TYPE_SPEEDCAM,
   CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_INTERSECTION,
   CFG_USER_SAFETY_INFO_TYPE_DANGEROUS_TURN
};

enum SPEED_CAM_TYPES {
    SPEED_CAM_MOBILE = 5, //- 5 - мобильная
    SPEED_CAM_SPEEDCAM = 1, // - стационарная
    SPEED_CAM_REDLIGHT = 3, // - светофор
    SPEED_CAM_REDLIGHT_CAM = 2, // - встроенная в светофор
    SPEED_CAM_SPEEDCAM_TWIN = 4// - измеряющая скорость на участке
};

#pragma pack(push,1)
struct CFG_HEADER
{
   quint8 nMajorVersion;
   quint8 nMinorVersion;
   quint16 nFieldCount;
   quint8 reserved[2];
};
#pragma pack(pop)

typedef struct SafeRecord_V1
{
   qint32         pos_x;
   qint32         pos_y;
   unsigned char   type;
   unsigned char   speed;
   char         name[128];
}safeRecordV1_t;

typedef struct SafePoint {
    QString idx; //индекс. может быть любым
    double lon;//X - долгота
    double lat;//Y - широта
    quint8 pntType;
    quint8 speed;
    quint8 dirtype;
    quint8 direction;
    bool checked;
    quint16 iconNum;
    QUuid uuid;
    QString name;
}safePoint_t;

Q_DECLARE_METATYPE(safePoint_t)

typedef QList<safePoint_t> SafePointsList;

void pntToRawPnt(safePoint_t &pnt, safeRecordV1_t *rawPnt);
safePoint_t trRawPointToPoint(safeRecordV1_t &safeRawPoint);
void addRawPointToPointList(safeRecordV1_t &safeRawPoint, SafePointsList &list);
quint8 txtType2PGType(quint8 txt_type);
quint8 PGType2txtType(quint8 pg_type);

#endif // GLOBALDEFS_H
