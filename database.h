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
#include "datatypes.h"

class QSqlQuery;

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

    bool createSchemaTables();

    QSqlError lastError() const;
    void closeConnection();
    bool printAllData() const;

    QList<BookDisplayInfo> getAllBooksForDisplay() const;

    QList<BookDisplayInfo> getBooksByGenre(const QString &genre, int limit = 10) const;

    QList<AuthorDisplayInfo> getAllAuthorsForDisplay() const;

    CustomerLoginInfo getCustomerLoginInfo(const QString &email) const;

    CustomerProfileInfo getCustomerProfileInfo(int customerId) const;

    QList<OrderDisplayInfo> getCustomerOrdersForDisplay(int customerId) const;

    bool registerCustomer(const CustomerRegistrationInfo &regInfo, int &newCustomerId);

    bool updateCustomerPhone(int customerId, const QString &newPhone);

    bool updateCustomerName(int customerId, const QString &firstName, const QString &lastName);

    bool updateCustomerAddress(int customerId, const QString &newAddress);

    bool addLoyaltyPoints(int customerId, int pointsToAdd);

    QList<SearchSuggestionInfo> getSearchSuggestions(const QString &prefix, int limit = 10) const;

    BookDetailsInfo getBookDetails(int bookId) const;

    QList<CommentDisplayInfo> getBookComments(int bookId) const;

    BookDisplayInfo getBookDisplayInfoById(int bookId) const;

    double createOrder(int customerId, const QMap<int, int> &items, const QString &shippingAddress, const QString &paymentMethod, int &newOrderId);

    bool addComment(int bookId, int customerId, const QString &commentText, int rating);

    bool hasUserCommentedOnBook(int bookId, int customerId) const;

    OrderDisplayInfo getOrderDetailsById(int orderId) const;

    AuthorDetailsInfo getAuthorDetails(int authorId) const;

    QList<BookDisplayInfo> getSimilarBooks(int currentBookId, const QString &genre, int limit = 5) const;

    QList<BookDisplayInfo> getFilteredBooksForDisplay(const BookFilterCriteria &criteria) const;

    QStringList getAllGenres() const;
    QStringList getAllLanguages() const;

    QMap<int, int> getCartItems(int customerId) const;
    bool addOrUpdateCartItem(int customerId, int bookId, int quantity);
    bool removeCartItem(int customerId, int bookId);
    bool clearCart(int customerId);

    bool isConnected() const;
    QSqlDatabase& database();
    QSqlDatabase m_db;
    bool m_isConnected = false;


    bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);

    bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);

private:
    bool loadSqlQueries(const QString& directory = "sql");
    bool parseSqlFile(const QString& filePath);
    QString getSqlQuery(const QString& queryName) const;

    QMap<QString, QString> m_sqlQueries;
};

#endif // DATABASE_H
