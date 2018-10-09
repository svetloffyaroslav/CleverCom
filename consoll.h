
//+++++++++++++++++++++++Консоль(в расширение окна доп.инфо).h++++++++++++++++++++++++++++
//+++ Вызывается из menu->Допололнительная информация или по нажатию клавиш Ctrl+W     +++
//+++ Описывает все операции,связанные с консолью, выводимую информацию, стиль         +++
//+++                                                                                  +++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//------------------------------------------------------------------------------Светлов Я.

#ifndef CONSOLL_H
#define CONSOLL_H
#include <QPlainTextEdit>
#include<QString>
#include<QDateTime>
#include <QPalette>

class consoll:public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit consoll(QWidget *parent = 0);

public:
    QString OnTimeAndDate;                              //Строка, в которую помещается дата и время запуска ПО- для сохранения
    QPalette p;


public slots:
    void putDataSend(QString data);                         //помещает данные в консоль
    void putDataArrive(QString data);
    void afterclean();                                  //Вывод сохраненной информации - после очистки
    void showcommandTime();                             //вывод времени в консоль - перед каждым сообщением

signals:
   void showcommandTimeSignal();                        //сигнал, который вызывает слот - показывающий время


//"Впитывает" события - не позволяет никому ничего вписать в консоль

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
    virtual void contextMenuEvent(QContextMenuEvent *e);
};



#endif // CONSOLL_H
