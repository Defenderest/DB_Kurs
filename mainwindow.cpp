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
#include <QPainter>         // Додано для малювання круглої маски
#include <QBitmap>          // Додано для QBitmap (використовується з QPainter)
#include <QDate>            // Додано для форматування дати
#include "profiledialog.h" // Додано для діалогу профілю
// Залежності для анімації видалені

MainWindow::MainWindow(DatabaseManager *dbManager, int customerId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dbManager(dbManager) // Ініціалізуємо вказівник переданим значенням
    , m_currentCustomerId(customerId) // Зберігаємо ID користувача
{
    ui->setupUi(this);

    // Перевіряємо, чи передано менеджер БД
    if (!m_dbManager) {
        qCritical() << "MainWindow: DatabaseManager is null! Cannot function properly.";
        QMessageBox::critical(this, tr("Критична помилка"), tr("Менеджер бази даних не ініціалізовано."));
        // Можливо, закрити вікно або заблокувати функціонал
        return;
    }

    // Перевіряємо ID користувача
    if (m_currentCustomerId <= 0) {
         qWarning() << "MainWindow: Invalid customer ID received:" << m_currentCustomerId;
         // Можна показати повідомлення або обмежити функціонал
         ui->statusBar->showMessage(tr("Помилка: Не вдалося визначити користувача."), 0);
    } else {
         ui->statusBar->showMessage(tr("Вітаємо!"), 5000);
         qInfo() << "MainWindow initialized for customer ID:" << m_currentCustomerId;
    }

    // Підключаємо сигнали кнопок навігації до слотів (використовуємо імена з UI)
    connect(ui->navHomeButton, &QPushButton::clicked, this, &MainWindow::on_navHomeButton_clicked);
    connect(ui->navBooksButton, &QPushButton::clicked, this, &MainWindow::on_navBooksButton_clicked);
    connect(ui->navAuthorsButton, &QPushButton::clicked, this, &MainWindow::on_navAuthorsButton_clicked);
    connect(ui->navOrdersButton, &QPushButton::clicked, this, &MainWindow::on_navOrdersButton_clicked);
    // connect(ui->navButtonProfile, ...); // Видалено, немає такої кнопки в панелі

    // Підключаємо кнопку профілю з хедера
    connect(ui->profileButton, &QPushButton::clicked, this, &MainWindow::on_profileButton_clicked);

    // Код для анімації та збереження тексту кнопок видалено

    // --- Завантаження та відображення даних для початкової сторінки (Головна) ---
    // Переконуємося, що відповідні layout'и існують перед заповненням
    // (Тепер вони всередині ui->discoverPage)
    qInfo() << "Завантаження даних для головної сторінки...";
    if (ui->classicsRowLayout) {
        QList<BookDisplayInfo> classicsBooks = m_dbManager->getBooksByGenre("Класика", 8); // Використовуємо getBooksByGenre
        displayBooksInHorizontalLayout(classicsBooks, ui->classicsRowLayout);
    } else {
        qWarning() << "classicsRowLayout is null!";
    }
    if (ui->fantasyRowLayout) {
        QList<BookDisplayInfo> fantasyBooks = m_dbManager->getBooksByGenre("Фентезі", 8);
        displayBooksInHorizontalLayout(fantasyBooks, ui->fantasyRowLayout);
    } else {
        qWarning() << "fantasyRowLayout is null!";
    }
    if (ui->nonFictionRowLayout) {
        QList<BookDisplayInfo> nonFictionBooks = m_dbManager->getBooksByGenre("Науково-популярне", 8);
        displayBooksInHorizontalLayout(nonFictionBooks, ui->nonFictionRowLayout);
    } else {
        qWarning() << "nonFictionRowLayout is null!";
    }
    qInfo() << "Завершено завантаження даних для головної сторінки.";

    // --- Завантаження даних для інших сторінок (можна зробити ледачим завантаженням при першому відкритті) ---
    // Завантаження книг для сторінки "Книги" (ui->booksPage)
    if (!ui->booksContainerLayout) {
         qCritical() << "booksContainerLayout is null!";
    } else {
        QList<BookDisplayInfo> books = m_dbManager->getAllBooksForDisplay();
        displayBooks(books); // Заповнюємо сторінку "Книги"
        if (!books.isEmpty()) {
             ui->statusBar->showMessage(tr("Книги успішно завантажено."), 4000);
        } else {
             qWarning() << "Не вдалося завантажити книги для сторінки 'Книги'.";
             // Повідомлення вже обробляється в displayBooks
        }
    }

    // Завантаження авторів для сторінки "Автори" (ui->authorsPage)
    if (!ui->authorsContainerLayout) {
        qCritical() << "authorsContainerLayout is null!";
    } else {
        QList<AuthorDisplayInfo> authors = m_dbManager->getAllAuthorsForDisplay();
        displayAuthors(authors);
        if (!authors.isEmpty()) {
             qInfo() << "Автори успішно завантажені.";
        } else {
             qWarning() << "Не вдалося завантажити авторів.";
             // Повідомлення вже обробляється в displayAuthors
        }
    }

    // Завантаження профілю більше не потрібне тут, воно відбувається при кліку на profileButton

    // Завантаження замовлень (якщо потрібно при старті)
    // loadAndDisplayOrders(); // Потрібно реалізувати цю функцію та відповідну сторінку ui->ordersPage

    // Блок else для помилки підключення більше не потрібен тут,
    // оскільки dbManager передається і перевіряється на початку конструктора.
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
    // cardFrame->setMaximumSize(250, 350); // Максимальний розмір можна прибрати або залишити, якщо потрібно обмеження
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // Дозволяємо розтягуватись в комірці
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
    QLabel *authorLabel = new QLabel(bookInfo.authors.isEmpty() ? tr("Невідомий автор") : bookInfo.authors);
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
    // Очищаємо попередні віджети з booksContainerLayout (всередині pageBooks)
    if (!ui->booksContainerLayout) {
        qWarning() << "displayBooks: booksContainerLayout is null!";
        // Можливо, показати повідомлення в statusBar або в самій області
        QLabel *errorLabel = new QLabel(tr("Помилка: Не вдалося знайти область для відображення книг."), ui->booksContainerWidget);
        errorLabel->setAlignment(Qt::AlignCenter);
        ui->booksContainerWidget->setLayout(new QVBoxLayout()); // Потрібен layout для додавання мітки
        ui->booksContainerWidget->layout()->addWidget(errorLabel);
        return;
    }
    clearLayout(ui->booksContainerLayout);

    if (books.isEmpty()) {
        QLabel *noBooksLabel = new QLabel(tr("Не вдалося завантажити книги або їх немає в базі даних."), ui->booksContainerWidget);
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setWordWrap(true);
        // Додаємо мітку безпосередньо в layout, якщо він існує
        ui->booksContainerLayout->addWidget(noBooksLabel, 0, 0, 1, maxColumns); // Розтягнути на кілька колонок
        return; // Виходимо, якщо книг немає
    }


    int row = 0;
    int col = 0;
    const int maxColumns = 5; // Змінено кількість колонок у сітці на 5

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

    // Встановлюємо однакове розтягування для колонок з картками
    for (int c = 0; c < maxColumns; ++c) {
        ui->booksContainerLayout->setColumnStretch(c, 1);
    }
    // Встановлюємо розтягування для колонки після карток (де горизонтальний спейсер)
    // Це гарантує, що колонки з картками будуть однакової ширини, а зайвий простір піде в останню колонку.
    ui->booksContainerLayout->setColumnStretch(maxColumns, 99); // Велике значення, щоб забрати весь зайвий простір

    // Видаляємо попередні розширювачі, якщо вони були додані раніше (про всяк випадок)
    QLayoutItem* item;
    // Видаляємо вертикальний розширювач знизу (якщо він є)
    item = ui->booksContainerLayout->itemAtPosition(row + 1, 0);
    if (item && item->spacerItem()) {
        ui->booksContainerLayout->removeItem(item);
        delete item;
    }
     // Видаляємо горизонтальний розширювач справа (якщо він є)
    item = ui->booksContainerLayout->itemAtPosition(0, maxColumns);
     if (item && item->spacerItem()) {
        ui->booksContainerLayout->removeItem(item);
        delete item;
    }

    // Додаємо горизонтальний розширювач в першому рядку після останньої колонки карток,
    // щоб притиснути картки вліво.
    ui->booksContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);
    // Додаємо вертикальний розширювач під останнім рядком карток,
    // щоб притиснути картки вгору.
    ui->booksContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);


    // Переконуємося, що контейнер оновився
    ui->booksContainerWidget->updateGeometry();
    // ui->booksScrollArea->updateGeometry(); // Зазвичай не потрібно викликати для ScrollArea
}


// Допоміжна функція для відображення книг у горизонтальному layout
void MainWindow::displayBooksInHorizontalLayout(const QList<BookDisplayInfo> &books, QHBoxLayout* layout)
{
    if (!layout) {
        qWarning() << "Target layout for horizontal display is null!";
        return;
    }
    // Очищаємо попередні віджети та розширювачі
    clearLayout(layout);

    if (books.isEmpty()) {
        // Якщо книг немає, показуємо повідомлення
        QLabel *noBooksLabel = new QLabel(tr("Для цього розділу книг не знайдено."));
        noBooksLabel->setAlignment(Qt::AlignCenter);
        noBooksLabel->setStyleSheet("QLabel { color: #777; font-style: italic; }");
        layout->addWidget(noBooksLabel, 1); // Додаємо з розтягуванням
    } else {
        for (const BookDisplayInfo &bookInfo : books) {
            QWidget *bookCard = createBookCardWidget(bookInfo);
            if (bookCard) {
                // Встановлюємо фіксовану або максимальну ширину для карток у горизонтальному ряду
                bookCard->setMinimumWidth(180);
                bookCard->setMaximumWidth(220);
                layout->addWidget(bookCard);
            }
        }
        // Додаємо розширювач в кінці, щоб притиснути картки вліво
        layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
    }

    // Оновлюємо геометрію батьківського віджета (QWidget всередині QScrollArea)
    if (layout->parentWidget()) {
        layout->parentWidget()->updateGeometry();
    }
}


// Метод для створення віджету картки автора
QWidget* MainWindow::createAuthorCardWidget(const AuthorDisplayInfo &authorInfo)
{
    // Основний віджет картки (QFrame)
    QFrame *cardFrame = new QFrame();
    cardFrame->setFrameShape(QFrame::StyledPanel);
    cardFrame->setFrameShadow(QFrame::Raised);
    cardFrame->setLineWidth(1);
    cardFrame->setMinimumSize(180, 250); // Розмір картки автора
    cardFrame->setMaximumSize(220, 280);
    cardFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // Фіксована висота
    cardFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; }");

    // Вертикальний layout для вмісту картки
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setSpacing(6);
    cardLayout->setContentsMargins(10, 10, 10, 10);

    // 1. Фото автора (QLabel)
    QLabel *photoLabel = new QLabel();
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setMinimumSize(150, 150); // Розмір фото
    photoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Фіксована висота
    QPixmap photoPixmap(authorInfo.imagePath);
    if (photoPixmap.isNull() || authorInfo.imagePath.isEmpty()) {
        // Плейсхолдер, якщо фото немає
        photoLabel->setText(tr("👤")); // Іконка користувача
        photoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border-radius: 75px; font-size: 80pt; qproperty-alignment: AlignCenter; }"); // Круглий фон
    } else {
        // Масштабуємо фото і робимо його круглим (якщо можливо)
        QPixmap scaledPixmap = photoPixmap.scaled(150, 150, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        // Створення круглої маски
        QBitmap mask(scaledPixmap.size());
        mask.fill(Qt::color0); // Прозорий фон
        QPainter painter(&mask);
        painter.setBrush(Qt::color1); // Непрозора маска
        painter.drawEllipse(0, 0, scaledPixmap.width(), scaledPixmap.height());
        painter.end();
        scaledPixmap.setMask(mask);
        photoLabel->setPixmap(scaledPixmap);
        photoLabel->setStyleSheet("QLabel { border-radius: 75px; }"); // Додатково для рамки, якщо потрібно
    }
    cardLayout->addWidget(photoLabel, 0, Qt::AlignHCenter); // Центруємо фото

    // 2. Ім'я та Прізвище (QLabel)
    QLabel *nameLabel = new QLabel(authorInfo.firstName + " " + authorInfo.lastName);
    nameLabel->setWordWrap(true);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 11pt; margin-top: 5px; }");
    cardLayout->addWidget(nameLabel);

    // 3. Національність (QLabel - опціонально)
    if (!authorInfo.nationality.isEmpty()) {
        QLabel *nationalityLabel = new QLabel(authorInfo.nationality);
        nationalityLabel->setAlignment(Qt::AlignCenter);
        nationalityLabel->setStyleSheet("QLabel { color: #777; font-size: 9pt; }");
        cardLayout->addWidget(nationalityLabel);
    }

    // Додаємо розтягувач, щоб притиснути кнопку вниз (якщо вона буде)
    cardLayout->addStretch(1);

    // 4. Кнопка "Переглянути книги" (приклад)
    QPushButton *viewBooksButton = new QPushButton(tr("Переглянути книги"));
    viewBooksButton->setStyleSheet("QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 4px; padding: 6px; font-size: 9pt; } QPushButton:hover { background-color: #106ebe; }");
    viewBooksButton->setToolTip(tr("Переглянути книги автора %1 %2").arg(authorInfo.firstName, authorInfo.lastName));
    // connect(viewBooksButton, &QPushButton::clicked, this, [this, authorInfo](){ /* логіка перегляду книг автора */ });
    cardLayout->addWidget(viewBooksButton);


    cardFrame->setLayout(cardLayout);
    return cardFrame;
}

// Метод для відображення авторів у сітці
void MainWindow::displayAuthors(const QList<AuthorDisplayInfo> &authors)
{
    // Очищаємо попередні віджети з authorsContainerLayout (всередині pageAuthors)
     if (!ui->authorsContainerLayout) {
        qWarning() << "displayAuthors: authorsContainerLayout is null!";
        QLabel *errorLabel = new QLabel(tr("Помилка: Не вдалося знайти область для відображення авторів."), ui->authorsContainerWidget);
        errorLabel->setAlignment(Qt::AlignCenter);
        ui->authorsContainerWidget->setLayout(new QVBoxLayout());
        ui->authorsContainerWidget->layout()->addWidget(errorLabel);
        return;
    }
    clearLayout(ui->authorsContainerLayout);

    if (authors.isEmpty()) {
        QLabel *noAuthorsLabel = new QLabel(tr("Не вдалося завантажити авторів або їх немає в базі даних."), ui->authorsContainerWidget);
        noAuthorsLabel->setAlignment(Qt::AlignCenter);
        noAuthorsLabel->setWordWrap(true);
        ui->authorsContainerLayout->addWidget(noAuthorsLabel, 0, 0, 1, maxColumns); // Розтягнути на кілька колонок
        return;
    }

    int row = 0;
    int col = 0;
    const int maxColumns = 5; // Кількість колонок (можна змінити)

    for (const AuthorDisplayInfo &authorInfo : authors) {
        QWidget *authorCard = createAuthorCardWidget(authorInfo);
        if (authorCard) {
            ui->authorsContainerLayout->addWidget(authorCard, row, col);
            col++;
            if (col >= maxColumns) {
                col = 0;
                row++;
            }
        }
    }

    // Встановлюємо розтягування колонок та додаємо розширювачі (аналогічно displayBooks)
    for (int c = 0; c < maxColumns; ++c) {
        ui->authorsContainerLayout->setColumnStretch(c, 1);
    }
    ui->authorsContainerLayout->setColumnStretch(maxColumns, 99);

    // Видаляємо старі розширювачі (про всяк випадок)
    QLayoutItem* itemV = ui->authorsContainerLayout->itemAtPosition(row + 1, 0);
    if (itemV && itemV->spacerItem()) { delete ui->authorsContainerLayout->takeAt(ui->authorsContainerLayout->indexOf(itemV)); }
    QLayoutItem* itemH = ui->authorsContainerLayout->itemAtPosition(0, maxColumns);
    if (itemH && itemH->spacerItem()) { delete ui->authorsContainerLayout->takeAt(ui->authorsContainerLayout->indexOf(itemH)); }

    // Додаємо нові розширювачі
    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, maxColumns);
    ui->authorsContainerLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding), row + 1, 0, 1, maxColumns);

    // Оновлюємо геометрію
    ui->authorsContainerWidget->updateGeometry();
    // ui->authorsScrollArea->updateGeometry(); // Зазвичай не потрібно
}

// TODO: Додати метод для завантаження та відображення замовлень
// void MainWindow::loadAndDisplayOrders() { ... }


// --- Реалізація слотів та функцій ---

// Слоти для кнопок навігації (перейменовані)
void MainWindow::on_navHomeButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->discoverPage); // Використовуємо ім'я сторінки з UI
}

void MainWindow::on_navBooksButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->booksPage); // Використовуємо ім'я сторінки з UI
    // Можна додати ледаче завантаження тут, якщо не зроблено в конструкторі
}

void MainWindow::on_navAuthorsButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->authorsPage); // Використовуємо ім'я сторінки з UI
    // Можна додати ледаче завантаження тут
}

void MainWindow::on_navOrdersButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->ordersPage); // Використовуємо ім'я сторінки з UI
    // Потрібно завантажити дані замовлень
    // loadAndDisplayOrders();
    qWarning() << "Сторінка замовлень ще не реалізована повністю.";
    // Тимчасово: очистити layout або показати повідомлення
    if(ui->ordersPageLayout) { // Перевіряємо layout сторінки замовлень
        clearLayout(ui->ordersPageLayout);
        QLabel *todoLabel = new QLabel(tr("Сторінка 'Мої замовлення' в розробці."), ui->ordersPage);
        todoLabel->setAlignment(Qt::AlignCenter);
        ui->ordersPageLayout->addWidget(todoLabel);
    }
}

// Слот для кнопки профілю в хедері
void MainWindow::on_profileButton_clicked()
{
    qInfo() << "Profile button clicked. Loading profile for customer ID:" << m_currentCustomerId;

    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Неможливо завантажити профіль, оскільки користувач не визначений."));
        return;
    }

    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
         return;
    }

    // 1. Отримуємо дані профілю з бази даних
    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);

    // 2. Перевіряємо, чи знайдені дані
    if (!profile.found) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Не вдалося знайти інформацію для вашого профілю."));
        return;
    }

    // 3. Створюємо та показуємо діалог профілю
    ProfileDialog profileDialog(profile, this); // Передаємо дані та батьківський віджет
    profileDialog.exec(); // Показуємо діалог модально
}


// Функції для анімації (setupSidebarAnimation, toggleSidebar, eventFilter) видалені.

// Функція populateProfilePanel видалена, оскільки діалог сам обробляє дані.

// --- Кінець реалізації слотів та функцій ---
