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

int FileSelect::exec()
{
    ui->fileSelect->setFocus() ;
    ui->fileSelect->setFilename() ;
    ui->fileSelect->update() ;
    return QDialog::exec() ;
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

QString& FileSelect::getFileName(bool includefolder, bool includebackupdate)
{
    return ui->fileSelect->getFileName(includefolder, includebackupdate) ;
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

bool FileSelect::isCreateFolder()
{
    return ui->fileSelect->isCreateFolder() ;
}

bool FileSelect::isRenameFolder()
{
    return ui->fileSelect->isRenameFolder() ;
}

bool FileSelect::isCreateFile()
{
    return ui->fileSelect->isCreateFile() ;
}

bool FileSelect::isDeleteFolder()
{
    return ui->fileSelect->isDeleteFolder() ;
}
