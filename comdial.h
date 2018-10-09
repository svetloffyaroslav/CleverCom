
//+++++++++++++++++++++++Окно подключения ком-порта(с настройками).cpp++++++++++++++++++++
//+++ Вызывается из menu->Com-порт или по нажатию клавиш Ctrl+S                        +++
//+++ Содержит 2 кнопки и  5 combobox                                                  +++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//------------------------------------------------------------------------------Светлов Я.
//-------------------------------------------------------------svetloff.yaroslav@gmail.com
#ifndef COMDIAL_H
#define COMDIAL_H

#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include<port.h>
#include<mainwindow.h>


namespace Ui {
class comdial;
}

class comdial : public QDialog
{
    Q_OBJECT

public:
    explicit comdial(QWidget *parent = 0);
    ~comdial();


signals:
    void savesettings(QString name,int baudrate, int DataBits, int Parity, int StopBits); //сигнал вызываемый нажатии клавиши ок - передает выбранные хар-ки в port

private slots:
    void on_buttonBox_accepted();                               // нажатие кнопки OK

private:
    Ui::comdial *ui;
};

#endif // COMDIAL_H
