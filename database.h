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
    bool found = false; // Прапорець, чи знайдено книгу
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

// Структура для передачі детальної інформації про книгу в UI
struct BookDetailsInfo {
    int bookId = -1;
    QString title;
    QString authors; // Об'єднані імена авторів
    double price = 0.0;
    QString coverImagePath;
    int stockQuantity = 0;
    QString genre;
    QString description;
    QString publisherName;
    QDate publicationDate;
    QString isbn;
    int pageCount = 0;
    QString language;
    bool found = false; // Прапорець, чи знайдено книгу
    // Поля для рейтингу та коментарів
    // double averageRating; // Можна додати середній рейтинг
    QList<struct CommentDisplayInfo> comments; // Список коментарів
};

// Структура для відображення одного коментаря
struct CommentDisplayInfo {
    QString authorName; // Ім'я та прізвище автора коментаря
    QDateTime commentDate;
    int rating; // 0-5 (0 - без оцінки)
    QString commentText;
};


// Структура для передачі даних для реєстрації нового користувача
struct CustomerRegistrationInfo {
    QString firstName;
    QString lastName;
    QString email;
    QString password; // Пароль у відкритому вигляді перед хешуванням
};

// Структура для позиції замовлення в UI
struct OrderItemDisplayInfo {
    QString bookTitle;
    int quantity;
    double pricePerUnit;
};

// Структура для статусу замовлення в UI
struct OrderStatusDisplayInfo {
    QString status;
    QDateTime statusDate;
    QString trackingNumber;
};

// Структура для повного замовлення в UI
struct OrderDisplayInfo {
    int orderId;
    QDateTime orderDate;
    double totalAmount;
    QString shippingAddress;
    QString paymentMethod;
    QList<OrderItemDisplayInfo> items;
    QList<OrderStatusDisplayInfo> statuses;
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

    // Новий метод для отримання замовлень користувача для відображення
    QList<OrderDisplayInfo> getCustomerOrdersForDisplay(int customerId) const;

    // Новий метод для реєстрації користувача
    bool registerCustomer(const CustomerRegistrationInfo &regInfo, int &newCustomerId);

    // Новий метод для оновлення телефону користувача
    bool updateCustomerPhone(int customerId, const QString &newPhone);

    // Новий метод для оновлення імені та прізвища користувача
    bool updateCustomerName(int customerId, const QString &firstName, const QString &lastName);

    // Новий метод для оновлення адреси користувача
    bool updateCustomerAddress(int customerId, const QString &newAddress);

    // Новий метод для отримання пропозицій пошуку (автодоповнення)
    QStringList getSearchSuggestions(const QString &prefix, int limit = 10) const;

    // Новий метод для отримання детальної інформації про книгу за ID
    BookDetailsInfo getBookDetails(int bookId) const;

    // Новий метод для отримання коментарів до книги
    QList<CommentDisplayInfo> getBookComments(int bookId) const;

    // Новий метод для отримання BookDisplayInfo за ID (для кошика)
    BookDisplayInfo getBookDisplayInfoById(int bookId) const;

    // Новий метод для створення замовлення
    bool createOrder(int customerId, const QMap<int, int> &items, const QString &shippingAddress, const QString &paymentMethod, int &newOrderId);


private:
    QSqlDatabase m_db;
    bool m_isConnected = false;

    // Вспомогательная функция для выполнения запроса с проверкой ошибки
    bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);

    // Вспомогательная функция для выполнения подготовленного INSERT с возвратом ID
    bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);
};

#endif // DATABASE_H
