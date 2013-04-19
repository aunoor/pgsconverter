#ifndef SAFE_BIN_H
#define SAFE_BIN_H

#include <QtCore>
#include "globaldefs.h"

struct TPOINT
{
   double X, Y;
};
//Долгота(longitude)(X) и широта(latitude)(Y) (в этом порядке), умноженная на 1.0e5.

#pragma pack(push,1)
enum
{
   SAFETY_LINKS=2
};

typedef quint16 SAFETY_LINK;

typedef struct SAFETY_RECORD
{
   quint8  Type;
   quint8  speed;
   quint16 form;
   quint8  reserved[4];
   struct TPOINT pos;
   SAFETY_LINK links[SAFETY_LINKS];
} SystemSafeRecord_V2_t;
#pragma pack(pop)

#pragma pack(push,4)
struct SAFETY_CACHE_HEADER_V2
{
   char tag[16]; // "PROGOROD Safety"
   quint8  nVersionMajor; // ==2
   quint8  nVersionMinor; // 0
   quint16 nRecordSize; // == sizeof(SAFETY_RECORD)
   quint16 nIndexBits;   // не важно
   quint32 nSafetyCnt; // количество SAFETY_RECORD в файле
   quint32 nSafetyOffset; // смещение от начала файла первой SAFETY_RECORD
   quint8  hash[16];   // для проверки целостности
   quint8  map_hash[16];  // для проверки соответствия картам
};
#pragma pack(pop)

/*
Поле form - упакованные угол обзора и направление:
Угол обзора: (form%90)*2 (0-178 градусов)
Направление: (form/90)%360 (0-359 градусов)
*/


safePoint_t trSystemRawPointToPoint(SystemSafeRecord_V2_t &safeRawPoint);
bool loadSystemSafeRecords(QString fileName, SafePointsList &list, bool verbose=false);

#endif // SAFE_BIN_H
