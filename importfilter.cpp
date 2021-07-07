#include "importfilter.h"

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QApplication>
#include <QDebug>

#include "../Lib/supportfunctions.h"
#include "../Lib/iniconfig.h"
#include "formatselect.h"
#include "filetypes.h"


extern bool gDebugEnabled ;

//////////////////////////////////////////////////////////////////////////
///
/// Record Class
///

Record::Record(QString ext, QString codec, QString command)
{
    dext = ext ;
    dcodec = codec ;
    dcommand = command ;
}

QString Record::ext()
{
    return dext ;
}

QString Record::codec()
{
    return dcodec ;
}

QString Record::command()
{
    return dcommand ;
}

bool Record::isEmpty()
{
    return dcommand.isEmpty() ;
}

////////////////////////////////////////////////////////////////////////
///
/// ImportFilter Class
///


ImportFilter::ImportFilter()
{
    conf.clear() ;

    Record x("", "", "") ;
    Record n(TXT.replace(".",""), "UTF-8", "-") ;
    Record e(ENC.replace(".",""), "UTF-8", "Z") ;
    Record b(BAK.replace(".",""), "UTF-8", "-") ;
    Record z(ENB.replace(".",""), "UTF-8", "Z") ;
    conf.append(x) ;
    conf.append(n) ;
    conf.append(e) ;
    conf.append(b) ;
    conf.append(z) ;
    enc=NULL ;
}

bool ImportFilter::init(IniConfig& ini, QString inipath, Encryption *enc)
{
    // Save Encryption
    this->enc = enc ;

    // Initialise import filter and Codec Dialog filter
    int numentries = ini.numSections() ;
    codecSelectDialog.addItem(" System Default Text Format ", "System") ;

    for (int i=0; i<numentries; i++) {
        QString section = ini.getSection(i) ;
        if (section.left(6).toLower().compare(QString("import"))==0) {
            QString ext = ini.get(section, QString("extension")) ;
            QString codec = ini.get(section, QString("codec")) ;
            QString cmd = ini.get(section, QString("command")) ;
            Record n(ext, codec, cmd) ;
            conf.append(n) ;

        } else if (section.left(4).toLower().compare("load")==0) {
            QString codec = ini.get(section, "codec") ;
            QString description = ini.get(section, "description") ;
            codecSelectDialog.addItem(description, codec) ;
        }
    }
    this->inipath = inipath ;
    return true ;
}

bool ImportFilter::registerTypes()
{
#ifdef  Q_OS_WIN
    // Register extension with operating system
    QString appexe = QApplication::applicationFilePath().replace("/","\\") ;

    QSettings appsettings("HKEY_CURRENT_USER\\Software\\Classes\\EasyNotepad\\shell\\open\\command", QSettings::NativeFormat);
    appsettings.setValue(".", QString("\"") + appexe + QString("\" \"%1\"")) ;

    QSettings iconsettings("HKEY_CURRENT_USER\\Software\\Classes\\EasyNotepad\\DefaultIcon", QSettings::NativeFormat);
    iconsettings.setValue(".", QString("\"") + appexe + QString("\"")) ;

    for (int i=0; i<conf.size(); i++) {
        // Register extension with operating system
        Record& rec = conf[i] ;
        QString ext = rec.ext() ;
        if (!ext.isEmpty()) {
            QSettings extsettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\.") + ext, QSettings::NativeFormat) ;
            extsettings.setValue(".", QString("EasyNotepad")) ;
        }
    }
    return true ;
#endif
}

Record& ImportFilter::FindFilter(QString filename)
{
    filename = filename.replace(".autosave","") ;
    QRegExp rx(".*\\.([^\\.]*)") ;
    if (rx.indexIn(filename)!=-1) {
        QString ext = rx.cap(1).toLower() ;
        for (int i=0, sz=conf.size(); i<sz; i++) {
            Record& thisrecord = (Record &)conf.at(i) ;
            if (ext.compare(thisrecord.ext())==0) return (Record &)conf.at(i) ;
        }
    }
    return (Record &)conf.at(0) ;
}


bool ImportFilter::LoadFile(QString filename, QString& contents, QString datadir)
{
    Record filter = FindFilter(filename) ;
    QString command = filter.command() ;
    QString codec = filter.codec() ;

    contents = "" ;

    if (command.compare("Z")==0 && enc) {

        // Open the Encrypted UTF-8 File

        int t=0 ;
        while (!enc->loggedIn() && t<3) {
            enc->login() ;
            t++ ;
        }
        if (!enc->loggedIn()) {
            contents = "Unable to login.  Please check your login password\n" ;
            return false ;
        }

        QByteArray text ;
        if (!enc->load(filename, text)) {
            contents = "Error loading file.  Did you have the correct password and key\n" ;
            return false ;
        } else {
            QTextCodec *codec = QTextCodec::codecForName("UTF-8");
            QTextDecoder *decoderWithoutBom = codec->makeDecoder( QTextCodec::IgnoreHeader ) ;
            contents = decoderWithoutBom->toUnicode(text) ;
            return true ;
        }

    }

    if (command.compare("-")==0) {

        // Open the file
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {

            contents = "Unable to open file: " + filename + "\n\n";
            return false ;

        } else {

            QByteArray filedata=file.readAll() ;
            file.close() ;

            // Attempt file load as UTF-8
            QTextCodec::ConverterState state;
            QTextCodec *c = QTextCodec::codecForName("UTF-8");
            contents = c->toUnicode(filedata.constData(), filedata.size(), &state);
            if (state.invalidChars > 0) {
                // Error loading, so ask for correct Codec
                codecSelectDialog.exec() ;
                QString altcodec = codecSelectDialog.getSelection() ;
                c = QTextCodec::codecForName(altcodec.toLatin1().data()) ;
                if (c==NULL) {
                    contents = "Unrecognised format, or unsupported / unknown codec in ini file\n\n" ;
                    return false ;
                } else {
                    contents = c->toUnicode(filedata.constData(), filedata.size(), &state);
                }
            }
        }
        return true ;

    } else if (command.isEmpty()) {

        // Error, No Appropriate Filter
        contents = "Unsupported file type for file: " + filename + "\n" ;
        contents = contents + "check that the import filter is correctly configured in the ini file\n" ;
        return false ;

    } else {

        // Run Program to Filter Input
        command = ExpandVars(command, filename, datadir) ;

        QProcess myProcess ;
        QStringList args = command.split( QRegularExpression(" (?=[^\"]*(\"[^\"]*\"[^\"]*)*$)") );
        for (int a=0; a<args.length(); a++) {
          QString entry = args.at(a) ;
          entry = entry.replace("\\\"", "***REALQUOTE***") ;
          entry = entry.replace("\"", "") ;
          entry = entry.replace("***REALQUOTE***", "\\\"") ;
          args.replace(a, entry) ;
        }
        QString cmd = args.at(0) ;
        args.removeAt(0) ;

        if (gDebugEnabled) {
            warningOkDialog(0, "Debug", QString("Executing: ") + command) ;
        }

        myProcess.start(cmd, args) ;
        myProcess.waitForFinished() ;
        QByteArray filedata = myProcess.readAllStandardOutput();
        QString error = myProcess.readAllStandardError() ;
//        QProcess::ExitStatus status = myProcess.exitStatus() ;
        myProcess.close() ;

        QTextCodec *c  ;

        // Convert filedata contents to correct codec

        if (codec.compare("auto")==0) {

            bool isutf8 = true ;
            for (int a=filedata.length()-1; a>0; a--) {
                unsigned char ch0 = filedata.at(a-1) ;
                unsigned char ch1 = filedata.at(a) ;
                unsigned char ch2 = filedata.at(a+1) ;
                if (ch1>127 && ch0<=127 && ch2<=127) isutf8=false ;
            }
            if (isutf8) {
                c=QTextCodec::codecForName("UTF8") ;
            } else {
                c=QTextCodec::codecForLocale() ;
            }

        } else {

            c=QTextCodec::codecForName(codec.toLatin1().data()) ;
            if (codec.isEmpty() || c==NULL) c=QTextCodec::codecForName("UTF8") ;

        }
        contents=c->toUnicode(filedata) ;

        if (!contents.isEmpty()) {
            return true ;
        } else {
            contents = "File is empty, or error running: " + command + "\n" + error ;
            return false ;
        }
    }
}

QString ImportFilter::ExpandVars(QString src, QString filepath, QString datadir)
{
    src = src.replace("$FILEPATH", filepath) ;
    src = src.replace("$APPDIR", QApplication::applicationDirPath()) ;
    src = src.replace("ENDIR", datadir) ;
    src = src.replace("$HOME", QStandardPaths::writableLocation(QStandardPaths::HomeLocation)) ;
    src = src.replace("$CONFIG", QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/TrumptonApps") ;
    src = src.replace("$INI", inipath) ;
    return src ;
}
