#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// Forward declaration to avoid including database.h here if possible
// If we need DatabaseManager methods directly in the header, include it.
// For now, a pointer is enough.
class DatabaseManager;

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

private:
    Ui::MainWindow *ui;
    DatabaseManager *m_dbManager; // Указатель на менеджер БД
};
#endif // MAINWINDOW_H
