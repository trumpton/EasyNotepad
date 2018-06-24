#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedMemory>
#include <QLabel>
#include "importfilter.h"
#include "fileselect.h"
#include "search.h"
#include "help.h"
#include "../Lib/iniconfig.h"
#include "encryption.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    IniConfig ini ;


public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    void setPath(QString directory = QString(""), QString fullpath = QString("")) ;

private:
    bool Close() ;
    bool Save(bool ask=false, bool force=false) ;
    bool Load(QString filepath = QString("")) ;
    qint64 masterPID(QSharedMemory *sharedMem) ;
    bool Backup() ;
    void beep() ;
    void msg(QString msg) ;

private slots:
    void on_action_Open_triggered();
    void on_action_Save_triggered();
    void on_action_Find_triggered();
    void on_actionFind_Next_triggered();
    void on_actionE_xit_triggered();
    void on_actionFind_Global_triggered();
    void on_actionFind_Old_triggered();
    void on_action_Delete_triggered();
    void on_action_Undelete_triggered();
    void on_actionRename_File_triggered();
    void on_action_Insert_Template_triggered();
    void on_action_Help_triggered();
    void on_action_Email_triggered();
    void on_action_About_triggered();
    void on_actionSet_Encryption_Key_triggered();
    void on_actionChange_Password_triggered();

    void on_action_Logout_triggered();

    void on_textEdit_textChanged();

    void on_action_SetAsDefault_triggered();

private:
    Encryption *enc;
    bool debugenabled ;
    QString databasedir ;
    ImportFilter ifilter ;
    QString emailcommand ;
    QSharedMemory *editor ;
    QString buffer ;
    FileSelect fs ;
    QString searchtext ;
    Ui::MainWindow *ui;
    Help help ;
    bool isimportable ;
    QLabel *readonlylabel, *encryptedlabel, *dirtylabel ;
};

#endif // MAINWINDOW_H
