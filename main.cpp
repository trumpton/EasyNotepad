#include "mainwindow.h"
#include <QApplication>
#include "filetypes.h"
#include "../Lib/supportfunctions.h"
#include <iostream>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    a.setApplicationName("EasyNotepad") ;
    a.setApplicationVersion(GITHASH) ;

    MainWindow w;

    QString directory = "", filename = "" ;

    if (argc>1 && argv[1][0]!='-') filename=argv[1] ;
    if (argc>2) directory=argv[2] ;
    filename.replace("\\", "/") ;
    directory.replace("\\", "/") ;

#ifdef DEBUG
    for (int i=0; i<argc; i++) {
        warningOkDialog(&w, "Debug", argv[i]) ;
    }
#endif

    w.setPath(directory, filename) ;
    w.show();


    return a.exec();
}
