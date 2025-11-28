#ifndef CREATEQUERYDIALOG_H
#define CREATEQUERYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class CreateQueryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateQueryDialog(QWidget *parent = nullptr);

    QString getDescription() const;
    QString getSqlScript() const;

private slots:
    void onConfirm();

private:
    void setupUI();

    QLineEdit *descriptionEdit;
    QTextEdit *sqlEdit;
    QPushButton *confirmButton;
};

#endif
