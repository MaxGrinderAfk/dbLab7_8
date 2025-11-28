#include "createquerydialog.h"
#include <QMessageBox>

CreateQueryDialog::CreateQueryDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void CreateQueryDialog::setupUI()
{
    setWindowTitle("Создать запрос");
    setMinimumSize(600, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *descLabel = new QLabel("Описание запроса(что он делает):", this);
    mainLayout->addWidget(descLabel);

    descriptionEdit = new QLineEdit(this);
    descriptionEdit->setMinimumHeight(40);
    mainLayout->addWidget(descriptionEdit);

    QLabel *sqlLabel = new QLabel("SQL - скрипт для выполнения запроса:", this);
    mainLayout->addWidget(sqlLabel);

    sqlEdit = new QTextEdit(this);
    sqlEdit->setMinimumHeight(200);
    mainLayout->addWidget(sqlEdit);

    confirmButton = new QPushButton("подтвердить", this);
    connect(confirmButton, &QPushButton::clicked, this, &CreateQueryDialog::onConfirm);
    mainLayout->addWidget(confirmButton);
}

QString CreateQueryDialog::getDescription() const
{
    return descriptionEdit->text().trimmed();
}

QString CreateQueryDialog::getSqlScript() const
{
    return sqlEdit->toPlainText().trimmed();
}

void CreateQueryDialog::onConfirm()
{
    if (getDescription().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите описание запроса");
        return;
    }

    if (getSqlScript().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите SQL скрипт");
        return;
    }

    accept();
}
