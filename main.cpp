#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

QFile debugFile;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream logStream(&debugFile);
    logStream << msg << Qt::endl;

    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    debugFile.setFileName("debug_" + QString::number(QDateTime::currentSecsSinceEpoch()) + ".txt");
    debugFile.open(QIODevice::ReadWrite | QIODevice::Text);

    qInstallMessageHandler(myMessageOutput);
    MainWindow w;
    w.show();
    auto ret = a.exec();
    debugFile.close();
    return ret;
}
