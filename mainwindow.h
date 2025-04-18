#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList> // Для списку книг
#include <QHBoxLayout>
#include "database.h"
#include <QWidget> // Додано для QWidget* у конструкторі та типів повернення
#include <QLayout>      // Додано для QLayout* у clearLayout
#include <QDate>        // Додано для QDate у CustomerProfileInfo
#include "profiledialog.h" // Додано для діалогу профілю

// Forward declarations
class DatabaseManager;
struct CustomerProfileInfo;
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame;
class QStackedWidget; // Додано

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
    void on_navBooksButton_clicked();  // Перейменовано
    void on_navAuthorsButton_clicked();// Перейменовано
    void on_navOrdersButton_clicked(); // Перейменовано
    void on_profileButton_clicked();   // Слот для кнопки профілю в хедері

private:
    // Допоміжні функції для відображення даних
    void displayBooks(const QList<BookDisplayInfo> &books); // Метод для відображення книг
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    // void populateProfilePanel(const CustomerProfileInfo &profileInfo); // Видалено, діалог сам заповнюється

    // Допоміжна функція для очищення layout
    void clearLayout(QLayout* layout);

    // Члени класу
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager; // Вказівник на менеджер БД
    int m_currentCustomerId;      // ID поточного користувача

    // Члени для анімації видалені

};
#endif // MAINWINDOW_H
