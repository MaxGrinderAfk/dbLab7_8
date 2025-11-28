#include "addtabledialog.h"
#include <QMessageBox>
#include <QInputDialog>

ColumnInputWidget::ColumnInputWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    QLabel *nameLabel = new QLabel("Название столбца:", this);
    nameEdit = new QLineEdit(this);

    QLabel *typeLabel = new QLabel("Тип данных:", this);
    typeCombo = new QComboBox(this);
    typeCombo->addItem("int");
    typeCombo->addItem("text");

    QLabel *pkLabel = new QLabel("Primary key:", this);
    primaryKeyCombo = new QComboBox(this);
    primaryKeyCombo->addItem("нет");
    primaryKeyCombo->addItem("да");

    layout->addWidget(nameLabel);
    layout->addWidget(nameEdit);
    layout->addWidget(typeLabel);
    layout->addWidget(typeCombo);
    layout->addWidget(pkLabel);
    layout->addWidget(primaryKeyCombo);
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
    return primaryKeyCombo->currentText() == "да";
}

AddTableDialog::AddTableDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void AddTableDialog::setupUI()
{
    setWindowTitle("Добавить таблицу");
    setMinimumSize(800, 400);

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

    columnsContainer = new QWidget();
    columnsLayout = new QVBoxLayout(columnsContainer);
    columnsLayout->setSpacing(10);

    scrollArea->setWidget(columnsContainer);
    mainLayout->addWidget(scrollArea);

    confirmButton = new QPushButton("подтвердить", this);
    connect(confirmButton, &QPushButton::clicked, this, &AddTableDialog::onConfirm);
    mainLayout->addWidget(confirmButton);
}

void AddTableDialog::onAddColumn()
{
    ColumnInputWidget *columnWidget = new ColumnInputWidget(this);
    columnsLayout->addWidget(columnWidget);
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
        col.type = widget->getColumnType();
        col.isPrimaryKey = widget->isPrimaryKey();
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
