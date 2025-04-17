#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QDate>    // Додано для QDate
#include <QVariant> // Нужно для lastInsertId() или query.value()
#include <QVector>  // Для хранения сгенерированных ID
#include <QDebug>
#include <QList>    // Потрібно для QList<BookDisplayInfo>
#include <QCryptographicHash> // Додано для хешування паролів

// Структура для передачі даних книги в UI (перенесено з .cpp)
struct BookDisplayInfo {
    int bookId;
    QString title;
    QString authors; // Об'єднані імена авторів
    double price;
    QString coverImagePath;
    int stockQuantity;
    QString genre; // Додано поле жанру
};

// Структура для передачі даних автора в UI
struct AuthorDisplayInfo {
    int authorId;
    QString firstName;
    QString lastName;
    QString nationality;
    QString imagePath; // Шлях до зображення автора
};

// Структура для передачі даних для входу
struct CustomerLoginInfo {
    int customerId = -1;
    QString passwordHash;
    bool found = false; // Прапорець, чи знайдено користувача
};

// Структура для передачі повної інформації профілю користувача в UI
struct CustomerProfileInfo {
    int customerId = -1;
    QString firstName;
    QString lastName;
    QString email;
    QString phone;
    QString address;
    QDate joinDate;
    bool loyaltyProgram = false;
    int loyaltyPoints = 0;
    bool found = false; // Прапорець, чи знайдено користувача
};


class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool connectToDatabase(const QString &host,
                           int port,
                           const QString &dbName,
                           const QString &user,
                           const QString &password);

    // Метод для создания структуры таблиц
    bool createSchemaTables();

    // Новый метод для заполнения таблиц тестовыми данными
    bool populateTestData(int numberOfRecords = 20); // По умолчанию 20 записей

    QSqlError lastError() const;
    void closeConnection();
    bool printAllData() const;

    // Новий метод для отримання книг для відображення
    QList<BookDisplayInfo> getAllBooksForDisplay() const;

    // Новий метод для отримання книг за жанром
    QList<BookDisplayInfo> getBooksByGenre(const QString &genre, int limit = 10) const;

    // Новий метод для отримання авторів для відображення
    QList<AuthorDisplayInfo> getAllAuthorsForDisplay() const;

    // Новий метод для отримання даних для входу за email
    CustomerLoginInfo getCustomerLoginInfo(const QString &email) const;

    // Новий метод для отримання повної інформації профілю користувача за ID
    CustomerProfileInfo getCustomerProfileInfo(int customerId) const;


private:
    QSqlDatabase m_db;
    bool m_isConnected = false;

    // Вспомогательная функция для выполнения запроса с проверкой ошибки
    bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);

    // Вспомогательная функция для выполнения подготовленного INSERT с возвратом ID
    bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);
};

#endif // DATABASE_H
