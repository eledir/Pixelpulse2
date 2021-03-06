/// simple implementation of bossac flashing and verification

#ifndef BOSSAC_H
#define BOSSAC_H

#include <QProcess>
#include <QObject>
#include <QTextStream>
#include <QTimer>
#include <QFileInfo>
#include <QApplication>
#include <stdlib.h>
#include <QStandardPaths>
#include <QDir>

#if defined(Q_OS_MAC)
#include "CoreFoundation/CoreFoundation.h"
#endif

#include <QDebug>

class BossacWrapper: public QObject
{
    Q_OBJECT

private:

QString getEnvVar(const QString varName)
{
    return getenv(varName.toStdString().c_str());
}

public slots:

    bool checkFileExists(const QString& filepath)
    {
        QFileInfo fInfo(filepath);

        return fInfo.exists();
    }

    QString getTmpPathForFirmware()
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        path += "/pixelpulse2/m1k_firmware";
        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(".");
       return dir.path();
    }

    QString getBossacPath() {
	QString current_path = QCoreApplication::applicationDirPath() + "/";

    #ifdef Q_OS_LINUX
        return (current_path + "bossac");
    #elif defined(Q_OS_WIN32)
        return (current_path + "bossac.exe");
    #elif defined(Q_OS_MAC)
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        char path[4096];
        if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, 4096))
        {
                // error!
        }
        CFRelease(resourcesURL);
        QString s((const char *)path);
        s.append("/bossac");
        return s;
    #else
        #error "We don't support this platform yet."
    #endif
    }

    QString flashByFilename(const QString& image) {
        QTimer timer;
        bool devReadyToProg = false;
        bool timeout = false;
        QString ret = "";

        if (!checkFileExists(image)) {
            ret = "Cannot find " + image + " file.";
            return ret;
        }

        // At this point device is set in programming mode. Make sure bossac sees it before
        // attempting to flash it.
        timer.setInterval(10000);
        timer.start();
        while (!devReadyToProg && !timeout) {
            QString output = deviceInformation();
            qDebug() << output;
            if (output.contains("Device found", Qt::CaseInsensitive)) {
                devReadyToProg = true;
            } else {
                if (timer.remainingTime() == 0) {
                    timeout = true;
                    ret = "The process of programming of the device has timeout!";
                }
            }
        }
        if (timeout)
          return ret;

        // Flash the device
        QProcess bossacThread;
        QString program = getBossacPath();
        QStringList arguments;
        arguments << "-e" /*erase*/ <<  "-w" /*write*/ <<  "-v" /*verify*/ << "-b" /*boot into flash*/ << image;
        bossacThread.setProcessChannelMode(QProcess::MergedChannels);

        if (devReadyToProg) {
            bossacThread.start(program, arguments);
            if (bossacThread.waitForFinished()) {
                qDebug() << bossacThread.readAllStandardOutput();
            } else {
                ret = "Failed to run the bossac process.";
            }
        }

        return ret;
    }

    QString deviceInformation()
    {
        QProcess bossacThread;
        QString program = getBossacPath();
        QStringList arguments;
        arguments << "-i" /*info*/;

        bossacThread.setProcessChannelMode(QProcess::MergedChannels);
        bossacThread.start(program, arguments);
        bossacThread.waitForFinished();

        return bossacThread.readAllStandardOutput();
    }

public:
    BossacWrapper() {}
};

#endif // BOSSAC_H
