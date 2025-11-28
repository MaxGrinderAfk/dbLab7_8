#include "mainwindow.h"
#include "tablemanagementwindow.h"
#include "querymanagementwindow.h"
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("База данных Библиотека");
    setMinimumSize(600, 300);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(50, 50, 50, 50);

    QLabel *titleLabel = new QLabel("Операции с БД", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    mainLayout->addStretch();

    workWithTablesButton = new QPushButton("Работа с таблицами", this);
    workWithTablesButton->setMinimumHeight(50);
    workWithTablesButton->setStyleSheet("QPushButton { font-size: 14px; }");
    connect(workWithTablesButton, &QPushButton::clicked, this, &MainWindow::onWorkWithTablesClicked);
    mainLayout->addWidget(workWithTablesButton);

    workWithQueriesButton = new QPushButton("Работа с запросами", this);
    workWithQueriesButton->setMinimumHeight(50);
    workWithQueriesButton->setStyleSheet("QPushButton { font-size: 14px; }");
    connect(workWithQueriesButton, &QPushButton::clicked, this, &MainWindow::onWorkWithQueriesClicked);
    mainLayout->addWidget(workWithQueriesButton);

    mainLayout->addStretch();
}

void MainWindow::onWorkWithTablesClicked()
{
    TableManagementWindow *tableWindow = new TableManagementWindow(this);
    tableWindow->setAttribute(Qt::WA_DeleteOnClose);
    tableWindow->show();
}

void MainWindow::onWorkWithQueriesClicked()
{
    QueryManagementWindow *queryWindow = new QueryManagementWindow(this);
    queryWindow->setAttribute(Qt::WA_DeleteOnClose);
    queryWindow->show();
}
