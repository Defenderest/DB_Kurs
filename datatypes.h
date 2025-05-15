#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>

struct BookDisplayInfo {
    int bookId;
    QString title;
    QString authors;
    double price;
    QString coverImagePath;
    int stockQuantity;
    QString genre;
    bool found = false;
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

struct BookDetailsInfo {
    int bookId = -1;
    QString title;
    QString authors;
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
    bool found = false;
    QList<struct CommentDisplayInfo> comments;
};

struct CommentDisplayInfo {
    QString authorName;
    QDateTime commentDate;
    int rating;
    QString commentText;
};

struct CustomerRegistrationInfo {
    QString firstName;
    QString lastName;
    QString email;
    QString password;
};

struct OrderItemDisplayInfo {
    QString bookTitle;
    int quantity;
    double pricePerUnit;
};

struct OrderStatusDisplayInfo {
    QString status;
    QDateTime statusDate;
    QString trackingNumber;
};

struct OrderDisplayInfo {
    int orderId;
    QDateTime orderDate;
    double totalAmount;
    QString shippingAddress;
    QString paymentMethod;
    QList<OrderItemDisplayInfo> items;
    QList<OrderStatusDisplayInfo> statuses;
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

struct CartItem {
    BookDisplayInfo book;
    int quantity;
};

struct SearchSuggestionInfo {
    enum SuggestionType { Book, Author };

    QString displayText;
    SuggestionType type;
    int id;
    QString imagePath;
    double price = 0.0;
};

struct AuthorDetailsInfo {
    int authorId = -1;
    QString firstName;
    QString lastName;
    QString nationality;
    QString imagePath;
    QString biography;
    QDate birthDate;
    QList<BookDisplayInfo> books;
    bool found = false;
};

struct BookFilterCriteria {
    QStringList genres;
    QStringList languages;
    double minPrice = -1.0;
    double maxPrice = -1.0;
    bool inStockOnly = false;
};

#endif // DATATYPES_H
