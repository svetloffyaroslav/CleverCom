
//+++++++++++++++++++++++++++++++++Работа с Com-gортом.h++++++++++++++++++++++++++++++++
//+++ Файл который регламентирует работу с последовательным портом - пока              +++
//+++ только откытие и закрытие. Здесь устанавливается настройки, описаны процессы     +++
//+++ отключения и подключения.                                                        +++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//------------------------------------------------------------------------------Светлов Я.
//-------------------------------------------------------------svetloff.yaroslav@gmail.com

#ifndef PORT_H
#define PORT_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>





//Структура  - содержащая настройки com-порта
struct Settings {
    QString name;                               //имя порта
    qint32 baudRate;                            //бодовая скорость
    QSerialPort::DataBits dataBits;             //длина слова
    QSerialPort::Parity parity;                 //Паритетность
    QSerialPort::StopBits stopBits;             //стоповых бит
    QSerialPort::FlowControl flowControl;       //текущий контроль
};



class port : public QObject
{
    Q_OBJECT
public:
    explicit port(QObject *parent = 0);
      ~port();
    
    QSerialPort thisPort;                        //создание порта
    
   Settings    SettingsPort;                   //Инициализациz структуры - настройка

   QByteArray arr;                              // Массив байт принятых сообщений
     
signals:
       void finished_Port();

       void error_(QString err);                   //Вывод сообщение в виде ошибки

       void infotomain(QString name,QString rate,QString databits,int parity,QString stopbits);             // отправка информации в главное окно

       void infotostatus(QString message);                                                                      // отправка информации в статус главного окна

       void connecttomenu(bool b);                                                                              // состояние главного окна

       void sendDataMain(QByteArray);


public slots:
      void process_Port();                                                                                               // слот вызывается при включении порта в поток

      void Write_Settings_Port(QString name,int baudrate, int DataBits, int Parity, int StopBits);                     // запись настроек

      void ConnectPort(void);                                                                                           //Подключение порта

      void readyRead_Slot();                                                                                            // чтение данных с порты

      void  WriteToPort(QByteArray data);                                                                               // запись данных в порт
      void DisconnectPort();                                                                                            //отключение порта


private slots:
      void handleError(QSerialPort::SerialPortError error); // в случае ошибки

};

#endif // PORT_H
