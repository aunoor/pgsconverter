#ifndef GLOBALDEFS_H
#define GLOBALDEFS_H

#include <QtCore>

#define VERSION "v1.0.0"

#define MIME_RAW_POINT_TYPE "application/x-rawpointlist"

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

typedef struct SafePoints {
    QString idx; //индекс. может быть любым
    double lon;
    double lat;
    quint8 pntType;
    quint8 speed;
    quint8 dirtype;
    quint8 direction;
    bool checked;
    quint16 iconNum;
    QUuid uuid;
    QString name;
}safePoints_t;

Q_DECLARE_METATYPE(safePoints_t)

typedef QList<safePoints_t> SafePointsList;

void pntToRawPnt(safePoints_t &pnt, safeRecordV1_t *rawPnt);
safePoints_t trRawPointToPoint(safeRecordV1_t &safeRawPoint);
void addRawPointToPointList(safeRecordV1_t &safeRawPoint, SafePointsList &list);

#endif // GLOBALDEFS_H
