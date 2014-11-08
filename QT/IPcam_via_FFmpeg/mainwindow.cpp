#include "mainwindow.h"
#include "ui_mainwindow.h"
/* -------------------------------------------------------- */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->Video_widget->set_reflashtime(30);
}
/* -------------------------------------------------------- */
MainWindow::~MainWindow()
{
    delete ui;
}
/* -------------------------------------------------------- */
GLwidget *MainWindow::get_Viewer()
{
    return ui->Video_widget;
}
/* -------------------------------------------------------- */
