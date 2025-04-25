#ifndef TESTDATA_H
#define TESTDATA_H

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QVariant> // Потрібно для QVariant у executeInsertQuery

// Forward declarations
class DatabaseManager; // Потрібно для вказівника у populateTestData
class QSqlQuery;       // Потрібно для executeQuery/executeInsertQuery у .cpp

// --- Структури даних для генерації (перенесено з database.cpp) ---
struct PublisherData {
    QString name;
    QString contactInfo;
    int dbId = -1; // Для збереження ID після вставки
};

struct AuthorData {
    QString firstName;
    QString lastName;
    QDate birthDate;
    QString nationality;
    QString imagePath;
    QString biography; // Додано поле біографії
    int dbId = -1; // Для збереження ID після вставки
};

struct BookData {
    QString title;
    QString isbn;
    QDate publicationDate;
    QString publisherName; // Посилання на видавця за іменем
    double price;
    int stockQuantity;
    QString description;
    QString language;
    int pageCount;
    QString coverImagePath;
    QStringList authorLastNames; // Посилання на авторів за прізвищем
    QString genre;
    int dbId = -1; // Для збереження ID книги
    int publisherDbId = -1; // ID видавця з БД
    QList<int> authorDbIds; // ID авторів з БД
};

// --- Оголошення функцій ---

// Функція для заповнення таблиць тестовими даними
// Приймає вказівник на DatabaseManager для доступу до БД та допоміжних методів
bool populateTestData(DatabaseManager *dbManager, int numberOfRecords = 20);

// Допоміжні функції генерації (оголошення)
QDate randomDate(const QDate &minDate, const QDate &maxDate);
QDateTime randomDateTime(const QDateTime &minDateTime, const QDateTime &maxDateTime);

// Допоміжні функції для виконання запитів (оголошення, якщо вони потрібні тут,
// але краще залишити їх приватними в DatabaseManager і передавати dbManager)
// bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);
// bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);


#endif // TESTDATA_H
