#include "encryption.h"
#include "ui_encryption.h"
#include "../Lib/aes.h"

#include <QCryptographicHash>
#include <QSettings>
#include <QMessageBox>
#include <QFile>

// Hash is 512 bits / 64 bytes long
#define HASHALG QCryptographicHash::Sha512
#define HASHB 64
#define HALFHASHB 32
#define BLOCKSIZE 16

#ifdef _MSC_VER
#define random() rand()
#endif

//
// Shared Memory Data
//
typedef struct {
    bool dirty ;
    bool plaintextkeypresent ;
    unsigned char plaintextkey[HALFHASHB] ;    // CiphertextKey[0..255] ^ Password[0..255]
    unsigned char plaintexthash[HALFHASHB] ;   // CiphertextKey[256..512] ^ Password[0..255]
    bool ciphertextkeypresent ;
    unsigned char ciphertextkey[HALFHASHB] ;   // CiphertextKey[0..255]
    unsigned char ciphertexthash[HALFHASHB] ;  // CiphertextKey[256..512]
    unsigned char passwordhash[HALFHASHB] ;    // Password[HALFHASHB..512]
} ShMem ;


Encryption::Encryption(QString domain, QString application, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Encryption)
{
    sDomain = domain ;
    sApplication = application ;

    sharedmem.setKey(sDomain + QString(".") + sApplication) ;
    if (!sharedmem.attach()) {
        if (!sharedmem.create(sizeof(ShMem))) {
            // Error Creating Shared Memory
        } else {
            // Create and Initialise Shared Memory
            if (sharedmem.lock()) {
                sharedmem.create(sizeof(ShMem)) ;
                ShMem *mem = (ShMem *)sharedmem.data() ;
                mem->plaintextkeypresent=false ;
                mem->ciphertextkeypresent=false ;
                mem->dirty=false ;
                QSettings ini(sDomain, sApplication) ;
                QByteArray encryptedKey=ini.value("A").toByteArray() ;
                QByteArray encryptedHash=ini.value("B").toByteArray() ;
                QByteArray passwordHash=ini.value("C").toByteArray() ;
                if (!encryptedKey.isEmpty() && !passwordHash.isEmpty() && !encryptedHash.isEmpty()) {
                    mem->ciphertextkeypresent=true ;
                    for (int i=0; i<HALFHASHB; i++) {
                        mem->ciphertextkey[i] = encryptedKey.at(i) ;
                        mem->ciphertexthash[i] = encryptedHash.at(i) ;
                        mem->passwordhash[i] = passwordHash.at(i) ;
                    }
                    mem->dirty=false ;
                }
                sharedmem.unlock() ;

            }
        }
    }
    ui->setupUi(this);
}

Encryption::~Encryption()
{
    sharedmem.detach() ;
    delete ui;
}

bool Encryption::saveKey()
{
    QSettings ini(sDomain, sApplication) ;

    if (!sharedmem.lock()) {
        return false ;

    } else {

        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;
        if (shm->dirty) {
            QByteArray encryptedKey ;
            QByteArray encryptedHash ;
            QByteArray hashPassword ;
            encryptedKey.setRawData((char *)shm->ciphertextkey, HALFHASHB) ;
            encryptedHash.setRawData((char *)shm->ciphertexthash, HALFHASHB) ;
            hashPassword.setRawData((char *)shm->passwordhash, HALFHASHB) ;
            ini.setValue("A", encryptedKey) ; // A = Confidentiality Key
            ini.setValue("B", encryptedHash) ;// B = Signature Hash Key
            ini.setValue("C", hashPassword) ; // C = Password Hash
            shm->dirty=false ;
        }
        sharedmem.unlock() ;

        ini.sync() ;
        return true ;
    }
}


//
// Status
//
bool Encryption::hasKey()
{
    bool hasciphertextkey = false ;
    if (sharedmem.lock()) {
        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;
        hasciphertextkey = shm->ciphertextkeypresent ;
        sharedmem.unlock() ;
    }
    return hasciphertextkey ;
}

bool Encryption::loggedIn()
{
    bool hasplaintextkey = false ;
    if (sharedmem.lock()) {
        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;
        hasplaintextkey = shm->plaintextkeypresent ;
        sharedmem.unlock() ;
    }
    return hasplaintextkey ;
}


//
// Manage Key and Password
//

void Encryption::setKey()
{
    QByteArray hashKey, hashPass ;

    ui->label0->setVisible(true) ;
    ui->lineEdit0->setVisible(true) ;
    ui->label1->setVisible(true) ;
    ui->lineEdit1->setVisible(true) ;
    ui->label2->setVisible(true) ;
    ui->lineEdit2->setVisible(true) ;
    ui->label3->setVisible(true) ;
    ui->lineEdit3->setVisible(true) ;

    ui->label0->setText("Encryption Key") ;
    ui->label1->setText("Re-enter") ;
    ui->label2->setText("Initial Password") ;
    ui->label3->setText("Re-enter") ;
    ui->lineEdit0->clear() ;
    ui->lineEdit1->clear() ;
    ui->lineEdit2->clear() ;
    ui->lineEdit3->clear() ;
    ui->lineEdit0->setFocus() ;
    this->setWindowTitle("Enter Key and Password") ;

    if (this->exec()==QDialog::Accepted) {

        if (ui->lineEdit0->text().compare(ui->lineEdit1->text())!=0) {
            // Error, Mismatch
            QMessageBox::critical(this, "Set Key", "Key mismatch, update not processed", QMessageBox::Ok) ;
            return ;
        }
        hashKey = QCryptographicHash::hash(ui->lineEdit0->text().toLocal8Bit(), HASHALG);

        if (ui->lineEdit2->text().compare(ui->lineEdit3->text())!=0) {
            // Error, Mismatch
            QMessageBox::critical(this, "Set Key", "Password mismatch, update not processed", QMessageBox::Ok) ;
            return ;
        }
        hashPass = QCryptographicHash::hash(ui->lineEdit2->text().toLocal8Bit(), HASHALG);

    } else {

        // Cancelled
        return ;
    }

    if (hashKey.length()==HASHB && hashPass.length()==HASHB) {
        if (!sharedmem.lock()) {
            // Error locking memory
            QMessageBox::critical(this, "Set Key", "Unable to lock shared memory", QMessageBox::Ok) ;
            return ;
        } else {
            ShMem *shm ;
            shm = (ShMem*) sharedmem.data() ;

            for (int i=0; i<HALFHASHB; i++) {
                shm->plaintextkey[i] = hashKey[i] ;
                shm->plaintexthash[i] = hashKey[i+HALFHASHB] ;
                shm->ciphertextkey[i] = hashKey[i] ^ hashPass[i] ;
                shm->ciphertexthash[i] = hashKey[i+HALFHASHB] ^ hashPass[i] ;
                shm->passwordhash[i] = hashPass[i+HALFHASHB] ;
            }

            shm->plaintextkeypresent=true ;
            shm->ciphertextkeypresent=true ;
            shm->dirty=true ;
            sharedmem.unlock() ;
            if (!saveKey()) {
                // Error saving key
                QMessageBox::critical(this, "Set Key", "Unable to store the key", QMessageBox::Ok) ;
                return ;
            }
        }
    }
}

void Encryption::changePassword()
{
    ui->label0->setVisible(true) ;
    ui->lineEdit0->setVisible(true) ;
    ui->label1->setVisible(true) ;
    ui->lineEdit1->setVisible(true) ;
    ui->label2->setVisible(true) ;
    ui->lineEdit2->setVisible(true) ;
    ui->label3->setVisible(false) ;
    ui->lineEdit3->setVisible(false) ;

    ui->label0->setText("Old Password") ;
    ui->label1->setText("New Password") ;
    ui->label2->setText("Re-enter") ;
    ui->lineEdit0->clear() ;
    ui->lineEdit1->clear() ;
    ui->lineEdit2->clear() ;
    ui->lineEdit3->clear() ;
    ui->lineEdit0->setFocus() ;
    this->setWindowTitle("Change Password") ;

    if (this->exec()==QDialog::Accepted) {

        // Check Password Match
        if (ui->lineEdit2->text().compare(ui->lineEdit1->text())!=0) {
            // Error, Mismatch
            QMessageBox::critical(this, "Change Password", "Password mismatch, update aborted", QMessageBox::Ok) ;
            return ;
        }

        if (!sharedmem.lock()) {
            // Error locking shared memory
            QMessageBox::critical(this, "Change Password", "Unable to lock shared memory", QMessageBox::Ok) ;
            return ;

        } else {
            ShMem *shm ;
            shm = (ShMem*) sharedmem.data() ;

            // Calculate and Check Old Password
            QByteArray hashOldPassword = QCryptographicHash::hash(ui->lineEdit0->text().toLocal8Bit(), HASHALG);
            for (int i=0; i<HALFHASHB; i++) {
                if ((unsigned char)hashOldPassword[i+HALFHASHB]!=shm->passwordhash[i]) {
                    sharedmem.unlock() ;
                    // Invalid Old Password
                    QMessageBox::critical(this, "Change Password", "Old Password incorrect, update aborted", QMessageBox::Ok) ;
                    return ;
                }
            }

            // Calculate New Password and Generate New Ciphertext Key
            QByteArray hashPassword = QCryptographicHash::hash(ui->lineEdit1->text().toLocal8Bit(), HASHALG);
            for (int i=0; i<HALFHASHB; i++) {
                shm->ciphertextkey[i] = shm->ciphertextkey[i] ^ hashPassword[i] ^ hashOldPassword[i] ;
                shm->ciphertexthash[i] = shm->ciphertexthash[i] ^ hashPassword[i] ^ hashOldPassword[i] ;
                shm->passwordhash[i] = hashPassword[i+HALFHASHB] ;
            }

            shm->plaintextkeypresent = true ;
            shm->ciphertextkeypresent = true ;
            shm->dirty=true ;
            sharedmem.unlock() ;

            if (!saveKey()) {
                // Error saving Key
                QMessageBox::critical(this, "Change Password", "Error saving key, update aborted", QMessageBox::Ok) ;
                return ;
            }
        }
    }

}


void Encryption::login()
{
    QString password ;

    ui->label0->setVisible(true) ;
    ui->lineEdit0->setVisible(true) ;
    ui->label1->setVisible(false) ;
    ui->lineEdit1->setVisible(false) ;
    ui->label2->setVisible(false) ;
    ui->lineEdit2->setVisible(false) ;
    ui->label3->setVisible(false) ;
    ui->lineEdit3->setVisible(false) ;

    ui->label0->setText("Password") ;
    ui->lineEdit0->clear() ;
    ui->lineEdit1->clear() ;
    ui->lineEdit2->clear() ;
    ui->lineEdit3->clear() ;
    ui->lineEdit0->setFocus() ;
    this->setWindowTitle("Login") ;

    if (!this->exec()==QDialog::Accepted) {
        return ;
    }

    password = ui->lineEdit0->text() ;

    if (!sharedmem.lock()) {
        // Error Locking Memory
        QMessageBox::critical(this, "Login", "Error locking memory, login aborted", QMessageBox::Ok) ;
        return ;

    } else {

        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;

        if (!shm->ciphertextkeypresent) {
            sharedmem.unlock() ;
            // No Ciphertext Key Present
            QMessageBox::critical(this, "Login", "No Key is present, login aborted", QMessageBox::Ok) ;
            return ;

        } else {

            QByteArray hashPass = QCryptographicHash::hash(password.toLocal8Bit(), HASHALG);

            // Check Password
            for (int i=0; i<HALFHASHB; i++) {
                if ((unsigned char)hashPass[i+HALFHASHB]!=shm->passwordhash[i]) {
                    // Error, unable to login
                    shm->plaintextkeypresent=false ;
                    sharedmem.unlock() ;
                    return ;
                }
            }

            // Generate Plaintext Key
            for (int i=0; i<HALFHASHB; i++) {
                shm->plaintextkey[i] = shm->ciphertextkey[i] ^ hashPass[i] ;
                shm->plaintexthash[i] = shm->ciphertexthash[i] ^ hashPass[i] ;
            }

            shm->plaintextkeypresent=true ;
            sharedmem.unlock() ;

        }
    }
}



void Encryption::logout()
{
    if (!sharedmem.lock()) {
        // Error Locking Memory
        QMessageBox::critical(this, "Logout", "Error locking memory, logout aborted", QMessageBox::Ok) ;
        return ;

    } else {

        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;

        shm->plaintextkeypresent=false ;

        for (int i=0; i<sizeof(shm->plaintextkey); i++) shm->plaintextkey[i]='\0' ;
        for (int i=0; i<sizeof(shm->plaintexthash); i++) shm->plaintexthash[i]='\0' ;

        sharedmem.unlock() ;

    }

}

//
// File Load and Save
//

//
// File Format:
//
// IV
// [Block]
// [Block]
// [Block]
// [Block Pad]
// [LenTail]
// [Tail]
//
// IDENT - File Type Identifier
// IV - a 128-bit sequence, containing an identifier, randomisation and length
// Block - 128-bits of encrypted data
// LenTail - Padding Quantity identifier
// Tail - signature tail
//

typedef struct {
    unsigned long int length ;
    unsigned char random[BLOCKSIZE - sizeof(unsigned long int)] ;
} IV ;

//
// Load text from an encrypted file
//
bool Encryption::load(QString filename, QByteArray &data)
{
    if (!loggedIn()) return false ;

    if (!sharedmem.lock()) {
        return false ;

    } else {

        AES aes(256) ;
        unsigned char buf[BLOCKSIZE], outputbuf[BLOCKSIZE] ;
        unsigned char *databuf = (unsigned char *)data.data() ;

        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;

        // Open the File
        QFile file(filename) ;
        if (!file.open(QIODevice::ReadOnly)) {
            sharedmem.unlock() ;
            return false ;
        }

        // Read and check the Header
        if (file.read((char *)buf, BLOCKSIZE)!=BLOCKSIZE) {
            file.close() ;
            sharedmem.unlock() ;
            return false ;
        }
        const char *validheader="&!ENCRY-00000001" ;
        for (int i=0; i<BLOCKSIZE; i++) {
            if (validheader[i]!=buf[i]) {
                file.close() ;
                sharedmem.unlock() ;
                return false ;
            }
        }

        // Read the IV
        if (file.read((char *)buf, BLOCKSIZE)!=BLOCKSIZE) {
            file.close() ;
            sharedmem.unlock() ;
            return false ;
        }

        long int len = file.size() ;
        long int pos = 0 ;

        do {

            // Generate the next buffer
            aes.encrypt(shm->plaintextkey, buf, outputbuf) ;

            // Read the IV
            if (file.read((char *)buf, BLOCKSIZE)!=BLOCKSIZE) {
                file.close() ;
                sharedmem.unlock() ;
                return false ;
            }

            for (int i=0; i<BLOCKSIZE; i++) {
                unsigned char ch = (buf[i] ^ outputbuf[i]) ;
                data.append(ch) ;
            }

            pos+=16 ;

        } while (pos<(len-BLOCKSIZE*3)) ;

        // Extract the padding
        long int padding = (unsigned char)data.at(pos-BLOCKSIZE) ;

        // Truncate the data to remove the padding & padding count
        data.truncate(pos+padding-BLOCKSIZE*2) ;

        // Calculate the check
        aes.encrypt(shm->plaintexthash, buf, outputbuf) ;

        // Read and Check the Check
        if (file.read((char *)buf, BLOCKSIZE)!=BLOCKSIZE) {
            file.close() ;
            sharedmem.unlock() ;
            return false ;
        }

        QByteArray dochash = QCryptographicHash::hash(data, HASHALG);
        for (int i=0; i<BLOCKSIZE; i++) {
            unsigned char recoveredhash = (outputbuf[i] ^ buf[i]) ;
            if ((unsigned char)dochash[i] != recoveredhash) {
                file.close() ;
                sharedmem.unlock() ;
                return false ;
            }
        }

        file.close() ;
        sharedmem.unlock() ;
        return true ;

    }
}


//
// Save text to an encrypted file
//
bool Encryption::save(QString filename, QByteArray data)
{
    if (!loggedIn()) return false ;

    if (!sharedmem.lock()) {
        return false ;

    } else {

        AES aes(256) ;
        unsigned char buf[BLOCKSIZE], outputbuf[BLOCKSIZE] ;
        unsigned char *databuf = (unsigned char *)data.data() ;

        ShMem *shm ;
        shm = (ShMem*) sharedmem.data() ;

        // Open the File
        QFile file(filename) ;
        if (!file.open(QIODevice::WriteOnly)) {
            sharedmem.unlock() ;
            return false ;
        }

        // Output the Header
        if (!file.write("&!ENCRY-00000001", 16)) {
            file.close() ;
            sharedmem.unlock() ;
            return false ;
        }

        // Build The first buffer (IV)
        for (unsigned int i=0; i<sizeof(buf); i++) {
            buf[i] = random()&0xFF ;
        }

        // Write the IV
        if (!file.write((const char *)buf, sizeof(buf))) {
            file.close() ;
            sharedmem.unlock() ;
            return false ;
        }

        long int len = data.length() ;
        long int pos = 0 ;

        do {

            if (pos<len) {

                // Encrypt the block
                aes.encrypt(shm->plaintextkey, buf, outputbuf) ;
                for (int i=0; i<BLOCKSIZE; i++) {
                    if (pos+i<len) {
                        buf[i] = outputbuf[i] ^ databuf[pos+i] ;
                    } else {
                        buf[i] = random()&0xFF ;
                    }
                }

            } else {

                // Last Data Block is Flags (amount of padding)
                aes.encrypt(shm->plaintextkey, buf, outputbuf) ;
                buf[0]=(unsigned char)(len%BLOCKSIZE) ^ outputbuf[0] ;
                for (unsigned int i=1; i<sizeof(buf); i++) {
                    buf[i]=random()&0xFF ^ outputbuf[i];
                }

            }

            // Write the buffer
            if (!file.write((const char *)buf, sizeof(buf))) {
                file.close() ;
                sharedmem.unlock() ;
                return false ;
            }

            pos+= 16 ;

        } while (pos<(len+BLOCKSIZE)) ;

        // Now calculate tail as of data (using the first 128 bits of the document hash)
        QByteArray dochash = QCryptographicHash::hash(data, HASHALG);
        aes.encrypt(shm->plaintexthash, buf, outputbuf) ;
        for (int i=0; i<BLOCKSIZE; i++) {
            buf[i] = outputbuf[i] ^ dochash.at(i);
        }

        // Write the tail
        if (!file.write((const char *)buf, sizeof(buf))) {
            file.close() ;
            sharedmem.unlock() ;
            return false ;
        }

        file.close() ;
        sharedmem.unlock() ;

    }

    return true ;
}
