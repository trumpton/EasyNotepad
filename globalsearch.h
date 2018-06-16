#ifndef GLOBALSEARCH_H
#define GLOBALSEARCH_H

#include <QDialog>
#include <QStringList>
#include "listviewstrings.h"
#include "importfilter.h"

namespace Ui {
class Search;
}

class GlobalSearch : public QDialog
{
    Q_OBJECT
    
private:
    QString getSelectionResponse, getSelectionFilenameResponse ;
    QString path ;
    bool currentfiles ;
    QString searchtext ;

    Ui::Search *ui;
    ListViewStrings strings ;
    int selectedindex ;
    int entries ;

    ImportFilter *importfilter ;


    QString& parseBackupDate(QString backupdate) ;



public:
    explicit GlobalSearch(QWidget *parent = 0);
    ~GlobalSearch();


    void setSearch(ImportFilter *importfilter, QString& path, bool searchcurrent, QString& searchtext) ;
    int exec() ;
    QString& getSelection() ;
    QString& getSelectionFileName() ;

private slots:
    void on_listSearchResults_clicked(const QModelIndex &index);

private:
    void clearResults() ;
    void addString(QString resulttext, QString resulthint) ;
    bool searchInFile(QString filename, QString filepath, QString text) ;

};

#endif // GLOBLSEARCH_H
