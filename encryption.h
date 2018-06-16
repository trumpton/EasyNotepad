#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <QDialog>

namespace Ui {
class Encryption;
}

class Encryption : public QDialog
{
    Q_OBJECT

public:
    explicit Encryption(QWidget *parent = 0);
    ~Encryption();

private:
    Ui::Encryption *ui;
};

#endif // ENCRYPTION_H
