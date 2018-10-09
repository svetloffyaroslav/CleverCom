//+++++++++++++++++++++++++++++++++Главный метод программы+++++++++++++++++++++++++++++++++++++++++
//+++ Описывает действия предпринимаемые в главном окне - нажатие кнопок, отправку сооюбщений,  +++
//+++ вывод информации, индикация. Здесь осуществляется формирование команд на передачу и       +++
//+++ прием сообщений с контроллера.                                                            +++
//++++      /**/ - обозначаются строчки кода, где присутсвтуют двухзначыне номера кнопок        +++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//----------------------------------------------------------------------------------------Светлов Я.
//----------------------------------------------------------------------svetloff.yaroslav@gmail.com




#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <port.h>
#include <QFileDialog>
#include "comdial.h"
#include <QMap>
#include "consoll.h"
#include <QTimer>
#include <QTimerEvent>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QHideEvent>
#include <QDataStream>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QAction>
#include <QStringList>
#include <QtEndian>
#include <QMenu>
#include <QStyle>
#include <QClipboard>
#include <QPalette>



class comdial;                  // класс диалогового окна осуществляющего настройку и подлкючениие к Com- порту

struct verOne                   //  Структура для режима работы  кнопок 1
{
    int color;                  // цвет, в который будет окрашиваться кнопка после нажатия
    int endcolor;
    QByteArray arrStart;        // массив байт, который будет передаваться при первом нажатии
    QByteArray arrEnd;          // массив бай, который будет передаваться при повторном нажатии
    int CSbyte;
};

struct verTwo                   //  Структура для режима работы 2
{
     QByteArray arrSend;         // массив байт, который будет отправляться
     QByteArray arrCheck;        // массив байт, с которым будет сравниваться ответ
     int check_byte;             // номер байта, с которого будет осущетсвляться сравнение
     int CSbyte;
};


struct verThree                 // структура для режима работы кнопок  3
{
     QByteArray arrSend;        // массив байт, который отправляется при нажатии
     int byte_Order;
     int byte_StartView;        // номер байта с которого будет осущетсвлятся показ ответа в окне СООБЩЕНИЕ в 10-чном виде
     int byte_EndView;          // номер байта с которого будет заканчиваться показ ответа в окне СООБЩЕНИЕ в 10-чном виде
     int CSbyte;
};





namespace Ui
{
        class MainWindow;
}



class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:

 void closeEvent(QCloseEvent * event);                          // Вируальная функция родительского класса в нашем классе, переопределяется для изменения поведения



public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


    int ParINT;

  void writeSettings();

  void readSettings();
  int m_nCounterin;                             // подсчет количества

   QString NameofFile;
   quint8 Adr;                                  // переменная для адреса модуля

   void AnswerVerTWO();                         // функции обработки ответов, для режима работы кнопок 2

   void AnswerVerTHREE();                       // функция обработки ответов,  для режима работы кнопок 3

   int colSEND;                                 // подсчет количества отправленных команд

   int colARR;                                  // подсчет количества принятых  ответов

                                                // Некоторые переменные для режима работы кнопок 2
   int verA;                                    // какой режим работы кнопки, что вы сейчакс нажали
   int numberButton;                            // номер кнопки
   int checkByte;                               // байт с которого начать сравнение
   QByteArray CheckWith;                        // Массив байт с которым будет осущетсвляться проверка

   int byteOrd;                                 // Некоторые переменные для режима работы кнопок 3
   int startVByte;                              // байт с которого начнется отображение в поле СООБЩЕНИЕ
   int endVByte;


   void  ErrorMes(int num);
   port *PortNNew;                              // Инициализация класса порта

  comdial *ConnectWithPort;                     // Инициализация класса диалогового окна подключения порта

    verOne VI;                                  // Инициализация структур для 3-х режимов работы кнопок

    verTwo VII;

    verThree VIII;

    QByteArray AnswerArray;

   void ProcessforVerOne(int num, QString RDColor, QString RDEColor, QString RDStartArr, QString RDEndArr, QString RDCS);          // функции необходимые для обработки настроечного файла
   void ProcessforVerTWO(int num,QString RDSendArr, QString RDCompareArr, QString byteCompare, QString RDCS);
   void ProcessforVerThree(int num, QString RDSendArr, QString RDByteOrder, QString ByteStartView, QString ByteEndView, QString RDCS);






   QMap <QString,QByteArray> arraysSend;                                // контейнер, для хранения  ВСЕХ  массивов байт, готовых  к отправке
   QMap<QString,int> mode;                                              // контейнер, для хранения  ВСЕХ  режимов работы инициализированных кнопок
   QMap<QString,int> bytes;                                             // контейнер для хранения ВСЕХ номеров байт для сравнения, отображения
   QMap<QString,QString> buttonNames;
   QMap<QString,int> color;
   QMap <QString,bool> enableBut;

/**/   void Button11(int ver,bool press);                               // функции в которых осущетсвляется работа кнопок
/**/   void Button12(int ver,bool press);
/**/   void Button13(int ver,bool press);
/**/   void Button14(int ver,bool press);
/**/   void Button15(int ver,bool press);
/**/   void Button16(int ver,bool press);


/**/   void Button21(int ver,bool press);
/**/   void Button22(int ver,bool press);
/**/   void Button23(int ver,bool press);
/**/   void Button24(int ver,bool press);
/**/   void Button25(int ver,bool press);
/**/   void Button26(int ver,bool press);


/**/   void Button31(int ver,bool press);
/**/   void Button32(int ver,bool press);
/**/   void Button33(int ver,bool press);
/**/   void Button34(int ver,bool press);
/**/   void Button35(int ver,bool press);
/**/   void Button36(int ver,bool press);

  void  pressButtonBlockOthers(bool press);                             // бклоирует кнопки при нажатии

  QTimer *waitTime;                                                   // 4 секунды ожидания ответа

  QTimer *colorTime;                                                  // таймер смены цвета на фоновый для режима работы кнопок 1


signals:

    void dis();                                                        // отключение порта

    void WriteDatatoPort(QByteArray data);                              // запись данных в порт

    void savesettingsPrevious(QString,int,int,int,int);

private slots:
    void on_action_ConnectComPort_triggered();                          // подключение Com-порта в меню

    void on_action_DisconnectComPort_triggered();                       // отключение Com-порта в меню

    void on_action_OpenTXT_triggered();                                 // открытие файла настройки

    void ShowComPortState(QString name, QString rate, QString dataBitsString, int parity, QString stopbit);         // вывод информации о состоянии порта

    void showStat(QString dataTostatus);                                        // показывает статус 5 сек

    void  AbleFromComPort(bool port);                                          // Блокировка кнопок при отключении порта

     void putDataInConsole(QByteArray data);                                   // Вывод информации о отправленной команде

     void on_pushButton_11_pressed();                                           // нажатие на кнопки

     void on_pushButton_12_pressed();
     void on_pushButton_13_pressed();

     void on_pushButton_14_pressed();

     void on_pushButton_15_pressed();

     void on_pushButton_16_pressed();

     void on_pushButton_21_pressed();

     void on_pushButton_22_pressed();

     void on_pushButton_23_pressed();

     void on_pushButton_24_pressed();

     void on_pushButton_25_pressed();

     void on_pushButton_26_pressed();

     void on_pushButton_31_pressed();

     void on_pushButton_32_pressed();

     void on_pushButton_33_pressed();

     void on_pushButton_34_pressed();

     void on_pushButton_35_pressed();

     void on_pushButton_36_pressed();

     void on_pushButton_CleanSEND_clicked();                                    // нажатие на кнопку очистить консоль отправленных

     void on_pushButton_CleanARR_clicked();                                     // нажатие на кнопку очистить консоль принятых

     void on_pushButton_SAVESEND_clicked();                                     // сохранить отправленные команды

     void on_pushButton_SAVEARR_clicked();                                      // сохранить принятые ответы

     void devicekeepSilence();                                                  // функция  - вызывается когда ответ не пришел

     void  Send_DataSlot(QByteArray answer);                                     // прием ответа модуля

     void changeColorDef();                                                     //Возвращение кнопок к первоначальному  цвету  - если режим их работы 2 и они были активированы

     void on_action_OpenTXT_changed();

     void on_action_triggered(bool trey);

     void iconActivated(QSystemTrayIcon::ActivationReason reason);              // слот, котораый будет принимать сигнал от события нажатия на иконку приложения в трее

     void on_pushButton_Copy_clicked();

     void on_action_Invize_triggered(bool vizible);

//     void on_action_darkWindow_triggered(bool isDark);

//     void on_action_darkWindow_triggered();

private:
    Ui::MainWindow *ui;

    QSettings  m_settings;

    int        m_nCounter;

    QSystemTrayIcon *trayIcon;                          // Объявляем объект будущей иконки приложения для трея
};

#endif // MAINWINDOW_H
