
#include <QDialog>
#include "formatselect.h"
#include "ui_formatselect.h"
#include "../Lib/alertsound.h"
#include "../Lib/iniconfig.h"

FormatSelect::FormatSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FormatSelect)
{
    ui->setupUi(this);
}

void FormatSelect::addItem(QString description, QString codec)
{
    ui->selectCodec->addItem(description, codec) ;
    ui->selectCodec->setCurrentIndex(0) ;
}

QString FormatSelect::getSelection()
{
    if (ui->selectCodec->currentIndex()<0) return QString("") ;
    else return ui->selectCodec->currentData() ;
}

int FormatSelect::exec()
{
    ui->selectCodec->setCurrentIndex(0) ;
    ui->helpText->setFocus() ;
    play(Query) ;
    return QDialog::exec() ;
}

FormatSelect::~FormatSelect()
{
    delete ui;
}
