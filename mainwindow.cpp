#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QInputDialog>
#include <QStandardPaths>
#include <QDateTime>
#include <QRegExp>
#include <QProcess>
#include <QStringList>
#include <QTextDocumentFragment>
#include <QLabel>

#include "globalsearch.h"
#include "filetypes.h"
#include "../Lib/supportfunctions.h"
#include "../Lib/itemselect.h"
#include "../Lib/alertsound.h"
#include "../Lib/iniconfig.h"
#include "version.h"

#ifdef WIN32
#include <windows.h>
#endif

//
// TODO: Bug
//
// Search for file and open it
// Edit it
// Re-search for same file
// Say 'Yes' to save (note filename it suggests)
// Data is saved to the wrong file
//

bool gDebugEnabled ;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Connect Handlers
    connect(ui->menuKeyAndPassword, SIGNAL(aboutToShow()), this, SLOT(refreshMenus())) ;

    // Add Statusbar widgets
    QStatusBar * status_bar = statusBar();

    encryptedlabel = new QLabel("", status_bar);
    QPixmap encryptedpixmap(QPixmap(":/encrypted-file.png"));
    encryptedlabel->setPixmap(encryptedpixmap);
    encryptedlabel->setToolTip(tr("Encrypted File"));
    status_bar->addWidget(encryptedlabel);
    encryptedlabel->setVisible(false) ;

    readonlylabel = new QLabel("", status_bar);
    QPixmap readonlypixmap(QPixmap(":/read-only.png"));
    readonlylabel->setPixmap(readonlypixmap);
    readonlylabel->setToolTip(tr("Read-Only File"));
    status_bar->addWidget(readonlylabel);
    readonlylabel->setVisible(false) ;

    dirtylabel = new QLabel("", status_bar);
    QPixmap dirtypixmap(QPixmap(":/file-dirty.png"));
    dirtylabel->setPixmap(dirtypixmap);
    dirtylabel->setToolTip(tr("Edited File"));
    status_bar->addWidget(dirtylabel);
    dirtylabel->setVisible(false) ;

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    ui->textEdit->setReadOnly(true) ;

    // Initialise encryption i/o
    enc = new Encryption(QString("trumpton.uk"), QString("TextFileEncryption")) ;

    // TODO: Reset all vars
    editor=NULL ;
    Close() ;

    // Load INI file from TrumptonApps directory, default database directory, otherwise application directory

    bool iniok=false ;
    QString inifile ;

    if (!iniok) {
        inifile = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QString("/TrumptonApps/EasyNotepad.ini") ;
        iniok = ini.init(inifile) ;
    }

    if (!iniok) {
        inifile = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QString("/EasyNotepad/EasyNotepad.ini") ;
        iniok = ini.init(inifile) ;
    }

    if (!iniok) {
        QString application = QApplication::applicationFilePath() ;
        inifile = application.replace(".exe","",Qt::CaseInsensitive) + QString(".ini") ;
        iniok = ini.init(inifile) ;
    }

    if (iniok) {

        emailcommand = ini.get(QString("email"), QString("command")) ;

        ifilter.init(ini, ini.canonicalPath(), enc) ;
        databasedir = ini.get(QString("database"), QString("path")) ;
        if (databasedir.isEmpty()) {
            databasedir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QString("/EasyNotepad") ;
        }

        QString debugstr = ini.get(QString("general"), QString("debug")).toLower() ;
        gDebugEnabled = (debugstr.contains("yes") || debugstr.contains("enable") || debugstr.contains("true")) ;

    } else {
        errorOkDialog(this, "Easy Notepad Error", "Unable to find EasyNotepad.ini (F1 for help).") ;
        gDebugEnabled=false ;
    }

}


MainWindow::~MainWindow()
{
    Save(true) ;
    Close() ;
    if (dirtylabel) delete dirtylabel ;
    if (readonlylabel) delete readonlylabel ;
    if (encryptedlabel) delete encryptedlabel ;

    delete ui;
}

void MainWindow::setPath(QString directory, QString fullpath)
{
    QString home ;

    if (!directory.isEmpty()) {
        home=directory ;
    } else {
        home = databasedir ;
    }

    fs.setPath(home) ;
    Backup() ;

    if (!fullpath.isEmpty()) {

        Load(fullpath) ;

        // BUGFIX: These two lines are necessary to force the focus update to get through to the accessible
        //         interface.  It is only required if the textEdit is not Editable !!!!
        qApp->processEvents() ;
        ui->textEdit->moveCursor(QTextCursor::Start) ;

    }

}


//
// File Save and Load
//

bool MainWindow::Save(bool ask, bool force)
{
    bool dosave=true ;
    bool success=true ;
    QString newbuffer = ui->textEdit->toPlainText() ;

    // If the file is read-only / importable, don't need to save
    if (isimportable) return true ;

    // No save necessary, clear dirty flag if set and return
    if (!force && newbuffer.compare(buffer)==0) {
        dirtylabel->setVisible(false) ;
        return true ;
    }

    // TODO: Replace with Yes/No/Cancel (return false on cancel, set dosave=false on no
    if (ask && !warningYesNoDialog(this,
          "EasyNote Save?",
           "The File \"" + fs.getFileDescription(true, false) + "\" has changed. Do you wish to save?")) {
       dosave=false ;
    }


    if (dosave || force) {

        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextEncoder *encoderWithoutBom = codec->makeEncoder( QTextCodec::IgnoreHeader );
        QByteArray bytes  = encoderWithoutBom ->fromUnicode( newbuffer );

        QString directory = fs.getFolderPath() ;
        if (!QDir(directory).exists()) QDir().mkdir(directory) ;

        // Save file
        QString filepath = fs.getFilePath() ;
        if (fs.isFolderEncrypted()) {
            if (!enc->loggedIn()) {
                enc->login() ;
            }
            if (!enc->loggedIn()) {
                success=false ;
            } else {
                if (!enc->save(filepath, bytes)) {
                    success=false ;
                }
            }
        } else {
            if (!writeToFile(filepath, bytes)) {
                success=false ;
            }
        }

        QString backupdirectory = fs.getBackupFolderPath() ;
        if (!QDir(backupdirectory).exists()) QDir().mkdir(backupdirectory) ;

        // Save Backup
        QString backupfilepath = fs.getBackupFilePath() ;
        if (fs.isFolderEncrypted()) {
            if (!enc->save(backupfilepath, bytes)) {
                success=false ;
            }
        } else {
            if (!writeToFile(backupfilepath, bytes)) {
                success=false ;
            }
        }

        if (success) {
            dirtylabel->setVisible(false) ;
            play(FileSave) ;
            buffer = newbuffer ;
        } else {
            play(Error) ;
        }
    }

    return success ;
}

bool MainWindow::Close()
{
    if (editor!=NULL) {
        editor->detach() ;
        delete editor ;
        editor=NULL ;
    }

    buffer="" ;
    ui->textEdit->clear() ;
    setWindowTitle("Easy Notepad") ;
    ui->textEdit->setAccessibleName("Easy Notepad.") ;
    isimportable=false ;
    ui->action_Email->setEnabled(false) ;
    ui->action_Delete->setEnabled(false) ;
    ui->action_Undelete->setEnabled(false) ;
    ui->action_Find->setEnabled(false) ;
    ui->actionFind_Next->setEnabled(false) ;
    ui->action_Save->setEnabled(false) ;
    ui->actionRename_File->setEnabled(false) ;
    ui->action_Move_File->setEnabled(false) ;
    ui->textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard) ;
    encryptedlabel->setVisible(false) ;
    readonlylabel->setVisible(false) ;
    dirtylabel->setVisible(false) ;
    return true ;
}

bool MainWindow::Load(QString path)
{
    QString filepath, description ;
    bool isreadonly=true ;
    bool isdeleted=false ;
    isimportable=false ;

    Close() ;

    // TODO: Normalise Paths i.e. \\ to /

    isimportable=false ;

    if (!path.isEmpty()) {
        QString basedir = fs.getPath() ;
        if (path.length()<basedir.length() || path.left(basedir.length()).compare(basedir)!=0) {
            // Path is not in the EasyNotepad area
            isimportable=true ;
            fs.clearFilename() ;
        } else {
            // Path is in the EasyNotepad area, so fs will be up-to-date
            if (!fileExists(path)) {
                // Create File that doesn't exist
                if (fs.isFolderEncrypted()) {
                    enc->save(path, QByteArray("")) ;
                } else {
                    writeToFile(path, QString("")) ;
                }
            }
            // New file created, so update fs
            fs.setFilenameFromPath(path) ;
        }
    }

    if (isimportable) {
        path=path.replace("\\","/") ;
        QRegExp rx("(.*)/([^/]*)") ;
        rx.setMinimal(false) ;
        if (rx.indexIn(path)!=-1) {
            description = rx.cap(2) ;
        } else {
            description = path ;
        }
        filepath = path ;
        isreadonly = true ;
        isdeleted = false ;

    } else {
        filepath = fs.getFilePath() ;
        description = fs.getFileDescription(false, true) ;
        isreadonly = fs.isReadOnly() ;
        isdeleted = !fileExists(fs.getEditableFilePath()) ;

    }

    qint64 master ;

    editor = new QSharedMemory("EasyNotepad" + filepath);
    editor->attach() ;
    master = masterPID(editor) ;

    if (master>0) {
        // Switch to other file, and close down

#ifdef WIN32
        WPARAM wp = SC_RESTORE ;
        LPARAM lp = 0 ;
        if (PostMessage((HWND)master, WM_SYSCOMMAND, wp, lp))
            SetForegroundWindow((HWND)master);
        else
            // Send QMessage
#endif
            warningOkDialog(this, "File Already Open",
                 "The file " + fs.getFileDescription(false, false) + " is already open, Press Alt-Tab to find it") ;

        buffer="" ;
        ui->textEdit->setPlainText(buffer) ;
        on_actionE_xit_triggered();

        return false ;

    } else {

        if (!ifilter.LoadFile(path, buffer, filepath)) {
            errorOkDialog(this, "Easy Notepad - File Open Error", buffer) ;
            buffer="" ;

            return false ;

        } else {

            ui->textEdit->setPlainText(buffer) ;

            setWindowTitle(description + " - Easy Notepad") ;
            ui->textEdit->setAccessibleName(description + ", Easy Notepad.") ;

            if (isreadonly) {
                // Disable Delete and make readonly
                ui->action_Email->setEnabled(true) ;
                ui->action_Delete->setEnabled(false) ;
                ui->action_Save->setEnabled(false) ;
                ui->actionRename_File->setEnabled(false) ;
                ui->action_Move_File->setEnabled(false) ;
                ui->textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard) ;
                readonlylabel->setVisible(true) ;

            } else {
                ui->action_Email->setEnabled(true) ;
                ui->action_Delete->setEnabled(true) ;
                ui->action_Save->setEnabled(true) ;
                ui->actionRename_File->setEnabled(true) ;
                ui->action_Move_File->setEnabled(true) ;
                ui->textEdit->setTextInteractionFlags(Qt::TextEditable | Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard) ;
                readonlylabel->setVisible(false) ;
            }

            ui->textEdit->setFocus() ;

            if (isdeleted) {
                // Enable Undelete
                ui->action_Undelete->setEnabled(true) ;
            } else {
                ui->action_Undelete->setEnabled(false) ;
            }

            ui->action_Find->setEnabled(true) ;
            ui->actionFind_Next->setEnabled(true) ;

            // Set Encryption Status Flag
            encryptedlabel->setVisible(fs.isFileEncrypted()) ;
        }

        dirtylabel->setVisible(false) ;
        return true ;

    }

}

void MainWindow::refreshMenus()
{
    if (enc->loggedIn()) {
        ui->actionChange_Password->setEnabled(false) ;
        ui->actionSet_Encryption_Key->setEnabled(false) ;
        ui->action_Logout->setEnabled(true) ;
    } else {
        ui->actionChange_Password->setEnabled(true) ;
        ui->actionSet_Encryption_Key->setEnabled(true) ;
        ui->action_Logout->setEnabled(false) ;
    }

}

// Get the Process ID of the first app
// If not found, return -1
qint64 MainWindow::masterPID(QSharedMemory *sharedMem)
{
    qint64 PID, MYPID ;

#ifdef WIN32
    MYPID = winId() ;
#else
    MYPID = QCoreApplication::applicationPid() ;
#endif

    // Try to create twice.
    // This clears stale shared memory if a Unix application has aborted / crashed
    if (sharedMem->create(sizeof(qint64)) || sharedMem->create(sizeof(qint64)) ) {
        // Created so write PID
        void *ptr = sharedMem->data() ;
        qint64 *qptr = (qint64 *)ptr ;
        *qptr = MYPID ;
        PID=-1 ;
    } else {
        // Already exists, so read PID
        sharedMem->attach() ;
        void *ptr = sharedMem->data() ;
        qint64 *qptr = (qint64*) ptr ;
        PID = *qptr ;
        // TODO: If the process does not exist, don't detach, write our PID, and return -1
        sharedMem->detach() ;
    }
    return PID ;
}


//
// Backup Management
//

bool MainWindow::Backup()
{
    // TODO: Tidy backup files
    return true ;
}

//
// Notification
//

void MainWindow::msg(QString msg)
{
    ui->statusBar->showMessage(msg, 6000) ;
}

//
// Menu Items
//

void MainWindow::on_action_Open_triggered()
{
    bool cancel=false, finished=false ;
    Save(true) ;

    // Close current file
    // Because FileSelect doesn't remember path
    // to current file when cancel or folder/file
    // operations are selected.

    // Alternatively: Save current path, and attempt to restore it on cancel
    // on cancel, if file no longer exists, select close.

    Close() ;

    do {
        play(Query) ;

        switch (fs.execDialog()) {

            case FileSelect::Cancel: {
                cancel=true ;
            }
            break ;

            case FileSelect::RenameFolder: {
                bool ok ;
                QString newFolderName ;

                Save(true) ;

                warningOkDialog(this, "Warning", "Please close all instances of Easy Notepad that are using the folder you wish to rename before continuing!") ;

                newFolderName = inputDialog(this, tr("Rename"),  tr("Rename folder to?"), QLineEdit::Normal, newFolderName, &ok) ;
                if (ok && newFolderName.isEmpty()) {
                    warningOkDialog(this, "Information", "Rename has been aborted") ;
                } else {

                    QString currentName = fs.getFolderPath() ;


                    // If Encrypted, force new name to be also encrypted
                    if (fs.isFolderEncrypted()) {
                        newFolderName = newFolderName + ENF ;
                    }

                    QString newName = fs.getFolderPath(newFolderName) ;

                    QString currentLower = currentName.toLower() ;
                    QString newLower = newName.toLower() ;

                    bool dorename=true ;

                    if (currentLower.compare(newLower)==0) {
                        errorOkDialog(this, "Error", "The new name is the same as the current name") ;
                        dorename=false ;
                    } else if (directoryExists(newName)) {
                        errorOkDialog(this, "Error", "A folder of that name already exists") ;
                        dorename=false ;
                    }

                    if (dorename) {

                        QDir dir ;
                        dir.rename(currentName, newName) ;
                        fs.setFilename(newFolderName) ;

                        if (directoryExists(currentName)) {
                            errorOkDialog(this, "Error", "An error has occured renaming the folder - check the disk or folder is not write protected") ;
                        } else {
                            Close() ;
                        }
                    }
                }
            }
            break ;

            case FileSelect::DeleteFolder: {
                QString& foldername = fs.getFileDescription(true, false) ;
                if (warningYesNoDialog(this, "Warning", "Deleting '" + foldername + "' will cause all backups stored within to be lost.  Do you want to continue?")) {
                    QString& folderpath = fs.getFolderPath() ;
                    QDir dir(folderpath) ;

                    Q_FOREACH(QFileInfo subdirinfo, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs, QDir::DirsFirst)) {
                        QString sdpath = subdirinfo.absoluteFilePath() ;
                        QDir subdir(sdpath) ;
                        Q_FOREACH(QFileInfo fileinfo, subdir.entryInfoList(QDir::Files, QDir::DirsFirst)) {
                            QString filename = fileinfo.absoluteFilePath() ;
                            if (filename.contains(QRegExp("\\.[bB][aA][kK]$"))) {
                                QFile::remove(filename);
                            }
                        }
                        subdir.rmdir(sdpath);
                    }
                    dir.rmdir(folderpath) ;
                    if (dir.exists()) {
                        warningOkDialog(this, "Error", "Unable to remove folder - it contains some non-EasyNotepad files") ;
                    }
                }
            }
            break ;


            case FileSelect::CreateFolder: {
                bool ok ;
                QString foldername ;
                foldername = inputDialog(this, tr("Create Folder"),  tr("Enter Name of Folder to Create"), QLineEdit::Normal, foldername, &ok) ;
                if (ok) {
                    QDir dir ;
                    QString& folderpath = fs.getFolderPath(foldername) ;
                    dir.mkpath(folderpath) ;
                    fs.setFilename(foldername) ;
                }
            }
            break ;

            case FileSelect::CreateEncryptedFolder: {
                bool ok ;
                QString foldername ;
                foldername = inputDialog(this, tr("Create Folder"),  tr("Enter Name of Folder to Create"), QLineEdit::Normal, foldername, &ok) ;
                if (ok) {
                    QDir dir ;
                    foldername = foldername + ENF ;
                    QString& folderpath = fs.getFolderPath(foldername) ;
                    dir.mkpath(folderpath) ;
                    fs.setFilename(foldername) ;
                }
            }
            break ;

            case FileSelect::CreateEncryptedFile:
            case FileSelect::CreateAFile: {
                bool ok ;
                QString filename ;
                filename = inputDialog(this, tr("Create File"),  tr("Enter Name of File to Create"), QLineEdit::Normal, filename, &ok) ;
                if (ok) {

                    // Get the path to the new file
                    QString& filepath = fs.getFilePath(filename) ;

                    // File exists, so do nothing
                    if (fileExists(filepath)) {
                        warningOkDialog(this, "File Exists", "File not created, it already exists") ;

                    } else {

                        // Create a dummmy file
                        if (fs.isFolderEncrypted()) {
                            enc->save(filepath, QByteArray("")) ;
                        } else {
                            writeToFile(filepath, QString("")) ;
                        }

                        fs.setFilenameFromPath(filepath) ;

                        // If a backup exists, copy the latest into the dummy file
                        QString& latestbackuppath = fs.getBackupFilePath(false) ;
                        if (!latestbackuppath.isEmpty()) {
                            warningOkDialog(this, "Restoring Deleted File", "A Deleted Version of this file has been found.  If you wish to load the previous contents, please select the backup.") ;
                            copyFile(latestbackuppath, filepath) ;
                        }
                    }
                }
            }
            break ;

            case FileSelect::LoadFile:
            case FileSelect::LoadBackup: {
                QString& filepath = fs.getFilePath() ;
                if (Load(filepath)) play(FileOpen) ;
                dirtylabel->setVisible(false) ;
                finished=true ;
            }
            break ;

        }

    } while (!finished && !cancel) ;
}


void MainWindow::on_action_Save_triggered()
{
    Save(false) ;
}


void MainWindow::on_action_Find_triggered()
{
    bool ok ;
    searchtext = inputDialog(this, tr("Find"),  tr("Search For"), QLineEdit::Normal, searchtext, &ok) ;
    if (ok) on_actionFind_Next_triggered() ;
}

void MainWindow::on_actionFind_Next_triggered()
{

    if (!searchtext.isEmpty()) {

        QTextCursor currentpos = ui->textEdit->textCursor();
        QTextCursor cursor = currentpos ;
        bool found ;

        // Move cursor on one character
        cursor.movePosition(QTextCursor::NextCharacter) ;
        ui->textEdit->setTextCursor(cursor);

        // Search the current textedit field
        if (ui->textEdit->find(searchtext)) {
            play(Found) ;
            found = true ;
        } else {
            // Not found, so save cursor, wrap and try again
            QTextCursor top = ui->textEdit->textCursor();
            top.movePosition(QTextCursor::Start) ;
            ui->textEdit->setTextCursor(top);
            if (ui->textEdit->find(searchtext)) {
                found = true ;
                play(Wrapped) ;
            } else {
                found = false ;
                play(NotFound) ;
            }
        }

        if (found) {
            // Move cursor to start of selection
            cursor = ui->textEdit->textCursor() ;
            int pos = cursor.selectionStart() ;
            cursor.clearSelection() ;
            cursor.setPosition(pos) ;
            ui->textEdit->setTextCursor(cursor) ;
            ui->textEdit->ensureCursorVisible() ;
        } else {
            // Move cursor back to where it started
            ui->textEdit->setTextCursor(currentpos) ;
        }

    }

}

void MainWindow::on_actionE_xit_triggered()
{
    QApplication::quit() ;
}

void MainWindow::on_actionFind_Global_triggered()
{
    bool ok ;
    QString filepath="", filename="" ;

    searchtext = inputDialog(this, tr("Find Global"),  tr("Find Globally"), QLineEdit::Normal, searchtext, &ok) ;
    if (ok) {
        GlobalSearch search ;
        search.setSearch(&ifilter, fs.getPath(), true, searchtext);
        if (search.exec()) {
            filepath = search.getSelection() ;
            filename = search.getSelectionFileName() ;
        }
        if (!filepath.isEmpty()) {
            if (Save(true)) {
                fs.setFilenameFromPath(filepath) ;
                if (Load(filepath)) play(FileOpen) ;
                dirtylabel->setVisible(false) ;
            }
        } else {
            warningOkDialog(this, "", "No Match Found.  Use the Old search, Control-Shift-G to include deleted files.") ;
        }
    }
}


void MainWindow::on_actionFind_Old_triggered()
{
    bool ok ;
    QString filepath="", filename="" ;

    searchtext = inputDialog(this, tr("Find Old Files"),  tr("Find Old Files"), QLineEdit::Normal, searchtext, &ok) ;
    if (ok) {
        GlobalSearch search ;
        search.setSearch(&ifilter, fs.getPath(), false, searchtext);
        if (search.exec()) {
            filepath = search.getSelection() ;
            filename = search.getSelectionFileName() ;
        }
        if (!filepath.isEmpty()) {
            if (Save(true)) {
                fs.setFilenameFromPath(filepath) ;
                if (Load(filepath)) play(FileOpen) ;
                dirtylabel->setVisible(false) ;
            }
        } else {
            warningOkDialog(this, "", "No Match Found.") ;
        }
    }

}

void MainWindow::on_action_Delete_triggered()
{
    if (warningYesNoDialog(this, "EasyNote Delete?",
               "Do you want to delete \"" + fs.getFileDescription(true, false) + "\"?")) {

        Save(false, true) ;
        QFile::remove(fs.getFilePath()) ;
        Close() ;
        fs.setFilename() ; // Refresh
    }
}



void MainWindow::on_action_Undelete_triggered()
{
    QString filename = fs.getEditableFilePath() ;
    if (fileExists(filename)) {
        warningOkDialog(this, "Error", "File cannot be undeleted as it is is not a deleted file.\nUse Control-Shift-G to search for deleted files.") ;
    } else {
        copyFile(fs.getFilePath(), filename) ;
        fs.setFilename() ;
        Load(filename) ;
        Save(false, true) ;
    }
}

void MainWindow::on_actionRename_File_triggered()
{
    bool ok ;
    QString newFileName ;
    newFileName = inputDialog(this, tr("Rename"),  tr("Rename file to?"), QLineEdit::Normal, newFileName, &ok) ;
    if (ok && !newFileName.isEmpty()) {
        Save() ;
        QString currentName = fs.getFilePath() ;
        QString newName = fs.getFolderPath() + "/" + newFileName + "." + TXT ;
        QString currentBackup = fs.getBackupFolderPath() ;
        QString newBackup = fs.getFolderPath() + "/" + newFileName ;
        bool dorename=true ;
        QString currentLower = currentName.toLower() ;
        QString newLower = newName.toLower() ;

        if (currentLower.compare(newLower)==0) {
            warningOkDialog(this, "Error", "The newname is the same as the current name") ;
            dorename=false ;
        } else if (fileExists(newName)) {
            warningOkDialog(this, "Error", "A file of that name already exists") ;
            dorename=false ;
        } else if (directoryExists(newBackup)) {
            bool confirm = warningYesNoDialog(this, "Backup Exists", "A backup for a previously deleted file with that name exists.\nDo you wish to delete this backup and continue with the rename?") ;
            if (!confirm) {
                dorename=false ;
            } else {
                if (!deleteDirectoryAndFiles(newBackup, BAK)) {
                        warningOkDialog(this, "Error", "Unable to remove backup for " + newFileName + " - you must do it manually") ;
                        dorename=false ;
                }
            }
        }

        if (dorename) {

            QDir dir;
            bool success=true ;

            if (!dir.rename( currentName, newName)) {
                success=false ;
            } else {
                if (directoryExists(currentBackup)) {
                    if (!dir.rename(currentBackup, newBackup)) {
                        success=false ;
                        dir.rename(newName, currentName) ;
                    }
                }
            }

            if (success) {
                Load(newName) ;
            } else {
                warningOkDialog(this, "Error", "The file could not be renamed - check the disk or folder is not write protected") ;
            }

        }

    }
}

void MainWindow::on_action_Insert_Template_triggered()
{
    ItemSelect list ;
    list.setTitle("Select Template - Easy Notepad", "Template") ;
    QStringList files, filenames ;
    QString templatepath = fs.getPath() + "/Templates" ;

    if (parseDirectory(templatepath, filenames, TXT)) {
        for (int i=0, sz=filenames.size(); i<sz; i++) {
            QString filename = filenames.at(i) ;
            list.addEntry(filename.replace(TXT,"").replace(ENC,""), templatepath + "/" + filenames.at(i)) ;
        }
    }

    if (list.size()==0) {
        list.addEntry("Create a folder called Templates to see files here", "") ;
    } else {
        list.setIndex(0) ;
    }

    if (list.exec()) {
        QString filedata ;

        // TODO: Currently, templates cannot be encrypted
        if (!list.getData().isEmpty() && readFromFile(list.getData(), filedata, "UTF-8")) {

            QString filename = fs.getFileDescription(true, false) ;
            QString folder = filename ;
            QDateTime datetime = QDateTime::currentDateTime() ;

            filedata.replace("$WEEKDAY", datetime.date().toString("dddd")) ;
            filedata.replace("$MONTHNAME", datetime.date().toString("MMMM")) ;
            filedata.replace("$FULLYEAR", datetime.date().toString("yyyy")) ;

            filedata.replace("$DAY", datetime.date().toString("dd")) ;
            filedata.replace("$MONTH", datetime.date().toString("MM")) ;
            filedata.replace("$YEAR", datetime.date().toString("yy")) ;

            filedata.replace("$FILEPATH", filename) ;
            filename.replace(QRegExp(".*: "), "") ;
            folder.replace(QRegExp(":.*"), "") ;
            filedata.replace("$FILENAME", filename) ;
            filedata.replace("$FOLDERNAME", folder) ;
            filedata.replace("$TIME", datetime.time().toString("hh:mm")) ;

            QTextCursor cursor = ui->textEdit->textCursor() ;

            int pos = cursor.position() ;
            ui->textEdit->insertPlainText(filedata) ;
            dirtylabel->setVisible(true) ;
            cursor.setPosition(pos) ;
            ui->textEdit->setTextCursor(cursor) ;

            play(Ok) ;

        } else {
            play(Error) ;
        }
    }

}

void MainWindow::on_action_Help_triggered()
{
    help.exec() ;
}

void MainWindow::on_action_Email_triggered()
{
  if (ui->textEdit->toPlainText().isEmpty()) {
      return ;
  }

  QString cmd = emailcommand ;

  if (cmd.isEmpty()) {

      errorOkDialog(this, "Send Via Email", "This is not configured.  Missing [email] command in ini file\n") ;

  } else {

    QString selection = ui->textEdit->textCursor().selection().toPlainText() ;
    if (selection.isEmpty()) {
      warningOkDialog(this, "Send Via Email", "No text is selected.  Select text and press Control-L to forward as attachment\n") ;
      return ;
    } else if (warningYesNoDialog(this, "Send Via Email?",
       "Do you want to save and send this file as an email attachment?")) {

       QString now = QDateTime::currentDateTimeUtc().toLocalTime().toString("yyMMdd-hhmmss_") ;
       QString forwarded = fs.getTempFolderPath() + "/" + now + fs.getFileDescription(false, false) + ".txt" ;
       writeToFile(forwarded, selection) ;

       // thunderbird -compose "to='john@example.com,kathy@example.com',cc='britney@example.com',subject='dinner',body='How about dinner tonight?',attachment='file://C:\temp\info.doc,file://C:\temp\food.doc'"
       cmd = cmd.replace("$FILEPATH", forwarded) ;
       cmd = cmd.replace("$FILENAME", fs.getFileDescription(false, false)) ;

       QProcess *myProcess = new QProcess(NULL) ;
       QStringList args = cmd.split( QRegExp(" (?=[^\"]*(\"[^\"]*\"[^\"]*)*$)") );
       for (int a=0; a<args.length(); a++) {
         QString entry = args.at(a) ;
         entry = entry.replace("\\\"", "***REALQUOTE***") ;
         entry = entry.replace("\"", "") ;
         entry = entry.replace("***REALQUOTE***", "\\\"") ;
         args.replace(a, entry) ;
       }

       QString app = args.at(0) ;
       args.removeAt(0) ;

       if (gDebugEnabled) {
           warningOkDialog(0, "Debug", QString("Executing: ") + cmd) ;
       }

       play(Ok) ;
       myProcess->start(app, args) ;

       // TODO: Check for error launching
    }
  }
}


void MainWindow::on_action_About_triggered()
{
    warningOkDialog(this, QString("About Easy Notepad"), QString("This is version " BUILDVERSION " (" BUILDDATE ") of Easy Notepad, and was built on :\n  " COMPILEDATE))  ;
}


void MainWindow::on_actionSet_Encryption_Key_triggered()
{
    enc->setKey() ;
}

void MainWindow::on_actionChange_Password_triggered()
{
    enc->changePassword();
}

void MainWindow::on_action_Logout_triggered()
{
    if (enc->loggedIn()) enc->logout() ;
}

void MainWindow::on_textEdit_textChanged()
{
    dirtylabel->setVisible(true) ;
}

void MainWindow::on_action_SetAsDefault_triggered()
{
    ifilter.registerTypes();
    warningOkDialog(this, QString("Easy Notepad"), QString("EasyNotepad has been registered as the default application for its configured types"))  ;
}
