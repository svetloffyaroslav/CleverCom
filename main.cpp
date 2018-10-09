
//+++++++++++++++++++++++++++++++++++++++++++++++++++main.сpp+++++++++++++++++++++++++++++++++++++++++++++++++++
//+++ Описывает запуск программы и появление загрузочной заставки                                            +++
//+++                                                                                                        +++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//-----------------------------------------------------------------------------------------------------Светлов Я
//----------------------------------------------------------------------------------svetloff.yarolsav@gmail.com


#include "mainwindow.h"
#include <QApplication>
#include "qthread.h"             //подключние библиотеки создание потоков
#include <QSplashScreen>         //Вывод заставки



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Создание потока~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class I: public QThread
{
public:
    static void sleep(unsigned long secs){QThread::sleep(secs);}               //sleep -  вынуждает текущий поток спать в течение (sec) секунд
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Запуск программы~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;


    //QSplashScreen *splash= new QSplashScreen;                                 // Объявление заставки
    //splash->setPixmap(QPixmap(":/Images/splash.png"));                           // Выбор картинки для заставки
    //splash->show();                                                           //Заставка - видима
    //I::sleep(2);                                                              //опраделение времени, сколько она задержится на экране монитора
    w.show();
   //splash->finish(&w);                                                        // Ждет пока не появится главный экран
   // delete splash;                                                             //Удаляет заставку
    return a.exec();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
