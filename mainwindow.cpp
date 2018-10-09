//------------------------------------ТПО_МОДУЛЯ УМ-600--------------------------------------------
//+++++++++++++++++++++++++++++++++Главный метод программы+++++++++++++++++++++++++++++++++++++++++
//+++ Описывает действия предпринимаемые в главном окне - нажатие кнопок, отправку сооюбщений,  +++
//+++ вывод информации, индикация. Здесь осуществляется формирование команд на передачу и       +++
//+++ прием сообщений с контроллера.                                                            +++
//++++      /**/ - обозначаются строчки кода, где присутсвтуют двухзначыне номера кнопок        +++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//----------------------------------------------------------------------------------------Светлов Я.
//----------------------------------------------------------------------svetloff.yaroslav@gmail.com


#include "mainwindow.h"
#include "ui_mainwindow.h"

                                        // РАБОТАЕТ - НЕ ТРОГАЙ!
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_settings("NII SHTIL","TEST-SOFT UM-600")
{

    ui->setupUi(this);                                                                  // загрузка интерфейса

    this->setFixedSize(755,842);                                                         // установка размера окна

     ConnectWithPort = new comdial(this);                                               // объявления диалогового окна подключения к Com-порту
     waitTime        = new QTimer(this);                                                // Объявления таймера ожидания ответа модуля
     colorTime       = new QTimer(this);                                                // Объявления таймера ответа возвращения к первоначальному цвету кнопок ( для режима работа 2)


     trayIcon = new QSystemTrayIcon(this);                                              // инициализируем иконку трея
     trayIcon->setIcon( QIcon(":/Images/icon.ico"));                                    // устанавливаем иконку
     trayIcon->setToolTip("УМ-600" "\n"
                          "Тестовое программное обеспечение");                          // всплывающаяся подсказка

     QMenu   *menu = new QMenu(this);                                                   // создаем контекстное меню из 2-х пунктов
     QAction *viewWindow = new QAction(trUtf8("Развернуть окно"),this);                 // в контекстном меню будут две кнопки  - развернуть окно
     QAction *quitAction = new QAction(trUtf8("Выход"),this);                           //  и выход


//___________________________________________________COM-ПОРТ_____________________________________________________________________________________________
    QThread *thread_New = new QThread;                                              // создание потока
    PortNNew = new port();                                                          // Инициализация потока
    PortNNew->moveToThread(thread_New);                                              // помещаем в поток
    PortNNew->thisPort.moveToThread(thread_New);                                     // помешаем файл порта  в поток
    connect(thread_New, SIGNAL(started()), PortNNew, SLOT(process_Port()));          // Переназначения метода run`
    connect(PortNNew, SIGNAL(finished_Port()), thread_New, SLOT(quit()));            // Переназначение метода выход
    connect(thread_New, SIGNAL(finished()), PortNNew, SLOT(deleteLater()));          // удаление порта вместе с потоком
    thread_New->start();                                                             // начало потока
//=========================================================================================================================================================


//__________________________________________________Коннекты____________________________________________________________________________________________________________________
    connect (viewWindow,        SIGNAL(triggered()),this,                                          SLOT(show()));                                                                                                   // разварачивает приложение из трея
    connect (quitAction,        SIGNAL(triggered()),this,                                          SLOT(close()));                                                                                                 // завершает работу работу программы
    connect (PortNNew,          SIGNAL(sendDataMain(QByteArray)),this,                             SLOT(Send_DataSlot(QByteArray)));                                                                 // Передача пришедших на порт файлов на обработку
    connect (ConnectWithPort,   SIGNAL(savesettings(QString,int,int,int,int)),PortNNew,    SLOT(Write_Settings_Port(QString,int,int,int,int)));                        //при нажатии "Настройки com-порта"  - выборе настроек  и нажатии кнопки - "Ок" - происходит передача выбранной информации - имя порта, скорости передачи, количества -стоповых бит и другое
    connect (this,              SIGNAL(savesettingsPrevious(QString,int,int,int,int)),PortNNew,    SLOT(Write_Settings_Port(QString,int,int,int,int)));
    connect (this,              SIGNAL(dis()),PortNNew,                                            SLOT(DisconnectPort()));                                                                                               // отключение порта
    connect (PortNNew,          SIGNAL(infotomain(QString,QString,QString,int,QString)),this,  SLOT(ShowComPortState(QString,QString,QString,int,QString)));        // вывод в главном окне информации о подключении
    connect (PortNNew,          SIGNAL(infotostatus(QString)),this,                                SLOT(showStat(QString)));                                                                              // Вывод информации в статус-бар
    connect (PortNNew,          SIGNAL(connecttomenu(bool)),this,                                  SLOT(AbleFromComPort(bool)));                                                                            // заведует включением/отключением кнопок при включении/ отключении порта
    connect (this,              SIGNAL(WriteDatatoPort(QByteArray)),PortNNew,                      SLOT(WriteToPort(QByteArray)));                                                                  // запись данных в порт
    connect (this,              SIGNAL(WriteDatatoPort(QByteArray)),this,                          SLOT(putDataInConsole(QByteArray)));                                                                 // вывод информации о ОТПРАВЛЕННОЙ команде в консоль                                                                                            // таймер ожидание ответа
    connect (colorTime,         SIGNAL(timeout()),this,                                            SLOT(changeColorDef()));                                                                                          // время индикации кнопки - 3 секунды заканчивается - цвет первоначальный возвращай
    connect (waitTime,          SIGNAL(timeout()),this,                                            SLOT(devicekeepSilence()));                                                                                        // время ожидания ответа модуля
    connect (trayIcon,          SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,         SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));                        // подключаем сигнал нажатия на иконку к обработчику данного нажатия
//================================================================================================================================================================================

     menu->addAction(viewWindow);                                               // в контектсное меню устанавливаются действия
     menu->addAction(quitAction);


     trayIcon->setContextMenu(menu);                                            // Устанавливаем контектсное меню на иконку
     trayIcon->show();                                                          // показываем иконку приложения в трее
     readSettings();

     colSEND = 0;                        // переменные int для подсчета количества ОТПРАВЛЕННЫХ сообщений
     colARR  = 0;                        // переменные int для подсчета количества ПРИНЯТЫХ сообщений


}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Удаление окна при закрытии~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MainWindow::~MainWindow()
{

    delete ui;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Момент закрытия программы~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::closeEvent(QCloseEvent * event)
{
   if(this->isVisible()&&ui->action->isChecked())
   {
       event->ignore();
       this->hide();

        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);

        trayIcon->showMessage("ТПО УМ-600",
                                     trUtf8("Программа работает в  фоновом режиме Для того чтобы, "
                                            "развернуть окно программмы, нажмите на иконку"),
                                     icon,
                                     1000);
   }
   else
   {
    writeSettings();            //сохраняются все настройки
   }
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~В случае нажатия на иконку трея~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
        case QSystemTrayIcon::Trigger:                  // если вы нажали на иконку трея
            {
                if(ui->action->isChecked())             // если выбрано в меню - Сворачивать программу в трей
                {
                    if(!this->isVisible())              // если окно не показано
                    {
                        this->show();                  // показать

                    }
                    else
                    {
                       this->hide();                    // иначе - спрячься
                    };
               }
            }
                break;
    default:
        break;
    }
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Сохранение настроек~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::writeSettings()
{
    m_nCounter++;                                                                               // увеличивается количество число обозначующее количество запусков программы                                                                                                // возможно эта штука и  вовсе излишняя, но я ее оставлю - т.к. все работает и хорошо
    if(m_nCounter>1)                                                                            // если запусков программы больше 1 сохраняй
    {
        m_settings.beginGroup("/Settings");                                                     // начало группы настроек
        m_settings.setValue("/counter", m_nCounter);                                            // сохраняет число запусков программы
        m_settings.setValue("/filePath",ui->lineEdit_fileName->text());                         // сохраняй путь до файла
        m_settings.setValue("/PortName",ui->label_COM->text());                                 // сохраняй имя порта
        m_settings.setValue("/BaudRate",ui->lineEdit_rate->text());                             // сохраняй скорость
        m_settings.setValue("/DataBits",ui->lineEdit_dataBits->text());                         // сохраняй длину слова
        m_settings.setValue("/Parity",  ParINT);                             // сохраняй паритетность
        m_settings.setValue("/StopBits",ui->lineEdit_stopbits->text());                         // сохраняй стоповые биты
        m_settings.endGroup();
                                                                     // группа настроек заканчивается
    }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Чтение настроек~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void  MainWindow::readSettings()
{
    m_settings.beginGroup("/Settings");                                                 // начало группы настройки
    m_nCounter = m_settings.value("/counter", 1).toInt();                               // сохраняется число запрусков программы

  if(m_nCounter>1)                                                                      // если их количество больше одного - в принципе это можно убрать, но не буду - оно все работает, а никто не заказывал тут оптимизированную программу.
  {                                                                                     // нет, совесть не мучает
    NameofFile = m_settings.value("/filePath").toString();                              // все что сохранено в "/filePath" - присваивается публичной строковой переменной
    QString nameCom = m_settings.value("/PortName").toString();                         // читается имя порта
    nameCom.replace(" ","");                                                            // удаляются пробелы
   if(nameCom !="COM")                                                                  // если предыдущие сессии работы не закончались отключением от порта
   {
    savesettingsPrevious(nameCom,m_settings.value("/BaudRate").toInt(),                // сигнал, который идет в port и переносит все настройки для подключения
                         m_settings.value("/DataBits").toInt(),
                         m_settings.value("/Parity").toInt(),
                         m_settings.value("/StopBits").toInt());
   }

    if(!NameofFile.isEmpty())                                                           // Если имя пути файла не пусто
    {
         on_action_OpenTXT_triggered();                                                 // вызывай открытие настроек, где этот файл считается
    }
  }
    m_settings.endGroup();                                                               //  закрывается группа настроек
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Если модуль молчит~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::devicekeepSilence()
{   showStat("Нет ответа от модуля!");                                                                                   //вывод информации в консоль
    waitTime->stop();                                                                 //остановка таймера
    pressButtonBlockOthers(true);                                                      //передача булевой переменной - отключающей все другие кнопки, пока прибор не ответит

 if(verA==2)
   {
       AnswerArray.clear();
       AnswerVerTWO();
   }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Возвращение кнопок к первоначальному  цвету  - если режим их работы 2 и они были активированы~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::changeColorDef()
{

    //__________________________________________________________групп-бокс FIELD 1_________________________________________________________
/**/        if(ui->pushButton_11->styleSheet() == "background-color:red;"||ui->pushButton_11->styleSheet() == "background-color:#5cff00;"){
/**/          ui->pushButton_11->setStyleSheet("default");
             }
/**/        if(ui->pushButton_12->styleSheet() == "background-color:red;"||ui->pushButton_12->styleSheet() == "background-color:#5cff00;"){
/**/           ui->pushButton_12->setStyleSheet("default");
             }
/**/         if(ui->pushButton_13->styleSheet() == "background-color:red;"||ui->pushButton_13->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_13->setStyleSheet("default");
             }
/**/         if(ui->pushButton_14->styleSheet() == "background-color:red;"||ui->pushButton_14->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_14->setStyleSheet("default");
             }
/**/        if(ui->pushButton_15->styleSheet() == "background-color:red;"||ui->pushButton_15->styleSheet() == "background-color:#5cff00;"){
/**/           ui->pushButton_15->setStyleSheet("default");
             }
/**/        if(ui->pushButton_16->styleSheet() == "background-color:red;"||ui->pushButton_16->styleSheet() == "background-color:#5cff00;"){
/**/             ui->pushButton_16->setStyleSheet("default");
             }


    //__________________________________________________________групп-бокс FIELD 2________________________________________________

/**/     if(ui->pushButton_21->styleSheet() == "background-color:red;"||ui->pushButton_21->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_21->setStyleSheet("default");
           }
/**/     if(ui->pushButton_22->styleSheet() == "background-color:red;"||ui->pushButton_22->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_22->setStyleSheet("default");
           }
/**/     if(ui->pushButton_23->styleSheet() == "background-color:red;"||ui->pushButton_23->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_23->setStyleSheet("default");
           }
/**/    if(ui->pushButton_24->styleSheet() == "background-color:red;"||ui->pushButton_24->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_24->setStyleSheet("default");
           }
/**/     if(ui->pushButton_25->styleSheet() == "background-color:red;"||ui->pushButton_25->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_35->setStyleSheet("default");
           }
/**/     if(ui->pushButton_26->styleSheet() == "background-color:red;"||ui->pushButton_26->styleSheet() == "background-color:#5cff00;"){
/**/            ui->pushButton_26->setStyleSheet("default");
           }


    //__________________________________________________________груп-бокс FIELD 3____________________________________________________

/**/     if(ui->pushButton_31->styleSheet() == "background-color:red;"||ui->pushButton_31->styleSheet() == "background-color:#5cff00;"){
/**/        ui->pushButton_31->setStyleSheet("default");
          }
/**/     if(ui->pushButton_32->styleSheet() == "background-color:red;"||ui->pushButton_32->styleSheet() == "background-color:#5cff00;"){
/**/        ui->pushButton_32->setStyleSheet("default");
          }
/**/      if(ui->pushButton_33->styleSheet() == "background-color:red;"||ui->pushButton_33->styleSheet() == "background-color:#5cff00;"){
/**/         ui->pushButton_33->setStyleSheet("default");
          }
/**/      if(ui->pushButton_34->styleSheet() == "background-color:red;"||ui->pushButton_34->styleSheet() == "background-color:#5cff00;"){
/**/        ui->pushButton_34->setStyleSheet("default");
           }
/**/      if(ui->pushButton_35->styleSheet() == "background-color:red;"||ui->pushButton_35->styleSheet() == "background-color:#5cff00;"){
/**/         ui->pushButton_35->setStyleSheet("default");
           }
/**/     if(ui->pushButton_36->styleSheet() == "background-color:red;"||ui->pushButton_36->styleSheet() == "background-color:#5cff00;"){
/**/         ui->pushButton_36->setStyleSheet("default");
           }

        colorTime->stop();                                       // остановка таймера возвращения цвета
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Обработка ответа на нажатие кнопки, работющей в режиме 3~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::AnswerVerTHREE()
{

    // тут много лишнего накручено, но все работает

   QByteArray ANSV3 = AnswerArray.mid(startVByte,endVByte-startVByte+1);                       // из ответа вырезается та чсть, которая задана в txt переменными StartView  и EndView (байты начала  и конца отображения)

   int sizeMes = ANSV3.size();


   qDebug()<<"Полученное сообщение"<<ANSV3.toHex()<<"\n Размером:"<<sizeMes;
   if(sizeMes>4)                                                                           // если колество отображаемых байт больше 4
   {
       QMessageBox checkBoxMessage;                                                             // выводи сообщение
       checkBoxMessage.setWindowTitle("Ошибка режима 3");
       checkBoxMessage.setText("Неверно заданы <b>StartView</b> и <b>EndView</b> \n Проверьте файл настройки");
       checkBoxMessage.setFixedHeight(sizeHint().height());
       checkBoxMessage.exec();
   }
   else
   {
        QString message;
                                                                    // объявляется переменная для вывода массива
      if(byteOrd == 1)                                            // MOTOROLA -  от старшего младшему                                                                                // Если MOTOROLA
        {

            switch(sizeMes)
            {

            case 1:  {
                        quint8  numBer= qFromBigEndian<quint8>((const uchar*)ANSV3.constData());
                        message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                        ui->plainTextEdit_message->insertPlainText(message+"\n");
                       };
                    break;

            case 2:   {
                         quint16  numBer= qFromBigEndian<quint16>((const uchar*)ANSV3.constData());
                         message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                         ui->plainTextEdit_message->insertPlainText(message+"\n");
                       };
                     break;
             case 3: {
                       quint8 fd = 0x00;
                       ANSV3.append(fd);
                       quint32  numBer= qFromBigEndian<quint32>((const uchar*)ANSV3.constData());
                       message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                       ui->plainTextEdit_message->insertPlainText(message+"\n");
                      };
                     break;
            case 4: {

                      quint32  numBer= qFromBigEndian<quint32>((const uchar*)ANSV3.constData());
                      message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                      ui->plainTextEdit_message->insertPlainText(message+"\n");
                     };
               break;

            }

        }
        else if(byteOrd ==2)                                               // INTEL - от младшего старшему                                                                           // Если INTEL
        {
            switch(sizeMes){

            case 1:  {
                        quint8  numBer= qFromLittleEndian<quint8>((const uchar*)ANSV3.constData());
                        message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                        ui->plainTextEdit_message->insertPlainText(message+"\n");

                        };
                    break;

            case 2:   {
                         quint16  numBer= qFromLittleEndian<quint16>((const uchar*)ANSV3.constData());
                         message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                         ui->plainTextEdit_message->insertPlainText(message+"\n");

                        };
                     break;
             case 3: {
                          quint8 fd = 0x00;
                          ANSV3.prepend(fd);

                          quint32  numBer= qFromLittleEndian<quint32>((const uchar*)ANSV3.constData());
                          message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                          ui->plainTextEdit_message->insertPlainText(message+"\n");
                      };
                break;
            case 4: {

                         quint32  numBer= qFromLittleEndian<quint32>((const uchar*)ANSV3.constData());
                         message = QString::number(numBer,10);                                                       // что получилось перегоняется в строковую переменную
                         ui->plainTextEdit_message->insertPlainText(message+"\n");
                     };
               break;

            }
        }

                                                 // вывод отображаемых битов  в СООБЩЕНИЕ
        ui->plainTextEdit_message->moveCursor(QTextCursor::End,QTextCursor::KeepAnchor);                    // Курсор после вывод в конец


       // message.clear();                                                                                    // очищается значение строковой переменной, где отображаются байты сообщения
        ANSV3.clear();                                                                                      // очищается байтовый массив - где были "вырезаны" отображаемые байты
        AnswerArray.clear();                                                                                // очищается байтовый массив, где содержится ответ
   }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Обработка ответа на нажатие кнопки, работющей в режиме 2~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::AnswerVerTWO()
{

      int szv=CheckWith.size();                                                     // подсчитывается размер массива байт ответа
    QByteArray checkANS = AnswerArray.mid(checkByte,szv);                                // вырезается то, с чем необходимо сравнивать

        if(checkANS!=CheckWith)                                                             // если то, что выделели из ответа не совпадает с тем, что было задано в настройках в переменной ByteComp
         {
           switch(numberButton)                                                         // в зависимости от того, какую кнопку нажал
           {

                //__________________________групп-бокс FIELD1_____________________
 /**/               case 11: {ui->pushButton_11->setStyleSheet("background-color:red;");    // она окрашивается в КРАСНЫЙ
                                                };break;
/**/                case 12: {ui->pushButton_12->setStyleSheet("background-color:red;");
                                                 };break;
/**/                case 13: {ui->pushButton_13->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 14: {ui->pushButton_14->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 15: {ui->pushButton_15->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 16: {ui->pushButton_16->setStyleSheet("background-color:red;");
                                                };break;

               //_____________________________групп-бокс FIELD2_____________________
/**/                case 21: {ui->pushButton_21->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 22: {ui->pushButton_22->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 23: {ui->pushButton_23->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 24: {ui->pushButton_24->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 25: {ui->pushButton_25->setStyleSheet("background-color:red;");
                                                 };break;
/**/                case 26: {ui->pushButton_26->setStyleSheet("background-color:red;");
                                                };break;
                 //_______________________________групп-бокс FIELD3__________________
/**/                case 31: {ui->pushButton_31->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 32: {ui->pushButton_32->setStyleSheet("background-color:red;");
                                                 };break;
/**/                case 33: {ui->pushButton_33->setStyleSheet("background-color:red;");
                                                  };break;
/**/                case 34: {ui->pushButton_34->setStyleSheet("background-color:red;");
                                                };break;
/**/                case 35: {ui->pushButton_35->setStyleSheet("background-color:red;");
                                                 };break;
/**/                case 36: {ui->pushButton_36->setStyleSheet("background-color:red;");
                                                 };break;
            }
    }
     else if(checkANS==CheckWith)                                               // если все пучком совпадает
    {

        switch(numberButton)                                                    // в зависимости от того, какую кнопку нажад
        {
                 //_____________________________групп-бокс FIELD1_____________________

/**/             case 11: {ui->pushButton_11->setStyleSheet("background-color:#5cff00;");                  // она окрашивается в ЗЕЛЕНЫЙ
                                        };break;
/**/             case 12: {ui->pushButton_12->setStyleSheet("background-color:#5cff00;");
                                         };break;
/**/             case 13: {ui->pushButton_13->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/             case 14: {ui->pushButton_14->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/             case 15: {ui->pushButton_15->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/             case 16: {ui->pushButton_16->setStyleSheet("background-color:#5cff00;");
                                        };break;

                 //_____________________________групп-бокс FIELD2_____________________
/**/                case 21: {ui->pushButton_21->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 22: {ui->pushButton_22->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 23: {ui->pushButton_23->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 24: {ui->pushButton_24->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 25: {ui->pushButton_25->setStyleSheet("background-color:#5cff00;");
                                         };break;
/**/                case 26: {ui->pushButton_26->setStyleSheet("background-color:#5cff00;");
                                        };break;
                //______________________________групп-бокс FIELD3_____________________
/**/                case 31: {ui->pushButton_31->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 32: {ui->pushButton_32->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 33: {ui->pushButton_33->setStyleSheet("background-color:#5cff00;");
                                          };break;
/**/                case 34: {ui->pushButton_34->setStyleSheet("background-color:#5cff00;");
                                        };break;
/**/                case 35: {ui->pushButton_35->setStyleSheet("background-color:#5cff00;");
                                         };break;
/**/                case 36: {ui->pushButton_36->setStyleSheet("background-color:#5cff00;");
                                         };break;

        }

         }

        AnswerArray.clear();
        CheckWith.clear();// очищается байтовый массив, содержащий ответ
        colorTime->start(3000);                                                                 // запускается таймер  - спустя 3 секунды кнопки примут тот же цвет

}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Прием ответа от модуля из файла port~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Send_DataSlot(QByteArray answer)
{
    //AnswerArray.clear();                                        // публичный QByteArray - очищается

    qDebug()<<answer.toHex();
    AnswerArray.append(answer);                                 // и в него записывается ответ
                                                                // останавливатся таймер ожиадния
     waitTime->stop();
     pressButtonBlockOthers(true);

    QString s= answer.toHex();                                  // ответ преобразуется в строчную переменную
    for(int i = 0;i<s.size();i++)
    {
        if(i%3==0)                                              // после между байтами расставляются пробелы
        {
            s.insert(i," ");
        }
    }
     colARR++;                                                  // итерация переменной подсчета принятых сообщений
     if( colARR<100)                                            // если меньше 100
     {
         if( colARR<10)                                         // если меньше 10
         {
         ui->widget_arrive->insertPlainText(QString::number(colARR,10)+"    ");                //выводи номер принятой посылки и  ставь самый большой пробел
         }
         else
         {
            ui->widget_arrive->insertPlainText(QString::number(colARR,10)+"   ");             // иначе  - пробел поменьше
         }
     }
     else                                                                                       // если больше 100
     {
         ui->widget_arrive->insertPlainText(QString::number(colARR,10)+"  ");                  // совсем маленький
     }

     ui->widget_arrive->putDataArrive(s.toUpper());                                             // выводи - то ,что пришло на порт


        if(verA==2)                                             // в зависимости от того, кнопку с каким режимом работы вы нажали
        {

                 AnswerVerTWO();                                // вызывается та
        }
        else if(verA==3)
        {
                 AnswerVerTHREE();                              // или иная функция
        }
        s.clear();                                                                                 // очищай строку

}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Делает все кнопки в трех групп боксах ДОСТУПНЫМИ или НЕДОСТУПНЫМИ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::pressButtonBlockOthers(bool press)
{
    ui->groupBox_field1->setEnabled(press);                                     // кнопки Групп бокс 1
    ui->groupBox_field2->setEnabled(press);                                     // кнопки групп бокс 2
    ui->groupBox_field3->setEnabled(press);                                     // кнопки групп бокс 3
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Вывод информации о отправленной команде~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::putDataInConsole(QByteArray data)
{
    QString s= data.toHex();                                                                    // перевод байтового массива в строку
    for(int i = 0;i<s.size();i++)
    {
        if(i%3==0)
        {
            s.insert(i," ");                                                                    // расставим пробелы
        }
    }
    colSEND++;                                                                                  // увеличим количество отправленных сообщений

    if(colSEND<100)                                                                             // - та же тема с пробелами в консоле, что и в функции выше
    {
        if(colSEND<10)
        {
            ui->widget_send->insertPlainText(QString::number(colSEND,10)+"    ");
        }
        else
        {
           ui->widget_send->insertPlainText(QString::number(colSEND,10)+"   ");
        }
    }
    else
    {
        ui->widget_send->insertPlainText(QString::number(colSEND,10)+"  ");
    }
    ui->widget_send->putDataSend(s.toUpper());                                                  // вывод информаци в строку
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Блокировка кнопок при отключении порта~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::AbleFromComPort(bool port)
{
    ui->action_ConnectComPort->setDisabled(port);                                       // в меню - сом-порт  - подключение становится недоступной
    ui->action_DisconnectComPort->setEnabled(port);                                     // отключение становится неодоступным
    ui->action_OpenTXT->setEnabled(port);                                               // открытие txt файла настроек


    ui->groupBox_field1->setEnabled(port);
    ui->groupBox_field2->setEnabled(port);
    ui->groupBox_field3->setEnabled(port);


    VI.arrEnd.clear();                                                      // структура режима работы 1 очищается
    VI.arrStart.clear();
    VI.endcolor = 0;
    VI.color = 0;

    VII.arrCheck.clear();                                                   // структура режима работы 2 очищается
    VII.arrSend.clear();
    VII.check_byte = 0;

    VIII.arrSend.clear();                                                   //  структура режима работы 3 очищается
    VIII.byte_EndView = 0;
    VIII.byte_StartView = 0;

}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Вывод информации о подключенном порте~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::ShowComPortState(QString name,QString rate,QString dataBitsString,int parity,QString stopbit)
{
     name.insert(3," ");                        // пробел после COM 15
     ui->label_COM->setText(name);              // вставка имени com-порта
     ui->lineEdit_rate->setText(rate);           // скорости
     ui->lineEdit_dataBits->setText(dataBitsString);    // длина слова
    ParINT =  parity;


        switch (ParINT)
        {
        case 0 : {
                    ui->lineEdit_Parity->setText(QStringLiteral("None"));
                  } break;

        case 2: {
                   ui->lineEdit_Parity->setText(QStringLiteral("Even"));
                 } break;
        case 3: {
                   ui->lineEdit_Parity->setText(QStringLiteral("Odd"));
                 } break;

        case 4: {
                   ui->lineEdit_Parity->setText(QStringLiteral("Space"));
                 } break;
        case 5: {
                   ui->lineEdit_Parity->setText(QStringLiteral("Mark"));
                 } break;
          case 13:
                 {
                   ui->lineEdit_Parity->clear();
                 } break;
        default:
                 break;

        }

                // паритета
     ui->lineEdit_stopbits->setText(stopbit);           // стоповых бит


}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Вывод информации в консоль на 5 сек~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::showStat(QString dataTostatus)
{
    ui->statusBar->showMessage(dataTostatus,5000);
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~В меню - Com-порт  - подключение~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_action_ConnectComPort_triggered()
{
    showStat("Подключение к COM-порту");                                // вывод информации в статус
    ConnectWithPort->show();                                            // диалоговое окно становится видимым
    ConnectWithPort->activateWindow();                                  // с него нельзя переключится на главное окно
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~В меню - Com-порт  - отключение~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_action_DisconnectComPort_triggered()
{
    showStat("Отключение COM-порта");
    emit dis();                                 //сигнал, соединенные со слотом в port.cpp


}
//-----------------------------------------------------------------Информация об ошибке при открытии настроечного файла--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void MainWindow::ErrorMes(int num)
{
    QMessageBox checkBoxMessage;                                                             // выводи сообщение
    checkBoxMessage.setWindowTitle("Ошибка файла настройки");

    switch (num)
    {
    case 0:  checkBoxMessage.setText("Неверно задан адресс");                                                                                   break;
    case 1:  checkBoxMessage.setText("Неверно задано название групп бокса 1 - проверьте файл-настройки");                                       break;
    case 2:  checkBoxMessage.setText("Неверно задано название групп бокса 2 - проверьте файл-настройки");                                       break;
    case 3:  checkBoxMessage.setText("Неверно задано название групп бокса 3 - проверьте файл-настройки");                                       break;
    case 4:  checkBoxMessage.setText("Ошибка в задании кнопки - проверьте файл-настройки");                                                     break;
    case 5:  checkBoxMessage.setText("Нет такого номера кнопки- проверьте файл-настройки");                                                     break;
    case 6:  checkBoxMessage.setText("Проверьте наименование кнопок - не хватает точки запятой ");                                              break;
    case 7:  checkBoxMessage.setText("Проверьте синтаксис настроечного файла  - ошибка в написании переменной ButName=  ");                     break;
    case 8:  checkBoxMessage.setText("Проверьте синтаксис настроечного файла  - ошибка в написании переменной ButVer=  ");                      break;
    case 9 : checkBoxMessage.setText("Проверьте синтаксис настроечного файла  - не хватает точки запятой после ButVer=  ");                     break;
    case 10: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  - ошибка в написании переменной {SetColor= режим работы 1");      break;
    case 11: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  - пропущенна точка с запятой после {SetColor= режим работы 1");   break;
    case 12: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  - пропущенна точка с запятой после StartArr= режим работы 1");    break;
    case 13: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной StartArr= режим работы 1");      break;
    case 14: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущенна точка с запятой после EndArr= режим работы 1");     break;
    case 15: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной EndArr режим работы 1");         break;
    case 16: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущенна точка с запятой после {SendArr= режим работы 2");   break;
    case 17: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной {SendArr= режим работы 2");      break;
    case 18: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущена точка с запятой после CompareArr= режим работы 2");  break;
    case 19: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной CompareArr= режим работы 2");    break;
    case 20: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущена точка с запятой после ByteComp= режим работы 2");    break;
    case 21: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной ByteComp= режим работы 2 ");     break;
    case 22: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущена точка с запятой после {SendArr= режима работы 3");   break;
    case 23: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной {SendArr= режима работы 3");     break;
    case 24: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущенна точка с запятой после StartView= режима работы 3"); break;
    case 25: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной StartView= режима работы 3");    break;
    case 26: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущенна точка с запятой после EndView= режима работы 3");   break;
    case 27: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной EndView= режима работы 3");      break;
    case 28: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущена точка с запятой после EndColor= режима работы 1");   break;
    case 29: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  ошибка в написании переменной EndColor= режима работы 1");     break;
    case 30: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущена точка с запятой после ByteOrder= режим работы 3");   break;
    case 31: checkBoxMessage.setText("Проверьте синтаксис настроечного файла  -  пропущена точка с запятой после CS= ");                        break;
    case 32: checkBoxMessage.setText("ПРоверьте синтаксис настроечного файла  -  неверное значение подсчета контрольной суммы");                break;
    default: checkBoxMessage.setText("Неизвестная ошибка");                                                                                     break;
    }


    checkBoxMessage.setFixedHeight(sizeHint().height());
    checkBoxMessage.exec();
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~В меню -> Настройка -> Открыть файл настройки~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_action_OpenTXT_triggered()
{
   showStat("Открытия файла с настройками");                             // в статусе показывается информация

   arraysSend.clear();                                                  //очищаются контейнеры, содержащие ВСЮ информацию о ВСЕХ  кнопках - ИХ РЕЖИМ РАБОТЫ, ВСЯКИЕ БАЙТЫ,ЦВЕТА
   mode.clear();
   bytes.clear();
   buttonNames.clear();
   enableBut.clear();
   color.clear();


   ui->lineEdit_adressofModul->clear();

   ui->groupBox_field1->setEnabled(false);                               // групп боксы становятся доступными
   ui->groupBox_field2->setEnabled(false);
   ui->groupBox_field3->setEnabled(false);

   ui->groupBox_field1->setTitle("   ");                                // очищаются имя групп-боксах
   ui->groupBox_field2->setTitle("   ");
   ui->groupBox_field3->setTitle("   ");
//_____________________________________________________________________________________
   //______________FIELD1____________________
   ui->pushButton_11->setStyleSheet("default");                         // устанавливаются стандартный дизайн
   ui->pushButton_12->setStyleSheet("default");
   ui->pushButton_13->setStyleSheet("default");
   ui->pushButton_14->setStyleSheet("default");
   ui->pushButton_15->setStyleSheet("default");
   ui->pushButton_16->setStyleSheet("default");

 //______________FIELD2____________________
   ui->pushButton_21->setStyleSheet("default");
   ui->pushButton_22->setStyleSheet("default");
   ui->pushButton_23->setStyleSheet("default");
   ui->pushButton_24->setStyleSheet("default");
   ui->pushButton_25->setStyleSheet("default");
   ui->pushButton_26->setStyleSheet("default");

//______________FIELD3____________________
   ui->pushButton_31->setStyleSheet("default");
   ui->pushButton_32->setStyleSheet("default");
   ui->pushButton_33->setStyleSheet("default");
   ui->pushButton_34->setStyleSheet("default");
   ui->pushButton_35->setStyleSheet("default");
   ui->pushButton_36->setStyleSheet("default");

//_____________________________________________________________________________________

//_____________________________________________________________________________________
        //____________FIELD1__________
        ui->pushButton_11->setText("11");                                                 // очищаются названия кнопок
        ui->pushButton_12->setText("12");
        ui->pushButton_13->setText("13");
        ui->pushButton_14->setText("14");
        ui->pushButton_15->setText("15");
        ui->pushButton_16->setText("16");

        //____________FIELD2__________
        ui->pushButton_21->setText("21");
        ui->pushButton_22->setText("22");
        ui->pushButton_23->setText("23");
        ui->pushButton_24->setText("24");
        ui->pushButton_25->setText("25");
        ui->pushButton_26->setText("26");

        //____________FIELD3__________
        ui->pushButton_31->setText("31");
        ui->pushButton_32->setText("32");
        ui->pushButton_33->setText("33");
        ui->pushButton_34->setText("34");
        ui->pushButton_35->setText("35");
        ui->pushButton_36->setText("36");
//_____________________________________________________________________________________

//_____________________________________________________________________________________
        //____________FIELD1__________
        ui->pushButton_11->setEnabled(false);                                                 // очищаются названия кнопок
        ui->pushButton_12->setEnabled(false);
        ui->pushButton_13->setEnabled(false);
        ui->pushButton_14->setEnabled(false);
        ui->pushButton_15->setEnabled(false);
        ui->pushButton_16->setEnabled(false);

        //____________FIELD2__________
        ui->pushButton_21->setEnabled(false);
        ui->pushButton_22->setEnabled(false);
        ui->pushButton_23->setEnabled(false);
        ui->pushButton_24->setEnabled(false);
        ui->pushButton_25->setEnabled(false);
        ui->pushButton_26->setEnabled(false);

        //____________FIELD3__________
        ui->pushButton_31->setEnabled(false);
        ui->pushButton_32->setEnabled(false);
        ui->pushButton_33->setEnabled(false);
        ui->pushButton_34->setEnabled(false);
        ui->pushButton_35->setEnabled(false);
        ui->pushButton_36->setEnabled(false);
//_____________________________________________________________________________________

        ui->lineEdit_fileName->clear();                                     // очищается строка - содержащая путь к файлу настройки

 if(NameofFile.isEmpty())
 {

  NameofFile = QFileDialog::getOpenFileName(this, tr("Открыть файл с настройками"), "/home/",tr("Текстовый документ (*.txt)"));          // открытие диалогового окна - УКАЗАТЬ ПУТЬ К НАСТРОЕЧНОМУ ФАЙЛУ
}
   ui->lineEdit_fileName->setText(NameofFile);                                                                                                    // если файл не записывается выводи ошибку
   qDebug()<<NameofFile;
   QFile in(NameofFile);                                                 // открытие файла

   NameofFile.clear();
   if(!in.open(QIODevice::ReadOnly))                                    // если файл не удается открыть
   {
       QMessageBox::information(0,"Ошибка",in.errorString());           // выводи информацию - сообщение
       ui->groupBox_field1->setEnabled(false);                          // групп боксы становятся не доступными
       ui->groupBox_field2->setEnabled(false);
       ui->groupBox_field3->setEnabled(false);
   }
   else                                                                 // иначе
   {
       QTextStream textstr(&in);                                        // созадатся текстовый поток и в него помещается открывающийся файл
       QString comments = ";";                                          // строка, в которой содержится - значение того(;) после чего будут удалятся все дальнейшие символы - короче, это для удаления комментов
       QString variable;                                                // еще какая-то переменная - я ее везде юзаю - там смотри ниже

       ui->groupBox_field1->setEnabled(true);                          // групп боксы становятся доступными
       ui->groupBox_field2->setEnabled(true);
       ui->groupBox_field3->setEnabled(true);

  while(!textstr.atEnd())                                               // И пока не иссякнет текстовый поток
  {
      textstr.skipWhiteSpace();                                         // удаляй все пробелы из потока - (нихрена не работает , если хочешь внутри аргумента фцнкции startWith(балабала=) вставить пробел)
      QString line =  textstr.readLine();

        if(line.startsWith("adress="))                                  // Если прочитанная строка начинается с этих слов
        {
            bool ok;
            variable="adress=";                                         // переменная varible присваивается одноименное значение
            line.remove(line.indexOf(comments),1200);                   // все, что после ";" на 1200 символов - удаляется к чертям
            line.remove(line.indexOf(variable),7);                      // ВОТ ТУТ КОСЯК - ЕСЛИ ХОЧЕШЬ ЧТОБЫ БЕЗ ПРОБЕЛОВ -  здесь удаляется то, что variable
            Adr = line.trimmed().toInt(&ok,16);                              // остается адресс - он и записывается в переменную Adr
            if(Adr==0)
            {
                ErrorMes(0);
                return;                                                 // ОШИБКА если адресс модуля 0
            }
            ui->lineEdit_adressofModul->setText(line.trimmed());        // В интерфейсе  - в строке "Адрес модуля" - устанавливается значение

        }
        else if(line.startsWith("field1="))                             // Если принятая строка начинается с field1=
        {
             variable="field1=";                                        // переменная varible присваивается одноименное значение
             line.remove(line.indexOf(comments),1200);                  // все, что после ";" на 1200 символов - удаляется к чертям
             line.remove(line.indexOf(variable),7);                      // здесь удаляется то, что variable
             if (line.indexOf("//")>0)
             {
                 ErrorMes(1);                                           // ОШИБКА если пропущена точка с запятой
                 return;
             }

            if(line.isEmpty())
            {
                ui->groupBox_field1->setTitle("         ");
            }
            else
            {
                 ui->groupBox_field1->setTitle(line.trimmed());          // устанавливается имя групп-бокса 1
            }




        }
        else if(line.startsWith("//"))                                  // ВСЕ КОММЕНТАРИИ НЕ ВОСПРИНИМАЮТСЯ ПРОГРАММОЙ
        {
             variable="//";
             line.remove(line.indexOf(variable),12000);

        }
        else if(line.startsWith("field2="))                            // Если принятая строка начинается с field2=
        {
            variable="field2=";                                        // переменная varible присваивается одноименное значение
            line.remove(line.indexOf(comments),1200);                  // все, что после ";" на 1200 символов - удаляется к чертям
            line.remove(line.indexOf(variable),7);                     // здесь удаляется то, что variable
            if (line.indexOf("//")>0){
                ErrorMes(2);                                           // ОШИБКА если пропущена точка с запятой
                return;
            }
            ui->groupBox_field2->setTitle(line.trimmed());             // устанавливается имя групп-бокса 2
        }

        else if(line.startsWith("field3="))                             // Если принятая строка начинается с field3=
        {
            variable="field3=";                                         // переменная varible присваивается одноименное значение
            line.remove(line.indexOf(comments),1200);                   // все, что после ";" на 1200 символов - удаляется к чертям
            line.remove(line.indexOf(variable),7);                      // здесь удаляется то, что variable
            if (line.indexOf("//")>0){
                ErrorMes(3);                                           // ОШИБКА если пропущена точка с запятой
                return;
            }
            ui->groupBox_field3->setTitle(line.trimmed());              // устанавливается имя групп-бокса 3
        }
        else if(line.startsWith("BUTTONN="))                            // Если принятая строка начинается с BUTTONN=
        {
            variable="BUTTONN=";                                        // переменная varible присваивается одноименное значение
            line.remove(line.indexOf(comments),1200);                   // все, что после ";" на 1200 символов - удаляется к чертям
            line.remove(line.indexOf(variable),8);                      // здесь удаляется то, что variable

            int butNum = line.trimmed().toInt();                        // переменной butNum присваивается то, что стоит после BUTTONN=

            QString readName;
            QString readVer;


            switch (butNum)                                                             //  в зависимости от переменной номера кнопки
            {

            //____________________________________________________________________________FIELD1_______________________________________________________________________________
             /**/       case 11:{                                                                    // кнопка 11

                            //ЧЕТУТТАКОЕ
                                    ui->pushButton_11->setEnabled(true);                            //Если номер кнопки совпадает с case - она становится активной
                                    readName=textstr.readLine();                                    // Считывается следующая строка
                                    if(readName.startsWith("ButName="))                             // Если эта строка начинается ButName=
                                   {
                                            variable = "ButName=";                                  // переменной присваивается значение этой строки
                                            readName.remove(readName.indexOf(comments),1200);       // 1200 символов после ; - удаляется
                                            readName.remove(readName.indexOf(variable),8);          // удаляется начало строки "ButName="
             /**/                           ui->pushButton_11->setText(readName.trimmed());         // то, что остается - является именем кнопки
                                            if (readName.indexOf("//")>0)
                                            {
                                                ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                return;
                                            }
                                            readVer=textstr.readLine();                             // читается следующая строка
                                                    if(readVer.startsWith("ButVer="))               // если строка начинается с "ButVer = "
                                                    {
                                                            variable = "ButVer=";                                  // переменной присваивается значение этой строки
                                                            readVer.remove(readVer.indexOf(comments),1200);         // 1200 символов после ; - удаляется
                                                            readVer.remove(readVer.indexOf(variable),7);            // удаляется начало строки "ButVer="

                                                            if (readVer.indexOf("//")>0)
                                                            {
                                                                     ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                    return;
                                                            }
                                                                                                                    // далее необходимо единовременно считать 3 строки
                                                                    QString oneLine;                                // строка 1
                                                                    QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                    QString twoLine;                                // строка 2
                                                                    QString threeLine;                              // строка 3
                                                                    QString fourLine;                               // строка 4


                                                                     switch(readVer.trimmed().toInt())              // в зависимости от того, что содержалось после ButVer
                                                                    {
                                                                            case 1: {                               // определяется режим работы кнопки
                                                                                            oneLine=textstr.readLine();     //считывает 4 строки
                                                                                            oneMoreLine=textstr.readLine();
                                                                                            twoLine = textstr.readLine();
                                                                                            threeLine = textstr.readLine();
                                                                                            fourLine = textstr.readLine();


                                                                                            ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);             // посылает на обработку в  соответствующую функцию

                                                                                    }; break;

                                                                            case 2:{                                        // определяется режим работы кнопки
                                                                                            oneLine = textstr.readLine();   //считывает 3 строки
                                                                                            twoLine = textstr.readLine();
                                                                                            threeLine =  textstr.readLine();
                                                                                            fourLine = textstr.readLine();
                                                                                           ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);           // посылает на обработку в  соответствующую функцию
                                                                                    };break;

                                                                            case 3: {
                                                                                            oneLine = textstr.readLine();   // определяется режим работы кнопки
                                                                                            oneMoreLine=textstr.readLine();
                                                                                            twoLine = textstr.readLine();
                                                                                            threeLine =  textstr.readLine();
                                                                                            fourLine = textstr.readLine();
                                                                                            ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);   // посылает на обработку в  соответствующую функцию
                                                                                     };break;

                                                                      }
                                                      }
                                                    else
                                                    {
                                                        ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                        return;
                                                    }

                                   }
                                    else
                                    {
                                         ErrorMes(7);
                                         return;        // ОШИБКА если неверно написана переменная ButName
                                    }
                              };break;
                        case 12:{
             /**/
                                            ui->pushButton_12->setEnabled(true);                    // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ                                readName=textstr.readLine();
                                            readName=textstr.readLine();
                                            if(readName.startsWith("ButName="))
                                                    {

                                                            variable = "ButName=";
                                                            readName.remove(readName.indexOf(comments),1200);
                                                            readName.remove(readName.indexOf(variable),8);
             /**/                                           ui->pushButton_12->setText(readName.trimmed());
                                                            if (readName.indexOf("//")>0)
                                                            {

                                                                ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                                return;
                                                            }

                                                            readVer=textstr.readLine();
                                                                    if(readVer.startsWith("ButVer="))
                                                                    {
                                                                             variable = "ButVer=";
                                                                             readVer.remove(readVer.indexOf(comments),1200);
                                                                             readVer.remove(readVer.indexOf(variable),7);
                                                                             if (readVer.indexOf("//")>0)
                                                                             {
                                                                                      ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                                     return;
                                                                             }

                                                                             QString oneLine;
                                                                             QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                             QString twoLine;
                                                                             QString threeLine;
                                                                             QString fourLine;                               // строка 4

                                                                        switch(readVer.trimmed().toInt())
                                                                        {
                                                                              case 1: {

                                                                                            oneLine=textstr.readLine();
                                                                                            oneMoreLine=textstr.readLine();
                                                                                            twoLine = textstr.readLine();
                                                                                            threeLine = textstr.readLine();
                                                                                            fourLine = textstr.readLine();
                                                                                             ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);

                                                                                     }; break;

                                                                             case 2:{
                                                                                            oneLine = textstr.readLine();
                                                                                            twoLine = textstr.readLine();
                                                                                            threeLine =  textstr.readLine();
                                                                                            fourLine = textstr.readLine();
                                                                                           ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                                                       };break;

                                                                             case 3: {

                                                                                            oneLine = textstr.readLine();
                                                                                            oneMoreLine=textstr.readLine();
                                                                                            twoLine = textstr.readLine();
                                                                                            threeLine =  textstr.readLine();
                                                                                            fourLine = textstr.readLine();
                                                                                            ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);

                                                                                         };break;
                                                                        }
                                                           }
                                                         else{
                                                                 ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                                 return;
                                                            }

                                            }

                                    };break;

                            case 13:{
               /**/                         ui->pushButton_13->setEnabled(true);                                            // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                            readName=textstr.readLine();
                                            if(readName.startsWith("ButName="))
                                                    {

                                                            variable = "ButName=";
                                                            readName.remove(readName.indexOf(comments),1200);
                                                            readName.remove(readName.indexOf(variable),8);
                                                            ui->pushButton_13->setText(readName.trimmed());
                                                            if (readName.indexOf("//")>0)
                                                            {

                                                                ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                                return;
                                                            }
              /**/
                                                            readVer=textstr.readLine();
                                                            if(readVer.startsWith("ButVer="))
                                                            {
                                                                     variable = "ButVer=";
                                                                     readVer.remove(readVer.indexOf(comments),1200);
                                                                     readVer.remove(readVer.indexOf(variable),7);

                                                                     if (readVer.indexOf("//")>0)
                                                                     {
                                                                              ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                             return;
                                                                     }

                                                                    QString oneLine;
                                                                    QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                    QString twoLine;
                                                                    QString threeLine;
                                                                    QString fourLine;                               // строка 4
                                                             switch(readVer.trimmed().toInt())
                                                          {
                                                               case 1: {

                                                                                    oneLine=textstr.readLine();
                                                                                    oneMoreLine=textstr.readLine();
                                                                                    twoLine = textstr.readLine();
                                                                                    threeLine = textstr.readLine();
                                                                                    fourLine = textstr.readLine();
                                                                                    ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                         }; break;

                                                               case 2:{

                                                                                    oneLine = textstr.readLine();
                                                                                    twoLine = textstr.readLine();
                                                                                    threeLine =  textstr.readLine();
                                                                                    fourLine = textstr.readLine();
                                                                                    ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                                         };break;

                                                               case 3: {
                                                                                    oneLine = textstr.readLine();
                                                                                    oneMoreLine=textstr.readLine();
                                                                                    twoLine = textstr.readLine();
                                                                                    threeLine =  textstr.readLine();
                                                                                    fourLine = textstr.readLine();
                                                                                    ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                         };break;
                                                              }
                                                     }
                                                     else{
                                                            ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                             return;
                                                         }
                                             }
                               };break;


                            case 14:{
              /**/                          ui->pushButton_14->setEnabled(true);                                    // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                            readName=textstr.readLine();
                                            if(readName.startsWith("ButName="))
                                              {
                                                            variable = "ButName=";
                                                            readName.remove(readName.indexOf(comments),1200);
                                                            readName.remove(readName.indexOf(variable),8);
            /**/                                            ui->pushButton_14->setText(readName.trimmed());
                                                            if (readName.indexOf("//")>0)
                                                            {

                                                                ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                                return;
                                                            }

                                                            readVer=textstr.readLine();
                                                            if(readVer.startsWith("ButVer="))
                                                             {
                                                                   variable = "ButVer=";
                                                                   readVer.remove(readVer.indexOf(comments),1200);
                                                                   readVer.remove(readVer.indexOf(variable),7);

                                                                   if (readVer.indexOf("//")>0)
                                                                   {
                                                                            ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                           return;
                                                                   }

                                                                  QString oneLine;
                                                                  QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                  QString twoLine;
                                                                  QString threeLine;
                                                                  QString fourLine;                               // строка 4
                                                             switch(readVer.trimmed().toInt())
                                                         {
                                                            case 1: {

                                                                                    oneLine=textstr.readLine();
                                                                                    oneMoreLine=textstr.readLine();
                                                                                    twoLine = textstr.readLine();
                                                                                    threeLine = textstr.readLine();
                                                                                    fourLine = textstr.readLine();
                                                                                     ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                             }; break;

                                                            case 2:{
                                                                                     oneLine = textstr.readLine();
                                                                                     twoLine = textstr.readLine();
                                                                                     threeLine =  textstr.readLine();
                                                                                     fourLine = textstr.readLine();
                                                                                    ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                              };break;

                                                             case 3: {
                                                                                    oneLine = textstr.readLine();
                                                                                    oneMoreLine=textstr.readLine();
                                                                                    twoLine = textstr.readLine();
                                                                                    threeLine =  textstr.readLine();
                                                                                    fourLine = textstr.readLine();
                                                                                   ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                         };break;
                                                    }
                                            }
                                    }
                            };break;


                            case 15:{
            /**/                            ui->pushButton_15->setEnabled(true);                    // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                            readName=textstr.readLine();

                                             if(readName.startsWith("ButName="))
                                            {
                                                            variable = "ButName=";
                                                            readName.remove(readName.indexOf(comments),1200);
                                                            readName.remove(readName.indexOf(variable),8);
            /**/                                            ui->pushButton_15->setText(readName.trimmed());
                                                            if (readName.indexOf("//")>0)
                                                            {

                                                                ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                                return;
                                                            }

                                                            readVer=textstr.readLine();
                                                            if(readVer.startsWith("ButVer="))
                                                            {
                                                                    variable = "ButVer=";
                                                                    readVer.remove(readVer.indexOf(comments),1200);
                                                                    readVer.remove(readVer.indexOf(variable),7);

                                                                    if (readVer.indexOf("//")>0)
                                                                    {
                                                                             ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                            return;
                                                                    }

                                                                            QString oneLine;
                                                                            QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                            QString twoLine;
                                                                            QString threeLine;
                                                                            QString fourLine;                               // строка 4
                                                     switch(readVer.trimmed().toInt())
                                                    {
                                                        case 1: {
                                                                    oneLine=textstr.readLine();
                                                                    oneMoreLine=textstr.readLine();
                                                                    twoLine = textstr.readLine();
                                                                    threeLine = textstr.readLine();
                                                                    fourLine = textstr.readLine();
                                                                     ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                        }; break;

                                                       case 2:{
                                                                     oneLine = textstr.readLine();
                                                                     twoLine = textstr.readLine();
                                                                     threeLine =  textstr.readLine();
                                                                     fourLine = textstr.readLine();
                                                                     ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                        };break;

                                                    case 3: {
                                                                    oneLine = textstr.readLine();
                                                                    oneMoreLine=textstr.readLine();
                                                                    twoLine = textstr.readLine();
                                                                    threeLine =  textstr.readLine();
                                                                    fourLine = textstr.readLine();
                                                                    ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                        };break;
                                             }
                                    }
                                     else
                                     {
                                             ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                               return;
                                     }
                             }
                    };break;


                        case 16:{
             /**/                            ui->pushButton_16->setEnabled(true);           // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                             readName=textstr.readLine();

                                             if(readName.startsWith("ButName="))
                                           {
                                                    variable = "ButName=";
                                                    readName.remove(readName.indexOf(comments),1200);
                                                    readName.remove(readName.indexOf(variable),8);
             /**/                                   ui->pushButton_16->setText(readName.trimmed());
                                                    if (readName.indexOf("//")>0)
                                                    {

                                                        ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                        return;
                                                    }

                                                     readVer=textstr.readLine();
                                                    if(readVer.startsWith("ButVer="))
                                                   {
                                                            variable = "ButVer=";
                                                            readVer.remove(readVer.indexOf(comments),1200);
                                                            readVer.remove(readVer.indexOf(variable),7);
                                                            if (readVer.indexOf("//")>0)
                                                            {
                                                                     ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                    return;
                                                            }

                                                                     QString oneLine;
                                                                     QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                     QString twoLine;
                                                                     QString threeLine;
                                                                     QString fourLine;                               // строка 4
                                     switch(readVer.trimmed().toInt())
                                    {
                                            case 1: {
                                                            oneLine=textstr.readLine();
                                                            oneMoreLine=textstr.readLine();
                                                            twoLine = textstr.readLine();
                                                            threeLine = textstr.readLine();
                                                            fourLine = textstr.readLine();
                                                            ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                               }; break;

                                            case 2:{
                                                            oneLine = textstr.readLine();
                                                            twoLine = textstr.readLine();
                                                            threeLine =  textstr.readLine();
                                                            fourLine = textstr.readLine();
                                                            ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                            };break;

                                             case 3: {
                                                            oneLine = textstr.readLine();
                                                            oneMoreLine=textstr.readLine();
                                                            twoLine = textstr.readLine();
                                                            threeLine =  textstr.readLine();
                                                            fourLine = textstr.readLine();
                                                           ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                            };break;
                                         }
                                    }
                                   else
                                   {
                                        ErrorMes(8);                        // ОШИБКА если неверно написана переменная ButVer
                                        return;
                                    }
                            }
                    };break;

            //________________________________________________________________________________FIELD2____________________________________________________________________________________________________________________________________________________________
                        case 21:{

                                ui->pushButton_21->setEnabled(true);                            //Если номер кнопки совпадает с case - она становится активной
                                readName=textstr.readLine();                                    // Считывается следующая строка
                                if(readName.startsWith("ButName="))                             // Если эта строка начинается ButName=
                               {
                                        variable = "ButName=";                                  // переменной присваивается значение этой строки
                                        readName.remove(readName.indexOf(comments),1200);       // 1200 символов после ; - удаляется
                                        readName.remove(readName.indexOf(variable),8);          // удаляется начало строки "ButName="
                                        ui->pushButton_21->setText(readName.trimmed());         // то, что остается - является именем кнопки
                                        if (readName.indexOf("//")>0)
                                        {

                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                            return;
                                        }

                                        readVer=textstr.readLine();                             // читается следующая строка
                                                if(readVer.startsWith("ButVer="))               // если строка начинается с "ButVer = "
                                                {
                                                         variable = "ButVer=";                                  // переменной присваивается значение этой строки
                                                        readVer.remove(readVer.indexOf(comments),1200);         // 1200 символов после ; - удаляется
                                                        readVer.remove(readVer.indexOf(variable),7);            // удаляется начало строки "ButVer="

                                                        if (readVer.indexOf("//")>0)
                                                        {
                                                                 ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                return;
                                                        }
                                                                                                                // далее необходимо единовременно считать 3 строки
                                                                QString oneLine;                                // строка 1
                                                                QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                QString twoLine;                                // строка 2
                                                                QString threeLine;                              // строка 3
                                                                QString fourLine;                               // строка 4

                                                                 switch(readVer.trimmed().toInt())              // в зависимости от того, что содержалось после ButVer
                                                                {
                                                                        case 1: {                               // определяется режим работы кнопки
                                                                                        oneLine=textstr.readLine();     //считывает 3 строки
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine = textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                         ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);             // посылает на обработку в  соответствующую функцию

                                                                                }; break;

                                                                        case 2:{                                        // определяется режим работы кнопки
                                                                                        oneLine = textstr.readLine();   //считывает 3 строки
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);           // посылает на обработку в  соответствующую функцию
                                                                                };break;

                                                                        case 3: {
                                                                                        oneLine = textstr.readLine();   // определяется режим работы кнопки
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();   //считывает 3 строки
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);    // посылает на обработку в  соответствующую функцию
                                                                                 };break;

                                                                  }
                                                  }
                                                else
                                                {
                                                        ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                          return;
                                                }

                                   }
                                };break;
                    case 22:{
            /**/                        ui->pushButton_22->setEnabled(true);                     // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();
                                        if(readName.startsWith("ButName="))
                                                {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                        ui->pushButton_22->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                                if(readVer.startsWith("ButVer="))
                                                                {
                                                                         variable = "ButVer=";
                                                                         readVer.remove(readVer.indexOf(comments),1200);
                                                                         readVer.remove(readVer.indexOf(variable),7);

                                                                         if (readVer.indexOf("//")>0)
                                                                         {
                                                                                  ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                                 return;
                                                                         }

                                                                         QString oneLine;
                                                                         QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                         QString twoLine;
                                                                         QString threeLine;
                                                                         QString fourLine;                               // строка 4
                                                                    switch(readVer.trimmed().toInt())
                                                                    {
                                                                          case 1: {

                                                                                        oneLine=textstr.readLine();
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine = textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                                 }; break;

                                                                         case 2:{
                                                                                        oneLine = textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                                                   };break;

                                                                         case 3: {

                                                                                        oneLine = textstr.readLine();
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);

                                                                                     };break;
                                                                    }
                                                       }
                                        }
                                };break;

                        case 23:{
            /**/                        ui->pushButton_23->setEnabled(true);                         // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();
                                        if(readName.startsWith("ButName="))
                                                {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                        ui->pushButton_23->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                        if(readVer.startsWith("ButVer="))
                                                        {
                                                                 variable = "ButVer=";
                                                                 readVer.remove(readVer.indexOf(comments),1200);
                                                                 readVer.remove(readVer.indexOf(variable),7);

                                                                 if (readVer.indexOf("//")>0)
                                                                 {
                                                                          ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                         return;
                                                                 }

                                                                QString oneLine;
                                                                QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                QString twoLine;
                                                                QString threeLine;
                                                                QString fourLine;                               // строка 4
                                                         switch(readVer.trimmed().toInt())
                                                      {
                                                           case 1: {

                                                                                oneLine=textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine = textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                     }; break;

                                                           case 2:{
                                                                                oneLine = textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine =  textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                               ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                                     };break;

                                                           case 3: {
                                                                                oneLine = textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine =  textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                     };break;
                                                          }
                                                 }
                                                else
                                                 {
                                                     ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                      return;
                                                 }
                                         }
                           };break;


                        case 24:{
            /**/                        ui->pushButton_24->setEnabled(true);                                 // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();
                                        if(readName.startsWith("ButName="))
                                          {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                        ui->pushButton_24->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                        if(readVer.startsWith("ButVer="))
                                                         {
                                                               variable = "ButVer=";
                                                               readVer.remove(readVer.indexOf(comments),1200);
                                                               readVer.remove(readVer.indexOf(variable),7);

                                                               if (readVer.indexOf("//")>0)
                                                               {
                                                                        ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                       return;
                                                               }

                                                              QString oneLine;
                                                              QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                              QString twoLine;
                                                              QString threeLine;
                                                              QString fourLine;                               // строка 4
                                                         switch(readVer.trimmed().toInt())
                                                     {
                                                        case 1: {

                                                                                oneLine=textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine = textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                 ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                         }; break;

                                                        case 2:{
                                                                                 oneLine = textstr.readLine();
                                                                                 twoLine = textstr.readLine();
                                                                                 threeLine =  textstr.readLine();
                                                                                 fourLine = textstr.readLine();
                                                                                ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                          };break;

                                                         case 3: {
                                                                                oneLine = textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine =  textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                     };break;
                                                }
                                        }
                                       else
                                       {
                                           ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                            return;
                                        }
                                }
                        };break;


                        case 25:{
            /**/                       ui->pushButton_25->setEnabled(true);                                         // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                      readName=textstr.readLine();

                                         if(readName.startsWith("ButName="))
                                        {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                         ui->pushButton_25->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }



                                                        readVer=textstr.readLine();
                                                        if(readVer.startsWith("ButVer="))
                                                        {
                                                                variable = "ButVer=";
                                                                readVer.remove(readVer.indexOf(comments),1200);
                                                                readVer.remove(readVer.indexOf(variable),7);

                                                                if (readVer.indexOf("//")>0)
                                                                {
                                                                         ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                        return;
                                                                }

                                                                QString oneLine;
                                                                QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                QString twoLine;
                                                                QString threeLine;
                                                                QString fourLine;                               // строка 4
                                     switch(readVer.trimmed().toInt())
                                                {
                                                    case 1: {
                                                                oneLine=textstr.readLine();
                                                                oneMoreLine=textstr.readLine();
                                                                twoLine = textstr.readLine();
                                                                threeLine = textstr.readLine();
                                                                fourLine = textstr.readLine();
                                                                 ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                    }; break;

                                                   case 2:{
                                                                 oneLine = textstr.readLine();
                                                                 twoLine = textstr.readLine();
                                                                 threeLine =  textstr.readLine();
                                                                 fourLine = textstr.readLine();
                                                                 ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                    };break;

                                                case 3: {
                                                                oneLine = textstr.readLine();
                                                                oneMoreLine=textstr.readLine();
                                                                twoLine = textstr.readLine();
                                                                threeLine =  textstr.readLine();
                                                                fourLine = textstr.readLine();
                                                                ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                    };break;
                                         }
                                }
                              else
                            {
                                    ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                    return;
                           }
                         }
                };break;


                    case 26:{
            /**/                         ui->pushButton_26->setEnabled(true);                               // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                         readName=textstr.readLine();

                                         if(readName.startsWith("ButName="))
                                       {
                                                variable = "ButName=";
                                                readName.remove(readName.indexOf(comments),1200);
                                                readName.remove(readName.indexOf(variable),8);
            /**/                                ui->pushButton_26->setText(readName.trimmed());
                                                if (readName.indexOf("//")>0)
                                                {

                                                    ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                    return;
                                                }

                                                 readVer=textstr.readLine();
                                                if(readVer.startsWith("ButVer="))
                                               {
                                                        variable = "ButVer=";
                                                        readVer.remove(readVer.indexOf(comments),1200);
                                                        readVer.remove(readVer.indexOf(variable),7);

                                                        if (readVer.indexOf("//")>0)
                                                        {
                                                                 ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                return;
                                                        }

                                                                 QString oneLine;
                                                                 QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                 QString twoLine;
                                                                 QString threeLine;
                                                                 QString fourLine;                               // строка 4
                                 switch(readVer.trimmed().toInt())
                                {
                                        case 1: {
                                                        oneLine=textstr.readLine();
                                                        oneMoreLine=textstr.readLine();
                                                        twoLine = textstr.readLine();
                                                        threeLine = textstr.readLine();
                                                        fourLine = textstr.readLine();
                                                          ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                           }; break;

                                        case 2:{
                                                        oneLine = textstr.readLine();
                                                        twoLine = textstr.readLine();
                                                        threeLine =  textstr.readLine();
                                                        fourLine = textstr.readLine();
                                                        ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                        };break;

                                         case 3: {
                                                        oneLine = textstr.readLine();
                                                        oneMoreLine=textstr.readLine();
                                                        twoLine = textstr.readLine();
                                                        threeLine =  textstr.readLine();
                                                        fourLine = textstr.readLine();
                                                        ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                        };break;
                                     }
                                }

                            else
                            {
                             ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                             return;
                            }
                        }
                };break;

            //________________________________________________________________________________FIELD3____________________________________________________________________________________________________________________________________________________________

                        case 31:{

             /**/               ui->pushButton_31->setEnabled(true);                            //Если номер кнопки совпадает с case - она становится активной
                                readName=textstr.readLine();                                    // Считывается следующая строка
                                if(readName.startsWith("ButName="))                             // Если эта строка начинается ButName=
                               {
                                        variable = "ButName=";                                  // переменной присваивается значение этой строки
                                        readName.remove(readName.indexOf(comments),1200);       // 1200 символов после ; - удаляется
                                        readName.remove(readName.indexOf(variable),8);          // удаляется начало строки "ButName="
             /**/                           ui->pushButton_31->setText(readName.trimmed());         // то, что остается - является именем кнопки

                                        if (readName.indexOf("//")>0)
                                        {

                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                            return;
                                        }


                                        readVer=textstr.readLine();                             // читается следующая строка
                                                if(readVer.startsWith("ButVer="))               // если строка начинается с "ButVer = "
                                                {
                                                         variable = "ButVer=";                                  // переменной присваивается значение этой строки
                                                        readVer.remove(readVer.indexOf(comments),1200);         // 1200 символов после ; - удаляется
                                                        readVer.remove(readVer.indexOf(variable),7);            // удаляется начало строки "ButVer="

                                                        if (readVer.indexOf("//")>0)
                                                        {
                                                                 ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                return;
                                                        }
                                                                                                                // далее необходимо единовременно считать 3 строки
                                                                QString oneLine;                                // строка 1
                                                                QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                QString twoLine;                                // строка 2
                                                                QString threeLine;                              // строка 3
                                                                QString fourLine;                               // строка 4


                                                                 switch(readVer.trimmed().toInt())              // в зависимости от того, что содержалось после ButVer
                                                                {
                                                                        case 1: {                               // определяется режим работы кнопки
                                                                                        oneLine=textstr.readLine();     //считывает 3 строки
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine = textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);           // посылает на обработку в  соответствующую функцию

                                                                                }; break;

                                                                        case 2:{                                        // определяется режим работы кнопки
                                                                                        oneLine = textstr.readLine();   //считывает 3 строки
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                        ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);            // посылает на обработку в  соответствующую функцию
                                                                                };break;

                                                                        case 3: {
                                                                                        oneLine = textstr.readLine();   // определяется режим работы кнопки
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();   //считывает 3 строки
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                       ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);     // посылает на обработку в  соответствующую функцию
                                                                                 };break;

                                                                  }
                                                  }
                                                else
                                                {
                                                 ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                 return;
                                                }

                                   }
                                };break;
                    case 32:{
            /**/                       ui->pushButton_32->setEnabled(true);                    // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();
                                        if(readName.startsWith("ButName="))
                                                {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                         ui->pushButton_32->setText(readName.trimmed());

                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                                if(readVer.startsWith("ButVer="))
                                                                {
                                                                         variable = "ButVer=";
                                                                         readVer.remove(readVer.indexOf(comments),1200);
                                                                         readVer.remove(readVer.indexOf(variable),7);

                                                                         if (readVer.indexOf("//")>0)
                                                                         {
                                                                                  ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                                 return;
                                                                         }

                                                                         QString oneLine;
                                                                         QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                         QString twoLine;
                                                                         QString threeLine;
                                                                          QString fourLine;                               // строка 4
                                                                    switch(readVer.trimmed().toInt())
                                                                    {
                                                                          case 1: {

                                                                                        oneLine=textstr.readLine();
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine = textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                          ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);

                                                                                 }; break;

                                                                         case 2:{
                                                                                        oneLine = textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                       ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                                                   };break;

                                                                         case 3: {

                                                                                        oneLine = textstr.readLine();
                                                                                        oneMoreLine=textstr.readLine();
                                                                                        twoLine = textstr.readLine();
                                                                                        threeLine =  textstr.readLine();
                                                                                        fourLine = textstr.readLine();
                                                                                       ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);

                                                                                     };break;
                                                                    }
                                                       }
                                                 else
                                                 {
                                                    ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                   return;
                                                 }
                                        }
                                };break;

                        case 33:{
            /**/                        ui->pushButton_33->setEnabled(true);                                // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();
                                        if(readName.startsWith("ButName="))
                                                {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                        ui->pushButton_33->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                        if(readVer.startsWith("ButVer="))
                                                        {
                                                                 variable = "ButVer=";
                                                                 readVer.remove(readVer.indexOf(comments),1200);
                                                                 readVer.remove(readVer.indexOf(variable),7);

                                                                 if (readVer.indexOf("//")>0)
                                                                 {
                                                                          ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                         return;
                                                                 }

                                                                QString oneLine;
                                                                QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                QString twoLine;
                                                                QString threeLine;
                                                                QString fourLine;                               // строка 4
                                                         switch(readVer.trimmed().toInt())
                                                      {
                                                           case 1: {

                                                                                oneLine=textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine = textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                 ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                     }; break;

                                                           case 2:{
                                                                                oneLine = textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine =  textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                                     };break;

                                                           case 3: {
                                                                                oneLine = textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine =  textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                               ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                                     };break;
                                                          }
                                                 }
                                               else
                                               {
                                                  ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                                 return;
                                               }
                                         }
                           };break;


                        case 34:{
            /**/                        ui->pushButton_34->setEnabled(true);                // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();
                                        if(readName.startsWith("ButName="))
                                          {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
             /**/                                       ui->pushButton_34->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {

                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                        if(readVer.startsWith("ButVer="))
                                                         {
                                                               variable = "ButVer=";
                                                               readVer.remove(readVer.indexOf(comments),1200);
                                                               readVer.remove(readVer.indexOf(variable),7);

                                                               if (readVer.indexOf("//")>0)
                                                               {
                                                                        ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                       return;
                                                               }

                                                              QString oneLine;
                                                              QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                              QString twoLine;
                                                              QString threeLine;
                                                               QString fourLine;                               // строка 4
                                                         switch(readVer.trimmed().toInt())
                                                     {
                                                        case 1: {

                                                                                oneLine=textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine = textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                         }; break;

                                                        case 2:{
                                                                                 oneLine = textstr.readLine();
                                                                                 twoLine = textstr.readLine();
                                                                                 threeLine =  textstr.readLine();
                                                                                 fourLine = textstr.readLine();
                                                                                 ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                          };break;

                                                         case 3: {
                                                                                oneLine = textstr.readLine();
                                                                                oneMoreLine=textstr.readLine();
                                                                                twoLine = textstr.readLine();
                                                                                threeLine =  textstr.readLine();
                                                                                fourLine = textstr.readLine();
                                                                                ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                     };break;
                                                }
                                        }
                                      else
                                     {
                                        ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                        return;
                                     }
                                }
                        };break;


                        case 35:{
             /**/                      ui->pushButton_35->setEnabled(true);                                 // Хочешь понять, что тут происходит? Ctrl+F + //ЧЕТУТТАКОЕ
                                        readName=textstr.readLine();

                                         if(readName.startsWith("ButName="))
                                        {
                                                        variable = "ButName=";
                                                        readName.remove(readName.indexOf(comments),1200);
                                                        readName.remove(readName.indexOf(variable),8);
            /**/                                        ui->pushButton_35->setText(readName.trimmed());
                                                        if (readName.indexOf("//")>0)
                                                        {
                                                            ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                            return;
                                                        }

                                                        readVer=textstr.readLine();
                                                        if(readVer.startsWith("ButVer="))
                                                        {
                                                                variable = "ButVer=";
                                                                readVer.remove(readVer.indexOf(comments),1200);
                                                                readVer.remove(readVer.indexOf(variable),7);

                                                                if (readVer.indexOf("//")>0)
                                                                {
                                                                         ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                        return;
                                                                }

                                                                        QString oneLine;
                                                                        QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                        QString twoLine;
                                                                        QString threeLine;
                                                                        QString fourLine;                               // строка 4
                                                 switch(readVer.trimmed().toInt())
                                                {
                                                    case 1: {
                                                                oneLine=textstr.readLine();
                                                                oneMoreLine=textstr.readLine();
                                                                twoLine = textstr.readLine();
                                                                threeLine = textstr.readLine();
                                                                fourLine = textstr.readLine();
                                                                  ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                    }; break;

                                                   case 2:{
                                                                 oneLine = textstr.readLine();
                                                                 twoLine = textstr.readLine();
                                                                 threeLine =  textstr.readLine();
                                                                 fourLine = textstr.readLine();
                                                                 ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                                    };break;

                                                   case 3: {
                                                                oneLine = textstr.readLine();
                                                                oneMoreLine=textstr.readLine();
                                                                twoLine = textstr.readLine();
                                                                threeLine =  textstr.readLine();
                                                                fourLine = textstr.readLine();
                                                               ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                                    };break;
                                         }
                                }
                               else
                               {
                                 ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                                 return;
                                }
                         }
                };break;


                    case 36:{
              /**/                       ui->pushButton_36->setEnabled(true);
                                         readName=textstr.readLine();

                                         if(readName.startsWith("ButName="))
                                       {
                                                variable = "ButName=";
                                                readName.remove(readName.indexOf(comments),1200);
                                                readName.remove(readName.indexOf(variable),8);
              /**/                              ui->pushButton_36->setText(readName.trimmed());
                                                if (readName.indexOf("//")>0)
                                                {
                                                    ErrorMes(6);                                           // ОШИБКА если пропущена точка с запятой
                                                    return;
                                                }

                                                 readVer=textstr.readLine();
                                                if(readVer.startsWith("ButVer="))
                                               {
                                                        variable = "ButVer=";
                                                        readVer.remove(readVer.indexOf(comments),1200);
                                                        readVer.remove(readVer.indexOf(variable),7);

                                                        if (readVer.indexOf("//")>0)
                                                        {
                                                                 ErrorMes(9);                                   // Ошибка если пропущена точка с запятой
                                                                return;
                                                        }

                                                                 QString oneLine;
                                                                 QString oneMoreLine;                            // строка 1,5 - тут цвет задать
                                                                 QString twoLine;
                                                                 QString threeLine;
                                                                 QString fourLine;                               // строка 4
                                 switch(readVer.trimmed().toInt())
                                {
                                        case 1: {
                                                        oneLine=textstr.readLine();
                                                        oneMoreLine=textstr.readLine();
                                                        twoLine = textstr.readLine();
                                                        threeLine = textstr.readLine();
                                                        fourLine = textstr.readLine();
                                                         ProcessforVerOne(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                           }; break;

                                         case 2:{
                                                        oneLine = textstr.readLine();
                                                        twoLine = textstr.readLine();
                                                        threeLine =  textstr.readLine();
                                                        fourLine = textstr.readLine();
                                                        ProcessforVerTWO(butNum,oneLine,twoLine,threeLine,fourLine);
                                        };break;

                                         case 3: {
                                                        oneLine = textstr.readLine();
                                                        oneMoreLine=textstr.readLine();
                                                        twoLine = textstr.readLine();
                                                        threeLine =  textstr.readLine();
                                                        fourLine = textstr.readLine();
                                                      ProcessforVerThree(butNum,oneLine,oneMoreLine,twoLine,threeLine,fourLine);
                                        };break;
                                     }
                                }
                           else
                           {
                             ErrorMes(8);// ОШИБКА если неверно написана переменная ButVer
                            return;
                           }

                        }
                };break;

            default:{
                        ErrorMes(5);


                    };break;

            }
        }
     }
   }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Обработка параметорв для режима 1~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::ProcessforVerOne(int num, QString RDColor, QString RDEColor,QString RDStartArr, QString RDEndArr, QString RDCS)
{

    QString variable;                           // строка куда будет вносится название переменных параметров
    QString comments = ";";

    VI.arrEnd.clear();                          //      очищается структура
    VI.arrStart.clear();                        //      для того,
    VI.color = 0;                               //       чтобы принять в себя записанные значения
    VI.endcolor =0;
    VI.CSbyte = 0;


      if(RDCS.startsWith("CS="))                                // строка в которой записан байт с которого начинается подсчет контрольной суммы
      {
         variable = "CS=";                                      // переменная равна началу строки
         RDCS.remove(RDCS.indexOf(comments),1200);              // удаляется все после точки с запятой" ...
         if(RDCS.indexOf("//")>10)                              // если пропущенна точка с запятой и вылез комментарий
         {
                ErrorMes(31);                                   // выведи  ошибку
                return;
         }

         RDCS.remove(RDCS.indexOf(variable),3);                 // удаляется "CS ="
         RDCS.replace(" ","");                                  // удаляются пробелы
         RDCS.toUpper();                                        // стока возводится в верхний регистр
         if(RDCS=="ZERO")                                      // если там осталась ZERO
         {
             VI.CSbyte = 0;                                     // байт с которого начинается подсчет будет 0
         }
         else if(RDCS=="FOUR")                                  // если там осталось FOUR
         {
             VI.CSbyte = 4;                                     // он будет подсчитывать с 3 байта
         }
         else
         {
             ErrorMes(32);                                      // если там написано что-то другое  - выводи ошибку
         }

      }

    if(RDColor.startsWith("{StartColor="))                        // сначала смотрим какой цвет
    {
        variable = "{StartColor=";
        RDColor.remove(RDColor.indexOf(comments),1200);        // на 1200 символов очищаем строку после точки запятой (стираем комментарии)
        if(RDColor.indexOf("//")>0)
        {
            ErrorMes(11);                                       // ОШИБКА - пропущена точка с запятой
            return;
        }
        RDColor.remove(RDColor.indexOf(variable),12);         // стирается в строке{SetColor=
        VI.color = RDColor.trimmed().toInt();                   // номер цвета вставляется в  public структурe
    }
    else
    {
        ErrorMes(10);                                           // ОШИБКА - не так написана переменная
        return;
    }

    if(RDEColor.startsWith("EndColor="))
    {
       variable = "EndColor=";
       RDEColor.remove(RDEColor.indexOf(comments),1200);
       if(RDColor.indexOf("//")>0)
       {
           ErrorMes(28);                                       // ОШИБКА - пропущена точка с запятой
           return;
       }
       RDEColor.remove(RDEColor.indexOf(variable),9);
       VI.endcolor = RDEColor.trimmed().toInt();

    }
    else
    {
        ErrorMes(10);                                           // ОШИБКА - не так написана переменная
        return;
    }

    if(RDStartArr.startsWith("StartArr="))                      // смотрим какой массив start записываем
    {
        variable = "StartArr=";
        RDStartArr.remove(RDStartArr.indexOf(comments),1200);  // на 1200 символов очищаем строку после точки запятой (стираем комментарии)
        if(RDStartArr.indexOf("//")>0){
            ErrorMes(12);                                       // ОШИБКА - пропущена точка с запятой
            return;
        }

        RDStartArr.remove(RDStartArr.indexOf(variable),9);     // стирается в строке{StartArr=
        RDStartArr.replace("0x","");                           // Все 0х удаляются
        RDStartArr.replace(" ","");                            // одиночные пробелы удаляются
        VI.arrStart.append(QByteArray::fromHex(RDStartArr.toLatin1())); //то, что остается преобразуется из hex  в байтовый массив
    }else
    {
        ErrorMes(13);                                           // ОШИБКА - в написании переменной StartArr=
        return;
    }

    if(RDEndArr.startsWith("EndArr="))                          // смотрим какой массив end записывает
    {
        variable = "EndArr=";
        RDEndArr.remove(RDEndArr.indexOf(comments),1200);       // на 1200 символов очищаем строку после точки запятой (стираем комментарии)
        if(RDEndArr.indexOf("//")>0)
        {
            ErrorMes(14);                                       // ОШИБКА - пропущена точка с запятой
            return;
        }
        RDEndArr.remove(RDEndArr.indexOf(variable),7);          // стирается в строке EndArr=
        RDEndArr.replace("0x","");                              // Все 0х удаляются
        RDEndArr.replace(" ","");                               // одиночные пробелы удаляются
        VI.arrEnd.append(QByteArray::fromHex(RDEndArr.toLatin1())); // то, что остается преобразуется из hex  в байтовый массив
    }
    else
    {
        ErrorMes(15);
        return;
    }
switch(num)                                             // вызывается функция с нуженой кнопкой - цифра 1 означает версию, false - запись в кнопку
{
        //____________FIELD1_____________
        case 11 :Button11(1,false);break;               // кнопка 11
        case 12: Button12(1,false);break;
        case 13: Button13(1,false);break;
        case 14: Button14(1,false);break;
        case 15: Button15(1,false);break;
        case 16: Button16(1,false);break;
        //____________FIELD2_____________
        case 21 :Button21(1,false);break;
        case 22: Button22(1,false);break;
        case 23: Button23(1,false);break;
        case 24: Button24(1,false);break;
        case 25: Button25(1,false);break;
        case 26: Button26(1,false);break;

        //____________FIELD3_____________
        case 31 :Button31(1,false);break;
        case 32: Button32(1,false);break;
        case 33: Button33(1,false);break;
        case 34: Button34(1,false);break;
        case 35: Button35(1,false);break;
        case 36: Button36(1,false);break;
        default:                   break;

}

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Обработка параметорв для режима 2~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::ProcessforVerTWO(int num, QString RDSendArr, QString RDCompareArr, QString byteCompare, QString RDCS)
{


    QString variable;                                                     // строка куда будет вносится название переменных параметров
    QString comments = ";";                                               // для удаления комментариев


    VII.arrCheck.clear();                                                         //      очищается структура
    VII.arrSend.clear();                                                         //      для того
    VII.check_byte = 0;                                                          //       чтобы принять в себя записанные значения

    if(RDCS.startsWith("CS="))                                // строка в которой записан байт с которого начинается подсчет контрольной суммы
    {
       variable = "CS=";                                      // переменная равна началу строки
       RDCS.remove(RDCS.indexOf(comments),1200);              // удаляется все после точки с запятой" ...
       if(RDCS.indexOf("//")>10)                              // если пропущенна точка с запятой и вылез комментарий
       {
              ErrorMes(31);                                   // выведи  ошибку
              return;
       }

       RDCS.remove(RDCS.indexOf(variable),3);                 // удаляется "CS ="
       RDCS.replace(" ","");                                  // удаляются пробелы
       RDCS.toUpper();                                        // стока возводится в верхний регистр
       if(RDCS=="ZERO")                                      // если там осталась ZERO
       {
           VII.CSbyte = 0;                                     // байт с которого начинается подсчет будет 0
       }
       else if(RDCS=="FOUR")                                  // если там осталось FOUR
       {
           VII.CSbyte = 4;                                     // он будет подсчитывать с 3 байта
       }
       else
       {
           ErrorMes(32);                                      // если там написано что-то другое  - выводи ошибку
       }

    }
    if(RDSendArr.startsWith("{SendArr="))                                 // В эту переменную записан массив для отправки
    {
        variable = "{SendArr=";
        RDSendArr.remove(RDSendArr.indexOf(comments),1200);             // на 1200 символов очищаем строку после точки запятой (стираем комментарии)

        if(RDSendArr.indexOf("//")>10)
        {
            ErrorMes(16);                                               // ОШИБКА - пропущена точка с запятой
            return;
        }

        RDSendArr.remove(RDSendArr.indexOf(variable),9);                 // стирается в строке{SetColor=
        RDSendArr.replace("0x","");                                     // Все 0х удаляются
        RDSendArr.replace(" ","");                                      // одиночные пробелы удаляются
        VII.arrSend.append(QByteArray::fromHex(RDSendArr.toLatin1()));  // то, что остается преобразуется из hex  в байтовый массив
    }
    else
    {
        ErrorMes(17);                                                   // ОШИБКА - не правильно написана переменная
        return;
    }

    if(RDCompareArr.startsWith("CompareArr="))                          // В эту переменную записан массив для сравнения
    {

         variable = "CompareArr=";
         RDCompareArr.remove(RDCompareArr.indexOf(comments),1200);      // на 1200 символов очищаем строку после точки запятой (стираем комментарии)
         if(RDCompareArr.indexOf("//")>10)
         {
             ErrorMes(18);                                               // ОШИБКА - пропущена точка с запятой
             return;
         }
         RDCompareArr.remove(RDCompareArr.indexOf(variable),9);         // стирается в строке CompareArr=
         RDCompareArr.replace("0x","");                                 // удаляются все 0x
         RDCompareArr.replace(" ","");                                  // одиночные пробелы удаляются
         VII.arrCheck.append(QByteArray::fromHex(RDCompareArr.toLatin1())); // то, что остается преобразуется из hex  в байтовый массив

    }
    else
    {
        ErrorMes(19);                                                    // ОШИБКА - не правильно написана переменная
        return;
    }

    if(byteCompare.startsWith("ByteComp="))                             // в эту переменную записан номер байта с которого осуществляется сравнение
    {
        variable = "ByteComp=";
        byteCompare.replace(" ","");                                    // Удаляются все пробелы
        byteCompare.remove(byteCompare.indexOf(comments),1200);         // на 1200 символов очищаем строку после точки запятой (стираем комментарии)
        if(byteCompare.indexOf("//")>10)
        {
            ErrorMes(20);                                               // ОШИБКА - пропущена точка с запятой
            return;
        }

        byteCompare.remove(byteCompare.indexOf(variable),9);            // в строке удаляются ByteComp=
        VII.check_byte = byteCompare.trimmed().toInt();                 // все что остается - преобразуется в int

    }
    else
    {
        ErrorMes(21);                                                   // ОШИБКА - не правильно написана переменная
        return;
    }

switch(num)
{
        //____________FIELD1_____________
        case 11 :Button11(2,false);break;                               // кнопка 11
        case 12: Button12(2,false);break;
        case 13: Button13(2,false);break;
        case 14: Button14(2,false);break;
        case 15: Button15(2,false);break;
        case 16: Button16(2,false);break;

        //____________FIELD2_____________
        case 21 :Button21(2,false);break;
        case 22: Button22(2,false);break;
        case 23: Button23(2,false);break;
        case 24: Button24(2,false);break;
        case 25: Button25(2,false);break;
        case 26: Button26(2,false);break;

        //____________FIELD3_____________
        case 31 :Button31(2,false);break;
        case 32: Button32(2,false);break;
        case 33: Button33(2,false);break;
        case 34: Button34(2,false);break;
        case 35: Button35(2,false);break;
        case 36: Button36(2,false);break;
        default :                  ;break;

  }

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Обработка параметорв для режима 3~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::ProcessforVerThree(int num,QString RDSendArr,QString RDByteOrder,QString ByteStartView,QString ByteEndView,QString RDCS)
{

    QString variable;                                   // переменная куда будут записываться название переменных в настроечном файле
    QString comments = ";";                             // для удаления комментариев


    VIII.arrSend.clear();                               //      очищается структура
    VIII.byte_EndView = 0;                              //      для того
    VIII.byte_EndView = 0;                              //       чтобы принять в себя записанные значения
    VIII.byte_Order = 0;


    if(RDCS.startsWith("CS="))                                // строка в которой записан байт с которого начинается подсчет контрольной суммы
    {
       variable = "CS=";                                      // переменная равна началу строки
       RDCS.remove(RDCS.indexOf(comments),1200);              // удаляется все после точки с запятой" ...
       if(RDCS.indexOf("//")>10)                              // если пропущенна точка с запятой и вылез комментарий
       {
              ErrorMes(31);                                   // выведи  ошибку
              return;
       }

       RDCS.remove(RDCS.indexOf(variable),3);                 // удаляется "CS ="
       RDCS.replace(" ","");                                  // удаляются пробелы
       RDCS.toUpper();                                        // стока возводится в верхний регистр

       if(RDCS=="ZERO")                                      // если там осталась ZERO
       {
           VIII.CSbyte = 0;                                     // байт с которого начинается подсчет будет 0
       }
       else if(RDCS=="FOUR")                                  // если там осталось FOUR
       {
           VIII.CSbyte = 4;                                     // он будет подсчитывать с 3 байта
       }
       else
       {
           ErrorMes(32);                                      // если там написано что-то другое  - выводи ошибку
       }

        if(RDByteOrder.startsWith("ByteOrder="))
        {
               variable = "ByteOrder=";
               RDByteOrder.remove(RDByteOrder.indexOf(comments),1200);
               if(RDByteOrder.indexOf("//")>10)
               {
                   ErrorMes(30);                                               // ОШИБКА - пропущена точка с запятой
                   return;
               }
               RDByteOrder.replace(" ","");
               RDByteOrder.remove(RDByteOrder.indexOf(variable),10);
               if(RDByteOrder=="MOTOROLA")
               {
                        VIII.byte_Order=1;
               }
               else if(RDByteOrder=="INTEL")
               {
                   VIII.byte_Order=2;
               }

        }
    if(RDSendArr.startsWith("{SendArr="))                               // Если строка начинается с {SendArr=
    {
        variable = "{SendArr=";
        RDSendArr.remove(RDSendArr.indexOf(comments),1200);             // удаляется на 1200 символов дальше точки с запятой
        if( RDSendArr.indexOf("//")>10)
        {
            ErrorMes(22);                                               // ОШИБКА - пропущена точка с запятой
            return;
        }
        RDSendArr.remove(RDSendArr.indexOf(variable),9);                // удаляеся название переменной {SendArr=
        RDSendArr.replace("0x","");                                     // удаляются пробелы между числами массива  и "0x"
        RDSendArr.replace(" ","");                                      // удаляются пробелы
        VIII.arrSend.append(QByteArray::fromHex(RDSendArr.toLatin1())); // то, что остается преобразуется из hex  в байтовый массив

    }
    else
    {
        ErrorMes(23);                                               // ОШИБКА  - не правильно написана переменная
        return;
    }

    if(ByteStartView.startsWith("StartView="))                          // Если строка начинается с StartView==
    {
        variable = "StartView=";
        ByteStartView.replace(" ","");                                    // удаляются все пробелы
        ByteStartView.remove(ByteStartView.indexOf(comments),1200);       // на 1200 символов после точки с запятой - удаляются комментарии
        if( ByteStartView.indexOf("//")>10)
        {
            ErrorMes(24);                                               // ОШИБКА - пропущена точка с запятой
            return;
        }

        ByteStartView.remove(ByteStartView.indexOf(variable),10);         // в строке удаляется  StartView =
        VIII.byte_StartView= ByteStartView.trimmed().toInt();             // то, что остается записывается в переменную, которая содержится в структуре

    }
    else{
        ErrorMes(25);                                               // ОШИБКА -  не правильно написана переменная
        return;
    }

    if(ByteEndView.startsWith("EndView="))                                     // если строка начинается с EndView =
    {
        variable = "EndView=";
        ByteEndView.replace(" ","");                                            // удаляй пробел
        ByteEndView.remove(ByteEndView.indexOf(comments),1200);                 // удаляй комментарии

        if( ByteEndView.indexOf("//")>10)
        {
            ErrorMes(26);                                               // ОШИБКА - пропущена точка с запятой
            return;
        }

        ByteEndView.remove(ByteEndView.indexOf(variable),8);                    // удаляй в строке EndView=
        VIII.byte_EndView = ByteEndView.trimmed().toInt();                      // все, что осталось преобразуй в int и записывай в массив

    }else
    {
        ErrorMes(27);                                               // ОШИБКА -  не правильно написана переменная
        return;
    }

    switch(num){
                 //____________FIELD1____________
                case 11 :Button11(3,false);break;                               // кнопка 11
                case 12: Button12(3,false);break;
                case 13: Button13(3,false);break;
                case 14: Button14(3,false);break;
                case 15: Button15(3,false);break;
                case 16: Button16(3,false);break;

                 //____________FIELD2____________
                case 21 :Button21(3,false);break;
                case 22: Button22(3,false);break;
                case 23: Button23(3,false);break;
                case 24: Button24(3,false);break;
                case 25: Button25(3,false);break;
                case 26: Button26(3,false);break;

                //____________FIELD3____________
                case 31 :Button31(3,false);break;
                case 32: Button32(3,false);break;
                case 33: Button33(3,false);break;
                case 34: Button34(3,false);break;
                case 35: Button35(3,false);break;
                case 36: Button36(3,false);break;
                default :                  break;
    }


}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//
//
//
//
//


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ОТРАБОТКА НАЖАТИЕ НА КНОПКИ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 11~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_11_pressed()
{
    Button11(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 12~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_12_pressed()
{
    Button12(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 13~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_13_pressed()
{
    Button13(0,true);                  // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                         // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);  // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 14~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_14_pressed()
{
    Button14(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 15~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_15_pressed()
{
    Button15(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 16~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_16_pressed()
{
    Button16(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 21~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_21_pressed()
{
    Button21(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                  // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 22~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_22_pressed()
{
    Button22(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);          // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 23~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_23_pressed()
{
    Button23(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
         pressButtonBlockOthers(false);         // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 24~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_24_pressed()
{
    Button24(0,true);                             // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                   // Если если версия кнопки  не равна 1
    {
         pressButtonBlockOthers(false);         // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 25~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_25_pressed()
{
    Button25(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);                  // делай недоступными все кнопки
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 26~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_26_pressed()
{
    Button26(0,true);                           // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);                  // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 31~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_31_pressed()
{
    Button31(0,true);                            // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                 // Если если версия кнопки  не равна 1
    {
    pressButtonBlockOthers(false);              // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 32~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_32_pressed()
{
    Button32(0,true);                                   // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                         // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);                  // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 33~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_33_pressed()
{
    Button33(0,true);                                    // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                          // Если если версия кнопки  не равна 1
    {
        pressButtonBlockOthers(false);                  // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 34~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_34_pressed()
{
    Button34(0,true);                                   // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)
    {
        pressButtonBlockOthers(false);                  // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 35~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_35_pressed()
{
    Button35(0,true);                                   // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                         // Если если версия кнопки  не равна 1
    {
    pressButtonBlockOthers(false);                      // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Нажатие на кнопку 36~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_36_pressed()
{
    Button36(0,true);                                   // вызывается функция  0 - никакая - версия ( там функция знает версию) и true - говорит о нажатии
    if(verA!=1)                                         // Если если версия кнопки  не равна 1
    {
    pressButtonBlockOthers(false);                      // делай недоступными все кнопки
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Кнопки групп-бокса 1++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 11~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow ::Button11(int ver,bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
        if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
        {
        switch(ver)                                                                                 // проверяется режим работы кнопки
            {
            case 1 :{                                                                               // режим работы 1

          /**/ ui->pushButton_11->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
         /**/  mode["button11"] = 1;                                                                            // режим работы =1

            int ch=0;                                                                            // переменная для подсчета контрольной суммы
               for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
               ch+=VI.arrStart.at(i);


               quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

               VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
               VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс


               CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
               ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


               for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                        ch+=VI.arrEnd.at(i);

               CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


               VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
               VI.arrEnd.prepend(Adr);

         /**/  arraysSend["arrStart11_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
         /**/  arraysSend["arrEnd11_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end




  /**/              color["SetColor11"]= VI.color;                                                   // цвет при нажатии
  /**/              color["EndColor11"]= VI.endcolor;                                                // цвет при отжатии
                    VI.color=0;
                    VI.endcolor=0;                                                                       // обнуление переменной

  /**/         ui->pushButton_11->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart11_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd11_V1"].toHex());

                };break;

                  case 2:{                                                                            // режим работы 2

                    /**/    ui->pushButton_11->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                    /**/    mode["button11"] = 2;                                                                       // режим работы = 2

                     /**/   bytes["checkByte11_V2"] = VII.check_byte;                                                 // байт с которого начинается сравнение

                            int ch=0;                                                                       // переменная для подсчета контрольной суммы
                            for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                       ch+=VII.arrSend.at(i);

                           quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                            VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                            VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                      /**/   arraysSend["arrSend11_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                      /**/   arraysSend["arrCheck11_V2"].append(VII.arrCheck);
                                                                                // Запись в public контейнер QMap массива для проверки

 /**/         ui->pushButton_11->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend11_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck11_V2"].toHex());

                };break;

             case 3:{
                      /**/   mode["button11"] = 3;                                                                         // режим работы
                      /**/    ui->pushButton_11->setCheckable(false);
                      /**/    bytes["byteOrder11"] = VIII.byte_Order;                                               // порядок байт
                      /**/     bytes["startView11_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                      /**/     bytes["EndView11_V3"]   = VIII.byte_EndView;                                            // байт с которого заканчивает считать отображение

                             int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                             for(int i=VII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                             ch+=VIII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                             VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                             VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                    /**/    arraysSend["arrSend11_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки


                /**/     ui->pushButton_11->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend11_V3"].toHex());

                      };break;

                              }
     }
    //=====================================================================================================================================================================================================================================

                                                                                                       // режим раобты


    //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
        if(press==true)                                                             // Если кнопка нажатия
        {
     /**/   if(mode["button11"]==1)                                                             // если режим работы 1
            {verA=1;
        /**/  if(ui->pushButton_11->isChecked())                                  // если кнопка выбрана
               {
        /**/       emit WriteDatatoPort(arraysSend["arrEnd11_V1"]);                // запись в порт массива end


     /**/        switch(color["EndColor11"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_11->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_11->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_11->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_11->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_11->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_11->setStyleSheet("default");break;             // фоновый цвет
                    }
                }
                 else
                {

        /**/     emit WriteDatatoPort(arraysSend["arrStart11_V1"]);               // запись в порт массива start
    /**/        switch(color["SetColor11"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_11->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_11->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_11->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_11->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_11->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_11->setStyleSheet("default");break;             // фоновый цвет

                    }


                }
            }
    /**/        else if(mode["button11"] == 2)                                                      // если режим работы 2
            {
       /**/     emit WriteDatatoPort(arraysSend["arrSend11_V2"]);                   // запись в порт массива для отправки

       /**/           numberButton = 11;
       /**/           CheckWith.append(arraysSend["arrCheck11_V2"]);


                 checkByte =  bytes["checkByte11_V2"];
                 verA=2;
                 waitTime->start(4000);                                             // таймер на 4 секунды

            }
       /**/    else if(mode["button11"]==3)                                                        // если режим работы 3
            {
        /**/    emit WriteDatatoPort(arraysSend["arrSend11_V3"]);                   // запись в порт массива для отправки

                verA=3;
        /**/    byteOrd = bytes["byteOrder11"];                                 // какой порядок байт для отображения
        /**/    startVByte= bytes["startView11_V3"];
         /**/   endVByte = bytes["EndView11_V3"] ;
                waitTime->start(4000);                                             // таймер на 4 секунды

            }
        }
    //==================================================================================================================================================================================================================================================
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 12~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button12(int ver,bool press)
{

//__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
    if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
    {
    switch(ver)                                                                                 // проверяется режим работы кнопки
        {
        case 1 :{                                                                               // режим работы 1

      /**/ ui->pushButton_12->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
      /**/ mode["button12"] = 1;                                                                            // режим работы =1
           int ch=0;                                                                            // переменная для подсчета контрольной суммы
     /**/  for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
           ch+=VI.arrStart.at(i);


           quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

           VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
           VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

           CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
           ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


   /**/   for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                    ch+=VI.arrEnd.at(i);

           CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


           VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
           VI.arrEnd.prepend(Adr);

     /**/  arraysSend["arrStart12_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
     /**/  arraysSend["arrEnd12_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


  /**/              color["SetColor12"]= VI.color;                                                   // цвет при нажатии
  /**/              color["EndColor12"]= VI.endcolor;                                                // цвет при отжатии
                    VI.color=0;
                    VI.endcolor=0;                                                                       // обнуление переменной                                                                         // обнуление переменной

 /**/         ui->pushButton_12->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart12_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd12_V1"].toHex());
                };break;

              case 2:{                                                                            // режим работы 2

                /**/    ui->pushButton_12->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/      mode["button12"] = 2;                                                                       // режим работы = 2
               /**/     bytes["checkByte12_V2"]= VII.check_byte;                                               // байт с которого начинается сравнение

                        int ch=0;                                                                       // переменная для подсчета контрольной суммы
               /**/    for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                   ch+=VII.arrSend.at(i);

                       quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                        VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                        VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                  /**/   arraysSend["arrSend12_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                  /**/   arraysSend["arrCheck12_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

            /**/         ui->pushButton_12->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend12_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck12_V2"].toHex());
                 };break;

         case 3:{
                  /**/    mode["button12"] = 3;                                                                         // режим работы
                  /**/    ui->pushButton_12->setCheckable(false);

                /**/     bytes["byteOrder12"] = VIII.byte_Order;                     // порядок байт
                 /**/    bytes["startView12_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                 /**/    bytes["EndView12_V3"]   = VIII.byte_EndView;  // байт с которого заканчивает считать отображение

                         int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
            /**/        for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                         ch+=VIII.arrSend.at(i);

                         quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                         VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                         VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                /**/    arraysSend["arrSend12_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки

                       ui->pushButton_12->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend12_V3"].toHex());
                  };break;

                          }
 }
//=====================================================================================================================================================================================================================================
                                                                                               // режим раобты


//________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
    if(press==true)                                                             // Если кнопка нажатия
    {
   /**/     if(mode["button12"]==1)                                                             // если режим работы 1
        { verA=1;
    /**/  if(ui->pushButton_12->isChecked())                                  // если кнопка выбрана
           {
    /**/       emit WriteDatatoPort(arraysSend["arrEnd12_V1"]);                // запись в порт массива end

   /**/             switch(color["EndColor12"])                                                                     // Выбор цвета кнопки
                {
                /**/    case 0 : ui->pushButton_12->setStyleSheet("default");break;             // фоновый цвет
                /**/    case 1 : ui->pushButton_12->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                /**/    case 2 : ui->pushButton_12->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                /**/    case 3 : ui->pushButton_12->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                /**/    case 4 : ui->pushButton_12->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                /**/    default :ui->pushButton_12->setStyleSheet("default");break;             // фоновый цвет

                }
           }
             else
            {
    /**/     emit WriteDatatoPort(arraysSend["arrStart12_V1"]);               // запись в порт массива start

    /**/            switch(color["SetColor12"])                                                                     // Выбор цвета кнопки
                {
                     /**/    case 0 : ui->pushButton_12->setStyleSheet("default");break;             // фоновый цвет
                     /**/    case 1 : ui->pushButton_12->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                     /**/    case 2 : ui->pushButton_12->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                     /**/    case 3 : ui->pushButton_12->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                     /**/    case 4 : ui->pushButton_12->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                     /**/    default :ui->pushButton_12->setStyleSheet("default");break;             // фоновый цвет
                }

            }

        }
  /**/      else if(mode["button12"]== 2)                                                      // если режим работы 2
        {
   /**/     emit WriteDatatoPort(arraysSend["arrSend12_V2"]);                   // запись в порт массива для отправки

    /**/         numberButton = 12;
   /**/          CheckWith.append(arraysSend["arrCheck12_V2"]);

   /**/          checkByte =  bytes["checkByte12_V2"];
                 verA=2;
            waitTime->start(4000);                                                // таймер на 4 секунды

        }
   /**/     else if(mode["button12"]==3)                                                        // если режим работы 3
        {
    /**/    emit WriteDatatoPort(arraysSend["arrSend12_V3"]);                   // запись в порт массива для отправки
            verA=3;
    /**/    byteOrd = bytes["byteOrder12"];                                 // какой порядок байт для отображения
    /**/    startVByte= bytes["startView12_V3"];
   /**/     endVByte = bytes["EndView12_V3"] ;
            waitTime->start(4000);                                                   // таймер на 4 секунды

        }
    }
//==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 13~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button13(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
        if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
        {
        switch(ver)                                                                                 // проверяется режим работы кнопки
            {
            case 1 :{                                                                               // режим работы 1

          /**/ ui->pushButton_13->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
          /**/ mode["button13"] = 1;                                                                            // режим работы =1
               int ch=0;                                                                            // переменная для подсчета контрольной суммы
      /**/     for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
               ch+=VI.arrStart.at(i);


               quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

               VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
               VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс


               CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
               ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


         /**/  for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                        ch+=VI.arrEnd.at(i);

               CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


               VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
               VI.arrEnd.prepend(Adr);

         /**/  arraysSend["arrStart13_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
         /**/  arraysSend["arrEnd13_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


       /**/              color["SetColor13"]= VI.color;                                                   // цвет при нажатии
       /**/              color["EndColor13"]= VI.endcolor;                                                // цвет при отжатии
                         VI.color=0;
                         VI.endcolor=0;                                                                       // обнуление переменной                                                                             // обнуление переменной

 /**/         ui->pushButton_13->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart13_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd13_V1"].toHex());
        };break;

                  case 2:{                                                                            // режим работы 2

                    /**/    ui->pushButton_13->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                    /**/      mode["button13"] = 2;                                                                       // режим работы = 2
                    /**/     bytes["checkByte13_V2"]= VII.check_byte;
                            int ch=0;                                                                       // переменная для подсчета контрольной суммы
                    /**/    for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                       ch+=VII.arrSend.at(i);

                           quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                            VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                            VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                      /**/   arraysSend["arrSend13_V3"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                      /**/   arraysSend["arrCheck13_V3"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

            /**/         ui->pushButton_13->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend13_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck13_V2"].toHex());


                     };break;

             case 3:{
                      /**/    mode["button13"] = 3;                                                                         // режим работы
                      /**/    ui->pushButton_13->setCheckable(false);
                        /**/     bytes["byteOrder13"] = VIII.byte_Order;                     // порядок байт
                     /**/         bytes["startView13_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                     /**/          bytes["EndView13_V3"]   = VIII.byte_EndView;                                            // байт с которого заканчивает считать отображение

                             int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                  /**/           for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                             ch+=VIII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                             VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                             VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                    /**/    arraysSend["arrSend13_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки

                    /**/    ui->pushButton_13->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend13_V3"].toHex());
                      };break;

                              }
     }
    //=====================================================================================================================================================================================================================================
                                                                                                   // режим раобты


    //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
        if(press==true)                                                             // Если кнопка нажатия
        {
       /**/     if(mode["button13"]==1)                                                             // если режим работы 1
            { verA=1;
        /**/  if(ui->pushButton_13->isChecked())                                  // если кнопка выбрана
               {
        /**/       emit WriteDatatoPort(arraysSend["arrEnd13_V1"]);                // запись в порт массива end
    /**/
               /**/        switch(color["EndColor13"])                                                                     // Выбор цвета кнопки
                                   {
                                        /**/    case 0 : ui->pushButton_13->setStyleSheet("default");break;             // фоновый цвет
                                        /**/    case 1 : ui->pushButton_13->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                                        /**/    case 2 : ui->pushButton_13->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                                        /**/    case 3 : ui->pushButton_13->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                                        /**/    case 4 : ui->pushButton_13->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                                        /**/    default :ui->pushButton_13->setStyleSheet("default");break;             // фоновый цвет
                                   }
                }
                 else
                {
        /**/     emit WriteDatatoPort(arraysSend["arrStart13_V1"]);               // запись в порт массива start

     /**/        switch(color["SetColor13"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_13->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_13->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_13->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_13->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_13->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_13->setStyleSheet("default");break;             // фоновый цвет

                    }
                }
            }
      /**/      else if(mode["button13"]== 2)                                                      // если режим работы 2
            {
       /**/     emit WriteDatatoPort(arraysSend["arrSend13_V3"]);                   // запись в порт массива для отправки
       /**/          numberButton = 13;
       /**/          CheckWith.append(arraysSend["arrCheck13_V3"]);


       /**/          checkByte =  bytes["checkByte13_V2"];
                    verA=2;
                    waitTime->start(4000);
            }
       /**/     else if(mode["button13"]==3)                                                        // если режим работы 3
            {
        /**/    emit WriteDatatoPort(arraysSend["arrSend13_V3"]);                   // запись в порт массива для отправки
                verA=3;
        /**/    byteOrd = bytes["byteOrder13"];                                 // какой порядок байт для отображения
        /**/    startVByte= bytes["startView13_V3"];
       /**/     endVByte = bytes["EndView13_V3"] ;
                waitTime->start(4000);                                                   // таймер на 4 секунды
                                                             // таймер на 4 секунды

            }
        }
    //==================================================================================================================================================================================================================================================


}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 14~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button14(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
        if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
        {
        switch(ver)                                                                                 // проверяется режим работы кнопки
            {
            case 1 :{                                                                               // режим работы 1

          /**/ ui->pushButton_14->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
          /**/ mode["button14"] = 1;                                                                            // режим работы =1
               int ch=0;                                                                            // переменная для подсчета контрольной суммы
       /**/    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
               ch+=VI.arrStart.at(i);


               quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

               VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
               VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

               CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
               ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


        /**/   for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                        ch+=VI.arrEnd.at(i);

               CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


               VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
               VI.arrEnd.prepend(Adr);

         /**/  arraysSend["arrStart14_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
         /**/  arraysSend["arrEnd14_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end

/**/              color["SetColor14"]= VI.color;                                                   // цвет при нажатии
/**/              color["EndColor14"]= VI.endcolor;                                                // цвет при отжатии
                  VI.color=0;
                  VI.endcolor=0;                                                                          // обнуление переменной

/**/         ui->pushButton_14->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart14_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd14_V1"].toHex());
                      };break;

                  case 2:{                                                                            // режим работы 2

                    /**/    ui->pushButton_14->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                   /**/      mode["button14"] = 2;                                                                       // режим работы = 2
                   /**/     bytes["checkByte14_V2"]= VII.check_byte;                                                // байт с которого начинается сравнение

                            int ch=0;                                                                       // переменная для подсчета контрольной суммы
                   /**/     for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                       ch+=VII.arrSend.at(i);

                           quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                            VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                            VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                      /**/   arraysSend["arrSend14_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                      /**/   arraysSend["arrCheck14_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                     /**/         ui->pushButton_14->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend14_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck14_V2"].toHex());

                     };break;

             case 3:{
                      /**/    mode["button14"] = 3;                                                                         // режим работы
                      /**/    ui->pushButton_14->setCheckable(false);
      /**/                          bytes["byteOrder12"] = VIII.byte_Order;                     // порядок байт
     /**/                            bytes["startView14_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
     /**/                            bytes["EndView14_V3"]   = VIII.byte_EndView;                                           // байт с которого заканчивает считать отображение

                             int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                             for(int i=VII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                             ch+=VIII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                             VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                             VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

              /**/    arraysSend["arrSend14_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
              /**/    ui->pushButton_14->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend14_V3"].toHex());
                      };break;

                              }
     }
    //=====================================================================================================================================================================================================================================
                                                                                                   // режим раобты


    //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
        if(press==true)                                                             // Если кнопка нажатия
        {
       /**/     if(mode["button14"]==1)                                                             // если режим работы 1
            { verA=1;
        /**/  if(ui->pushButton_14->isChecked())                                  // если кнопка выбрана
               {
        /**/       emit WriteDatatoPort(arraysSend["arrEnd14_V1"]);                // запись в порт массива end


       /**/        switch(color["EndColor14"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_14->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_14->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_14->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_14->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_14->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_14->setStyleSheet("default");break;             // фоновый цвет
                    }
              }
                 else
                {

     /**/     emit WriteDatatoPort(arraysSend["arrStart14_V1"]);               // запись в порт массива start

     /**/      switch(color["SetColor14"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_14->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_14->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_14->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_14->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_14->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_14->setStyleSheet("default");break;             // фоновый цвет

                    }


                }
            }
      /**/      else if(mode["button14"]== 2)                                                      // если режим работы 2
            {
       /**/     emit WriteDatatoPort(arraysSend["arrSend14_V2"]);                   // запись в порт массива для отправки
                /**/          numberButton = 14;
                /**/          CheckWith.append(arraysSend["arrCheck14_V2"]);

               /**/           checkByte =  bytes["checkByte14_V2"];
                              verA=2;
                              waitTime->start(4000);                                            // таймер на 4 секунды

            }
       /**/     else if(mode["button14"]==3)                                                        // если режим работы 3
            {
        /**/    emit WriteDatatoPort(arraysSend["arrSend14_V3"]);                   // запись в порт массива для отправки
                verA=3;
         /**/    byteOrd = bytes["byteOrder14"];                                 // какой порядок байт для отображе
        /**/    startVByte= bytes["startView14_V3"];
       /**/     endVByte = bytes["EndView14_V3"] ;
                waitTime->start(4000);                                               // таймер на 4 секунды

            }
        }
    //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 15~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button15(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
        if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
        {
        switch(ver)                                                                                 // проверяется режим работы кнопки
            {
            case 1 :{                                                                               // режим работы 1

          /**/ ui->pushButton_15->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
          /**/ mode["button15"] = 1;                                                                            // режим работы =1
               int ch=0;                                                                            // переменная для подсчета контрольной суммы
               for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
               ch+=VI.arrStart.at(i);


               quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

               VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
               VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

               CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
               ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


               for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                        ch+=VI.arrEnd.at(i);

               CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


               VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
               VI.arrEnd.prepend(Adr);

         /**/  arraysSend["arrStart15_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
         /**/  arraysSend["arrEnd15_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


               /**/              color["SetColor15"]= VI.color;                                                   // цвет при нажатии
               /**/              color["EndColor15"]= VI.endcolor;                                                // цвет при отжатии
                                 VI.color=0;
                                 VI.endcolor=0;                                                                     // обнуление переменной

        /**/         ui->pushButton_15->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart15_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd15_V1"].toHex());
        };break;

                  case 2:{                                                                            // режим работы 2

                    /**/    ui->pushButton_15->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                   /**/      mode["button15"] = 2;                                                                       // режим работы = 2
                    /**/     bytes["checkByte15_V2"]= VII.check_byte;                                                 // байт с которого начинается сравнение

                            int ch=0;                                                                       // переменная для подсчета контрольной суммы
                            for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                       ch+=VII.arrSend.at(i);

                           quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                            VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                            VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                      /**/   arraysSend["arrSend15_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                      /**/   arraysSend["arrCheck15_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                   /**/         ui->pushButton_15->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend15_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck15_V2"].toHex());

                     };break;

             case 3:{
                      /**/    mode["button15"] = 3;                                                                         // режим работы
                      /**/    ui->pushButton_15->setCheckable(false);

                     /**/      bytes["byteOrder14"] = VIII.byte_Order;                     // порядок байт
                    /**/       bytes["startView15_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                    /**/       bytes["EndView15_V3"]   = VIII.byte_EndView;                                             // байт с которого заканчивает считать отображение

                             int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                   /**/    for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                             ch+=VIII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                             VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                             VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                    /**/    arraysSend["arrSend15_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                   /**/    ui->pushButton_15->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend15_V3"].toHex());
                      };break;

                              }
     }
    //=====================================================================================================================================================================================================================================
                                                                                                   // режим раобты


    //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
        if(press==true)                                                             // Если кнопка нажатия
        {
       /**/     if(mode["button15"]==1)                                                             // если режим работы 1
            {verA=1;
        /**/  if(ui->pushButton_15->isChecked())                                  // если кнопка выбрана
               {
        /**/     emit WriteDatatoPort(arraysSend["arrEnd15_V1"]);                // запись в порт массива end


  /**/                  switch(color["EndColor15"])                                                                     // Выбор цвета кнопки
                       {
                       /**/    case 0 : ui->pushButton_15->setStyleSheet("default");break;             // фоновый цвет
                       /**/    case 1 : ui->pushButton_15->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                       /**/    case 2 : ui->pushButton_15->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                       /**/    case 3 : ui->pushButton_15->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                       /**/    case 4 : ui->pushButton_15->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                       /**/    default :ui->pushButton_15->setStyleSheet("default");break;             // фоновый цвет

                       }
               }
                 else
                {
        /**/     emit WriteDatatoPort(arraysSend["arrStart15_V1"]);               // запись в порт массива start

     /**/               switch(color["SetColor15"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_15->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_15->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_15->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_15->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_15->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_15->setStyleSheet("default");break;             // фоновый цвет
                    }

                }
            }
      /**/      else if(mode["button15"]== 2)                                                      // если режим работы 2
            {
       /**/     emit WriteDatatoPort(arraysSend["arrSend15_V2"]);                   // запись в порт массива для отправки
       /**/          numberButton = 15;
       /**/          CheckWith.append(arraysSend["arrCheck15_V2"]);
       /**/          checkByte =  bytes["checkByte15_V2"];
                     verA=2;
                     waitTime->start(4000);                                          // таймер на 4 секунды

            }
       /**/     else if(mode["button15"]==3)                                                        // если режим работы 3
            {
        /**/    emit WriteDatatoPort(arraysSend["arrSend15_V3"]);                   // запись в порт массива для отправки
                verA=3;
        /**/    byteOrd = bytes["byteOrder15"];                                 // какой порядок байт для отображе
        /**/    startVByte= bytes["startView15_V3"];
       /**/     endVByte = bytes["EndView15_V3"] ;
                waitTime->start(4000);                                               // таймер на 4 секунды

            }
        }
    //==================================================================================================================================================================================================================================================


}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 16~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button16(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
        if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
        {
        switch(ver)                                                                                 // проверяется режим работы кнопки
            {
            case 1 :{                                                                               // режим работы 1

          /**/ ui->pushButton_16->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
          /**/ mode["button16"] = 1;                                                                            // режим работы =1
               int ch=0;                                                                            // переменная для подсчета контрольной суммы
               for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
               ch+=VI.arrStart.at(i);


               quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

               VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
               VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

               CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
               ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


               for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                        ch+=VI.arrEnd.at(i);

               CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


               VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
               VI.arrEnd.prepend(Adr);

         /**/  arraysSend["arrStart16_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
         /**/  arraysSend["arrEnd16_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


               /**/              color["SetColor16"]= VI.color;                                                   // цвет при нажатии
               /**/              color["EndColor16"]= VI.endcolor;                                                // цвет при отжатии
                                 VI.color=0;
                                 VI.endcolor=0;                                                                            // обнуление переменной

   /**/         ui->pushButton_16->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart16_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd16_V1"].toHex());
                };break;

                  case 2:{                                                                            // режим работы 2

                    /**/    ui->pushButton_16->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                   /**/      mode["button16"] = 2;                                                                       // режим работы = 2
                    /**/     bytes["checkByte16_V2"]= VII.check_byte;                                                   // байт с которого начинается сравнение

                            int ch=0;                                                                       // переменная для подсчета контрольной суммы
                            for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                       ch+=VII.arrSend.at(i);

                           quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                            VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                            VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                      /**/   arraysSend["arrSend16_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                      /**/   arraysSend["arrCheck16_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки


                  /**/         ui->pushButton_16->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend16_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck16_V2"].toHex());

                     };break;

             case 3:{
                      /**/    mode["button16"] = 3;                                                                         // режим работы
                      /**/    ui->pushButton_16->setCheckable(false);
                      /**/     bytes["byteOrder16"] = VIII.byte_Order;                     // порядок байт
                     /**/            bytes["startView16_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                     /**/            bytes["EndView16_V3"]   = VIII.byte_EndView;                                            // байт с которого заканчивает считать отображение

                             int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                             for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                             ch+=VIII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                             VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                             VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                    /**/    arraysSend["arrSend16_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки

                    /**/    ui->pushButton_16->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend16_V3"].toHex());
                      };break;

                              }
     }
    //=====================================================================================================================================================================================================================================
                                                                                                   // режим раобты


    //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
        if(press==true)                                                             // Если кнопка нажатия
        {
       /**/     if(mode["button16"]==1)                                                             // если режим работы 1
            {verA=1;
        /**/  if(ui->pushButton_16->isChecked())                                  // если кнопка выбрана
               {
        /**/       emit WriteDatatoPort(arraysSend["arrEnd16_V1"]);                // запись в порт массива end

    /**/            switch(color["EndColor16"])                                                                     // Выбор цвета кнопки
                       {
                       /**/    case 0 : ui->pushButton_16->setStyleSheet("default");break;             // фоновый цвет
                       /**/    case 1 : ui->pushButton_16->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                       /**/    case 2 : ui->pushButton_16->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                       /**/    case 3 : ui->pushButton_16->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                       /**/    case 4 : ui->pushButton_16->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                       /**/    default :ui->pushButton_16->setStyleSheet("default");break;             // фоновый цвет

                       }
               }
                 else
                {
        /**/     emit WriteDatatoPort(arraysSend["arrStart16_V1"]);               // запись в порт массива start

     /**/        switch(color["SetColor16"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_16->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_16->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_16->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_16->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_16->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_16->setStyleSheet("default");break;             // фоновый цвет

                    }


                }
            }
      /**/      else if(mode["button16"]== 2)                                                      // если режим работы 2
            {
       /**/     emit WriteDatatoPort(arraysSend["arrSend16_V2"]);                   // запись в порт массива для отправки
                /**/          numberButton = 16;
                /**/          CheckWith.append(arraysSend["arrCheck16_V2"]);
                /**/          checkByte =  bytes["checkByte16_V2"];
                              verA=2;
                              waitTime->start(4000);                                              // таймер на 4 секунды

            }
       /**/     else if(mode["button16"]==3)                                                        // если режим работы 3
            {
        /**/    emit WriteDatatoPort(arraysSend["arrSend16_V3"]);                   // запись в порт массива для отправки
                verA=3;
        /**/    byteOrd = bytes["byteOrder16"];                                 // какой порядок байт для отобра
        /**/    startVByte= bytes["startView16_V3"];
       /**/     endVByte = bytes["EndView16_V3"] ;
                waitTime->start(4000);                                                  // таймер на 4 секунды

            }
        }
    //==================================================================================================================================================================================================================================================


}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Кнопки групп-бокса 2++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 21~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button21(int ver, bool press)
{
  //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
        if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
        {
        switch(ver)                                                                                 // проверяется режим работы кнопки
            {
            case 1 :{                                                                               // режим работы 1

          /**/ ui->pushButton_21->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
          /**/ mode["button21"] = 1;                                                                            // режим работы =1
               int ch=0;                                                                            // переменная для подсчета контрольной суммы
               for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
               ch+=VI.arrStart.at(i);


               quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

               VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
               VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

               CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
               ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


               for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                        ch+=VI.arrEnd.at(i);

               CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


               VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
               VI.arrEnd.prepend(Adr);

         /**/  arraysSend["arrStart21_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
         /**/  arraysSend["arrEnd21_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


               /**/              color["SetColor21"]= VI.color;                                                   // цвет при нажатии
               /**/              color["EndColor21"]= VI.endcolor;                                                // цвет при отжатии
                                 VI.color=0;
                                 VI.endcolor=0;                                                                             // обнуление переменной
          /**/         ui->pushButton_21->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart21_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd21_V1"].toHex());
                };break;

                  case 2:{                                                                            // режим работы 2

                    /**/    ui->pushButton_21->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                   /**/      mode["button21"] = 2;                                                                       // режим работы = 2
                   /**/     bytes["checkByte21_V2"]= VII.check_byte;                                                 // байт с которого начинается сравнение

                            int ch=0;                                                                       // переменная для подсчета контрольной суммы
                            for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                       ch+=VII.arrSend.at(i);

                           quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                            VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                            VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                      /**/   arraysSend["arrSend21_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                      /**/   arraysSend["arrCheck21_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                    /**/     ui->pushButton_21->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend21_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck21_V2"].toHex());

                     };break;

             case 3:{
                      /**/    mode["button21"] = 3;                                                                         // режим работы
                      /**/    ui->pushButton_21->setCheckable(false);

                        /**/     bytes["byteOrder21"] = VIII.byte_Order;                     // порядок байт
                      /**/       bytes["startView21_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                      /**/       bytes["EndView21_V3"]   = VIII.byte_EndView;

                             int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                      /**/   for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                             ch+=VIII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                             VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                             VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                    /**/    arraysSend["arrSend21_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                    /**/    ui->pushButton_21->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend21_V3"].toHex());
                      };break;

                              }
     }
    //=====================================================================================================================================================================================================================================
                                                                                                   // режим раобты


    //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
        if(press==true)                                                             // Если кнопка нажатия
        {
       /**/     if(mode["button21"]==1)                                                             // если режим работы 1
            {verA=1;
        /**/  if(ui->pushButton_21->isChecked())                                  // если кнопка выбрана
               {
        /**/       emit WriteDatatoPort(arraysSend["arrEnd21_V1"]);                // запись в порт массива end
   /**/            switch(color["EndColor21"])                                                                     // Выбор цвета кнопки
                       {
                       /**/    case 0 : ui->pushButton_21->setStyleSheet("default");break;             // фоновый цвет
                       /**/    case 1 : ui->pushButton_21->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                       /**/    case 2 : ui->pushButton_21->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                       /**/    case 3 : ui->pushButton_21->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                       /**/    case 4 : ui->pushButton_21->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                       /**/    default :ui->pushButton_21->setStyleSheet("default");break;             // фоновый цвет

                       }
                }
                 else
                {
        /**/     emit WriteDatatoPort(arraysSend["arrStart21_V1"]);               // запись в порт массива start

    /**/        switch(color["SetColor21"])                                                                     // Выбор цвета кнопки
                    {
                         /**/    case 0 : ui->pushButton_21->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_21->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_21->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_21->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_21->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_21->setStyleSheet("default");break;             // фоновый цвет

                    }



                }
            }
      /**/      else if(mode["button21"]== 2)                                                      // если режим работы 2
            {
       /**/     emit WriteDatatoPort(arraysSend["arrSend21_V2"]);                   // запись в порт массива для отправки
                /**/          numberButton = 21;
                /**/          CheckWith.append(arraysSend["arrCheck21_V2"]);
                /**/          checkByte =  bytes["checkByte21_V2"];
                              verA=2;
                              waitTime->start(4000);                                              // таймер на 4 секунды

            }
       /**/     else if(mode["button21"]==3)                                                        // если режим работы 3
            {
        /**/    emit WriteDatatoPort(arraysSend["arrSend21_V3"]);                   // запись в порт массива для отправки
                verA=3;
         /**/    byteOrd = bytes["byteOrder21"];                                 // какой порядок байт для отоб
        /**/    startVByte= bytes["startView21_V3"];
       /**/     endVByte = bytes["EndView21_V3"] ;
                waitTime->start(4000);                                                  // таймер на 4 секунды
                                                         // таймер на 4 секунды

            }
        }
    //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 22~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button22(int ver, bool press)
{
 //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
          if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
          {
          switch(ver)                                                                                 // проверяется режим работы кнопки
              {
              case 1 :{                                                                               // режим работы 1

            /**/ ui->pushButton_22->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
            /**/ mode["button22"] = 1;                                                                            // режим работы =1
                 int ch=0;                                                                            // переменная для подсчета контрольной суммы
           /**/  for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                 ch+=VI.arrStart.at(i);


                 quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                 VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                 VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                 CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                 ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                 for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                          ch+=VI.arrEnd.at(i);

                 CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                 VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                 VI.arrEnd.prepend(Adr);

           /**/  arraysSend["arrStart22_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
           /**/  arraysSend["arrEnd22_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                 /**/              color["SetColor22"]= VI.color;                                                   // цвет при нажатии
                 /**/              color["EndColor22"]= VI.endcolor;                                                // цвет при отжатии
                                   VI.color=0;
                                   VI.endcolor=0;                                                                          // обнуление переменной

        /**/         ui->pushButton_22->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart22_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd22_V1"].toHex());
                        };break;

                    case 2:{                                                                            // режим работы 2

                      /**/    ui->pushButton_22->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                     /**/      mode["button22"] = 2;                                                                       // режим работы = 2
                    /**/     bytes["checkByte22_V2"]= VII.check_byte;                                              // байт с которого начинается сравнение

                              int ch=0;                                                                       // переменная для подсчета контрольной суммы
                              for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                         ch+=VII.arrSend.at(i);

                             quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                              VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                              VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                        /**/   arraysSend["arrSend22_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                        /**/   arraysSend["arrCheck22_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

        /**/         ui->pushButton_22->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend22_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck22_V2"].toHex());
                       };break;

               case 3:{
                        /**/    mode["button22"] = 3;                                                                         // режим работы
                        /**/    ui->pushButton_22->setCheckable(false);
                        /**/     bytes["byteOrder22"] = VIII.byte_Order;                     // порядок байт
                        /**/          bytes["startView22_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                        /**/          bytes["EndView22_V3"]   = VIII.byte_EndView;                                              // байт с которого заканчивает считать отображение

                               int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                               for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                               ch+=VIII.arrSend.at(i);

                               quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                               VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                               VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                      /**/    arraysSend["arrSend22_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                      /**/    ui->pushButton_22->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend22_V3"].toHex());
                        };break;

                                }
       }
      //=====================================================================================================================================================================================================================================
                                                                                                     // режим раобты


      //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
          if(press==true)                                                             // Если кнопка нажатия
          {
         /**/     if(mode["button22"]==1)                                                             // если режим работы 1
              {verA=1;
          /**/  if(ui->pushButton_22->isChecked())                                  // если кнопка выбрана
                 {
          /**/       emit WriteDatatoPort(arraysSend["arrEnd22_V1"]);                // запись в порт массива end


                      switch(color["EndColor22"])                                                                     // Выбор цвета кнопки
                         {
                         /**/    case 0 : ui->pushButton_22->setStyleSheet("default");break;             // фоновый цвет
                         /**/    case 1 : ui->pushButton_22->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                         /**/    case 2 : ui->pushButton_22->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                         /**/    case 3 : ui->pushButton_22->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                         /**/    case 4 : ui->pushButton_22->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                         /**/    default :ui->pushButton_22->setStyleSheet("default");break;             // фоновый цвет

                         }
                 }
                   else
                  {
          /**/     emit WriteDatatoPort(arraysSend["arrStart22_V1"]);               // запись в порт массива start

                      switch(color["SetColor22"])                                                                     // Выбор цвета кнопки
                      {
                           /**/    case 0 : ui->pushButton_22->setStyleSheet("default");break;             // фоновый цвет
                           /**/    case 1 : ui->pushButton_22->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                           /**/    case 2 : ui->pushButton_22->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                           /**/    case 3 : ui->pushButton_22->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                           /**/    case 4 : ui->pushButton_22->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                           /**/    default :ui->pushButton_22->setStyleSheet("default");break;             // фоновый цвет

                      }
                  }
              }
        /**/      else if(mode["button22"]== 2)                                                      // если режим работы 2
              {
         /**/     emit WriteDatatoPort(arraysSend["arrSend22_V2"]);                   // запись в порт массива для отправки
                  /**/          numberButton = 22;
                  /**/          CheckWith.append(arraysSend["arrCheck22_V2"]);
                  /**/          checkByte =  bytes["checkByte22_V2"];
                                verA=2;
                                waitTime->start(4000);                                              // таймер на 4 секунды

              }
         /**/     else if(mode["button22"]==3)                                                        // если режим работы 3
              {
          /**/    emit WriteDatatoPort(arraysSend["arrSend22_V3"]);                   // запись в порт массива для отправки
                  verA=3;
         /**/    byteOrd = bytes["byteOrder22"];                                 // какой порядок байт для ото
         /**/    startVByte= bytes["startView22_V3"];
         /**/     endVByte = bytes["EndView22_V3"] ;
                  waitTime->start(4000);                                                  // таймер на 4 секунды

              }
          }
      //==================================================================================================================================================================================================================================================
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 23~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button23(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
         if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_23->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button23"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart23_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd23_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor23"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor23"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                          // обнуление переменной
                   /**/               ui->pushButton_23->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart23_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd23_V1"].toHex());
                           };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_23->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                         /**/     mode["button23"] = 2;                                                                       // режим работы = 2
                         /**/     bytes["checkByte23_V2"]= VII.check_byte;                                                   // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend23_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck23_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                      /**/         ui->pushButton_23->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend23_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck23_V2"].toHex());

                          };break;

                  case 3:{
                           /**/    mode["button23"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_23->setCheckable(false);
                           /**/     bytes["byteOrder23"] = VIII.byte_Order;                     // порядок байт
                           /**/     bytes["startView23_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                           /**/     bytes["EndView23_V3"]   = VIII.byte_EndView;                                              // байт с которого заканчивает считать отображение
                                                                       // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend23_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки

                         /**/    ui->pushButton_23->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend23_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button23"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_23->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd23_V1"]);                // запись в порт массива end

       /**/          switch(color["EndColor23"])                                                                     // Выбор цвета кнопки
                      {
                          /**/    case 0 : ui->pushButton_23->setStyleSheet("default");break;             // фоновый цвет
                          /**/    case 1 : ui->pushButton_23->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                          /**/    case 2 : ui->pushButton_23->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                          /**/    case 3 : ui->pushButton_23->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                          /**/    case 4 : ui->pushButton_23->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                          /**/    default :ui->pushButton_23->setStyleSheet("default");break;             // фоновый цвет
                       }


                     }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart23_V1"]);               // запись в порт массива start



          /**/          switch(color["SetColor23"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_23->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_23->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_23->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_23->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_23->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_23->setStyleSheet("default");break;             // фоновый цвет

                         }



                     }
                 }
           /**/      else if(mode["button23"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend23_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 23;
                     /**/          CheckWith.append(arraysSend["arrCheck23_V2"]);
                     /**/          checkByte =  bytes["checkByte23_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                           // таймер на 4 секунды

                 }
            /**/     else if(mode["button23"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend23_V3"]);                   // запись в порт массива для отправки
                     verA=3;
            /**/    byteOrd = bytes["byteOrder23"];                                 // какой порядок байт для о
            /**/    startVByte= bytes["startView23_V3"];
            /**/     endVByte = bytes["EndView23_V3"] ;
                     waitTime->start(4000);                                                  // таймер на 4 секунды
                                                                 // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 24~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button24(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_24->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button24"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart24_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd24_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor24"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor24"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                           // обнуление переменной
                   /**/               ui->pushButton_24->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart24_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd24_V1"].toHex());
                        };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_24->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button24"] = 2;                                                                       // режим работы = 2
                        /**/     bytes["checkByte24_V2"]= VII.check_byte;
                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend24_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck24_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки


               /**/         ui->pushButton_24->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend24_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck24_V2"].toHex());

                        };break;

                  case 3:{
                           /**/    mode["button24"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_24->setCheckable(false);
                           /**/     bytes["byteOrder23"] = VIII.byte_Order;                     // порядок байт
                           /**/      bytes["startView24_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                           /**/      bytes["EndView24_V3"]   = VIII.byte_EndView;                                               // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend24_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                          /**/    ui->pushButton_24->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend24_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button24"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_24->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd24_V1"]);                // запись в порт массива end
                         switch(color["EndColor24"])                                                                     // Выбор цвета кнопки
                         {
                            /**/    case 0 : ui->pushButton_24->setStyleSheet("default");break;             // фоновый цвет
                            /**/    case 1 : ui->pushButton_24->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                            /**/    case 2 : ui->pushButton_24->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                            /**/    case 3 : ui->pushButton_24->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                            /**/    case 4 : ui->pushButton_24->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                            /**/    default :ui->pushButton_24->setStyleSheet("default");break;             // фоновый цвет

                         }

                    }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart24_V1"]);               // запись в порт массива start
                         switch(color["SetColor24"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_24->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_24->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_24->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_24->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_24->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_24->setStyleSheet("default");break;             // фоновый цвет

                         }

                     }
                 }
           /**/      else if(mode["button24"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend24_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 24;
                     /**/          CheckWith.append(arraysSend["arrCheck24_V2"]);
                     /**/          checkByte =  bytes["checkByte24_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                              // таймер на 4 секунды

                 }
            /**/     else if(mode["button24"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend24_V3"]);                   // запись в порт массива для отправки
                     verA=3;
               /**/    byteOrd = bytes["byteOrder24"];                                 // какой порядок байт для о
               /**/    startVByte= bytes["startView24_V3"];
               /**/     endVByte = bytes["EndView24_V3"] ;
                     waitTime->start(4000);                                                 // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 25~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button25(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_25->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button25"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart25_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd25_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


               /**/              color["SetColor25"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor25"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                           // обнуление переменной

           /**/               ui->pushButton_25->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart25_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd25_V1"].toHex());

                        };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_25->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button25"] = 2;                                                                       // режим работы = 2
                        /**/     bytes["checkByte25_V2"]= VII.check_byte;                                                 // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend25_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck25_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                /**/         ui->pushButton_25->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend25_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck25_V2"].toHex());
                          };break;

                  case 3:{
                           /**/    mode["button25"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_25->setCheckable(false);

                         /**/     bytes["byteOrder25"] = VIII.byte_Order;                     // порядок байт
                           /**/     bytes["startView25_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                          /**/      bytes["EndView25_V3"]   = VIII.byte_EndView;



                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend25_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                         /**/    ui->pushButton_25->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend25_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button25"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_25->isChecked())                                      // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd25_V1"]);                // запись в порт массива end

             /**/        switch(color["EndColor25"])                                                                     // Выбор цвета кнопки
                            {
                            /**/    case 0 : ui->pushButton_25->setStyleSheet("default");break;             // фоновый цвет
                            /**/    case 1 : ui->pushButton_25->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                            /**/    case 2 : ui->pushButton_25->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                            /**/    case 3 : ui->pushButton_25->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                            /**/    case 4 : ui->pushButton_25->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                            /**/    default :ui->pushButton_25->setStyleSheet("default");break;             // фоновый цвет

                            }

                     }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart25_V1"]);               // запись в порт массива start

             /**/      switch(color["SetColor25"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_25->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_25->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_25->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_25->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_25->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_25->setStyleSheet("default");break;             // фоновый цвет

                         }

                     }
                 }
           /**/      else if(mode["button25"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend25_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 25;
                     /**/          CheckWith.append(arraysSend["arrCheck25_V2"]);
                     /**/          checkByte =  bytes["checkByte25_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                          // таймер на 4 секунды

                 }
            /**/     else if(mode["button25"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend25_V3"]);                   // запись в порт массива для отправки
                     verA=3;
                /**/    byteOrd = bytes["byteOrder25"];                                 // какой порядок байт для о
               /**/    startVByte= bytes["startView25_V3"];
               /**/     endVByte = bytes["EndView25_V3"] ;
                     waitTime->start(4000);                                                 // таймер на 4 секунды
                                                              // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 26~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button26(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_26->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button26"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart26_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd26_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor26"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor26"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                         // обнуление переменной
      /**/               ui->pushButton_26->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart26_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd26_V1"].toHex());

             };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_26->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button26"] = 2;                                                                       // режим работы = 2
                        /**/     bytes["checkByte26_V2"]= VII.check_byte;                                                      // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend26_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck26_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                    /**/         ui->pushButton_26->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend26_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck26+_V2"].toHex());  };break;

                  case 3:{
                           /**/    mode["button26"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_26->setCheckable(false);

                                /**/     bytes["byteOrder26"] = VIII.byte_Order;                     // порядок байт

                           /**/     bytes["startView26_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                           /**/      bytes["EndView26_V3"]   = VIII.byte_EndView;                                          // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend26_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                        /**/    ui->pushButton_26->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend26_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button26"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_26->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd26_V1"]);                // запись в порт массива end

                         switch(color["EndColor25"])                                                                     // Выбор цвета кнопки
                       {
                            /**/    case 0 : ui->pushButton_26->setStyleSheet("default");break;             // фоновый цвет
                            /**/    case 1 : ui->pushButton_26->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                            /**/    case 2 : ui->pushButton_26->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                            /**/    case 3 : ui->pushButton_26->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                            /**/    case 4 : ui->pushButton_26->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                            /**/    default :ui->pushButton_26->setStyleSheet("default");break;             // фоновый цвет

                       }

                    }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart26_V1"]);               // запись в порт массива start

                         switch(color["SetColor26"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_26->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_26->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_26->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_26->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_26->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_26->setStyleSheet("default");break;             // фоновый цвет
                         }




                     }
                 }
           /**/      else if(mode["button26"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend26_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 26;
                     /**/          CheckWith.append(arraysSend["arrCheck26_V2"]);
                     /**/          checkByte =  bytes["checkByte26_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                               // таймер на 4 секунды

                 }
            /**/     else if(mode["button26"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend26_V3"]);                   // запись в порт массива для отправки
                     verA=3;
                /**/    byteOrd = bytes["byteOrder26"];                                 // какой порядок байт для о
               /**/    startVByte= bytes["startView26_V3"];
               /**/     endVByte = bytes["EndView26_V3"] ;
                     waitTime->start(4000);                                                 // таймер на 4 секунды
                                                             // таймер на 4 секунды
                 }
             }
         //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Кнопки групп-бокса 3++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 31~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button31(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_31->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button31"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart31_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd31_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor31"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor31"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                         // обнуление переменной
             /**/   ui->pushButton_31->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart31_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd31_V1"].toHex());

                        };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_31->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button31"] = 2;                                                                       // режим работы = 2
                                 bytes["checkByte31_V2"] = VII.check_byte;                                                 // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend31_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck31_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                /**/         ui->pushButton_31->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend31_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck31_V2"].toHex());
                          };break;

                  case 3:{
                           /**/    mode["button31"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_31->setCheckable(false);
                            /**/     bytes["byteOrder31"] = VIII.byte_Order;                     // порядок байт
                            /**/      bytes["startView31_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                            /**/      bytes["EndView31_V3"]   = VIII.byte_EndView;                                         // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend31_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                        /**/    ui->pushButton_31->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend31_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button31"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_31->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd31_V1"]);                // запись в порт массива end

           /**/       switch(color["EndColor31"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_31->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_31->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_31->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_31->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_31->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_31->setStyleSheet("default");break;             // фоновый цвет

                         }


                    }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart31_V1"]);               // запись в порт массива start

             /**/      switch(color["SetColor31"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_31->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_31->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_31->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_31->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_31->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_31->setStyleSheet("default");break;             // фоновый цвет

                         }
                     }
                 }
           /**/      else if(mode["button31"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend31_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 31;
                     /**/          CheckWith.append(arraysSend["arrCheck31_V2"]);
                     /**/          checkByte =  bytes["checkByte31_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                                // таймер на 4 секунды

                 }
            /**/     else if(mode["button31"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend31_V3"]);                   // запись в порт массива для отправки
                       verA=3;
               /**/    byteOrd = bytes["byteOrder31"];                                 // какой порядок байт для о
               /**/    startVByte= bytes["startView31_V3"];
               /**/     endVByte = bytes["EndView31_V3"] ;
                     waitTime->start(4000);                                            // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 32~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button32(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {verA=1;
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_32->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button32"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart32_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd32_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor32"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor32"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                          // обнуление переменной

            /**/   ui->pushButton_32->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart32_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd32_V1"].toHex());
                           };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_32->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button32"] = 2;                                                                       // режим работы = 2
                        /**/     bytes["checkByte32_V2"]= VII.check_byte;                                                // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend32_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck32_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

           /**/         ui->pushButton_32->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend32_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck32_V2"].toHex());

                        };break;

                  case 3:{
                           /**/    mode["button32"] = 3;                                                                // режим работы
                           /**/    ui->pushButton_31->setCheckable(false);


                           /**/     bytes["byteOrder32"] = VIII.byte_Order;                                              // порядок байт
                           /**/    bytes["startView32_V3"] = VIII.byte_StartView;                                        // байт с которого начинает считать отображениие
                           /**/    bytes["EndView32_V3"]   = VIII.byte_EndView;
                                  int ch=0;                                                                                //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                  // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend32_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                        /**/    ui->pushButton_32->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend32_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button32"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_32->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd32_V1"]);                // запись в порт массива end

         /**/              switch(color["EndColor32"])                                                                     // Выбор цвета кнопки
                          {
                               /**/    case 0 : ui->pushButton_32->setStyleSheet("default");break;             // фоновый цвет
                               /**/    case 1 : ui->pushButton_32->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                               /**/    case 2 : ui->pushButton_32->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                               /**/    case 3 : ui->pushButton_32->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                               /**/    case 4 : ui->pushButton_32->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                               /**/    default :ui->pushButton_32->setStyleSheet("default");break;             // фоновый цвет

                          }
                     }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart32_V1"]);               // запись в порт массива start

              /**/        switch(color["SetColor32"])                                                                     // Выбор цвета кнопки
                             {
                                  /**/    case 0 : ui->pushButton_32->setStyleSheet("default");break;             // фоновый цвет
                                  /**/    case 1 : ui->pushButton_32->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                                  /**/    case 2 : ui->pushButton_32->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                                  /**/    case 3 : ui->pushButton_32->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                                  /**/    case 4 : ui->pushButton_32->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                                  /**/    default :ui->pushButton_32->setStyleSheet("default");break;             // фоновый цвет

                             }
                         }

                 }
           /**/      else if(mode["button32"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend32_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 32;
                     /**/          CheckWith.append(arraysSend["arrCheck32_V2"]);
                     /**/          checkByte =  bytes["checkByte32_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                                       // таймер на 4 секунды

                 }
            /**/     else if(mode["button32"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend32_V3"]);                   // запись в порт массива для отправки
                     verA=3;
               /**/    byteOrd = bytes["byteOrder32"];                                 // какой порядок байт для о
               /**/    startVByte= bytes["startView32_V3"];
               /**/     endVByte = bytes["EndView32_V3"] ;
                     waitTime->start(4000);                                                 // таймер на 4 секунды
                                                              // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 33~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button33(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_33->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button33"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart33_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd33_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end



                    /**/              color["SetColor33"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor33"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                          // обнуление переменной

                     /**/   ui->pushButton_33->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart33_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd33_V1"].toHex());
                           };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_33->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button33"] = 2;                                                                       // режим работы = 2
                       /**/     bytes["checkByte33_V2"]= VII.check_byte;                                                // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend33_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck33_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки


                  /**/         ui->pushButton_33->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend33_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck33_V2"].toHex());

                          };break;

                  case 3:{
                           /**/    mode["button33"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_33->setCheckable(false);
                          /**/     bytes["byteOrder33"] = VIII.byte_Order;                     // порядок байт
                          /**/    bytes["startView33_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                          /**/    bytes["EndView33_V3"]   = VIII.byte_EndView;                                        // байт с которого заканчивает считать отображение


                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend33_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки

                      /**/    ui->pushButton_33->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend33_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты

         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button33"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_33->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd33_V1"]);                // запись в порт массива end
             /**/    switch(color["SetColor33"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_33->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_33->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_33->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_33->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_33->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_33->setStyleSheet("default");break;             // фоновый цвет

                         }

                    }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart33_V1"]);               // запись в порт массива start

        /**/       switch(color["EndColor33"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_33->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_33->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_33->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_33->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_33->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_33->setStyleSheet("default");break;             // фоновый цвет

                         }
                     }
                 }
           /**/      else if(mode["button33"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend33_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 33;
                     /**/          CheckWith.append(arraysSend["arrCheck33_V2"]);
                     /**/          checkByte =  bytes["checkByte33_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                                   // таймер на 4 секунды

                 }
            /**/     else if(mode["button33"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend33_V3"]);                   // запись в порт массива для отправки
                     verA=3;
              /**/    byteOrd = bytes["byteOrder33"];                                 // какой порядок байт для о
             /**/    startVByte= bytes["startView33_V3"];
             /**/    endVByte = bytes["EndView33_V3"] ;
                   waitTime->start(4000);
                 }
             }
         //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 34~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button34(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_34->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button34"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart34_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd34_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor34"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor34"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                          // обнуление переменной
                                      /**/   ui->pushButton_34->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart34_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd34_V1"].toHex());
                           };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_34->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button34"] = 2;                                                                       // режим работы = 2
                       /**/     bytes["checkByte34_V2"]= VII.check_byte;                                                      // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend34_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck34_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                 /**/         ui->pushButton_34->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend34_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck34_V2"].toHex());

                          };break;

                  case 3:{
                           /**/    mode["button34"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_34->setCheckable(false);
                          /**/     bytes["byteOrder34"] = VIII.byte_Order;                     // порядок байт
                          /**/    bytes["startView34_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                          /**/    bytes["EndView34_V3"]   = VIII.byte_EndView;                                              // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend34_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                         /**/    ui->pushButton_34->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend34_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button34"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_34->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd34_V1"]);                // запись в порт массива end

      /**/             switch(color["EndColor34"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_34->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_34->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_34->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_34->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_34->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_34->setStyleSheet("default");break;             // фоновый цвет

                         }

                     }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart34_V1"]);               // запись в порт массива start

         /**/          switch(color["SetColor34"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_34->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_34->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_34->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_34->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_34->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_34->setStyleSheet("default");break;             // фоновый цвет

                         }


                     }
                 }
           /**/      else if(mode["button34"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend34_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 34;
                     /**/          CheckWith.append(arraysSend["arrCheck34_V2"]);
                     /**/          checkByte =  bytes["checkByte34_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                              // таймер на 4 секунды
                 }
            /**/     else if(mode["button34"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend34_V3"]);                   // запись в порт массива для отправки
                     verA=3;
             /**/    byteOrd = bytes["byteOrder34"];                                 // какой порядок байт для о
             /**/    startVByte= bytes["startView34_V3"];
             /**/    endVByte = bytes["EndView34_V3"] ;
                      waitTime->start(4000);                                                    // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 35~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button35(int ver, bool press)
{
    //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_35->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button35"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart35_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd35_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end


                    /**/              color["SetColor35"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor35"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                          // обнуление переменной
                   /**/   ui->pushButton_35->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart35_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd35_V1"].toHex());
                           };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_35->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button35"] = 2;                                                                       // режим работы = 2
                         /**/     bytes["checkByte36_V2"]= VII.check_byte;                                                 // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend35_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck35_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                      /**/         ui->pushButton_35->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend35_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck35_V2"].toHex());

                          };break;

                  case 3:{
                           /**/    mode["button35"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_35->setCheckable(false);
                          /**/     bytes["byteOrder35"] = VIII.byte_Order;                     // порядок байт
                           /**/    bytes["startView35_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                         /**/    bytes["EndView35_V3"]   = VIII.byte_EndView;                                            // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend35_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                        /**/    ui->pushButton_35->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend35_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button35"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_35->isChecked())                                  // если кнопка выбрана
                    {
             /**/       emit WriteDatatoPort(arraysSend["arrEnd35_V1"]);                // запись в порт массива end
                         switch(color["EndColor35"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_35->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_35->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_35->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_35->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_35->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_35->setStyleSheet("default");break;             // фоновый цвет

                         }

                     }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart35_V1"]);               // запись в порт массива start

                    /**/     switch(color["SetColor35"])                                                                     // Выбор цвета кнопки
                           {
                             /**/    case 0 : ui->pushButton_35->setStyleSheet("default");break;             // фоновый цвет
                             /**/    case 1 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                             /**/    case 2 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                             /**/    case 3 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                             /**/    case 4 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                             /**/    default :ui->pushButton_35->setStyleSheet("default");break;             // фоновый цвет
                           }

                     }
                 }
           /**/      else if(mode["button35"]== 2)                                                      // если режим работы 2
                 {
            /**/     emit WriteDatatoPort(arraysSend["arrSend35_V2"]);                   // запись в порт массива для отправки
                     /**/          numberButton = 35;
                     /**/          CheckWith.append(arraysSend["arrCheck35_V2"]);
                     /**/          checkByte =  bytes["checkByte35_V2"];
                                   verA=2;
                                   waitTime->start(4000);                                            // таймер на 4 секунды

                 }
            /**/     else if(mode["button35"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend35_V3"]);                   // запись в порт массива для отправки

                     verA=3;
              /**/    byteOrd = bytes["byteOrder35"];                                 // какой порядок байт для о
             /**/    startVByte= bytes["startView35_V3"];
             /**/    endVByte = bytes["EndView35_V3"] ;
                      waitTime->start(4000);                                         // таймер на 4 секунды
                 }
             }
         //==================================================================================================================================================================================================================================================

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Работа кнопки 36~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::Button36(int ver, bool press)
{
   //__________________________________________________________________Запись данных в кнопку__________________________________________________________________________________________________________________________________________________________
             if(press==false)                                                                            // Если переменная press== false - значит идет запись массивов на передачу и иную обработку "в кнопку"
             {
             switch(ver)                                                                                 // проверяется режим работы кнопки
                 {
                 case 1 :{                                                                               // режим работы 1

               /**/ ui->pushButton_36->setCheckable(true);                                               // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
               /**/ mode["button36"] = 1;                                                                            // режим работы =1
                    int ch=0;                                                                            // переменная для подсчета контрольной суммы
                    for(int i=VI.CSbyte;i< VI.arrStart.size();i++)                                               // Подсчет контрольной суммы
                    ch+=VI.arrStart.at(i);


                    quint8 CS=(quint8)ch&0xFF;                                                           // От контрольной суммы берется 8 бит

                    VI.arrStart.append(CS);                                                              // в байтовый массив start добавляется контрольная сумма
                    VI.arrStart.prepend(Adr);                                                            // в байтовый массив end добавляется адресс

                    CS=0;                                                                                // обнуление для подсчета контрольной суммы для другого байтового массива
                    ch=0;                                                                                // обнуление переменной для подсчета контрольной суммы


                    for(int i=VI.CSbyte;i<VI.arrEnd.size();i++)                                                  // Подсчет контрольной суммы
                             ch+=VI.arrEnd.at(i);

                    CS = (quint8)ch&0xFF;                                                                 // От контрольной суммы берется 8 бит


                    VI.arrEnd.append(CS);                                                                 // в байтовый массив start добавляется контрольная сумма
                    VI.arrEnd.prepend(Adr);

              /**/  arraysSend["arrStart36_V1"].append(VI.arrStart);                                      // Запись в public контейнер QMap массива start
              /**/  arraysSend["arrEnd36_V1"].append(VI.arrEnd);                                          // Запись в public контейнер QMap массива end

                    /**/              color["SetColor36"]= VI.color;                                                   // цвет при нажатии
                    /**/              color["EndColor36"]= VI.endcolor;                                                // цвет при отжатии
                                      VI.color=0;
                                      VI.endcolor=0;                                                                           // обнуление переменной
             /**/   ui->pushButton_36->setToolTip("Режим работы 1 \n Команда Start: "+arraysSend["arrStart36_V1"].toHex()+"\n"+"Команда End: "+arraysSend["arrEnd36_V1"].toHex());

             };break;

                       case 2:{                                                                            // режим работы 2

                         /**/    ui->pushButton_36->setCheckable(false);                                         // если режим кнопки - вариант 1 - кнопка фиксируется в момент нажатия
                        /**/      mode["button36"] = 2;                                                                       // режим работы = 2
                                 bytes["checkByte36_V2"] = VII.check_byte;                                                 // байт с которого начинается сравнение

                                 int ch=0;                                                                       // переменная для подсчета контрольной суммы
                                 for(int i=VII.CSbyte;i< VII.arrSend.size();i++)                                          // Подсчет контрольной суммы
                                            ch+=VII.arrSend.at(i);

                                quint8 CS=(quint8)ch&0xFF;                                                       // От контрольной суммы берется 8 бит

                                 VII.arrSend.append(CS);                                                          // в байтовый массив для отправки добавляется контрольная сумма
                                 VII.arrSend.prepend(Adr);                                                        // в байтовый массив для сумма добавляется адресс


                           /**/   arraysSend["arrSend36_V2"].append(VII.arrSend);                                  // Запись в public контейнер QMap массива для отправки
                           /**/   arraysSend["arrCheck36_V2"].append(VII.arrCheck);                                // Запись в public контейнер QMap массива для проверки

                   /**/         ui->pushButton_36->setToolTip("Режим работы 2 \n Команда Send: "+arraysSend["arrSend36_V2"].toHex()+"\n"+"Массив для сравнения: "+arraysSend["arrCheck36_V2"].toHex());
                          };break;

                  case 3:{
                           /**/    mode["button36"] = 3;                                                                         // режим работы
                           /**/    ui->pushButton_31->setCheckable(false);
                            /**/     bytes["byteOrder36"] = VIII.byte_Order;                     // порядок байт
                          /**/    bytes["startView36_V3"] = VIII.byte_StartView;                                         // байт с которого начинает считать отображениие
                          /**/    bytes["EndView36_V3"]   = VIII.byte_EndView;                                            // байт с которого заканчивает считать отображение
                                                                            // байт с которого заканчивает считать отображение

                                  int ch=0;                                                                           //перемеенная для подсчета контрольной суммы
                                  for(int i=VIII.CSbyte;i< VIII.arrSend.size();i++)                                                            // Подсчет контрольной суммы
                                  ch+=VIII.arrSend.at(i);

                                  quint8 CS=(quint8)ch&0xFF;                                                                         // От контрольной суммы берется 8 бит

                                  VIII.arrSend.append(CS);                                                                           // в байтовый массив для отправки добавляется контрольная сумма
                                  VIII.arrSend.prepend(Adr);                                                                         // в байтовый массив для отправки добавляется контрольная сумма

                         /**/    arraysSend["arrSend36_V3"].append(VIII.arrSend);                                                    // Запись в public контейнер QMap массива для проверки
                        /**/    ui->pushButton_36->setToolTip("Режим работы 3 \n Команда: "+arraysSend["arrSend36_V3"].toHex());
                           };break;

                                   }
          }
         //=====================================================================================================================================================================================================================================
                                                                                                        // режим раобты


         //________________________________________________________________________________________________Нажатие на кнопку___________________________________________________________________________________________________________________________
             if(press==true)                                                             // Если кнопка нажатия
             {
            /**/     if(mode["button36"]==1)                                                             // если режим работы 1
                 {verA=1;
             /**/  if(ui->pushButton_36->isChecked())                                  // если кнопка выбрана
                    {
            /**/       emit WriteDatatoPort(arraysSend["arrEnd36_V1"]);                // запись в порт массива end
            /**/    switch(color["SetColor36"])                                                                     // Выбор цвета кнопки
                         {
                              /**/    case 0 : ui->pushButton_36->setStyleSheet("default");break;             // фоновый цвет
                              /**/    case 1 : ui->pushButton_36->setStyleSheet("QPushButton::!checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                              /**/    case 2 : ui->pushButton_36->setStyleSheet("QPushButton::!checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                              /**/    case 3 : ui->pushButton_36->setStyleSheet("QPushButton::!checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                              /**/    case 4 : ui->pushButton_36->setStyleSheet("QPushButton::!checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                              /**/    default :ui->pushButton_36->setStyleSheet("default");break;             // фоновый цвет

                         }



                    }
                      else
                     {
             /**/     emit WriteDatatoPort(arraysSend["arrStart36_V1"]);               // запись в порт массива start

                         switch(color["SetColor36"])                                                                     // Выбор цвета кнопки
                         {
                           /**/    case 0 : ui->pushButton_35->setStyleSheet("default");break;             // фоновый цвет
                           /**/    case 1 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#a4ff70;border:1px;border-color:#a4ff70}");break;             // зеленый
                           /**/    case 2 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#70dbff;border:1px;border-color:#70dbff}");break;             // синий
                           /**/    case 3 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#fc7462;border:1px;border-color:#fc7462}");break;             // красный
                           /**/    case 4 : ui->pushButton_35->setStyleSheet("QPushButton::checked{background-color:#fff372;border:1px;border-color:#fff372}");break;             // желтый
                           /**/    default :ui->pushButton_35->setStyleSheet("default");break;             // фоновый цвет
                          }


                     }
                 }
              /**/      else if(mode["button36"]== 2)                                                      // если режим работы 2
                 {
               /**/     emit WriteDatatoPort(arraysSend["arrSend36_V2"]);                   // запись в порт массива для отправки
               /**/          numberButton = 36;
               /**/          CheckWith.append(arraysSend["arrCheck36_V2"]);
               /**/          checkByte =  bytes["checkByte36_V2"];
                             verA=2;
                             waitTime->start(4000);                                              // таймер на 4 секунды

                 }
            /**/     else if(mode["button36"]==3)                                                        // если режим работы 3
                 {
             /**/    emit WriteDatatoPort(arraysSend["arrSend36_V3"]);                   // запись в порт массива для отправки
                     verA=3;
             /**/    byteOrd = bytes["byteOrder36"];                                 // какой порядок байт для о
             /**/    startVByte= bytes["startView36_V3"];
             /**/    endVByte = bytes["EndView36s_V3"] ;
                     waitTime->start(4000);                                              // таймер на 4 секунды

                 }
             }
         //==================================================================================================================================================================================================================================================
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++






//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Очищение консоли отправленных сообщений~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_CleanSEND_clicked()
{
    colSEND = 0;                                                // очищения переменной отвечающей за подсчет количества отправленный в com-порт сообщений
    ui->widget_send->clear();                                   // очищение О Т П Р А В Л Е Н О

}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Очищение консоли принятых сообщений~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_CleanARR_clicked()
{
    colARR = 0;                                                         // очищения переменной отвечающей за подсчет количества отправленный в com-порт сообщений
    ui->widget_arrive->clear();                                         // очищение П Р И Н Я Т О
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Сохрание консоли отправленой сообщений~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_SAVESEND_clicked()
{
    //Сохранения всего того,что запечатлено в консоли в виде txt-файла по директории указанной ↘ здесь
    QString consoleName = QFileDialog::getSaveFileName(this,("Сохранить показания консоли отправленных сообщений"),"/home/SEND.txt","Текстовый файл(*.txt)");
    QFile file(consoleName);                                              // открытие файла
    if(file.open(QIODevice::WriteOnly|QIODevice::Text))                   // файл только на запись текста
    {
        file.write(ui->widget_send->toPlainText().toUtf8());              // все что в консоли, записывается в файл кодировка UTF8
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Очищение консоли отправленных сообщений~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_SAVEARR_clicked()
{
    //Сохранения всего того,что запечатлено в консоли в виде txt-файла по директории указанной ↘ здесь
    QString consoleName = QFileDialog::getSaveFileName(this,("Сохранить показания консоли принятых сообщений"),"/home/ARR.txt","Текстовый файл(*.txt)");
    QFile file(consoleName);                                            // открытие файла
    if(file.open(QIODevice::WriteOnly|QIODevice::Text))                 // файл только на запись текста
    {
        file.write(ui->widget_arrive->toPlainText().toUtf8());           // все что в консоли, записывается в файл кодировка UTF8
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~После запуска программы, если нажимаешь на Меню->Настройка->Открыть файл настройки~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_action_OpenTXT_changed()
{
    qDebug()<<"thth";
    NameofFile.clear();                                                 // очищается переменная содержащая строку пути к файлу
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Информация о сворачивании в трей~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_action_triggered(bool trey)
{
    if(trey)
    {
        showStat("При закрытии программа будет сворачиваться в трей");                        // информация выводится в статус

    }
    else
    {
        showStat("Функция свертки в трей отключена");
    }

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Кнопка CTRL+C~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_pushButton_Copy_clicked()
{
    QClipboard* copy = QApplication::clipboard();                                       // Создаем буфер обмена
    copy->setText(ui->lineEdit_fileName->text(),QClipboard::Clipboard);                 // помещаем данные в буфер обмена
    ui->lineEdit_fileName->selectAll();                                                 // выделяй весь путь до файла в lineEdit
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~В меню -> Настройки -> Cкрыть консоль~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MainWindow::on_action_Invize_triggered(bool vizible)
{
     if(vizible)                                                        // если нажмешь, что консоли видимы
     {
         ui->groupBox_consoll->setVisible(true);                        // консоль видима
         this->setFixedSize(755,842);                                   // устанавливает большой размер
     }else
     {
         ui->groupBox_consoll->setVisible(false);                       // невидимая
         this->setFixedSize(755,475);                                   // малый размер
     }

}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------




//void MainWindow::on_action_darkWindow_triggered(bool isDark)
//{

//   if(isDark)
//   {
//     QPalette darkPalette;


//    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));               // Настраиваем палитру для цветовых ролей элементов интерфейса
//    darkPalette.setColor(QPalette::WindowText, Qt::white);
//    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
//    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
//    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
//    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
//    darkPalette.setColor(QPalette::Text, Qt::white);
//    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
//    darkPalette.setColor(QPalette::ButtonText, Qt::white);
//    darkPalette.setColor(QPalette::BrightText, Qt::red);
//    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
//    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
//    darkPalette.setColor(QPalette::HighlightedText, Qt::black);


//        qApp->setPalette(darkPalette);                                           // Устанавливаем данную палитру
//    }
//    else
//    {



//    }
//}

//void MainWindow::on_action_darkWindow_triggered()
//{

//}
