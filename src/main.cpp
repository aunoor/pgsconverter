#include <QtGui>
#include <QObject>
#include "mainwindow.h"
#include "globaldefs.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationVersion(VERSION);
    a.setApplicationName("PGSConverter");

    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

    QTranslator translator;
    translator.load(":/translation/qt_ru.qm");
    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
