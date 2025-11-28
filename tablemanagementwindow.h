#ifndef TABLEMANAGEMENTWINDOW_H
#define TABLEMANAGEMENTWINDOW_H

#include "databasemanager.h"
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QCheckBox>
#include <QMap>

class CollapsibleTableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CollapsibleTableWidget(const QString &tableName, QWidget *parent = nullptr);
    QString getTableName() const { return tableName; }
    bool isSelected() const;
    void setSelected(bool selected);

signals:
    void needsRefresh();

private slots:
    void toggleCollapse();
    void onAddRow();
    void onAddColumn();
    void onDeleteRow();
    void onDeleteColumn();
    void onSaveTableState();
    void onCellChanged(int row, int column);
    void onHeaderDoubleClicked(int index);
    void onBeforeEdit(int row, int column);

private:
    void loadTableData();
    void setupUI();
    QStringList getPrimaryKeyColumns();
    QVariantList getRowPrimaryKeyValues(int row);

    QString tableName;
    QCheckBox *selectCheckBox;
    QPushButton *headerButton;
    QWidget *contentWidget;
    QTableWidget *tableWidget;
    QPushButton *addRowButton;
    QPushButton *addColumnButton;
    QPushButton *deleteRowButton;
    QPushButton *deleteColumnButton;
    QPushButton *saveStateButton;
    bool isCollapsed;
    QList<DatabaseManager::ColumnInfo> columns;
    QMap<int, QVariantList> oldPkByRow;
};

class TableManagementWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TableManagementWindow(QWidget *parent = nullptr);
    ~TableManagementWindow();

private slots:
    void onDeleteTable();
    void onAddTable();
    void onSaveDatabase();
    void onRestoreDatabase();
    void onRestoreTable();
    void refreshTablesList();
private:
    void setupUI();
    void loadTables();
    QList<CollapsibleTableWidget*> getSelectedTables();

    QScrollArea *scrollArea;
    QWidget *tablesContainer;
    QVBoxLayout *tablesLayout;
    QPushButton *deleteTableButton;
    QPushButton *addTableButton;
    QPushButton *saveDatabaseButton;
    QPushButton *restoreDatabaseButton;
    QPushButton *restoreTableButton;

    QList<CollapsibleTableWidget*> tableWidgets;
};

#endif
