#include "queryresultdialog.h"
#include "databasemanager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>

QueryResultDialog::QueryResultDialog(const QList<QVariantList> &data, const QStringList &headers, QWidget *parent)
    : QDialog(parent), data(data), headers(headers)
{
    setupUI();
    loadData();
}

void QueryResultDialog::setupUI()
{
    setWindowTitle("Результат запроса");
    setMinimumSize(800, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    resultTable = new QTableWidget(this);
    resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(resultTable);

    exportButton = new QPushButton("Экспорт результата", this);
    connect(exportButton, &QPushButton::clicked, this, &QueryResultDialog::onExportResult);
    mainLayout->addWidget(exportButton);
}

void QueryResultDialog::loadData()
{
    resultTable->setRowCount(data.size());
    resultTable->setColumnCount(headers.size());
    resultTable->setHorizontalHeaderLabels(headers);

    for (int row = 0; row < data.size(); ++row) {
        const auto &rowData = data[row];
        for (int col = 0; col < rowData.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[col].toString());
            resultTable->setItem(row, col, item);
        }
    }

    resultTable->resizeColumnsToContents();
}

void QueryResultDialog::onExportResult()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Экспорт результата", "", "CSV Files (*.csv)");

    if (filePath.isEmpty()) return;

    QString error;
    if (DatabaseManager::instance().exportQueryResultToCsv(data, headers, filePath, &error)) {
        QMessageBox::information(this, "Успех", "Результат успешно экспортирован");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось экспортировать результат: " + error);
    }
}
