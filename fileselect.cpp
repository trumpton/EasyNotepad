#include "fileselect.h"
#include "ui_fileselect.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDialog>

FileSelect::FileSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileSelect)
{
    ui->setupUi(this);
}

FileSelect::~FileSelect()
{
    delete ui;
}

FileSelect::InputType FileSelect::execDialog()
{
    ui->fileSelect->setFocus() ;
    ui->fileSelect->setFilename() ;
    if (QDialog::exec()==0) return FileSelect::Cancel ;
    if (ui->fileSelect->isRenameFolder()) return FileSelect::RenameFolder ;
    if (ui->fileSelect->isDeleteFolder()) return FileSelect::DeleteFolder ;
    if (ui->fileSelect->isCreateFolder()) return FileSelect::CreateFolder ;
    if (ui->fileSelect->isCreateEncryptedFolder()) return FileSelect::CreateEncryptedFolder ;
    if (ui->fileSelect->isCreateFile()) {
        if (ui->fileSelect->isFolderEncrypted()) return FileSelect::CreateEncryptedFile ;
        else return FileSelect::CreateAFile ;
    }
    return FileSelect::LoadFile ;
}

void FileSelect::setPath(QString path)
{
    ui->fileSelect->setPath(path) ;
}

QString& FileSelect::getPath()
{
    return ui->fileSelect->getPath() ;
}

void FileSelect::setFilename(QString folder, QString file)
{
    ui->fileSelect->setFilename(folder, file) ;
    ui->fileSelect->update() ;
}

void FileSelect::setFilenameFromPath(QString path)
{
    // remove .autosave tail
    path = path.replace(".autosave","") ;

    ui->fileSelect->setFilenameFromPath(path) ;
    ui->fileSelect->update() ;
}

void FileSelect::clearFilename()
{
    ui->fileSelect->clearFilename() ;
}

QString& FileSelect::getFilePath(QString filename, bool preferbackups)
{
    return ui->fileSelect->getFilePath(filename, preferbackups) ;
}


QString& FileSelect::getEditableFilePath(QString filename)
{
    return ui->fileSelect->getEditableFilePath(filename) ;
}

QString& FileSelect::getFolderPath(QString foldername)
{
    return ui->fileSelect->getFolderPath(foldername) ;
}

QString& FileSelect::getBackupFilePath(bool create)
{
    return ui->fileSelect->getBackupFilePath(create) ;
}

QString& FileSelect::getBackupFolderPath()
{
    return ui->fileSelect->getBackupFolderPath() ;
}

QString& FileSelect::getTempFolderPath()
{
    return ui->fileSelect->getTempFolderPath() ;
}

QString& FileSelect::getFileDescription(bool includefolder, bool includebackupdate)
{
    return ui->fileSelect->getFileDescription(includefolder, includebackupdate) ;
}

QString FileSelect::getFileDate()
{
    QString filename = getFilePath() ;
    QFileInfo inf(filename) ;
    QDateTime mod = inf.lastModified() ;
    return mod.toString("ddd dd MMM yyyy hh:mm") ;
}

bool FileSelect::isReadOnly()
{
    return ui->fileSelect->isBackup() ;
}

bool FileSelect::isFolderEncrypted()
{
    return ui->fileSelect->isFolderEncrypted() ;
}

bool FileSelect::isFileEncrypted()
{
    return ui->fileSelect->isFileEncrypted() ;
}

int FileSelect::getFileLength(QString path)
{
    QFile f(path) ;
    int length = f.size() ;
    return length ;
}
