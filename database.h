#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QDate>
#include <QVariant>
#include <QVector>
#include <QDebug>
#include <QList>
#include <QCryptographicHash>


struct BookDisplayInfo {
    int bookId;
    QString title;
    QString authors;
    double price;
    QString coverImagePath;
    int stockQuantity;
    QString genre;
};


struct AuthorDisplayInfo {
    int authorId;
    QString firstName;
    QString lastName;
    QString nationality;
    QString imagePath;
};


struct CustomerLoginInfo {
    int customerId = -1;
    QString passwordHash;
    bool found = false;
};


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
    bool found = false;
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

    bool createSchemaTables();

    bool populateTestData(int numberOfRecords = 20);

    QSqlError lastError() const;
    void closeConnection();
    bool printAllData() const;

    QList<BookDisplayInfo> getAllBooksForDisplay() const;

    QList<BookDisplayInfo> getBooksByGenre(const QString &genre, int limit = 10) const;

    QList<AuthorDisplayInfo> getAllAuthorsForDisplay() const;

    CustomerLoginInfo getCustomerLoginInfo(const QString &email) const;

    CustomerProfileInfo getCustomerProfileInfo(int customerId) const;


private:
    QSqlDatabase m_db;
    bool m_isConnected = false;

    bool executeQuery(QSqlQuery &query, const QString &sql, const QString &description);

    bool executeInsertQuery(QSqlQuery &query, const QString &description, QVariant &insertedId);
};

#endif
