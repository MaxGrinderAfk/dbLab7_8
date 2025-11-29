#include "addtabledialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QGridLayout>

ColumnInputWidget::ColumnInputWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);

    QLabel *nameLabel = new QLabel("Название:", this);
    nameEdit = new QLineEdit(this);

    QLabel *typeLabel = new QLabel("Тип:", this);
    typeCombo = new QComboBox(this);
    typeCombo->addItem("text");
    typeCombo->addItem("bigint");
    typeCombo->addItem("integer");
    typeCombo->addItem("date");
    typeCombo->addItem("boolean");
    typeCombo->addItem("numeric");
    typeCombo->addItem("varchar");

    primaryKeyCheck = new QCheckBox("Primary Key", this);
    identityCheck = new QCheckBox("Auto (SERIAL)", this);
    nullableCheck = new QCheckBox("Nullable", this);
    nullableCheck->setChecked(true);

    QLabel *defaultLabel = new QLabel("Default:", this);
    defaultEdit = new QLineEdit(this);
    defaultEdit->setPlaceholderText("NULL или значение");

    layout->addWidget(nameLabel, 0, 0);
    layout->addWidget(nameEdit, 0, 1);
    layout->addWidget(typeLabel, 0, 2);
    layout->addWidget(typeCombo, 0, 3);
    layout->addWidget(primaryKeyCheck, 0, 4);
    layout->addWidget(identityCheck, 0, 5);
    layout->addWidget(nullableCheck, 0, 6);
    layout->addWidget(defaultLabel, 0, 7);
    layout->addWidget(defaultEdit, 0, 8);
}

QString ColumnInputWidget::getColumnName() const
{
    return nameEdit->text().trimmed();
}

QString ColumnInputWidget::getColumnType() const
{
    return typeCombo->currentText();
}

bool ColumnInputWidget::isPrimaryKey() const
{
    return primaryKeyCheck->isChecked();
}

bool ColumnInputWidget::isIdentity() const
{
    return identityCheck->isChecked();
}

bool ColumnInputWidget::isNullable() const
{
    return nullableCheck->isChecked();
}

QString ColumnInputWidget::getDefaultValue() const
{
    return defaultEdit->text().trimmed();
}

AddTableDialog::AddTableDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void AddTableDialog::setupUI()
{
    setWindowTitle("Добавить таблицу");
    setMinimumSize(1200, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("Имя таблицы:", this);
    tableNameEdit = new QLineEdit(this);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(tableNameEdit);
    mainLayout->addLayout(nameLayout);

    QHBoxLayout *columnButtonsLayout = new QHBoxLayout();
    addColumnButton = new QPushButton("Добавить столбец", this);
    removeColumnButton = new QPushButton("Удалить столбец", this);
    connect(addColumnButton, &QPushButton::clicked, this, &AddTableDialog::onAddColumn);
    connect(removeColumnButton, &QPushButton::clicked, this, &AddTableDialog::onRemoveColumn);
    columnButtonsLayout->addWidget(addColumnButton);
    columnButtonsLayout->addWidget(removeColumnButton);
    columnButtonsLayout->addStretch();
    mainLayout->addLayout(columnButtonsLayout);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    columnsContainer = new QWidget();
    columnsLayout = new QVBoxLayout(columnsContainer);
    columnsLayout->setSpacing(10);
    columnsLayout->addStretch();

    scrollArea->setWidget(columnsContainer);
    mainLayout->addWidget(scrollArea);

    confirmButton = new QPushButton("Подтвердить", this);
    connect(confirmButton, &QPushButton::clicked, this, &AddTableDialog::onConfirm);
    mainLayout->addWidget(confirmButton);
}

void AddTableDialog::onAddColumn()
{
    ColumnInputWidget *columnWidget = new ColumnInputWidget(this);

    int count = columnsLayout->count();
    if (count > 0 && columnsLayout->itemAt(count - 1)->spacerItem()) {
        columnsLayout->insertWidget(count - 1, columnWidget);
    } else {
        columnsLayout->addWidget(columnWidget);
    }

    columnWidgets.append(columnWidget);
}

void AddTableDialog::onRemoveColumn()
{
    if (!columnWidgets.isEmpty()) {
        ColumnInputWidget *lastWidget = columnWidgets.takeLast();
        columnsLayout->removeWidget(lastWidget);
        lastWidget->deleteLater();
    }
}

void AddTableDialog::onConfirm()
{
    QString tableName = tableNameEdit->text().trimmed();

    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя таблицы");
        return;
    }

    if (columnWidgets.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы один столбец");
        return;
    }

    QList<DatabaseManager::ColumnInfo> columns;

    for (auto widget : columnWidgets) {
        QString colName = widget->getColumnName();
        if (colName.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Все столбцы должны иметь имя");
            return;
        }

        DatabaseManager::ColumnInfo col;
        col.name = colName;
        col.type = widget->getColumnType().contains("int") ? "int" : "text";
        col.fullType = widget->getColumnType();
        col.isPrimaryKey = widget->isPrimaryKey();
        col.isIdentity = widget->isIdentity();
        col.isNullable = widget->isNullable();
        col.defaultValue = widget->getDefaultValue();

        columns.append(col);
    }

    QString error;
    if (DatabaseManager::instance().createTable(tableName, columns, &error)) {
        QMessageBox::information(this, "Успех", "Таблица успешно создана");
        accept();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать таблицу: " + error);
    }
}
