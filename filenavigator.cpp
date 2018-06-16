//
// FileNavigator
//

// BACKGROUND
// ----------
//
// This function managed a file navigation widget, which is a line-edit style input
// with the text split into three distinct fields:
//
//  FOLDER : FILE : BACKUP
//
// Folders can contain plaintext or encrypted files.  Folders with encrypted files have a .enc extension.
// File extensions are .txt or .enctxt depending on whether they are encrypted or not.
// Backup file extensions are .bak or .encbak, depending on whether they are encrypted or not.
//

// CLASS VARIABLES
// ---------------
//
// directory     - Location of the save files
// tempdirectory - temporary directory, used for imports / exports
// folders       - menu list of the current folders
// files         - menu list of the files in the selected folders (if any)
// backups       - menu list of the backup files for the selected file (if any)
//

// EXAMPLE USE
// -----------
//
// FileNavigator nav ;          // Create Navigator
// nav.setPath("/home/data") ;  // Initialise and define storage area
// if (nav.exec()==0) return ;  // Get the user input
// if (nav.isRenameFolder()) {
//   QString& folder = nav.currentFolderPath() ;
//   // Input new folder name and rename
// } else if (nav.isDeleteFolder()) {
//   QString& folder = nav.currentFolderPath() ;
//   // Confirm and delete folder
// } else if (nav.isCreateFolder()) {
//   // Input and create new folder
// } else if (nav.isCreateEncryptedFolder()) {
//   // Input and create encrypted folder
// } else if (nav.isCreateFile()) {
//   // Input and create a dummy file
// }



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

//====================================================
//
// Set the storage path for all saved files
//

void FileNavigator::setPath(QString directory)
{
    QDir dir ;
    nofilename=false ;
    this->directory = directory ;
    this->tempdirectory = directory + "/__temp__" ;
    dir.mkpath(this->directory) ;
    dir.mkpath(this->tempdirectory) ;

    // Load Folders
    setFilename() ;

    if (folders.size()>2) {

        folderindex=2 ;

        // Load Files
        setFilename() ;

        if (files.size()>2) {

            fileindex=2 ;

            // Load Backups
            setFilename() ;

        } else {

            // Leave on Create File
            fileindex=1 ;
        }
    }

    update(0) ;
}


//====================================================
//
// Clear and Set the Filename
//

void FileNavigator::clearFilename()
{
    nofilename=true ;
}

//
// Re-scan the folders, and set the current folder/file/backup indexes
//

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

    // Capture the names of the current folder/file/backup

    if (!foldername.isEmpty()) folder=foldername ;
    else if (folderindex>0) folder = folders[folderindex] ;

    if (!filename.isEmpty()) file=filename ;
    else if (folderindex>0 && fileindex>0) file = files[fileindex] ;

    if (!backupname.isEmpty()) backup=backupname ;
    else if (folderindex>0 && fileindex>0 && backupindex>=0) backup = backups[backupindex] ;


    // load folders and search for folder by name

    loadFolders(directory) ;
    foundfolder=false ;
    for (int i=0, sz=folders.size(); i<sz && !foundfolder; i++) {
        if (folders.at(i).compare(folder)==0) {
            folderindex=i ;
            foundfolder=true ;
        }
    }

    // Load files and seach for file by name (without extension)

    if (foundfolder) {
        loadFiles(directory + "/" + folders[folderindex], TXT, ENC) ;
        foundfile=false ;
        for (int i=0, sz=files.size(); i<sz && !foundfile; i++) {
            if (file.compare(files.at(i))==0) {
                fileindex=i ;
                foundfile=true ;
            }
        }
    }

    // Load backups and seach for backup by name

    if (foundfile) {
        loadBackups(directory + "/" + folders[folderindex] + "/" + file.replace(TXT,"").replace(ENC,""), BAK, ENB) ;
        for (int i=0, sz=backups.size(); i<sz; i++) {
            if (backups.at(i).compare(backup)==0) {
                backupindex=i ;
            }
        }
    }

    // Hide backups if file is --
    if (isCreateFile()) backupindex=-1 ;

    // Set the isbackup (readonly) flag

    if (backupindex>=0) isbackup=true ;
    else isbackup=false ;

    // Force the current selection

    if (backupindex<0 && selection==2) selection=1 ;
    if (fileindex<0 && selection==1) selection=0 ;

    // Update the lineedit contents

    update(0) ;
}

//
// Set the current folder/file/backup indexes based on a given path
//

void FileNavigator::setFilenameFromPath(QString path)
{
    QString folder, file, backup ;
    QRegExp filex("/([^/]*)/([^/]*)") ;
    QRegExp backupx("/([^/]*)/([^/]*)/([^/]*)") ;

    path = path.replace(directory,"") ;
    nofilename=false ;

    if (backupx.indexIn(path)>=0) {
        folder = backupx.cap(1) ;
        file = backupx.cap(2) ;
        backup = backupx.cap(3) ;
    } else if (filex.indexIn(path)>=0) {
        folder = filex.cap(1) ;
        file = filex.cap(2) ;
        backup = "-" ;
    }
    setFilename(folder, file, backup) ;
}


//
// Update the lineedit contents
//

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
    entry.replace(ENF, " (encrypted) ") ;
    txt=entry ;
    if (selection==0) s2=txt.length() ;

    if (folderindex>1 && fileindex>=0) {
        if (cursorpos>=txt.length()) selection=1 ;
        txt = txt + ": " ;
        if (selection==1) s1 = txt.length() ;
        entry = files.at(fileindex) ;
        entry.replace("_", "") ;
        entry.replace(TXT, "") ;
        entry.replace(ENC, "") ;
        txt = txt + entry ;
        if (selection==1) s2 = txt.length() ;
    }

    txt = txt + " " ;

    if (folderindex>1 && fileindex>0 && backupindex>=0) {
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
    isbackup = backupindex>0 ;

    this->setText(txt);

    this->setCursorPosition(s1) ;
#ifndef BEFOREQT54
    // There are some bugs with respect to the braille diplay
    // so we can't show the selection
    this->setSelection(s1, s2-s1) ;
#endif
    reentrant-- ;
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
    parseDirectory(directory, folders, "", false, true) ;
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
    folders.insert(0, "-- create encrypted folder --") ;
    if (folders.size()==2) folderindex=0 ;
    else folderindex=2 ;
}

void FileNavigator::loadFiles(QString directory, QString mask1, QString mask2)
{

    parseDirectory(directory, files, mask1, false, true) ;
    parseDirectory(directory, files, mask2, false, false) ;

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

void FileNavigator::loadBackups(QString directory, QString mask1, QString mask2)
{
    backups.clear() ;
    if (isCreateFile() || isRenameFolder() || isDeleteFolder()) {
        backupindex=-1 ;
    } else {
        parseDirectory(directory, backups, mask1, true, true) ;
        parseDirectory(directory, backups, mask2, true, false) ;
        if (backups.size()>0) {
            backupindex=0 ;
        } else {
            backupindex=-1 ;
        }
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
        bool preferencrypted = isFolderEncrypted() ;
        editableFilePath = getFolderPath() + "/" ;
        if (filename.isEmpty()) {
            editableFilePath = editableFilePath + files.at(fileindex) ;
        } else {
            editableFilePath = editableFilePath + filename ;
        }
    }
    return editableFilePath ;
}

QString& FileNavigator::getFilePath(QString filename, bool preferbackups)
{
    bool preferencrypted = isFolderEncrypted() ;


    if (isDeleteFolder() || isRenameFolder()) {
        filePath = getFolderPath() ;

    } else if (isCreateFolder()) {
        filePath="" ;

    } else if (!filename.isEmpty()) {
        filePath = getFolderPath() + "/" + filename + (preferencrypted?ENC:TXT) ;

    } else {

        filePath = getFolderPath() ;
        if (backupindex>0 || preferbackups) {
            QString filename = files.at(fileindex) ;
            filename = filename.replace(ENC, "").replace(TXT,"") ;
            filePath = filePath + "/" + filename + "/" + backups.at(backupindex) ;
        } else {
            filePath = filePath + "/" + files.at(fileindex) ;
        }

    }
    return filePath ;
}

QString& FileNavigator::getBackupFolderPath()
{
    backupFolderPath = "" ;
    if (!isCreateFolder() && !isCreateFile() && !isRenameFolder()) {
        QString filename = files.at(fileindex) ;
        backupFolderPath = getFolderPath() + "/" + filename.replace(ENC,QString("")).replace(TXT,QString("")) ;
    }
    return backupFolderPath ;
}

// return latest backup
// or path to today's backup (create)
QString& FileNavigator::getBackupFilePath(bool create)
{
    bool preferencrypted = isFolderEncrypted() ;
    backupFilePath = "" ;
    backupFilePath = getBackupFolderPath() ;
    if (!backupFilePath.isEmpty()) {
        if (create) {
            QDate today = QDate::currentDate() ;
            backupFilePath = backupFilePath + "/" + today.toString("yyyyMMdd") + (preferencrypted?ENB:BAK) ;
        } else if (backupindex>=0) {
            backupFilePath = backupFilePath + "/" + backups.at(0) + (preferencrypted?ENB:BAK) ;
        } else {
            backupFilePath="" ;
        }
    }

    return backupFilePath ;
}

QString& FileNavigator::getFileDescription(bool includefolder, bool includebackupdate)
{
    fileName = "" ;

    if (nofilename) {

        fileName="<none>" ;

    } else {

        bool isdeletefolder = isDeleteFolder() ;
        bool isrenamefolder = isRenameFolder() ;

        if (includefolder) {
            QString foldername = folders.at(folderindex) ;
            if (folderindex>0) fileName = foldername.replace(ENF,"") + ": " ;
        }

        if (folderindex>0 && !isrenamefolder && !isdeletefolder && fileindex>0) {
            QString filename = files.at(fileindex) ;
            fileName = fileName + filename.replace(TXT,"").replace(ENC,"") ;
        }

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


bool FileNavigator::isCreateFolder()
{
   if (folderindex>=folders.size() || folderindex<0) return false ;
   return (folders.at(folderindex).compare("-- create folder --")==0) ;
}

bool FileNavigator::isDeleteFolder()
{
    if (fileindex>=files.size() || fileindex<0) return false ;
    return (files.at(fileindex).compare("-- delete folder --")==0) ;
}

bool FileNavigator::isRenameFolder()
{
    if (fileindex>=files.size() || fileindex<0) return false ;
    return (files.at(fileindex).compare("-- rename folder --")==0) ;
}

bool FileNavigator::isCreateEncryptedFolder()
{
   if (folderindex>=folders.size() || folderindex<0) return false ;
   return (folders.at(folderindex).compare("-- create encrypted folder --")==0) ;
}

bool FileNavigator::isCreateFile()
{
    if (fileindex>=files.size() || fileindex<0) return false ;
    return (files.at(fileindex).compare("-- create file --")==0) ;
}



bool FileNavigator::isBackup()
{
    return (backupindex>0) ;
}


bool FileNavigator::isFolderEncrypted()
{
    if (folderindex>=folders.size() || folderindex<0) return false ;
    return folders.at(folderindex).contains(ENF) ;
}

bool FileNavigator::isFileEncrypted()
{
    if (backupindex>0) {
        if (backupindex>=files.size()) return false ;
        return backups.at(backupindex).contains(ENB) ;
    } else {
        if (fileindex>=files.size() || fileindex<0) return false ;
        return files.at(fileindex).contains(ENC) ;
    }
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

    if (isCreateEncryptedFolder() || isCreateFolder()) {
        fileindex=-1 ;
        files.clear() ;
    }

    if (reloadfiles) {
        loadFiles(getFolderPath(), TXT, ENC) ;
    }

    if (isCreateFile() || isRenameFolder() || isDeleteFolder()) {
        backupindex=-1 ;
        backups.clear() ;
    }

    if (reloadbackups) {
        QString filename = files.at(fileindex) ;
        filename = filename.replace(TXT,"").replace(ENC,"") ;
        loadBackups(getFolderPath() + "/" + filename, BAK, ENB) ;
    }

    if (changed) update() ;

}



// Used by EasyNotepad
QString& FileNavigator::parseBackupDate(QString backupdate)
{
    static QString parsedBackupDate ;
    parsedBackupDate = backupdate ;
    if (backupdate.length()>=8) {
        int d, m, y ;
        y = backupdate.left(4).toInt() ;
        m = backupdate.mid(4,2).toInt() ;
        d = backupdate.mid(6,2).toInt() ;
        QDate date ;
        date.setDate(y, m, d) ;
        if (date.isValid()) parsedBackupDate = date.toString("dd MMM yyyy") ;
    }
    return parsedBackupDate ;
}
