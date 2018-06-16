#include "encryption.h"
#include "ui_encryption.h"

Encryption::Encryption(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Encryption)
{
    ui->setupUi(this);
}

Encryption::~Encryption()
{
    delete ui;
}
