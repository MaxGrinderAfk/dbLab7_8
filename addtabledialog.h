#ifndef ADDTABLEDIALOG_H
#define ADDTABLEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVector>
#include "databasemanager.h"

class ColumnInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColumnInputWidget(QWidget *parent = nullptr);

    QString getColumnName() const;
    QString getColumnType() const;
    bool isPrimaryKey() const;

private:
    QLineEdit *nameEdit;
    QComboBox *typeCombo;
    QComboBox *primaryKeyCombo;
};

class AddTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTableDialog(QWidget *parent = nullptr);

private slots:
    void onAddColumn();
    void onRemoveColumn();
    void onConfirm();

private:
    void setupUI();

    QLineEdit *tableNameEdit;
    QScrollArea *scrollArea;
    QWidget *columnsContainer;
    QVBoxLayout *columnsLayout;
    QPushButton *addColumnButton;
    QPushButton *removeColumnButton;
    QPushButton *confirmButton;

    QVector<ColumnInputWidget*> columnWidgets;
};

#endif
