#include <QObject>
#include <QFile>
#include <QMessageBox>
#include <cstring>
#include "safe_bin.h"

void addSystemRawPointToPointList(SystemSafeRecord_V2_t &safeRawPoint, SafePointsList &list) {
    list.append(trSystemRawPointToPoint(safeRawPoint));
}

bool loadSystemSafeRecords(QString fileName, SafePointsList &list, bool verbose)
{
    QFile file(fileName);
    if (!file.exists()) {
        if (verbose) QMessageBox::critical(0,QObject::tr("Ошибка"), QObject::tr("Файл не найден."));
        return false;
    }

    bool res = file.open(QIODevice::ReadOnly);

    if (!res) {
        if (verbose) QMessageBox::critical(0,QObject::tr("Ошибка"), QObject::tr("Ошибка при открытии файла."));
        return false;
    }

    list.clear();
    struct SAFETY_CACHE_HEADER_V2 header;
    file.read((char *)&header,sizeof(header));

    if (header.nVersionMajor!=2) {
        if (verbose) QMessageBox::critical(0,QObject::tr("Ошибка"), QObject::tr("Неизвестная версия файла."));
        return false;
    }

    res=file.seek(header.nSafetyOffset);
    if (!res) {
        if (verbose) QMessageBox::critical(0,QObject::tr("Ошибка"), QObject::tr("Ошибка при открытии файла."));
        return false;
    }

    for (uint i=0;i<header.nSafetyCnt;i++)
    {
        QByteArray ba = file.read(header.nRecordSize);
        if (ba.size()!=header.nRecordSize) {
            if (verbose) QMessageBox::critical(0,QObject::tr("Ошибка"), QObject::tr("Size of readed data != point data size!"));
            file.close();
            return false;
        }
        SystemSafeRecord_V2_t safeRecord;
        std::memcpy(&safeRecord,ba.data(),sizeof(SystemSafeRecord_V2_t));

        addSystemRawPointToPointList(safeRecord, list);
    }//for

    file.close();
    return true;
}

safePoint_t trSystemRawPointToPoint(SystemSafeRecord_V2_t &safeRawPoint) {
    safePoint_t point;
    point.lat = safeRawPoint.pos.X*.00001;
    point.lon = safeRawPoint.pos.Y*.00001;
    point.pntType = safeRawPoint.Type;
    point.name=QString();
    point.checked = true;
    QUuid uuid = QUuid::createUuid();
    point.idx = uuid.toString().left(6);
    point.uuid = uuid;
    point.speed = safeRawPoint.speed;
    return point;
}



