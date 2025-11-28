#ifndef QUERYRESULTDIALOG_H
#define QUERYRESULTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariantList>

class QueryResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QueryResultDialog(const QList<QVariantList> &data, const QStringList &headers, QWidget *parent = nullptr);

private slots:
    void onExportResult();

private:
    void setupUI();
    void loadData();

    QList<QVariantList> data;
    QStringList headers;
    QTableWidget *resultTable;
    QPushButton *exportButton;
};

#endif
