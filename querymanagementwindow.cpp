#include "querymanagementwindow.h"
#include "createquerydialog.h"
#include "queryresultdialog.h"
#include "databasemanager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <qsqlrecord.h>

QueryWidget::QueryWidget(const QueryInfo &query, QWidget *parent)
    : QWidget(parent), query(query), originalDescription(query.description)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    selectCheckBox = new QCheckBox(this);
    layout->addWidget(selectCheckBox);

    descriptionEdit = new QLineEdit(query.description, this);
    descriptionEdit->setMinimumHeight(40);
    descriptionEdit->setStyleSheet("QLineEdit { padding: 5px; }");
    connect(descriptionEdit, &QLineEdit::editingFinished, this, &QueryWidget::onDescriptionEdited);

    executeButton = new QPushButton("Выполнить запрос", this);
    executeButton->setMinimumWidth(150);

    connect(executeButton, &QPushButton::clicked, this, [this, query]() {
        emit executeRequested(query.sqlScript);
    });

    layout->addWidget(descriptionEdit, 1);
    layout->addWidget(executeButton);
}

QString QueryWidget::getDescription() const
{
    return descriptionEdit->text();
}

void QueryWidget::setDescription(const QString &desc)
{
    descriptionEdit->setText(desc);
    query.description = desc;
    originalDescription = desc;
}

bool QueryWidget::isSelected() const
{
    return selectCheckBox->isChecked();
}

void QueryWidget::setSelected(bool selected)
{
    selectCheckBox->setChecked(selected);
}

void QueryWidget::onDescriptionEdited()
{
    QString newDesc = descriptionEdit->text();
    if (newDesc != originalDescription) {
        emit descriptionChanged(originalDescription, newDesc);
        query.description = newDesc;
        originalDescription = newDesc;
    }
}

QueryManagementWindow::QueryManagementWindow(QWidget *parent)
    : QMainWindow(parent)
{
    if (!DatabaseManager::instance().connectToDatabase()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных");
    }

    setupUI();
    loadDefaultQueries();
    refreshQueriesList();
}

QueryManagementWindow::~QueryManagementWindow()
{
}

void QueryManagementWindow::setupUI()
{
    setWindowTitle("Работа с запросами");
    setMinimumSize(900, 600);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    createQueryButton = new QPushButton("Создать запрос", this);
    deleteQueryButton = new QPushButton("Удалить запрос", this);
    importQueriesButton = new QPushButton("Импорт запроса", this);
    saveQueriesButton = new QPushButton("Сохранить запрос", this);

    connect(createQueryButton, &QPushButton::clicked, this, &QueryManagementWindow::onCreateQuery);
    connect(deleteQueryButton, &QPushButton::clicked, this, &QueryManagementWindow::onDeleteQuery);
    connect(importQueriesButton, &QPushButton::clicked, this, &QueryManagementWindow::onImportQueries);
    connect(saveQueriesButton, &QPushButton::clicked, this, &QueryManagementWindow::onSaveQueries);

    buttonsLayout->addWidget(createQueryButton);
    buttonsLayout->addWidget(deleteQueryButton);
    buttonsLayout->addWidget(importQueriesButton);
    buttonsLayout->addWidget(saveQueriesButton);
    buttonsLayout->addStretch();

    mainLayout->addLayout(buttonsLayout);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    queriesContainer = new QWidget();
    queriesLayout = new QVBoxLayout(queriesContainer);
    queriesLayout->setSpacing(5);

    scrollArea->setWidget(queriesContainer);
    mainLayout->addWidget(scrollArea);
}

void QueryManagementWindow::loadDefaultQueries()
{
    queries.clear();

    QVector<QPair<QString, QString>> defaultQueries = {
        {"Все издательства", "SELECT * FROM publisher;"},
        {"Издательства основанные после 1900 года", "SELECT * FROM publisher WHERE foundation_year > 1900;"},
        {"Название, страна и город издательств", "SELECT name, country, city FROM publisher;"},
        {"Издательства отсортированные по году основания (по убыванию)", "SELECT * FROM publisher ORDER BY foundation_year DESC;"},
        {"Издательства с 'Pr' в названии", "SELECT * FROM publisher WHERE name LIKE '%Pr%';"},
        {"Издательства и их книги (INNER JOIN)", "SELECT p.name, p.country, b.title FROM publisher p INNER JOIN book b ON p.id = b.publisher_id;"},
        {"Все издательства с книгами или без (LEFT JOIN)", "SELECT p.name, p.country, b.title FROM publisher p LEFT JOIN book b ON p.id = b.publisher_id;"},
        {"Все книги и их издательства (RIGHT JOIN)", "SELECT p.name, p.country, b.title FROM publisher p RIGHT JOIN book b ON p.id = b.publisher_id;"},
        {"Все издательства и все книги (FULL OUTER JOIN)", "SELECT p.name, p.country, b.title FROM publisher p FULL OUTER JOIN book b ON p.id = b.publisher_id;"},
        {"Все категории книг", "SELECT * FROM category;"},
        {"Категории с рейтингом >= 4", "SELECT * FROM category WHERE rating >= 4;"},
        {"Название, описание и рейтинг категорий", "SELECT name, description, rating FROM category;"},
        {"Категории отсортированные по дате создания (по убыванию)", "SELECT * FROM category ORDER BY created_at DESC;"},
        {"Категории начинающиеся с 'Нау'", "SELECT * FROM category WHERE name LIKE 'Нау%';"},
        {"Категории и их книги (INNER JOIN)", "SELECT c.name, c.rating, b.title FROM category c INNER JOIN book b ON c.id = b.category_id;"},
        {"Все категории с книгами или без (LEFT JOIN)", "SELECT c.name, c.rating, b.title FROM category c LEFT JOIN book b ON c.id = b.category_id;"},
        {"Все книги и их категории (RIGHT JOIN)", "SELECT c.name, c.rating, b.title FROM category c RIGHT JOIN book b ON c.id = b.category_id;"},
        {"Все категории и все книги (FULL OUTER JOIN)", "SELECT c.name, c.rating, b.title FROM category c FULL OUTER JOIN book b ON c.id = b.category_id;"},
        {"Все книги", "SELECT * FROM book;"},
        {"Книги изданные после 2000 года с количеством страниц < 500", "SELECT * FROM book WHERE year > 2000 AND pages < 500;"},
        {"Название, ISBN, год и страницы книг", "SELECT title, isbn, year, pages FROM book;"},
        {"Книги отсортированные по году (убыв.) и страницам (возр.)", "SELECT * FROM book ORDER BY year DESC, pages ASC;"},
        {"Книги содержащие 'Д' в названии", "SELECT * FROM book WHERE title LIKE '%Д%';"},
        {"Книги и их издательства (INNER JOIN)", "SELECT b.title, b.year, p.name AS publisher_name FROM book b INNER JOIN publisher p ON b.publisher_id = p.id;"},
        {"Все книги с издательствами или без (LEFT JOIN)", "SELECT b.title, b.year, p.name AS publisher_name FROM book b LEFT JOIN publisher p ON b.publisher_id = p.id;"},
        {"Все издательства и их книги (RIGHT JOIN)", "SELECT b.title, b.year, p.name AS publisher_name FROM book b RIGHT JOIN publisher p ON b.publisher_id = p.id;"},
        {"Все книги и все издательства (FULL OUTER JOIN)", "SELECT b.title, b.year, p.name AS publisher_name FROM book b FULL OUTER JOIN publisher p ON b.publisher_id = p.id;"},
        {"Все авторы", "SELECT * FROM author;"},
        {"Авторы из Индии", "SELECT * FROM author WHERE country = 'Индия';"},
        {"ФИО, дата рождения и страна авторов", "SELECT fio, birth_date, country FROM author;"},
        {"Авторы отсортированные по дате рождения (по возрастанию)", "SELECT * FROM author ORDER BY birth_date ASC;"},
        {"Авторы с ФИО начинающимся на 'A'", "SELECT * FROM author WHERE fio LIKE 'A%';"},
        {"Все читатели", "SELECT * FROM reader;"},
        {"Читатели родившиеся после 1990-01-01", "SELECT * FROM reader WHERE birth_date > '1990-01-01';"},
        {"ФИО, телефон и email читателей", "SELECT fio, phone, email FROM reader;"},
        {"Читатели отсортированные по ФИО (по возрастанию)", "SELECT * FROM reader ORDER BY fio ASC;"},
        {"Читатели с Gmail почтой", "SELECT * FROM reader WHERE email LIKE '%@gmail.com';"},
        {"Все сотрудники", "SELECT * FROM employee;"},
        {"Сотрудники-библиотекари", "SELECT * FROM employee WHERE position = 'библиотекарь';"},
        {"ФИО, должность и дата найма сотрудников", "SELECT fio, position, hire_date FROM employee;"},
        {"Сотрудники отсортированные по дате найма (по убыванию)", "SELECT * FROM employee ORDER BY hire_date DESC;"},
        {"Сотрудники с 'ов' в ФИО", "SELECT * FROM employee WHERE fio LIKE '%ов%';"},
        {"Статистика издательств с CTE и UNION", "WITH publisher_stats AS (SELECT p.id, p.name, p.foundation_year, COUNT(b.id) AS book_count, AVG(b.pages) AS avg_pages FROM publisher p LEFT JOIN book b ON p.id = b.publisher_id GROUP BY p.id, p.name, p.foundation_year HAVING COUNT(b.id) > 2) SELECT * FROM publisher_stats WHERE id IN (SELECT id FROM publisher WHERE foundation_year > 1900) UNION SELECT id, name, foundation_year, 0, 0 FROM publisher WHERE id NOT IN (SELECT DISTINCT publisher_id FROM book WHERE publisher_id IS NOT NULL);"},
        {"Категории с книгами и подзапросами (UNION ALL)", "SELECT c.id, c.name, c.rating, COUNT(b.id) AS book_count, MAX(b.year) AS newest_book FROM category c LEFT JOIN book b ON c.id = b.category_id GROUP BY c.id, c.name, c.rating HAVING COUNT(b.id) > ANY(SELECT COUNT(*) FROM book WHERE category_id IS NOT NULL GROUP BY category_id HAVING COUNT(*) < 5) UNION ALL SELECT id, name, rating, 0, 0 FROM category WHERE description IS NOT NULL;"},
        {"Книги с одним или без авторов и ISBN (INTERSECT)", "SELECT b.id, b.title, b.year, COUNT(ba.author_id) AS author_count, MIN(b.pages) AS min_pages FROM book b LEFT JOIN book_author ba ON b.id = ba.book_id GROUP BY b.id, b.title, b.year HAVING COUNT(ba.author_id) <= 1 INTERSECT SELECT id, title, year, 0, pages FROM book WHERE isbn IS NOT NULL;"},
        {"Авторы со странами и написанными книгами (INTERSECT ALL)", "SELECT a.id, a.fio, COUNT(ba.book_id) AS books_written, SUM(b.pages) AS total_pages FROM author a LEFT JOIN book_author ba ON a.id = ba.author_id LEFT JOIN book b ON ba.book_id = b.id WHERE a.country IS NOT NULL GROUP BY a.id, a.fio HAVING COUNT(ba.book_id) > ALL(SELECT 0) INTERSECT ALL SELECT a.id, a.fio, COUNT(ba.book_id) AS books_written, SUM(b.pages) AS total_pages FROM author a LEFT JOIN book_author ba ON a.id = ba.author_id LEFT JOIN book b ON ba.book_id = b.id GROUP BY a.id, a.fio HAVING COUNT(ba.book_id) >= 1;"},
        {"Читатели взявшие книги, исключая родившихся до 1950 (EXCEPT)", "SELECT r.id, r.fio, COUNT(bi.book_id) AS books_borrowed FROM reader r LEFT JOIN book_issue bi ON r.id = bi.reader_id GROUP BY r.id, r.fio HAVING COUNT(bi.book_id) > 0 AND EXISTS (SELECT 1 FROM book_issue WHERE reader_id = r.id) EXCEPT SELECT id, fio, 0 FROM reader WHERE birth_date < '1950-01-01';"},
        {"Сотрудники выдавшие книги, исключая принятых до 2000 (EXCEPT ALL)", "SELECT e.id, e.fio, COUNT(bi.book_id) AS books_issued FROM employee e LEFT JOIN book_issue bi ON e.id = bi.employee_id GROUP BY e.id, e.fio HAVING COUNT(bi.book_id) > 0 EXCEPT ALL SELECT id, fio, 0 FROM employee WHERE hire_date < '2000-01-01';"}
    };

    for (const auto &pair : defaultQueries) {
        QueryInfo info;
        info.description = pair.first;
        info.sqlScript = pair.second;
        queries.append(info);
    }
}

void QueryManagementWindow::refreshQueriesList()
{
    for (auto widget : queryWidgets) {
        widget->deleteLater();
    }
    queryWidgets.clear();

    for (const auto &query : queries) {
        QueryWidget *queryWidget = new QueryWidget(query, this);
        connect(queryWidget, &QueryWidget::executeRequested, this, &QueryManagementWindow::onExecuteQuery);
        connect(queryWidget, &QueryWidget::descriptionChanged, this, &QueryManagementWindow::onQueryDescriptionChanged);
        queriesLayout->addWidget(queryWidget);
        queryWidgets.append(queryWidget);
    }

    queriesLayout->addStretch();
}

QList<QueryWidget*> QueryManagementWindow::getSelectedQueries()
{
    QList<QueryWidget*> selected;
    for (auto widget : queryWidgets) {
        if (widget->isSelected()) {
            selected.append(widget);
        }
    }
    return selected;
}

void QueryManagementWindow::onCreateQuery()
{
    CreateQueryDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QueryInfo info;
        info.description = dialog.getDescription();
        info.sqlScript = dialog.getSqlScript();
        queries.append(info);
        refreshQueriesList();
    }
}

void QueryManagementWindow::onDeleteQuery()
{
    QList<QueryWidget*> selectedQueries = getSelectedQueries();

    if (selectedQueries.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите запросы для удаления (используйте чекбоксы)");
        return;
    }

    auto reply = QMessageBox::question(this, "Подтверждение",
                                       QString("Вы уверены, что хотите удалить %1 запрос(ов)?").arg(selectedQueries.size()),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        for (auto widget : selectedQueries) {
            QString description = widget->getDescription();
            for (int i = 0; i < queries.size(); ++i) {
                if (queries[i].description == description) {
                    queries.removeAt(i);
                    break;
                }
            }
        }
        refreshQueriesList();
    }
}

void QueryManagementWindow::onImportQueries()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Импорт запросов", "", "JSON Files (*.json)");

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

    queries.clear();
    QJsonArray array = doc.array();

    for (const auto &value : array) {
        QJsonObject obj = value.toObject();
        QueryInfo info;
        info.description = obj["description"].toString();
        info.sqlScript = obj["sql"].toString();
        queries.append(info);
    }

    refreshQueriesList();
    QMessageBox::information(this, "Успех", "Запросы успешно импортированы");
}

void QueryManagementWindow::onSaveQueries()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Сохранить запросы", "", "JSON Files (*.json)");

    if (filePath.isEmpty()) return;

    QJsonArray array;
    for (const auto &query : queries) {
        QJsonObject obj;
        obj["description"] = query.description;
        obj["sql"] = query.sqlScript;
        array.append(obj);
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(array);
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, "Успех", "Запросы успешно сохранены");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл");
    }
}

void QueryManagementWindow::onExecuteQuery(const QString &sql)
{
    QString error;
    bool ok;
    QSqlQuery query = DatabaseManager::instance().executeQuery(sql, &ok, &error);

    if (!ok) {
        QMessageBox::critical(this, "Ошибка выполнения запроса", error);
        return;
    }

    if (sql.trimmed().toUpper().startsWith("SELECT") ||
        sql.trimmed().toUpper().startsWith("WITH")) {
        QList<QVariantList> data;
        QStringList headers;

        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            headers.append(record.fieldName(i));
        }

        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < record.count(); ++i) {
                row.append(query.value(i));
            }
            data.append(row);
        }

        QueryResultDialog *resultDialog = new QueryResultDialog(data, headers, this);
        resultDialog->setAttribute(Qt::WA_DeleteOnClose);
        resultDialog->show();
    } else {
        int rowsAffected = query.numRowsAffected();
        QMessageBox::information(this, "Результат",
                                 QString("Запрос выполнен успешно. Затронуто строк: %1").arg(rowsAffected));
    }
}

void QueryManagementWindow::onQueryDescriptionChanged(const QString &oldDesc, const QString &newDesc)
{
    for (int i = 0; i < queries.size(); ++i) {
        if (queries[i].description == oldDesc) {
            queries[i].description = newDesc;
            break;
        }
    }
}
