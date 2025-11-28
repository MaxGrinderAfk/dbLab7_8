#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QJsonObject>
#include <QJsonArray>

class DatabaseManager
{
public:
    static DatabaseManager& instance();
    bool connectToDatabase();
    void disconnectFromDatabase();
    bool isConnected() const;
    QStringList getTableNames();
    QSqlQuery executeQuery(const QString &queryStr, bool *ok = nullptr, QString *error = nullptr);
    int executeNonQuery(const QString &queryStr, QString *error = nullptr);

    struct ColumnInfo {
        QString name;
        QString type;
        bool isPrimaryKey;
        bool isIdentity;
        bool isNullable;
    };

    QSqlDatabase& getDatabase();
    QList<ColumnInfo> getTableColumns(const QString &tableName);
    QList<QVariantList> getTableData(const QString &tableName);
    bool createTable(const QString &tableName, const QList<ColumnInfo> &columns, QString *error = nullptr);
    bool dropTable(const QString &tableName, QString *error = nullptr);
    bool insertRow(const QString &tableName, const QVariantList &values, QString *error = nullptr);
    bool deleteRow(const QString &tableName, const QVariantList &primaryKeyValues, QString *error = nullptr);
    bool updateCell(const QString &tableName,
                                     const QString &columnName,
                                     const QVariant &value,
                                     const QVariantList &primaryKeyValues,
                                     QString *error);
    bool addColumn(const QString &tableName, const ColumnInfo &column, QString *error = nullptr);
    bool dropColumn(const QString &tableName, const QString &columnName, QString *error = nullptr);
    bool renameColumn(const QString &tableName, const QString &oldName, const QString &newName, QString *error = nullptr);
    QJsonObject exportTableToJson(const QString &tableName);
    bool importTableFromJson(const QJsonObject &json, QString *error = nullptr);
    QJsonArray exportDatabaseToJson();
    bool importDatabaseFromJson(const QJsonArray &json, QString *error = nullptr);
    bool exportTableToCsv(const QString &tableName, const QString &filePath, QString *error = nullptr);
    bool exportDatabaseToSql(const QString &filePath, QString *error = nullptr);
    bool exportQueryResultToCsv(const QList<QVariantList> &data, const QStringList &headers,
                                const QString &filePath, QString *error = nullptr);
    bool syncSequence(const QString &tableName, QString *error = nullptr);

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    QSqlDatabase db;
    QString schemaName;
};

#endif
