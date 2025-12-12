#include "tablemanagementwindow.h"
#include "addtabledialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QHeaderView>
#include <QInputDialog>

CollapsibleTableWidget::CollapsibleTableWidget(const QString &tableName, QWidget *parent)
    : QWidget(parent), tableName(tableName), isCollapsed(true)
{
    setupUI();
}

void CollapsibleTableWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QHBoxLayout *headerLayout = new QHBoxLayout();

    selectCheckBox = new QCheckBox(this);
    headerLayout->addWidget(selectCheckBox);

    headerButton = new QPushButton(tableName + " ▼", this);
    headerButton->setMinimumHeight(40);
    headerButton->setStyleSheet("QPushButton { text-align: left; padding-left: 10px; font-size: 13px; }");
    connect(headerButton, &QPushButton::clicked, this, &CollapsibleTableWidget::toggleCollapse);
    headerLayout->addWidget(headerButton, 1);

    mainLayout->addLayout(headerLayout);

    contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    addRowButton = new QPushButton("Добавить строку", this);
    addColumnButton = new QPushButton("Добавить столбец", this);
    deleteRowButton = new QPushButton("Удалить строку", this);
    deleteColumnButton = new QPushButton("Удалить столбец", this);

    connect(addRowButton, &QPushButton::clicked, this, &CollapsibleTableWidget::onAddRow);
    connect(addColumnButton, &QPushButton::clicked, this, &CollapsibleTableWidget::onAddColumn);
    connect(deleteRowButton, &QPushButton::clicked, this, &CollapsibleTableWidget::onDeleteRow);
    connect(deleteColumnButton, &QPushButton::clicked, this, &CollapsibleTableWidget::onDeleteColumn);

    buttonsLayout->addWidget(addRowButton);
    buttonsLayout->addWidget(addColumnButton);
    buttonsLayout->addWidget(deleteRowButton);
    buttonsLayout->addWidget(deleteColumnButton);
    buttonsLayout->addStretch();

    contentLayout->addLayout(buttonsLayout);

    tableWidget = new QTableWidget(this);
    tableWidget->setMinimumHeight(300);
    connect(tableWidget, &QTableWidget::cellChanged, this, &CollapsibleTableWidget::onCellChanged);
    connect(tableWidget, &QTableWidget::cellClicked, this, &CollapsibleTableWidget::onBeforeEdit);
    connect(tableWidget->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &CollapsibleTableWidget::onHeaderDoubleClicked);
    contentLayout->addWidget(tableWidget);

    saveStateButton = new QPushButton("Сохранить состояние таблицы", this);
    connect(saveStateButton, &QPushButton::clicked, this, &CollapsibleTableWidget::onSaveTableState);
    contentLayout->addWidget(saveStateButton);

    mainLayout->addWidget(contentWidget);
    contentWidget->hide();
}

bool CollapsibleTableWidget::isSelected() const
{
    return selectCheckBox->isChecked();
}

void CollapsibleTableWidget::onBeforeEdit(int row, int /*column*/)
{
    if (row < 0) return;
    oldPkByRow[row] = getRowPrimaryKeyValues(row);
}

void CollapsibleTableWidget::setSelected(bool selected)
{
    selectCheckBox->setChecked(selected);
}

void CollapsibleTableWidget::toggleCollapse()
{
    isCollapsed = !isCollapsed;

    if (isCollapsed) {
        contentWidget->hide();
        headerButton->setText(tableName + " ▼");
    } else {
        loadTableData();
        contentWidget->show();
        headerButton->setText(tableName + " ▲");
    }
}

void CollapsibleTableWidget::loadTableData()
{
    tableWidget->blockSignals(true);

    columns = DatabaseManager::instance().getTableColumns(tableName);
    auto data = DatabaseManager::instance().getTableData(tableName);

    tableWidget->clear();
    tableWidget->setRowCount(data.size());
    tableWidget->setColumnCount(columns.size());

    QStringList headers;
    for (const auto &col : columns) {
        QString header = col.name;
        if (col.isIdentity) {
            header += " (AUTO)";
        }
        headers.append(header);
    }
    tableWidget->setHorizontalHeaderLabels(headers);

    for (int row = 0; row < data.size(); ++row) {
        const auto &rowData = data[row];
        for (int col = 0; col < rowData.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[col].toString());

            if (columns[col].isIdentity) {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setBackground(QColor(230, 230, 230));
                item->setForeground(QColor(80, 80, 80));
            }

            tableWidget->setItem(row, col, item);
        }
    }

    tableWidget->resizeColumnsToContents();
    tableWidget->blockSignals(false);
}

QStringList CollapsibleTableWidget::getPrimaryKeyColumns()
{
    QStringList pkColumns;
    for (const auto &col : columns) {
        if (col.isPrimaryKey) {
            pkColumns.append(col.name);
        }
    }
    return pkColumns;
}

QVariantList CollapsibleTableWidget::getRowPrimaryKeyValues(int row)
{
    QVariantList pkValues;
    QStringList pkColumns = getPrimaryKeyColumns();

    for (const QString &pkCol : pkColumns) {
        int colIndex = -1;
        for (int i = 0; i < columns.size(); ++i) {
            if (columns[i].name == pkCol) {
                colIndex = i;
                break;
            }
        }

        if (colIndex >= 0) {
            QTableWidgetItem *item = tableWidget->item(row, colIndex);
            if (item) {
                QString txt = item->text();
                if (txt.isEmpty()) {
                    pkValues.append(QVariant());
                } else {
                    pkValues.append(QVariant(txt));
                }
            } else {
                pkValues.append(QVariant());
            }
        }
    }

    return pkValues;
}

void CollapsibleTableWidget::onAddRow()
{
    QStringList pkCols = getPrimaryKeyColumns();

    QVariantList values;
    values.resize(columns.size());

    for (int i = 0; i < columns.size(); ++i) {
        if (columns[i].isIdentity) {
            values[i] = QVariant();
            continue;
        }

        if (columns[i].isPrimaryKey || !columns[i].isNullable) {
            bool ok = false;
            QString columnLabel = columns[i].name;

            if (columns[i].isPrimaryKey) {
                columnLabel += " (PRIMARY KEY)";
            } else {
                columnLabel += " (NOT NULL)";
            }

            QString prompt = QString("Введите значение для столбца \"%1\":").arg(columnLabel);
            QString input = QInputDialog::getText(
                this, "Значение столбца", prompt, QLineEdit::Normal, "", &ok
                );

            if (!ok) return;

            if (input.trimmed().isEmpty()) {
                if (columns[i].isPrimaryKey) {
                    QMessageBox::warning(this, "Ошибка", "PRIMARY KEY не может быть пустым");
                } else {
                    QMessageBox::warning(this, "Ошибка",
                                         QString("Столбец \"%1\" имеет ограничение NOT NULL и не может быть пустым")
                                             .arg(columns[i].name));
                }
                return;
            }

            values[i] = input;
        }
        else {
            values[i] = QVariant();
        }
    }

    QString error;
    if (DatabaseManager::instance().insertRow(tableName, values, &error)) {
        loadTableData();
        emit needsRefresh();
        QMessageBox::information(this, "Строка добавлена",
                                 "Строка добавлена. Отредактируйте остальные поля при необходимости.");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить строку: " + error);
    }
}

void CollapsibleTableWidget::onAddColumn()
{
    bool ok;
    QString colName = QInputDialog::getText(this, "Добавить столбец", "Имя столбца:", QLineEdit::Normal, "", &ok);

    if (!ok || colName.isEmpty()) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Параметры столбца");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QHBoxLayout *typeLayout = new QHBoxLayout();
    QLabel *typeLabel = new QLabel("Тип данных:", &dialog);
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("text");
    typeCombo->addItem("bigint");
    typeCombo->addItem("integer");
    typeCombo->addItem("date");
    typeCombo->addItem("boolean");
    typeCombo->addItem("numeric");
    typeCombo->addItem("varchar");
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(typeCombo);
    layout->addLayout(typeLayout);

    QCheckBox *identityCheck = new QCheckBox("Auto (SERIAL)", &dialog);
    layout->addWidget(identityCheck);

    QCheckBox *nullableCheck = new QCheckBox("Nullable", &dialog);
    nullableCheck->setChecked(true);
    layout->addWidget(nullableCheck);

    QCheckBox *textConstraintCheck = new QCheckBox("Только текст (латиница и кириллица)", &dialog);
    layout->addWidget(textConstraintCheck);

    QHBoxLayout *defaultLayout = new QHBoxLayout();
    QLabel *defaultLabel = new QLabel("Default:", &dialog);
    QLineEdit *defaultEdit = new QLineEdit(&dialog);
    defaultEdit->setPlaceholderText("NULL или значение");
    defaultLayout->addWidget(defaultLabel);
    defaultLayout->addWidget(defaultEdit);
    layout->addLayout(defaultLayout);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", &dialog);
    QPushButton *cancelButton = new QPushButton("Отмена", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);
    layout->addLayout(buttonsLayout);

    if (dialog.exec() != QDialog::Accepted) return;

    DatabaseManager::ColumnInfo col;
    col.name = colName;
    col.fullType = typeCombo->currentText();
    col.type = col.fullType.contains("int") ? "int" : "text";
    col.isIdentity = identityCheck->isChecked();
    col.isPrimaryKey = false;
    col.isNullable = nullableCheck->isChecked();
    col.defaultValue = defaultEdit->text().trimmed();

    QString error;
    if (DatabaseManager::instance().addColumn(tableName, col, &error)) {
        if (textConstraintCheck->isChecked()) {
            QString constraintName = QString("%1_%2_text_check").arg(tableName, colName);
            QString constraintQuery = QString(
                                          "ALTER TABLE %1 ADD CONSTRAINT %2 CHECK (%3 ~ '^[a-zA-Zа-яА-ЯёЁ]*$')"
                                          ).arg(tableName, constraintName, colName);

            QSqlQuery query(DatabaseManager::instance().getDatabase());
            if (!query.exec(constraintQuery)) {
                QMessageBox::warning(this, "Предупреждение",
                                     "Столбец добавлен, но не удалось добавить ограничение: " + query.lastError().text());
            }
        }

        loadTableData();
        emit needsRefresh();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить столбец: " + error);
    }
}

void CollapsibleTableWidget::onDeleteRow()
{
    int currentRow = tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите строку для удаления");
        return;
    }

    QVariantList pkValues = getRowPrimaryKeyValues(currentRow);

    QString error;
    if (DatabaseManager::instance().deleteRow(tableName, pkValues, &error)) {
        loadTableData();
        emit needsRefresh();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить строку: " + error);
    }
}

void CollapsibleTableWidget::onDeleteColumn()
{
    int currentCol = tableWidget->currentColumn();
    if (currentCol < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите столбец для удаления");
        return;
    }

    QString colName = columns[currentCol].name;

    QString error;
    if (DatabaseManager::instance().dropColumn(tableName, colName, &error)) {
        loadTableData();
        emit needsRefresh();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить столбец: " + error);
    }
}

void CollapsibleTableWidget::onHeaderDoubleClicked(int index)
{
    if (index < 0 || index >= columns.size()) return;

    QString oldName = columns[index].name;

    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать столбец");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("Имя столбца:", &dialog);
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(oldName);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    layout->addLayout(nameLayout);

    QHBoxLayout *typeLayout = new QHBoxLayout();
    QLabel *typeLabel = new QLabel("Тип данных:", &dialog);
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("text");
    typeCombo->addItem("bigint");
    typeCombo->addItem("integer");
    typeCombo->addItem("date");
    typeCombo->addItem("boolean");
    typeCombo->addItem("numeric");
    typeCombo->addItem("varchar");

    int currentIndex = typeCombo->findText(columns[index].fullType);
    if (currentIndex >= 0) {
        typeCombo->setCurrentIndex(currentIndex);
    }

    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(typeCombo);
    layout->addLayout(typeLayout);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", &dialog);
    QPushButton *cancelButton = new QPushButton("Отмена", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);
    layout->addLayout(buttonsLayout);

    if (dialog.exec() != QDialog::Accepted) return;

    QString newName = nameEdit->text().trimmed();
    QString newType = typeCombo->currentText();

    if (newName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Имя столбца не может быть пустым");
        return;
    }

    QString error;
    bool success = true;

    if (newName != oldName) {
        if (!DatabaseManager::instance().renameColumn(tableName, oldName, newName, &error)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось переименовать столбец: " + error);
            success = false;
        }
    }

    if (success && newType != columns[index].fullType) {
        QString columnToChange = (newName != oldName) ? newName : oldName;
        if (!DatabaseManager::instance().changeColumnType(tableName, columnToChange, newType, &error)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось изменить тип столбца: " + error);
            success = false;
        }
    }

    if (success) {
        loadTableData();
        emit needsRefresh();
        QMessageBox::information(this, "Успех", "Столбец успешно изменен");
    }
}

void CollapsibleTableWidget::onSaveTableState()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Сохранить таблицу", "", "JSON Files (*.json);;CSV Files (*.csv)");

    if (filePath.isEmpty()) return;

    if (filePath.endsWith(".csv")) {
        QString error;
        if (DatabaseManager::instance().exportTableToCsv(tableName, filePath, &error)) {
            QMessageBox::information(this, "Успех", "Таблица успешно экспортирована в CSV");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось экспортировать таблицу: " + error);
        }
    } else {
        QJsonObject tableJson = DatabaseManager::instance().exportTableToJson(tableName);

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(tableJson);
            file.write(doc.toJson());
            file.close();
            QMessageBox::information(this, "Успех", "Состояние таблицы сохранено");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл");
        }
    }
}

void CollapsibleTableWidget::onCellChanged(int row, int column)
{
    if (row < 0 || column < 0 || column >= columns.size()) return;

    if (columns[column].isIdentity) {
        loadTableData();
        return;
    }

    QTableWidgetItem *item = tableWidget->item(row, column);
    if (!item) return;

    QString newValue = item->text();
    QString columnName = columns[column].name;
    QStringList pkColumns = getPrimaryKeyColumns();

    QVariantList oldPkValues;
    if (oldPkByRow.contains(row)) {
        oldPkValues = oldPkByRow[row];
        oldPkByRow.remove(row);
    } else {
        oldPkValues = getRowPrimaryKeyValues(row);
    }

    bool isPkColumn = false;
    int pkIndex = -1;
    for (int i = 0; i < pkColumns.size(); ++i) {
        if (pkColumns[i] == columnName) {
            isPkColumn = true;
            pkIndex = i;
            break;
        }
    }

    QString error;
    if (!isPkColumn) {
        if (!DatabaseManager::instance().updateCell(tableName, columnName, newValue, oldPkValues, &error)) {
            QMessageBox::critical(this, "Ошибка", "Не удалось обновить ячейку: " + error);
            loadTableData();
        } else {
        }
    } else {
        QVariantList newRowValues;
        newRowValues.resize(columns.size());
        for (int c = 0; c < columns.size(); ++c) {
            QTableWidgetItem *it = tableWidget->item(row, c);
            if (it) {
                QString t = it->text();
                if (t.isEmpty()) newRowValues[c] = QVariant();
                else newRowValues[c] = QVariant(t);
            } else {
                newRowValues[c] = QVariant();
            }
        }

        QSqlQuery tx(DatabaseManager::instance().getDatabase());
        if (!tx.exec("BEGIN")) {
            QMessageBox::critical(this, "Ошибка транзакции", tx.lastError().text());
            loadTableData();
            return;
        }

        if (!DatabaseManager::instance().insertRow(tableName, newRowValues, &error)) {
            tx.exec("ROLLBACK");
            QMessageBox::critical(this, "Ошибка", "Не удалось вставить новую строку при изменении PK: " + error);
            loadTableData();
            return;
        }

        if (!DatabaseManager::instance().deleteRow(tableName, oldPkValues, &error)) {
            tx.exec("ROLLBACK");
            QMessageBox::critical(this, "Ошибка", "Не удалось удалить старую строку при изменении PK: " + error);
            loadTableData();
            return;
        }

        if (!tx.exec("COMMIT")) {
            QMessageBox::critical(this, "Ошибка транзакции", tx.lastError().text());
            loadTableData();
            return;
        }

        loadTableData();
    }
}


TableManagementWindow::TableManagementWindow(QWidget *parent)
    : QMainWindow(parent)
{
    if (!DatabaseManager::instance().connectToDatabase()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных");
    }

    setupUI();
    loadTables();
}

TableManagementWindow::~TableManagementWindow()
{
}

void TableManagementWindow::setupUI()
{
    setWindowTitle("Таблицы");
    setMinimumSize(900, 600);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *topButtonsLayout = new QHBoxLayout();

    deleteTableButton = new QPushButton("Удалить таблицу", this);
    addTableButton = new QPushButton("Добавить таблицу", this);
    saveDatabaseButton = new QPushButton("Сохранить состояние БД", this);
    restoreDatabaseButton = new QPushButton("Восстановить БД", this);
    restoreTableButton = new QPushButton("Восстановить таблицу", this);

    connect(deleteTableButton, &QPushButton::clicked, this, &TableManagementWindow::onDeleteTable);
    connect(addTableButton, &QPushButton::clicked, this, &TableManagementWindow::onAddTable);
    connect(saveDatabaseButton, &QPushButton::clicked, this, &TableManagementWindow::onSaveDatabase);
    connect(restoreDatabaseButton, &QPushButton::clicked, this, &TableManagementWindow::onRestoreDatabase);
    connect(restoreTableButton, &QPushButton::clicked, this, &TableManagementWindow::onRestoreTable);

    topButtonsLayout->addWidget(deleteTableButton);
    topButtonsLayout->addWidget(addTableButton);
    topButtonsLayout->addWidget(saveDatabaseButton);
    topButtonsLayout->addWidget(restoreDatabaseButton);
    topButtonsLayout->addWidget(restoreTableButton);
    topButtonsLayout->addStretch();

    mainLayout->addLayout(topButtonsLayout);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    tablesContainer = new QWidget();
    tablesLayout = new QVBoxLayout(tablesContainer);
    tablesLayout->setSpacing(5);

    scrollArea->setWidget(tablesContainer);
    mainLayout->addWidget(scrollArea);
}

void TableManagementWindow::loadTables()
{
    for (auto widget : tableWidgets) {
        widget->deleteLater();
    }
    tableWidgets.clear();

    QStringList tableNames = DatabaseManager::instance().getTableNames();

    for (const QString &tableName : tableNames) {
        CollapsibleTableWidget *tableWidget = new CollapsibleTableWidget(tableName, this);
        connect(tableWidget, &CollapsibleTableWidget::needsRefresh, this, &TableManagementWindow::refreshTablesList);
        tablesLayout->addWidget(tableWidget);
        tableWidgets.append(tableWidget);
    }

    tablesLayout->addStretch();
}

QList<CollapsibleTableWidget*> TableManagementWindow::getSelectedTables()
{
    QList<CollapsibleTableWidget*> selected;
    for (auto widget : tableWidgets) {
        if (widget->isSelected()) {
            selected.append(widget);
        }
    }
    return selected;
}

void TableManagementWindow::onDeleteTable()
{
    QList<CollapsibleTableWidget*> selectedTables = getSelectedTables();

    if (selectedTables.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите таблицы для удаления (используйте чекбоксы)");
        return;
    }

    QString tablesList;
    for (auto widget : selectedTables) {
        tablesList += widget->getTableName() + "\n";
    }

    auto reply = QMessageBox::question(this, "Подтверждение",
                                       QString("Вы уверены, что хотите удалить следующие таблицы?\n\n%1").arg(tablesList),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool hasErrors = false;
        QString errors;

        for (auto widget : selectedTables) {
            QString error;
            if (!DatabaseManager::instance().dropTable(widget->getTableName(), &error)) {
                hasErrors = true;
                errors += widget->getTableName() + ": " + error + "\n";
            }
        }

        if (hasErrors) {
            QMessageBox::critical(this, "Ошибки при удалении", errors);
        }

        loadTables();
    }
}

void TableManagementWindow::onAddTable()
{
    AddTableDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        loadTables();
    }
}

void TableManagementWindow::onSaveDatabase()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Сохранить БД", "", "SQL Files (*.sql);;JSON Files (*.json)");

    if (filePath.isEmpty()) return;

    if (filePath.endsWith(".sql")) {
        QString error;
        if (DatabaseManager::instance().exportDatabaseToSql(filePath, &error)) {
            QMessageBox::information(this, "Успех", "База данных успешно экспортирована в SQL файл");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось экспортировать БД: " + error);
        }
    } else {
        QJsonArray dbJson = DatabaseManager::instance().exportDatabaseToJson();

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(dbJson);
            file.write(doc.toJson());
            file.close();
            QMessageBox::information(this, "Успех", "Состояние БД сохранено");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл");
        }
    }
}

void TableManagementWindow::onRestoreDatabase()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Восстановить БД", "", "JSON Files (*.json)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        QMessageBox::critical(this, "Ошибка", "Неверный формат файла");
        return;
    }

    QString error;
    if (DatabaseManager::instance().importDatabaseFromJson(doc.array(), &error)) {
        loadTables();
        QMessageBox::information(this, "Успех", "БД успешно восстановлена");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось восстановить БД: " + error);
    }
}

void TableManagementWindow::onRestoreTable()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Восстановить таблицу", "", "JSON Files (*.json)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        QMessageBox::critical(this, "Ошибка", "Неверный формат файла");
        return;
    }

    QString error;
    if (DatabaseManager::instance().importTableFromJson(doc.object(), &error)) {
        loadTables();
        QMessageBox::information(this, "Успех", "Таблица успешно восстановлена");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось восстановить таблицу: " + error);
    }
}

void TableManagementWindow::refreshTablesList()
{
    loadTables();
}
