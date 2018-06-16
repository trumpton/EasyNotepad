#ifndef ENCRYPTION_H
#define ENCRYPTION_H

//
// Encryption Class
//
// DESCRIPTION
//
// The Encryption Class is capable of saving and loading encrypted files.  All files are encrypted
// with the same key, which cannot be changed once set.
//
// The key itself is protected with a password, which must be entered each time the application is used.
// Note that multiple instances of he same application have access to the same key, so only the first
// application need login.
//
// THe login password can be changed if desired, and the key is re-encrypted with the new password.
// If the login password is forgotten, the {same} key can be re-installed, and a new login password
// can be created.  It is essential that the key is not lost.
//
//
// EXAMPLE
//
// Encryption *enc = new Encryption("company", "application") ;
//
// if (!hasKey()) { setKey() ; }
// if (!loggedin()) { login() ; }
//
// QString contents ;
//
// if (!load("file.enc", contents)) {
//    // Error, couldn't load
// }
//
// if (!save("file.enc", contents)) {
//    // Error, couldn't save
// }
//


#include <QDialog>
#include <QSharedMemory>

namespace Ui {
class Encryption;
}

class Encryption : public QDialog
{
    Q_OBJECT

public:
    typedef enum {
        NoKey,
        KeyLoaded,
        KeyDecrypted,
        KeyValidated
    } State ;

private:
    bool saveKey() ;

public:
    explicit Encryption(QString domain, QString application, QWidget *parent = 0);
    ~Encryption();

    // Dialog form functions
    void setKey() ;
    void changePassword() ;
    void login() ;
    void logout() ;

    // Current Status
    bool hasKey() ;
    bool loggedIn() ;

    // Load and Save File Data
    bool load(QString filename, QByteArray &data) ;
    bool save(QString filename, QByteArray data) ;



private:
    QString sDomain, sApplication ;
    Ui::Encryption *ui;
    QSharedMemory sharedmem ;

};

#endif // ENCRYPTION_H
