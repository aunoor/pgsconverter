#include <QApplication>
#include <QObject>
#include "mainwindow.h"
#include "globaldefs.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationVersion(VERSION);
    a.setApplicationName("PGSConverter");

#if (QT_VERSION < 0x050000)
      QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));
#endif

    QTranslator translator;
    translator.load(":/translation/qt_ru.qm");
    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
