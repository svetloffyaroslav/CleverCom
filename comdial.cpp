//+++++++++++++++++++++++Окно подключения ком-порта(с настройками).cpp++++++++++++++++++++
//+++ Вызывается из menu->Com-порт или по нажатию клавиш Ctrl+S                        +++
//+++ Содержит 2 кнопки и  5 combobox                                                  +++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//------------------------------------------------------------------------------Светлов Я.
//-------------------------------------------------------------svetloff.yaroslav@gmail.com


#include "comdial.h"
#include "ui_comdial.h"

comdial::comdial(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::comdial)
{
    ui->setupUi(this);
    setFixedSize(274,137);

     foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())         //ищет все порты ком-порты
     {
             ui->PortNameBox->addItem(info.portName());                                   //заносит в combobox списком
     }

       //Создание списка в комбобокс  - BaudRateBox - задание скорости обмена
        ui->BaudRateBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
        ui->BaudRateBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
        ui->BaudRateBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
        ui->BaudRateBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
        ui->BaudRateBox->setCurrentIndex(1);                                               //выставлено по умлочанию при открытии (число в скобке - номер в списке)

        //Создание списка в комбобокс  - DataBitsBox - задание длины слова
        ui->DataBitsBox->addItem(QStringLiteral("5"), QSerialPort::Data5);
        ui->DataBitsBox->addItem(QStringLiteral("6"), QSerialPort::Data6);
        ui->DataBitsBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
        ui->DataBitsBox->addItem(QStringLiteral("8"), QSerialPort::Data8);
        ui->DataBitsBox->setCurrentIndex(3);

       //Создание списка в комбобокс  - StopBitsBox - задание стоповых биты
        ui->StopBitsBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
        ui->StopBitsBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);
        ui->StopBitsBox->setCurrentIndex(0);

     //Создание списка в комбобокс  - ParityBox - задание паритета
        ui->ParityBox->addItem(QStringLiteral("None"), QSerialPort::NoParity);
        ui->ParityBox->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
        ui->ParityBox->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
        ui->ParityBox->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);
        ui->ParityBox->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);
        ui->ParityBox->setCurrentIndex(4);




}

comdial::~comdial()
{
    delete ui;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Обработка нажатия кнопки OK~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void comdial::on_buttonBox_accepted()
{
    emit savesettings(ui->PortNameBox->currentText(),
                      ui->BaudRateBox-> itemData(ui->BaudRateBox->currentIndex()).toInt(),
                      ui->DataBitsBox-> itemData(ui->DataBitsBox->currentIndex()).toInt(),       //принимает все действующие значение во всех комбобокс
                      ui->ParityBox  -> itemData(ui->ParityBox->currentIndex()).toInt(),
                      ui->StopBitsBox-> itemData(ui->StopBitsBox->currentIndex()).toInt());


    close();//после "ОК" - закрывает окно
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
