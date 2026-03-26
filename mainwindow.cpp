#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mapwidget.h"
#include "mapmarker.h"

#include <QDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(1024, 768);
    setWindowTitle("QT Map Application");

    // 创建地图组件
    mapwidget *mapWidget = new mapwidget(this);
    setCentralWidget(mapWidget);

    // 初始化菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *exitAction = fileMenu->addAction("Exit");

    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    setMenuBar(menuBar);
}

MainWindow::~MainWindow()
{
    delete ui;
}
