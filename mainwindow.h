#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList> // Для списку книг
#include <QHBoxLayout> // Додано для QHBoxLayout

// Forward declarations
class DatabaseManager;
struct BookDisplayInfo; // Потрібно для типу списку
class QLabel;
class QVBoxLayout;
class QGridLayout;
class QPushButton;
class QFrame; // Для створення "картки"
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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void displayBooks(const QList<BookDisplayInfo> &books); // Слот для відображення книг

private:
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager; // Указатель на менеджер БД

    // Допоміжна функція для очищення layout
    void clearLayout(QLayout* layout);
    // Допоміжна функція для створення картки книги
    QWidget* createBookCardWidget(const BookDisplayInfo &bookInfo);
    // Допоміжна функція для відображення книг у горизонтальному layout
    void displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout);

    // Допоміжні функції для вкладки "Автори"
    QWidget* createAuthorCardWidget(const AuthorDisplayInfo &authorInfo);
    void displayAuthors(const QList<AuthorDisplayInfo> &authors);

};
#endif // MAINWINDOW_H
