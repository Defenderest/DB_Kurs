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
#include <QList>    // Потрібно для QList
#include <QMap>     // Потрібно для QMap
#include <QFile>    // Для читання файлів
#include <QTextStream> // Для читання файлів
#include <QDir>     // Для роботи з директоріями
#include <QCryptographicHash> // Додано для хешування паролів
#include "datatypes.h" // Включаємо файл з визначеннями структур

// Forward declaration for QSqlQuery needed for helper methods
class QSqlQuery;

class DatabaseManager : public QObject
{
    Q_OBJECT

    // Forward declaration для QSqlDatabase, якщо потрібно
    // class QSqlDatabase;

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

    // Declaration for populateTestData removed (moved to testdata.h)

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

    // Новий метод для додавання бонусних балів користувачу
    bool addLoyaltyPoints(int customerId, int pointsToAdd);

    // Оновлений метод для отримання пропозицій пошуку (повертає розширену інформацію)
    QList<SearchSuggestionInfo> getSearchSuggestions(const QString &prefix, int limit = 10) const;

    // Новий метод для отримання детальної інформації про книгу за ID
    BookDetailsInfo getBookDetails(int bookId) const;

    // Новий метод для отримання коментарів до книги
    QList<CommentDisplayInfo> getBookComments(int bookId) const;

    // Новий метод для отримання BookDisplayInfo за ID (для кошика)
    BookDisplayInfo getBookDisplayInfoById(int bookId) const;

    // Оновлений метод для створення замовлення (повертає загальну суму або -1.0 при помилці)
    double createOrder(int customerId, const QMap<int, int> &items, const QString &shippingAddress, const QString &paymentMethod, int &newOrderId);

    // Новий метод для додавання коментаря
    bool addComment(int bookId, int customerId, const QString &commentText, int rating);

    // Новий метод для перевірки, чи користувач вже коментував книгу
    bool hasUserCommentedOnBook(int bookId, int customerId) const;

    // Новий метод для отримання деталей одного замовлення за ID
    OrderDisplayInfo getOrderDetailsById(int orderId) const;

    // Новий метод для отримання детальної інформації про автора за ID
    AuthorDetailsInfo getAuthorDetails(int authorId) const;

    // Новий метод для отримання схожих книг (за жанром)
    QList<BookDisplayInfo> getSimilarBooks(int currentBookId, const QString &genre, int limit = 5) const;

    // Новий метод для отримання книг з урахуванням фільтрів
    QList<BookDisplayInfo> getFilteredBooksForDisplay(const BookFilterCriteria &criteria) const;

    // Нові методи для отримання унікальних значень для фільтрів
    QStringList getAllGenres() const;
    QStringList getAllLanguages() const;

    // --- Методи для роботи з корзиною ---
    QMap<int, int> getCartItems(int customerId) const; // Повертає bookId -> quantity
    bool addOrUpdateCartItem(int customerId, int bookId, int quantity);
    bool removeCartItem(int customerId, int bookId);
    bool clearCart(int customerId);

    // Публічні методи доступу до стану та об'єкту БД
    bool isConnected() const;
    QSqlDatabase& database(); // Повертає посилання для можливості операцій (транзакції, запити)
    QSqlDatabase m_db;
    bool m_isConnected = false;


    // Вспомогательная функция для выполнения запроса с проверкой ошибки
    bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);

    // Вспомогательная функция для выполнения подготовленного INSERT с возвратом ID
    bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);

private:
    // Допоміжні функції для завантаження та отримання SQL запитів
    bool loadSqlQueries(const QString& directory = "sql"); // Завантажує запити з директорії
    bool parseSqlFile(const QString& filePath);           // Парсить один .sql файл
    QString getSqlQuery(const QString& queryName) const;   // Отримує запит за іменем

    QMap<QString, QString> m_sqlQueries; // Зберігання завантажених запитів "ім'я" -> "SQL"

    // Приватні допоміжні функції генерації даних (якщо вони були тут, їх слід видалити)
    // QDate randomDate(const QDate &minDate, const QDate &maxDate); // Removed
    // QDateTime randomDateTime(const QDateTime &minDateTime, const QDateTime &maxDateTime); // Removed
};

#endif // DATABASE_H
