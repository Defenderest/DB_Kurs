#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList> // Для списку книг
#include <QHBoxLayout>
#include "database.h"
#include <QWidget> // Додано для QWidget* у конструкторі та типів повернення
#include <QLayout>      // Додано для QLayout* у clearLayout
#include <QDate>        // Додано для QDate у CustomerProfileInfo
#include <QPropertyAnimation> // Для анімації бокової панелі
#include <QEvent>       // Для eventFilter
#include <QEnterEvent>  // Для подій наведення миші
#include <QMap>         // Для збереження тексту кнопок
#include <QLineEdit>    // Додано для редагування телефону
#include <QCompleter>   // Додано для автодоповнення
#include <QStringListModel> // Додано для моделі автодоповнення

// Forward declarations
class DatabaseManager;
struct CustomerProfileInfo;
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame;
class QStackedWidget; // Додано
class QPropertyAnimation; // Додано

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Змінено конструктор: приймає DatabaseManager та ID користувача
    explicit MainWindow(DatabaseManager *dbManager, int customerId, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слоти для кнопок навігації
    void on_navHomeButton_clicked();   // Перейменовано
    void on_navBooksButton_clicked();
    void on_navAuthorsButton_clicked();
    void on_navOrdersButton_clicked();
    void on_navProfileButton_clicked(); // Слот для кнопки профілю в бічній панелі
    void on_editProfileButton_clicked(); // Слот для кнопки редагування профілю
    void on_saveProfileButton_clicked(); // Слот для кнопки збереження профілю
    void updateSearchSuggestions(const QString &text); // Слот для оновлення пропозицій пошуку

private:
    // Допоміжні функції для відображення даних
    void displayBooks(const QList<BookDisplayInfo> &books); // Метод для відображення книг
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    void populateProfilePanel(const CustomerProfileInfo &profileInfo); // Заповнення сторінки профілю

    // Функції для відображення замовлень
    QWidget* createOrderWidget(const OrderDisplayInfo &orderInfo);
    void displayOrders(const QList<OrderDisplayInfo> &orders);
    void loadAndDisplayOrders(); // Завантаження та відображення замовлень

    // Допоміжна функція для очищення layout
    void clearLayout(QLayout* layout);

    // Керування режимом редагування профілю
    void setProfileEditingEnabled(bool enabled);

    // Налаштування анімації та стану бічної панелі
    void setupSidebarAnimation();
    void toggleSidebar(bool expand);
    void setupSearchCompleter(); // Налаштування автодоповнення для пошуку

    // Члени класу
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД
    int m_currentCustomerId;      // ID поточного користувача

    QPropertyAnimation *m_sidebarAnimation = nullptr; // Анімація для бокової панелі
    bool m_isSidebarExpanded = false;                 // Поточний стан панелі
    int m_collapsedWidth = 50;                        // Ширина згорнутої панелі
    int m_expandedWidth = 200;                       // Ширина розгорнутої панелі
    QMap<QPushButton*, QString> m_buttonOriginalText; // Зберігання оригінального тексту кнопок

    // Члени для автодоповнення пошуку
    QCompleter *m_searchCompleter = nullptr;
    QStringListModel *m_searchSuggestionModel = nullptr;

protected:
    // Перехоплення подій для sidebarFrame
    bool eventFilter(QObject *watched, QEvent *event) override;

};
#endif // MAINWINDOW_H
