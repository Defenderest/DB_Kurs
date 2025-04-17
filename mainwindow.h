#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList> // Для списку книг
#include <QHBoxLayout>
#include "database.h"
#include <QWidget> // Додано для QWidget* у конструкторі та типів повернення
#include <QLayout> // Додано для QLayout* у clearLayout
#include <QDate>   // Додано для QDate у CustomerProfileInfo
// #include <QPropertyAnimation> // Більше не потрібен

// Forward declarations
class DatabaseManager;
struct CustomerProfileInfo; // Додано forward declaration
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame; // Для створення "картки" та панелі профілю
// class QHBoxLayout; // Можна використовувати forward declaration, якщо include в .cpp

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
    void displayBooks(const QList<BookDisplayInfo> &books); // Слот для відображення книг
    void on_profileButton_clicked(); // Слот для кнопки профілю
    // void hideProfilePanel(); // Більше не потрібен

private:
    Ui::MainWindow *ui;
    // QFrame *m_profilePanel; // Більше не потрібен
    // QPropertyAnimation *m_profileAnimation; // Більше не потрібен
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД (передається ззовні)
    int m_currentCustomerId;      // ID поточного користувача

    // Допоміжна функція для очищення layout
    void clearLayout(QLayout* layout);
    // Допоміжна функція для створення картки книги
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    // Допоміжна функція для відображення книг у горизонтальному layout
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);

    // Допоміжні функції для вкладки "Автори"
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);

    // Допоміжні функції для панелі профілю більше не потрібні
    // void setupProfilePanelAnimation();
    // void showProfilePanel();
    // void populateProfilePanel(const CustomerProfileInfo &profileInfo);

};
#endif // MAINWINDOW_H
