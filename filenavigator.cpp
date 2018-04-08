#include <QDir>
#include <QRegExp>
#include <QDate>
#include <QApplication>
#include <QInputDialog>
#include "filetypes.h"
#include "filenavigator.h"
#include "../Lib/supportfunctions.h"
#include "../Lib/alertsound.h"

FileNavigator::FileNavigator(QWidget *parent) :
    QLineEdit(parent)
{
    folders.clear() ;
    files.clear() ;
    backups.clear() ;
    folderindex=0 ;
    fileindex=0 ;
    backupindex=0 ;
    selection=0 ;
    selectnew=false ;
    isbackup=false ;
    nofilename=true ;
    connect(this, SIGNAL(cursorPositionChanged(int, int)), this, SLOT(on_cursorPositionChanged(int, int))) ;

    update(0) ;
}

FileNavigator::~FileNavigator()
{
}

void FileNavigator::clearFilename()
{
    nofilename=true ;
}

void FileNavigator::setFilename(QString foldername, QString filename, QString backupname)
{
    QString folder ;
    QString file ;
    QString backup ;
    bool foundfolder=false ;
    bool foundfile=false ;

    nofilename=false ;
    folder="" ;
    file="" ;
    backup="" ;

    if (!foldername.isEmpty()) folder=foldername ;
    else if (folderindex>0) folder = folders[folderindex] ;

    if (!filename.isEmpty()) file=filename ;
    else if (folderindex>0 && fileindex>0) file = files[fileindex] ;

    if (!backupname.isEmpty()) backup=backupname ;
    else if (folderindex>0 && fileindex>0 && backupindex>=0) backup = backups[backupindex] ;

    folderindex=-1 ;

    loadFolders(directory) ;
    folderindex=0 ;
    for (int i=0, sz=folders.size(); i<sz; i++) {
        if (folders.at(i).compare(folder)==0) {
            folderindex=i ;
            foundfolder=true ;
        }
    }

    fileindex=-1 ;

    if (foundfolder) {
        loadFiles(directory + "/" + folders[folderindex], TXT) ;
        fileindex=0 ;
        for (int i=0, sz=files.size(); i<sz; i++) {
            if (files.at(i).compare(file)==0) {
                fileindex=i ;
                foundfile=true ;
            }
        }

    }

    backupindex=-1 ;

    if (foundfile) {
        loadBackups(directory + "/" + folders[folderindex] + "/" + files[fileindex], BAK) ;
        backupindex=-1 ;
        for (int i=0, sz=backups.size(); i<sz; i++) {
            if (backups.at(i).compare(backup)==0) {
                backupindex=i ;
            }
        }
    }

    if (backupindex>=0) isbackup=true ;
    else isbackup=false ;

    if (backupindex<0 && selection==2) selection=1 ;
    if (fileindex<0 && selection==1) selection=0 ;

    update(0) ;
}

void FileNavigator::setFilenameFromPath(QString path)
{
    QString folder, file, backup ;
    QRegExp filex(".*/([^/]*)/([^/]*)\\." + TXT) ;
    QRegExp backupx(".*/([^/]*)/([^/]*)/([^/]*)\\." + BAK) ;

    nofilename=false ;
    if (filex.indexIn(path)>=0) {
        folder = filex.cap(1) ;
        file = filex.cap(2) ;
        backup = "-" ;
    }
    if (backupx.indexIn(path)>=0) {
        folder = backupx.cap(1) ;
        file = backupx.cap(2) ;
        backup = backupx.cap(3) ;
    }
    setFilename(folder, file, backup) ;
}

void FileNavigator::update(int cursorpos)
{
    if (folders.size()==0) return ;

    static int reentrant=0 ;
    if (reentrant==1) return ;
    reentrant++ ;

    QString entry ;
    QString txt ;
    int s1=0, s2=0 ;

    entry = folders.at(folderindex)  ;
    entry.replace("_", "") ;
    txt=entry ;
    if (selection==0) s2=txt.length() ;

    if (folderindex>0 && fileindex>=0) {
        if (cursorpos>=txt.length()) selection=1 ;
        txt = txt + ": " ;
        if (selection==1) s1 = txt.length() ;
        entry = files.at(fileindex) ;
        entry.replace("_", "") ;
        txt = txt + entry ;
        if (selection==1) s2 = txt.length() ;
    }

    txt = txt + " " ;

    // TODO: Nasty Hack - backupindex=-1 on first visit
    if (backupindex<0 && backups.size()>0) backupindex=0 ;

    if (fileindex>0 && backupindex>=0) {
        if (cursorpos>=txt.length()) selection=2 ;
        QString backupdate ;
        if (backupindex==0) backupdate = "latest" ;
        else backupdate = parseBackupDate(backups.at(backupindex)) ;
        txt = txt + "(" ;
        if (selection==2) s1 = txt.length() ;
        txt = txt + backupdate ;
        if (selection==2) s2 = txt.length() ;
        txt = txt + ") " ;
    }

    this->setText(txt);

    this->setCursorPosition(s1) ;
#ifndef BEFOREQT54
    // There are some bugs with respect to the braille diplay
    // so we can't show the selection
    this->setSelection(s1, s2-s1) ;
#endif
    reentrant-- ;

}


void FileNavigator::setPath(QString directory)
{
    QDir dir ;
    nofilename=false ;
    this->directory = directory ;
    this->tempdirectory = directory + "/__temp__" ;
    dir.mkpath(this->directory) ;
    dir.mkpath(this->tempdirectory) ;
    loadFolders(this->directory) ;
    if (folderindex>=1) loadFiles(this->directory + "/" + folders.at(folderindex), TXT) ;
    if (fileindex>=1 && !isDeleteFolder()) loadBackups(this->directory + "/" + folders.at(folderindex) + "/" + files.at(fileindex), BAK) ;
    update() ;
}

QString& FileNavigator::getPath()
{
    return this->directory ;
}

QString& FileNavigator::getTempFolderPath()
{
    return this->tempdirectory ;
}

void FileNavigator::loadFolders(QString directory)
{
    parseDirectory(directory, folders, "") ;
    files.clear() ;
    fileindex=-1 ;
    backups.clear() ;
    backupindex=-1 ;

    // Find temp folder and remove it
    int idx=-1 ;
    for (int i=0; i<folders.length(); i++) {
        QString entry = folders.at(i) ;
        if (entry.compare("__temp__")==0) idx=i ;
    }
    if (idx>=0) folders.removeAt(idx) ;

    folders.insert(0, "-- create folder --") ;
    if (folders.size()==1) folderindex=0 ;
    else folderindex=1 ;
}

void FileNavigator::loadFiles(QString directory, QString mask)
{

    parseDirectory(directory, files, mask) ;

    backups.clear() ;
    backupindex=-1 ;

    files.insert(0, "-- create file --") ;
    files.insert(0, "-- rename folder --") ;
    if (files.size()==2) {
        files.insert(0, "-- delete folder --") ;
    }
    // Index is either first file, or --createfile--. Either way, it is index 2
    fileindex=2 ;
}

void FileNavigator::loadBackups(QString directory, QString mask)
{
    parseDirectory(directory, backups, mask, true) ;
    if (backups.size()>0) {
        backupindex=0 ;
    } else {
        backupindex=-1 ;
    }
}


QString& FileNavigator::getFolderPath(QString foldername)
{
    if (!foldername.isEmpty()) {
        folderPath = directory + "/" + foldername ;
    } else {
        folderPath = directory + "/" + folders.at(folderindex) ;
    }
    return folderPath ;
}

QString& FileNavigator::getEditableFilePath(QString filename)
{
    if (filename.isEmpty() && fileindex<0) {
        editableFilePath = "" ;
    } else {
        editableFilePath = getFolderPath() + "/" ;
        if (filename.isEmpty()) {
            editableFilePath = editableFilePath + files.at(fileindex) + "." + TXT ;
        } else {
            editableFilePath = editableFilePath + filename + "." + TXT ;
        }
    }
    return editableFilePath ;
}

QString& FileNavigator::getFilePath(QString filename, bool preferbackups)
{
    if (isDeleteFolder() || isRenameFolder()) {
        filePath = getFolderPath() ;

    } else if (isCreateFolder()) {
        filePath="" ;

    } else if (!filename.isEmpty()) {
            filePath = getFolderPath() + "/" + filename + "." + TXT ;

    } else {

        filePath = getFolderPath() ;
        if (backupindex>0 || preferbackups) {
            filePath = filePath + "/" + files.at(fileindex) + "/" + backups.at(backupindex) + "." + BAK ;
        } else {
            filePath = filePath + "/" + files.at(fileindex) + "." + TXT ;
        }

    }
    return filePath ;
}

QString& FileNavigator::getBackupFolderPath()
{
    backupFolderPath = "" ;
    if (!isCreateFolder() && !isCreateFile() && !isRenameFolder()) {
        backupFolderPath = getFolderPath() + "/" + files.at(fileindex) ;
    }
    return backupFolderPath ;
}

// return latest backup
// or path to today's backup (create)
QString& FileNavigator::getBackupFilePath(bool create)
{
    backupFilePath = "" ;

    backupFilePath = getBackupFolderPath() ;

    if (!backupFilePath.isEmpty()) {

        if (create) {
            QDate today = QDate::currentDate() ;
            backupFilePath = backupFilePath + "/" + today.toString("yyyyMMdd") + "." + BAK ;
        } else if (backupindex>=0) {
            backupFilePath = backupFilePath + "/" + backups.at(0) + "." + BAK ;
        } else {
            backupFilePath="" ;
        }
    }

    return backupFilePath ;
}

QString& FileNavigator::getFileName(bool includefolder, bool includebackupdate)
{
    fileName = "" ;

    if (nofilename) {

        fileName="<none>" ;

    } else {

        bool isdeletefolder = isDeleteFolder() ;
        bool isrenamefolder = isRenameFolder() ;

        if (includefolder) {
            if (folderindex>0) fileName = folders.at(folderindex) + ": " ;
        }

        if (folderindex>0 && !isrenamefolder && !isdeletefolder && fileindex>0) fileName = fileName + files.at(fileindex) ;

        if (includebackupdate) {
            QString backupdate ;
            if (backupindex<0) backupdate="" ;
            else if (backupindex==0) backupdate=" (latest)" ;
            else backupdate = " (" + parseBackupDate(backups.at(backupindex)) + ")" ;
            if (folderindex>0 && !isdeletefolder && !isrenamefolder && fileindex>0 && backupindex>=0) fileName = fileName + backupdate ;
        }

    }

    return fileName ;
}

bool FileNavigator::isDeleteFolder()
{
    if (fileindex>=files.size() || fileindex<0) return false ;
    return (files.at(fileindex).compare("-- delete folder --")==0) ;
}

bool FileNavigator::isCreateFolder()
{
   if (folderindex>=folders.size() || folderindex<0) return false ;
   return (folders.at(folderindex).compare("-- create folder --")==0) ;
}

bool FileNavigator::isCreateFile()
{
    if (fileindex>=files.size() || fileindex<0) return false ;
    return (files.at(fileindex).compare("-- create file --")==0) ;
}

bool FileNavigator::isRenameFolder()
{
    if (fileindex>=files.size() || fileindex<0) return false ;
    return (files.at(fileindex).compare("-- rename folder --")==0) ;
}


bool FileNavigator::isBackup()
{
    return (backupindex>0) ;
}

void FileNavigator::on_cursorPositionChanged(int from, int to)
{
    Q_UNUSED(from) ;

    // Nasty hack: the default is to leave the cursor at the end of the text
    // when the text is first added
    if (to>=text().length()-2) to=0 ;
    update(to) ;

}

void FileNavigator::keyPressEvent(QKeyEvent *event)
{
    bool changed=false, reloadfiles=false, reloadbackups=false ;

    int key = event->key() ;

    if (key==Qt::Key_Enter || key==Qt::Key_Return || key==Qt::Key_Escape) {
        QLineEdit::keyPressEvent(event);
        return ;
    }
    else if (key==Qt::Key_Left) {
        selection-- ;
        if (selection<0) selection=0 ;
        else changed=true ;
    }
    else if (key==Qt::Key_Right) {
        selection++;
        if (selection>2) selection=2 ;
        if (selection>1 && backups.size()<=0) selection=1 ;
        if (selection>0 && files.size()<=0) selection=0 ;
        changed=true ;
    }
    else if (key==Qt::Key_Up) {
        switch (selection) {
        case 0: folderindex-- ;
                if (folderindex<0) folderindex=0 ;
                else { reloadfiles=true ; reloadbackups=true ; }
                break ;
        case 1: fileindex-- ;
                if (fileindex<0) fileindex=0 ;
                else { reloadbackups=true ; }
                break ;
        case 2: backupindex-- ;
                if (backupindex<0) backupindex=0 ;
                break ;
        }
        changed=true ;
    }
    else if (key==Qt::Key_Down) {
        switch (selection) {
        case 0: folderindex++ ;
                if (folderindex>=folders.size()) folderindex=folders.size()-1 ;
                else { reloadfiles=true ; reloadbackups=true ; }
                break ;
        case 1: fileindex++ ;
                if (fileindex>=files.size()) fileindex=files.size()-1 ;
                else { reloadbackups=true ; }
            break ;
        case 2: backupindex++ ;
                if (backupindex>=backups.size()) backupindex=backups.size()-1 ;
                break ;
        }
        changed=true ;
    }
    else if (key==Qt::Key_Minus || (key>=Qt::Key_A && key<=Qt::Key_Z) || (key>=Qt::Key_0 && key<=Qt::Key_9)) {
        QChar letter ;
        if (key==Qt::Key_Minus) letter = '-' ;
        else if (key>=Qt::Key_A && key<=Qt::Key_Z) letter = ('a' + key - Qt::Key_A) ;
        else letter = ('0' + key - Qt::Key_0) ;

        switch (selection) {
        case 0:
            if (folders.size()<=0) {
                play(Error) ;
            } else {

                int currentindex=folderindex ;
                bool finished=false ;

                while (!finished) {
                    currentindex++ ;
                    if (currentindex>=folders.size()) currentindex=0 ;
                    QChar ch = folders.at(currentindex).at(0).toLower() ;
                    if (ch==letter) {
                        finished=true ;
                        reloadfiles=true ;
                        changed=true ;
                    }
                    if (currentindex==folderindex) {
                        finished=true ;
                    }
                }
                if (!changed) {
                    play(Error) ;
                } else if (currentindex>folderindex) {
                    play(Found) ;
                } else {
                    play(Wrapped) ;
                }
                folderindex=currentindex ;
            }
            break ;
        case 1:
            if (files.size()<=0) {
                play(Error) ;
            } else {

                int currentindex=fileindex ;
                bool finished=false ;

                while (!finished) {
                    currentindex++ ;
                    if (currentindex>=files.size()) currentindex=0 ;
                    QChar ch = files.at(currentindex).at(0).toLower() ;
                    if (ch==letter) {
                        finished=true ;
                        reloadbackups=true ;
                        changed=true ;
                    }
                    if (currentindex==fileindex) {
                        finished=true ;
                    }
                }
                if (!changed) {
                    play(Error) ;
                } else if (currentindex>fileindex) {
                    play(Found) ;
                } else {
                    play(Wrapped) ;
                }
                fileindex=currentindex ;
            }
            break ;
        case 2:
            play(Error) ;
            break ;
        }
    }

    if (!isCreateFolder() && reloadfiles) {
        loadFiles(getFolderPath(), TXT) ;
    }

    if (!isCreateFolder() && !isCreateFile() && !isRenameFolder() && !isDeleteFolder() && reloadbackups)
        loadBackups(getFolderPath() + "/" + files.at(fileindex), BAK) ;

    if (changed) update() ;

}

