#include "mainwindow.h"
#include "./ui_mainwindow.h" // Keep UI include first if it depends on Qt headers
#include "database.h"      // Include DatabaseManager implementation and BookDisplayInfo struct
#include <QStatusBar>      // For showing status messages
#include <QMessageBox>     // For showing error dialogs
#include <QDebug>          // For logging
#include <QLabel>          // Для відображення тексту та зображень
#include <QVBoxLayout>     // Для компонування елементів картки
#include <QGridLayout>     // Для розміщення карток у сітці
#include <QPushButton>     // Для кнопки "Додати в кошик" (приклад)
#include <QPixmap>         // Для роботи з зображеннями
#include <QFrame>          // Для рамки картки
#include <QSizePolicy>     // Для налаштування розмірів
#include <QScrollArea>     // Щоб переконатися, що вміст прокручується

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Создаем экземпляр DatabaseManager
    m_dbManager = new DatabaseManager(this); // 'this' устанавливает родителя для управления памятью

    // 2. Подключаемся к базе данных (замените на ваши реальные данные)
    bool connected = m_dbManager->connectToDatabase(
        "127.127.126.49",      // Хост
        5432,             // Порт
        "postgres",   // Имя базы данных
        "postgres",       // Имя пользователя
        "1234"            // Пароль (ВНИМАНИЕ: Не храните пароли в коде в реальных приложениях!)
    );

    // 3. Отображаем статус подключения в строке состояния
    if (connected) {
        ui->statusBar->showMessage(tr("Успішно підключено до бази даних."), 5000); // Сообщение на 5 секунд
        qInfo() << "Database connection successful.";

        // --- ВРЕМЕННО: Создаем схему и заполняем данными при запуске ---
        // В реальном приложении это должно быть по кнопке или при первом запуске
        if (!m_dbManager->createSchemaTables()) {
             QMessageBox::critical(this, tr("Помилка створення схеми"),
                                  tr("Не вдалося створити таблиці бази даних.\nДивіться логи для деталей."));
             ui->statusBar->showMessage(tr("Помилка створення схеми БД!"), 0); // Постоянное сообщение
        } else {
             ui->statusBar->showMessage(tr("Схема БД успішно створена/оновлена."), 5000);
             // Заполняем тестовыми данными (например, 30 записей)
             if (!m_dbManager->populateTestData(30)) {
                 QMessageBox::warning(this, tr("Помилка заповнення даних"),
                                      tr("Не вдалося заповнити таблиці тестовими даними.\nДивіться логи для деталей."));
                 ui->statusBar->showMessage(tr("Помилка заповнення БД тестовими даними!"), 0);
             } else {
                 ui->statusBar->showMessage(tr("Тестові дані успішно додані."), 5000);
                 // Опционально: вывести все данные в консоль для проверки
                 // m_dbManager->printAllData();
             }
        }
        // --- КОНЕЦ ВРЕМЕННОГО БЛОКА ---

    } else {
        ui->statusBar->showMessage(tr("Помилка підключення до бази даних!"), 0); // Постоянное сообщение об ошибке
        QMessageBox::critical(this, tr("Помилка підключення"),
                              tr("Не вдалося підключитися до бази даних.\nПеревірте налаштування та логи.\nДодаток може працювати некоректно."));
        qCritical() << "Database connection failed:" << m_dbManager->lastError().text();
    }

}

MainWindow::~MainWindow()
{
    // DatabaseManager будет удален автоматически благодаря установке родителя (this)
    // Но соединение лучше закрыть явно, если оно еще открыто
    if (m_dbManager) {
        m_dbManager->closeConnection();
        // delete m_dbManager; // Не нужно, если 'this' был родителем
    }
    delete ui;
}


// Метод для очищення Layout
void MainWindow::clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget(); // Видаляємо віджет
        }
        delete item; // Видаляємо елемент layout
    }
}

// Метод для створення віджету картки книги
QWidget* MainWindow::createBookCardWidget(const BookDisplayInfo &bookInfo)
{
    // Основний віджет картки (використовуємо QFrame для рамки)
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel); // Додає рамку
    cardFrame->setFrameShadow(QFrame::Raised);     // Додає тінь
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(200, 300); // Мінімальний розмір картки
    cardFrame->setMaximumSize(250, 350); // Максимальний розмір картки
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // Висота фіксована
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }"); // Стиль картки

    // Вертикальний layout для вмісту картки
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(8);
    cardLayout->setContentsMargins(10, 10, 10, 10);

    // 1. Обкладинка книги (QLabel)
    QLabel *coverLabel = new QLabel();
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setMinimumHeight(150); // Мінімальна висота для обкладинки
    coverLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Розтягувати
    QPixmap coverPixmap(bookInfo.coverImagePath);
    if (coverPixmap.isNull()) {
        // Якщо зображення не завантажилось, показуємо плейсхолдер
        coverLabel->setText(tr("Немає\nобкладинки"));
        coverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 4px; }");
    } else {
        // Масштабуємо зображення, зберігаючи пропорції
        coverLabel->setPixmap(coverPixmap.scaled(180, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    cardLayout->addWidget(coverLabel);

    // 2. Назва книги (QLabel)
    QLabel *titleLabel = new QLabel(bookInfo.title);
    titleLabel->setWordWrap(true); // Переносити текст
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; }");
    cardLayout->addWidget(titleLabel);

    // 3. Автор(и) (QLabel)
    QLabel *authorLabel = new QLabel(bookInfo.authors);
    authorLabel->setWordWrap(true);
    authorLabel->setAlignment(Qt::AlignCenter);
    authorLabel->setStyleSheet("QLabel { color: #555; font-size: 9pt; }");
    cardLayout->addWidget(authorLabel);

    // 4. Ціна (QLabel)
    QLabel *priceLabel = new QLabel(QString::number(bookInfo.price, 'f', 2) + tr(" грн"));
    priceLabel->setAlignment(Qt::AlignCenter);
    priceLabel->setStyleSheet("QLabel { font-weight: bold; color: #007bff; font-size: 10pt; margin-top: 5px; }");
    cardLayout->addWidget(priceLabel);

    // Додаємо розтягувач, щоб притиснути кнопку вниз (якщо вона є)
    cardLayout->addStretch(1);

    // 5. Кнопка "Додати в кошик" (QPushButton - приклад)
    QPushButton *addToCartButton = new QPushButton(tr("🛒 Додати"));
    addToCartButton->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; border-radius: 4px; padding: 8px; font-size: 9pt; } QPushButton:hover { background-color: #218838; }");
    addToCartButton->setToolTip(tr("Додати '%1' до кошика").arg(bookInfo.title));
    // Тут можна підключити сигнал кнопки до слота
    // connect(addToCartButton, &QPushButton::clicked, this, [this, bookInfo](){ /* логіка додавання в кошик */ });
    cardLayout->addWidget(addToCartButton);


    cardFrame->setLayout(cardLayout); // Встановлюємо layout для фрейму
    return cardFrame;
}


// Слот для відображення книг у сітці
void MainWindow::displayBooks(const QList<BookDisplayInfo> &books)
{
    // Очищаємо попередні віджети з booksContainerLayout
    clearLayout(ui->booksContainerLayout);

    if (!ui->booksContainerLayout) {
        qWarning() << "booksContainerLayout is null!";
        return;
    }

    int row = 0;
    int col = 0;
    const int maxColumns = 4; // Кількість колонок у сітці

    for (const BookDisplayInfo &bookInfo : books) {
        QWidget *bookCard = createBookCardWidget(bookInfo);
        if (bookCard) {
            ui->booksContainerLayout->addWidget(bookCard, row, col);
            col++;
            if (col >= maxColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Додаємо розтягувач в кінці, щоб картки не розтягувалися вертикально
    // Спочатку видаляємо старий розтягувач, якщо він є
    QLayoutItem* item;
    while ((item = ui->booksContainerLayout->takeAt(ui->booksContainerLayout->count() -1)) != nullptr && item->spacerItem()) {
        delete item;
    }
    // Додаємо новий вертикальний розтягувач в останній рядок
    ui->booksContainerLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);
    // Додаємо горизонтальний розтягувач в останній стовпець
    ui->booksContainerLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);


    // Переконуємося, що контейнер оновився
    ui->booksContainerWidget->updateGeometry();
    ui->booksScrollArea->updateGeometry();
}
