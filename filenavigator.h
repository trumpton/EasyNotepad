#ifndef FILENAVIGATOR_H
#define FILENAVIGATOR_H

#include <QLineEdit>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QString>
#include <QStringList>

class FileNavigator : public QLineEdit
{
    Q_OBJECT

private:
    QString directory ;
    QString tempdirectory ;
    QStringList folders ;
    QStringList files ;
    QStringList backups ;

    bool nofilename ;
    bool selectnew, isbackup ;
    int selection, folderindex, fileindex, backupindex ;
    QString backupFolderPath, folderPath, encBackupFilePath, backupFilePath, editableFilePath, encFilePath, filePath, fileName ;
    QString& parseBackupDate(QString backupdate) ;

    void loadFolders(QString directory) ;
    void loadFiles(QString directory, QString mask1, QString mask2);
    void loadBackups(QString directory, QString mask1, QString mask2) ;

    void keyPressEvent(QKeyEvent *event) ;

private slots:
    void on_cursorPositionChanged(int from, int to) ;



public:
    explicit FileNavigator(QWidget *parent = 0);
    ~FileNavigator() ;

    // Sets and Returns the path to the database
    void setPath(QString directory) ;
    QString& getPath() ;

    // Sets the folder / filename
    // If not specified, the current folder/filename is used, and
    // an appropriate refresh is performed.
    void setFilename(QString folder=QString(""), QString filename=QString(""), QString backup=QString("")) ;
    void setFilenameFromPath(QString path) ;
    void clearFilename() ;

    // Return the path to the specified folder
    // if not supplied, the currently selected folder path is returned
    QString& getFolderPath(QString foldername = QString("")) ;      // Returns the path to the folder (defaults selected)

    // Return the filen path and pretty version
    // If the filename is supplied, that one is returned, otherwise
    // the currently selected file path / name is returned
    QString& getFilePath(QString filename = QString(""), bool preferbackups=false) ; // Returns the path to the file (defaults selected)
    QString& getEditableFilePath(QString filename = QString("")) ;

    QString& getFileDescription(bool includefolder=true, bool includebackupdate=false) ;       // Returns the pretty version of the selected filename

    // Return the temp and backup folder and file paths
    QString& getTempFolderPath() ;
    QString& getBackupFolderPath() ;                    // Returns the path to the backup file
    QString& getBackupFilePath(bool create = true) ;    // Returns path to new backup file, or latest one if create=false

    // Returns status of current selection
    bool isRenameFolder() ;
    bool isDeleteFolder() ;
    bool isCreateFolder() ;
    bool isCreateEncryptedFolder() ;
    bool isCreateFile() ;
    bool isBackup() ;

    // Return whether the folder or the file/backup is encrypted or not
    bool isFolderEncrypted() ;
    bool isFileEncrypted() ;

    // TODO: should be able to make this private
    void update(int cursorpos=-1) ;


signals:

public slots:

};

#endif // FILENAVIGATOR_H
