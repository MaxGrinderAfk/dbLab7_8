#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onWorkWithTablesClicked();
    void onWorkWithQueriesClicked();

private:
    void setupUI();

    QPushButton *workWithTablesButton;
    QPushButton *workWithQueriesButton;
};

#endif
