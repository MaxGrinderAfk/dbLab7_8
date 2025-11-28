#ifndef QUERYMANAGEMENTWINDOW_H
#define QUERYMANAGEMENTWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QCheckBox>
#include <QVector>

struct QueryInfo {
    QString description;
    QString sqlScript;
};

class QueryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QueryWidget(const QueryInfo &query, QWidget *parent = nullptr);
    QString getDescription() const;
    QString getSqlScript() const { return query.sqlScript; }
    void setDescription(const QString &desc);
    bool isSelected() const;
    void setSelected(bool selected);

signals:
    void executeRequested(const QString &sql);
    void descriptionChanged(const QString &oldDesc, const QString &newDesc);

private slots:
    void onDescriptionEdited();

private:
    QueryInfo query;
    QString originalDescription;
    QCheckBox *selectCheckBox;
    QLineEdit *descriptionEdit;
    QPushButton *executeButton;
};

class QueryManagementWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QueryManagementWindow(QWidget *parent = nullptr);
    ~QueryManagementWindow();

private slots:
    void onCreateQuery();
    void onDeleteQuery();
    void onImportQueries();
    void onSaveQueries();
    void onExecuteQuery(const QString &sql);
    void onQueryDescriptionChanged(const QString &oldDesc, const QString &newDesc);

private:
    void setupUI();
    void loadDefaultQueries();
    void refreshQueriesList();
    QList<QueryWidget*> getSelectedQueries();

    QPushButton *createQueryButton;
    QPushButton *deleteQueryButton;
    QPushButton *importQueriesButton;
    QPushButton *saveQueriesButton;

    QScrollArea *scrollArea;
    QWidget *queriesContainer;
    QVBoxLayout *queriesLayout;

    QVector<QueryInfo> queries;
    QVector<QueryWidget*> queryWidgets;
};

#endif
