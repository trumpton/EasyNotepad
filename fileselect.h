#ifndef FILESELECT_H
#define FILESELECT_H

#include <QDialog>

namespace Ui {
class FileSelect;
}

class FileSelect : public QDialog
{
    Q_OBJECT

public:
    explicit FileSelect(QWidget *parent = 0);
    void setPath(QString path) ;
    ~FileSelect();

    void setFilename(QString folder = QString(""), QString file = QString("")) ;
    void setFilenameFromPath(QString path) ;
    void clearFilename() ;

    bool isReadOnly() ;
    bool isCreateFolder() ;
    bool isRenameFolder() ;
    bool isCreateFile() ;
    bool isDeleteFolder() ;

    int exec() ;

    QString& getPath() ;                                         // Returns the path to the database

    QString& getFolderPath(QString foldername = QString("")) ;   // Returns the path to the folder (defaults selected)
    QString& getFilePath(QString filename = QString(""), bool preferbackups=false) ;       // Returns the path to the file (defaults selected)
    QString& getEditableFilePath(QString filename = QString("")) ;
    QString& getFileName(bool includefolder, bool includebackupdate) ;  // Returns the pretty filename
    QString getFileDate() ;                                     // Returns the date of the file

    QString& getTempFolderPath() ;                               // Returns the temp folder name
    QString& getBackupFolderPath() ;                             // Returns the backup folder name for the current file
    QString& getBackupFilePath(bool create=true) ;               // Returns the path to the backup file of the last backup or the filename to create one

private:
    Ui::FileSelect *ui;
};

#endif // FILESELECT_H
