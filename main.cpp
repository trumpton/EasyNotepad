#include "mainwindow.h"
#include <QApplication>
#include "filetypes.h"
#include "../Lib/supportfunctions.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc>1 && argv[1][0]=='-' && argv[1][1]!='\0') {

        switch (argv[1][1]) {
        case 'v':
            std::cout << appHash().replace("v","").toLatin1().data() << "\n" ;
            break ;
        case 'l':
            std::cout << libVersion().replace("v","").toLatin1().data() << "\n" ;
            break ;
        case 'd':
            std::cout << buildDate().toLatin1().data() << "\n" ;
            break ;
        case 'a':
            std::cout << appHash().replace("v","").toLatin1().data() << "\n" ;
            std::cout << libVersion().replace("v","").toLatin1().data() << "\n" ;
            std::cout << buildDate().toLatin1().data() << "\n" ;
            break ;
        default:
            std::cout << "EasyNotepad -adlv\n -a - Show all\n -d - Build date\n -l - Library version\n -v - Application version\n" ;
            break ;
        }
        return 1 ;

    } else {

	    QApplication a(argc, argv);
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
}
