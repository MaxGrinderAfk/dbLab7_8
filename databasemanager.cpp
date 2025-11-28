#include "databasemanager.h"
#include <QSqlRecord>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QDebug>

DatabaseManager::DatabaseManager()
    : schemaName("libraryschema")
{
}

DatabaseManager::~DatabaseManager()
{
    disconnectFromDatabase();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::connectToDatabase()
{
    if (db.isOpen()) {
        return true;
    }

    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("library");
    db.setUserName("roflan");
    db.setPassword("Begemot12345");

    if (!db.open()) {
        qDebug() << "Database connection error:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.exec(QString("SET search_path TO %1").arg(schemaName));

    return true;
}

void DatabaseManager::disconnectFromDatabase()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool DatabaseManager::isConnected() const
{
    return db.isOpen();
}

QStringList DatabaseManager::getTableNames()
{
    QStringList tables;

    QSqlQuery query(db);
    QString queryStr = QString(
                           "SELECT table_name FROM information_schema.tables "
                           "WHERE table_schema = '%1' AND table_type = 'BASE TABLE' "
                           "ORDER BY table_name"
                           ).arg(schemaName);

    if (query.exec(queryStr)) {
        while (query.next()) {
            tables.append(query.value(0).toString());
        }
    }

    return tables;
}

QSqlQuery DatabaseManager::executeQuery(const QString &queryStr, bool *ok, QString *error)
{
    QSqlQuery query(db);
    bool success = query.exec(queryStr);

    if (ok) {
        *ok = success;
    }

    if (!success && error) {
        *error = query.lastError().text();
    }

    return query;
}

int DatabaseManager::executeNonQuery(const QString &queryStr, QString *error)
{
    QSqlQuery query(db);

    if (!query.exec(queryStr)) {
        if (error) {
            *error = query.lastError().text();
        }
        return -1;
    }

    return query.numRowsAffected();
}

QList<DatabaseManager::ColumnInfo> DatabaseManager::getTableColumns(const QString &tableName)
{
    QList<ColumnInfo> columns;

    QSqlQuery query(db);
    QString queryStr = QString(
                           "SELECT "
                           "    c.column_name, "
                           "    c.data_type, "
                           "    c.is_nullable, "
                           "    CASE WHEN pk.column_name IS NOT NULL THEN true ELSE false END AS is_primary, "
                           "    CASE "
                           "        WHEN c.column_default LIKE 'nextval%%' THEN true "
                           "        WHEN c.is_identity = 'YES' THEN true "
                           "        ELSE false "
                           "    END AS is_identity "
                           "FROM information_schema.columns c "
                           "LEFT JOIN ( "
                           "    SELECT ku.column_name "
                           "    FROM information_schema.table_constraints tc "
                           "    JOIN information_schema.key_column_usage ku "
                           "        ON tc.constraint_name = ku.constraint_name "
                           "        AND tc.table_schema = ku.table_schema "
                           "    WHERE tc.constraint_type = 'PRIMARY KEY' "
                           "        AND tc.table_schema = '%1' "
                           "        AND tc.table_name = '%2' "
                           ") pk ON c.column_name = pk.column_name "
                           "WHERE c.table_schema = '%1' AND c.table_name = '%2' "
                           "ORDER BY c.ordinal_position"
                           ).arg(schemaName, tableName);

    if (query.exec(queryStr)) {
        while (query.next()) {
            ColumnInfo col;
            col.name = query.value(0).toString();
            QString dataType = query.value(1).toString();
            col.type = dataType.contains("int") ? "int" : "text";
            col.isNullable = query.value(2).toString() == "YES";
            col.isPrimaryKey = query.value(3).toBool();
            col.isIdentity = query.value(4).toBool();
            columns.append(col);
        }
    }

    return columns;
}

QList<QVariantList> DatabaseManager::getTableData(const QString &tableName)
{
    QList<QVariantList> data;

    auto columns = getTableColumns(tableName);
    QStringList columnNames;
    for (const auto &col : columns) {
        columnNames.append(col.name);
    }

    QSqlQuery query(db);
    QString queryStr = QString("SELECT %1 FROM %2").arg(columnNames.join(", "), tableName);

    if (query.exec(queryStr)) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < columns.size(); ++i) {
                row.append(query.value(i));
            }
            data.append(row);
        }
    }

    return data;
}

bool DatabaseManager::createTable(const QString &tableName, const QList<ColumnInfo> &columns, QString *error)
{
    if (columns.isEmpty()) {
        if (error) *error = "No columns specified";
        return false;
    }

    QStringList columnDefs;
    QStringList primaryKeys;

    for (const auto &col : columns) {
        QString colDef;
        if (col.isIdentity) {
            colDef = QString("%1 BIGSERIAL").arg(col.name);
        } else {
            colDef = QString("%1 %2").arg(col.name, col.type.toUpper() == "INT" ? "BIGINT" : "TEXT");
        }
        columnDefs.append(colDef);

        if (col.isPrimaryKey) {
            primaryKeys.append(col.name);
        }
    }

    QString queryStr = QString("CREATE TABLE %1 (%2").arg(tableName, columnDefs.join(", "));

    if (!primaryKeys.isEmpty()) {
        queryStr += QString(", PRIMARY KEY (%1)").arg(primaryKeys.join(", "));
    }

    queryStr += ")";

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::dropTable(const QString &tableName, QString *error)
{
    QSqlQuery query(db);
    if (!query.exec(QString("DROP TABLE IF EXISTS %1 CASCADE").arg(tableName))) {
        if (error) *error = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::insertRow(const QString &tableName, const QVariantList &values, QString *error)
{
    auto columns = getTableColumns(tableName);

    if (values.size() != columns.size()) {
        if (error) *error = "Values count doesn't match columns count";
        return false;
    }

    QStringList columnNames;
    QStringList placeholders;
    QVariantList bindValues;

    for (int i = 0; i < columns.size(); ++i) {
        // Пропускаем identity (автоинкремент) — их не вставляем вручную
        if (columns[i].isIdentity) continue;

        columnNames.append(columns[i].name);
        placeholders.append("?");

        // Если значение пустое / null — положим явный NULL (QVariant())
        const QVariant &val = values[i];
        if (!val.isValid() || val.isNull() || (val.type() == QVariant::String && val.toString().trimmed().isEmpty())) {
            bindValues.append(QVariant(QVariant::String)); // явный пустой QVariant, будет интерпретирован как NULL
            bindValues.last() = QVariant(); // гарантируем NULL
        } else {
            bindValues.append(val);
        }
    }

    QString queryStr;
    if (columnNames.isEmpty()) {
        queryStr = QString("INSERT INTO %1 DEFAULT VALUES").arg(tableName);
    } else {
        queryStr = QString("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(tableName, columnNames.join(", "), placeholders.join(", "));
    }

    QSqlQuery query(db);
    query.prepare(queryStr);

    for (const auto &val : bindValues) query.addBindValue(val);

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::syncSequence(const QString &tableName, QString *error)
{
    auto columns = getTableColumns(tableName);

    for (const auto &col : columns) {
        if (col.isIdentity) {

            QString seqName = QString("%1_%2_seq").arg(tableName, col.name);

            QString queryStr = QString(
                                   "SELECT setval('%1', "
                                   "   COALESCE((SELECT MAX(%2) FROM %3), 1), "
                                   "   false)"
                                   ).arg(seqName, col.name, tableName);

            QSqlQuery query(db);
            if (!query.exec(queryStr)) {
                if (error) *error = query.lastError().text();
                return false;
            }
        }
    }

    return true;
}

bool DatabaseManager::deleteRow(const QString &tableName, const QVariantList &primaryKeyValues, QString *error)
{
    auto columns = getTableColumns(tableName);
    QStringList pkColumns;

    for (const auto &col : columns) {
        if (col.isPrimaryKey) {
            pkColumns.append(col.name);
        }
    }

    if (pkColumns.isEmpty()) {
        if (error) *error = "Table has no primary key";
        return false;
    }

    if (primaryKeyValues.size() != pkColumns.size()) {
        if (error) *error = "Invalid primary key specification";
        return false;
    }

    QStringList conditions;
    for (const auto &pkCol : pkColumns) {
        conditions.append(QString("%1 = ?").arg(pkCol));
    }

    QString queryStr = QString("DELETE FROM %1 WHERE %2")
                           .arg(tableName, conditions.join(" AND "));

    QSqlQuery query(db);
    query.prepare(queryStr);

    for (const auto &value : primaryKeyValues) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::updateCell(const QString &tableName,
                                 const QString &columnName,
                                 const QVariant &value,
                                 const QVariantList &primaryKeyValues,
                                 QString *error)
{
    auto columns = getTableColumns(tableName);
    QList<ColumnInfo> pkColumns;
    for (const auto &col : columns) {
        if (col.isPrimaryKey) pkColumns.append(col);
    }

    if (pkColumns.size() != primaryKeyValues.size()) {
        if (error) *error = "Primary key values count mismatch";
        return false;
    }

    QStringList conditions;
    for (const auto &col : pkColumns) {
        conditions.append(col.name + " = ?");
    }

    QString queryStr = QString("UPDATE %1 SET %2 = ? WHERE %3")
                           .arg(tableName, columnName, conditions.join(" AND "));

    QSqlQuery query(db);
    query.prepare(queryStr);

    query.addBindValue(value);
    for (const auto &val : primaryKeyValues) query.addBindValue(val);

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}


bool DatabaseManager::addColumn(const QString &tableName, const ColumnInfo &column, QString *error)
{
    QString dataType;
    if (column.isIdentity) {
        dataType = "BIGSERIAL";
    } else {
        dataType = column.type.toUpper() == "INT" ? "BIGINT" : "TEXT";
    }

    QString queryStr = QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                           .arg(tableName, column.name, dataType);

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::dropColumn(const QString &tableName, const QString &columnName, QString *error)
{
    QString queryStr = QString("ALTER TABLE %1 DROP COLUMN %2").arg(tableName, columnName);

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

QSqlDatabase& DatabaseManager::getDatabase()
{
    return db;
}

bool DatabaseManager::renameColumn(const QString &tableName, const QString &oldName, const QString &newName, QString *error)
{
    QString queryStr = QString("ALTER TABLE %1 RENAME COLUMN %2 TO %3")
    .arg(tableName, oldName, newName);

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        if (error) *error = query.lastError().text();
        return false;
    }

    return true;
}

QJsonObject DatabaseManager::exportTableToJson(const QString &tableName)
{
    QJsonObject tableObj;
    tableObj["name"] = tableName;

    auto columns = getTableColumns(tableName);
    QJsonArray columnsArray;
    for (const auto &col : columns) {
        QJsonObject colObj;
        colObj["name"] = col.name;
        colObj["type"] = col.type;
        colObj["isPrimaryKey"] = col.isPrimaryKey;
        colObj["isIdentity"] = col.isIdentity;
        columnsArray.append(colObj);
    }
    tableObj["columns"] = columnsArray;

    auto data = getTableData(tableName);
    QJsonArray dataArray;
    for (const auto &row : data) {
        QJsonArray rowArray;
        for (const auto &value : row) {
            rowArray.append(QJsonValue::fromVariant(value));
        }
        dataArray.append(rowArray);
    }
    tableObj["data"] = dataArray;

    return tableObj;
}

bool DatabaseManager::importTableFromJson(const QJsonObject &json, QString *error)
{
    QString tableName = json["name"].toString();

    if (getTableNames().contains(tableName)) {
        if (!dropTable(tableName, error)) {
            return false;
        }
    }

    QJsonArray columnsArray = json["columns"].toArray();
    QList<ColumnInfo> columns;

    for (const auto &colValue : columnsArray) {
        QJsonObject colObj = colValue.toObject();
        ColumnInfo col;
        col.name = colObj["name"].toString();
        col.type = colObj["type"].toString();
        col.isPrimaryKey = colObj["isPrimaryKey"].toBool();
        col.isIdentity = colObj["isIdentity"].toBool(false);
        columns.append(col);
    }

    if (!createTable(tableName, columns, error)) {
        return false;
    }

    QJsonArray dataArray = json["data"].toArray();
    for (const auto &rowValue : dataArray) {
        QJsonArray rowArray = rowValue.toArray();
        QVariantList row;

        for (const auto &cellValue : rowArray) {
            row.append(cellValue.toVariant());
        }

        if (!insertRow(tableName, row, error)) {
            return false;
        }
    }

    if (!syncSequence(tableName, error)) {
        return false;
    }

    return true;
}

QJsonArray DatabaseManager::exportDatabaseToJson()
{
    QJsonArray dbArray;

    QStringList tables = getTableNames();
    for (const auto &tableName : tables) {
        dbArray.append(exportTableToJson(tableName));
    }

    return dbArray;
}

bool DatabaseManager::importDatabaseFromJson(const QJsonArray &json, QString *error)
{
    for (const auto &tableValue : json) {
        QJsonObject tableObj = tableValue.toObject();
        if (!importTableFromJson(tableObj, error)) {
            return false;
        }
    }
    return true;
}

bool DatabaseManager::exportTableToCsv(const QString &tableName, const QString &filePath, QString *error)
{
    auto columns = getTableColumns(tableName);
    auto data = getTableData(tableName);

    QStringList headers;
    for (const auto &col : columns) {
        headers.append(col.name);
    }

    return exportQueryResultToCsv(data, headers, filePath, error);
}

bool DatabaseManager::exportDatabaseToSql(const QString &filePath, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = "Cannot open file for writing";
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "-- Library Database Backup\n";
    stream << "-- Generated by LibraryDB Application\n\n";

    QStringList tables = getTableNames();

    for (const QString &tableName : tables) {
        auto columns = getTableColumns(tableName);

        stream << "-- Table: " << tableName << "\n";
        stream << "DROP TABLE IF EXISTS " << tableName << " CASCADE;\n";
        stream << "CREATE TABLE " << tableName << " (\n";

        QStringList columnDefs;
        QStringList primaryKeys;

        for (const auto &col : columns) {
            QString colDef = "    " + col.name + " ";
            if (col.isIdentity) {
                colDef += "BIGSERIAL";
            } else {
                colDef += (col.type.toUpper() == "INT" ? "BIGINT" : "TEXT");
            }
            columnDefs.append(colDef);

            if (col.isPrimaryKey) {
                primaryKeys.append(col.name);
            }
        }

        stream << columnDefs.join(",\n");

        if (!primaryKeys.isEmpty()) {
            stream << ",\n    PRIMARY KEY (" << primaryKeys.join(", ") << ")";
        }

        stream << "\n);\n\n";

        auto data = getTableData(tableName);
        if (!data.isEmpty()) {
            for (const auto &row : data) {
                QStringList columnNames;
                QStringList values;

                for (int i = 0; i < row.size(); ++i) {
                    if (!columns[i].isIdentity) {
                        columnNames.append(columns[i].name);

                        if (row[i].isNull()) {
                            values.append("NULL");
                        } else {
                            QString val = row[i].toString();
                            val.replace("'", "''");

                            if (columns[i].type.toUpper() == "INT") {
                                values.append(val);
                            } else {
                                values.append("'" + val + "'");
                            }
                        }
                    }
                }

                if (!columnNames.isEmpty()) {
                    stream << "INSERT INTO " << tableName << " (" << columnNames.join(", ")
                    << ") VALUES (" << values.join(", ") << ");\n";
                } else {
                    stream << "INSERT INTO " << tableName << " DEFAULT VALUES;\n";
                }
            }
            stream << "\n";
        }
    }

    file.close();
    return true;
}

bool DatabaseManager::exportQueryResultToCsv(const QList<QVariantList> &data,
                                             const QStringList &headers,
                                             const QString &filePath,
                                             QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = "Cannot open file for writing";
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    stream << headers.join(",") << "\n";

    for (const auto &row : data) {
        QStringList rowStrings;
        for (const auto &cell : row) {
            QString cellStr = cell.toString();
            if (cellStr.contains(',') || cellStr.contains('"') || cellStr.contains('\n')) {
                cellStr.replace("\"", "\"\"");
                cellStr = "\"" + cellStr + "\"";
            }
            rowStrings.append(cellStr);
        }
        stream << rowStrings.join(",") << "\n";
    }

    file.close();
    return true;
}
