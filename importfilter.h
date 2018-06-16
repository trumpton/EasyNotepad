#ifndef IMPORTFILTER_H
#define IMPORTFILTER_H

#include <QList>
#include <QString>
#include <QTextCodec>

#include "formatselect.h"
#include "importfilter.h"
#include "../Lib/iniconfig.h"
#include "encryption.h"


class Record {
private:
    QString dext, dcodec, dcommand ;

public:
    Record(QString ext, QString codec, QString command) ;
    bool isEmpty() ;
    QString ext() ;
    QString codec() ;
    QString command() ;
};

class ImportFilter
{
private:
    Encryption *enc ;
    QList <Record> conf ;
    FormatSelect codecSelectDialog ;
    Record& FindFilter(QString filename) ;
    QStringList parseCombinedArgString(const QString &program) ;
    QString inipath ;
    QString ExpandVars(QString src, QString filepath = QString(""), QString datadir = QString("")) ;

public:
    ImportFilter();
    bool init(IniConfig ini, QString inipath, Encryption *enc) ;
    bool LoadFile(QString filename, QString& contents, QString datadir = QString(".")) ;


};

#endif // IMPORTFILTER_H
