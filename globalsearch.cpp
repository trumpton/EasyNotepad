#include <QFile>
#include <QTextStream>
#include <QString>
#include <QProgressDialog>
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
}

GlobalSearch::~GlobalSearch()
{
    delete ui;
}

void GlobalSearch::setSearch(QString& path, bool currentfiles, QString& searchtext)
{
    this->path=path ;
    this->currentfiles=currentfiles ;
    this->searchtext=searchtext ;
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

        progress.setValue(i) ;
        if (progress.wasCanceled())
            break ;
        QApplication::processEvents() ;

        QStringList files ;
        QString folderpath = path + "/" + directories.at(i) ;
        if (currentfiles) {
            parseDirectory(folderpath, files, TXT) ;
            for (int j=0, jsz=files.size(); j<jsz; j++) {
                QString filepath = folderpath + "/" + files.at(j) + "." + TXT ;
                if (searchInFile(files.at(j), filepath, searchtext) ) {
                    addString(directories.at(i) + ": " + files.at(j), filepath) ;
                }
            }
        } else {
            QStringList backups ;
            parseDirectory(folderpath, backups, "") ;
            for (int j=0, jsz=backups.size(); j<jsz; j++) {
                QString backuppath = folderpath + "/" + backups.at(j) ;
                parseDirectory(backuppath, files, BAK, true) ;
                for (int k=0, ksz=files.size(); k<ksz; k++) {
                    QString filepath = backuppath + "/" + files.at(k) + "." + BAK ;
                    if (searchInFile(files.at(k), filepath, searchtext)) {
                        addString(directories.at(i) + ": " + backups.at(j) + " (" + parseBackupDate(files.at(k)) + ")" , filepath) ;
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

void GlobalSearch::addString(QString resulttext, QString resulthint)
{
    strings.addString(resulttext, resulthint) ;
    entries++ ;
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

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&file);
    in.setCodec("UTF-8") ;
    QString filetext = in.readAll();
    file.close() ;

    filetext.replace("\n", "") ;
    filetext.replace("\r", "") ;
    filetext.replace("\t", "") ;
    filetext.replace(" ", "") ;

    re.setCaseSensitivity(Qt::CaseInsensitive) ;
    return (re.exactMatch(filetext)) ;
}

