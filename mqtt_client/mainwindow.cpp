#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::~MainWindow()
{
    delete ui;
}

MainWindow::MainWindow(QWidget *parent) :QMainWindow(parent),ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

void MainWindow::MainWinIinit(void)
{
    this->resize(1200,800);
    this->setWindowTitle("mqtt client v1.0");
    this->setFixedSize(1200,800);

    //分页类
    TabWidget.setParent(this);
    TabWidget.move(5, 5);
    TabWidget.resize(1185, 785);
    TabWidget.setTabPosition(QTabWidget::North);
    TabWidget.setTabShape(QTabWidget::Rounded);

    //通用窗口
    CommonWin.setParent(this);
    TabWidget.addTab(&CommonWin,"通用功能");   
}
