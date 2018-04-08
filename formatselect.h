#ifndef FORMATSELECT_H
#define FORMATSELECT_H

#include <QString>
#include <QDialog>

namespace Ui {
class FormatSelect;
}

class FormatSelect : public QDialog
{
    Q_OBJECT

public:
    explicit FormatSelect(QWidget *parent = 0);
    ~FormatSelect();
    void addItem(QString description, QString codec) ;
    QString getSelection() ;
    int exec() ;


private:
    Ui::FormatSelect *ui;
};

#endif // FORMATSELECT_H
