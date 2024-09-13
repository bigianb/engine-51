#include "mainwindow.h"
#include "./ui_mainwindow.h"





MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    dfsTreeModel = new DfsTreeModel();
    ui->treeView->setModel(dfsTreeModel);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dfsTreeModel;
}
