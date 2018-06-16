#include <QFile>
#include <QTextStream>
#include <QString>
#include <QProgressDialog>
#include <QPushButton>
#include "filetypes.h"
#include "globalsearch.h"
#include "ui_globalsearch.h"
#include "listviewstrings.h"
#include "../Lib/supportfunctions.h"

// TODO: Control-shift-G returns current file if latest backup is chosen

GlobalSearch::GlobalSearch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Search)
{
    getSelectionResponse = "" ;
    ui->setupUi(this);
    clearResults() ;
    ui->listSearchResults->setModel(strings.getModel()) ;

    QPushButton* okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    okBtn->setAutoDefault(true);
    okBtn->setDefault(true);

    QPushButton* caBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    caBtn->setAutoDefault(false);
    caBtn->setDefault(false);
}

GlobalSearch::~GlobalSearch()
{
    delete ui;
}

void GlobalSearch::setSearch(ImportFilter *importfilter, QString& path, bool currentfiles, QString& searchtext)
{
    this->path=path ;
    this->currentfiles=currentfiles ;
    this->searchtext=searchtext ;
    this->importfilter = importfilter ;
}


void GlobalSearch::addString(QString resulttext, QString resulthint)
{
    strings.addString(resulttext, resulthint) ;
    entries++ ;
}

QString& GlobalSearch::getSelection()
{
    int index = ui->listSearchResults->currentIndex().row() ;
    if (index>=0)
        getSelectionResponse = strings.hintAt(index) ;
    else
        getSelectionResponse = "" ;
    return getSelectionResponse ;
}

QString& GlobalSearch::getSelectionFileName()
{
    int index = ui->listSearchResults->currentIndex().row() ;
    if (index>=0)
        getSelectionFilenameResponse = strings.stringAt(index) ;
    else
        getSelectionFilenameResponse = "" ;
    return getSelectionFilenameResponse ;
}


int GlobalSearch::exec()
{
    clearResults() ;

    QStringList directories ;
    parseDirectory(path, directories, "") ;
    int isz=directories.size() ;

    QIcon icon(":icon-image.png");

    QProgressDialog progress("Easy Notepad Global Search", "Abort Search", 0, isz, this);
    progress.setAccessibleName("Easy Notepad Global Search Progress") ;
    progress.setMinimumDuration(0);
    progress.setWindowIcon(icon) ;

    progress.setWindowModality(Qt::WindowModal);
    progress.show() ;

    for (int i=0; i<isz; i++) {

        // Update progress bar and allow progress bar to update
        progress.setValue(i) ;
        qApp->processEvents();

        if (progress.wasCanceled())
            break ;
        QApplication::processEvents() ;

        QStringList files ;
        QString directory = directories.at(i) ;
        QString folderpath = path + "/" + directory ;

        if (currentfiles) {
            parseDirectory(folderpath, files, TXT, false, true) ;
            parseDirectory(folderpath, files, ENC, false, false) ;
            for (int j=0, jsz=files.size(); j<jsz; j++) {
                QString file = files.at(j) ;
                QString filepath = folderpath + "/" + file ;
                if (searchInFile(file, filepath, searchtext) ) {
                    addString(directory.replace(ENF,"") + ": " + file.replace(TXT,"").replace(ENC,""), filepath) ;
                }
            }

        } else {
            QStringList backups ;
            parseDirectory(folderpath, backups, "") ;
            for (int j=0, jsz=backups.size(); j<jsz; j++) {
                QString backup = backups.at(j) ;
                QString backuppath = folderpath + "/" + backup ;
                parseDirectory(backuppath, files, BAK, true, true) ;
                parseDirectory(backuppath, files, ENB, true, false) ;
                for (int k=0, ksz=files.size(); k<ksz; k++) {
                    QString file = files.at(k) ;
                    QString filepath = backuppath + "/" + file ;
                    if (searchInFile(file, filepath, searchtext)) {
                        addString(directory.replace(ENF,"") + ": " + backup.replace(BAK,"").replace(ENB,"") + " (" + parseBackupDate(files.at(k)) + ")" , filepath) ;
                    }
                }
            }
        }
    }

    progress.setValue(isz) ;
    progress.cancel() ;

    if (entries>0) {
        ui->listSearchResults->setFocus() ;
        return QDialog::exec() ;
    } else {
        return true ;
    }
}

// Used by EasyNotepad
QString& GlobalSearch::parseBackupDate(QString backupdate)
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

//
// Private Slots
//
void GlobalSearch::on_listSearchResults_clicked(const QModelIndex &index)
{
    Q_UNUSED(index) ;
    // TODO: store index so that serc form can be queried for the ID
    //selectedindex = ui->listSearchResults->currentIndex().row() ;
}


//
// Private Functions
//

void GlobalSearch::clearResults()
{
    ui->listSearchResults->reset() ;
    strings.clearStrings() ;
    selectedindex = -1 ;
    entries = 0 ;
}


bool GlobalSearch::searchInFile(QString filename, QString filepath, QString text)
{
    text.replace("\n", "") ;
    text.replace("\r", "") ;
    text.replace("\t","") ;
    text.replace(" ","") ;

    QRegExp re( ".*(" + text + ").*" ) ;
    re.setCaseSensitivity(Qt::CaseInsensitive) ;

    QString testfile = deAccent(filename) ;
    testfile.replace(" ", "") ;
    testfile.replace("\t", "") ;

    if (re.exactMatch(testfile)) return true ;

    QString filetext ;

    if (!importfilter->LoadFile(filepath, filetext)) return false ;

    filetext.replace("\n", "") ;
    filetext.replace("\r", "") ;
    filetext.replace("\t", "") ;
    filetext.replace(" ", "") ;

    re.setCaseSensitivity(Qt::CaseInsensitive) ;
    return (re.exactMatch(filetext)) ;
}

