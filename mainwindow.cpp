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
#include <QPropertyAnimation> // Додано для анімації
#include <QEvent>           // Для eventFilter
#include <QEnterEvent>      // Для подій наведення миші
#include <QMap>             // Для QMap
#include <QDateTime>        // Для форматування дати/часу замовлення
#include <QLocale>          // Для форматування чисел та дат
#include <QGroupBox>        // Для групування елементів замовлення
#include <QTableWidget>     // Для відображення позицій та статусів
#include <QHeaderView>      // Для налаштування заголовків таблиці
// #include "profiledialog.h" // Видалено

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
    connect(ui->navProfileButton, &QPushButton::clicked, this, &MainWindow::on_navProfileButton_clicked); // Додано з'єднання для кнопки профілю

    // Видалено з'єднання для кнопки профілю з хедера

    // Зберігаємо оригінальний текст кнопок (з .ui файлу, де він повний)
    m_buttonOriginalText[ui->navHomeButton] = tr("🏠 Головна");
    m_buttonOriginalText[ui->navBooksButton] = tr("📚 Книги");
    m_buttonOriginalText[ui->navAuthorsButton] = tr("👥 Автори");
    m_buttonOriginalText[ui->navOrdersButton] = tr("🛍️ Мої замовлення");
    m_buttonOriginalText[ui->navProfileButton] = tr("👤 Профіль"); // Додано текст для кнопки профілю

    // Налаштовуємо анімацію бокової панелі
    setupSidebarAnimation();

    // Встановлюємо фільтр подій на sidebarFrame для відстеження наведення миші
    ui->sidebarFrame->installEventFilter(this);
    // Переконуємось, що панель спочатку згорнута
    toggleSidebar(false); // Згорнути без анімації при старті

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

    // Завантаження профілю для сторінки "Профіль" (ui->pageProfile)
    // (Завантаження відбувається при кліку на кнопку профілю в бічній панелі)

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
    const int maxColumns = 5; // Змінено кількість колонок у сітці на 5

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
    // const int maxColumns = 5; // Перенесено на початок функції

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
    const int maxColumns = 5; // Кількість колонок (можна змінити)

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
    // const int maxColumns = 5; // Перенесено на початок функції

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

// Слоти для кнопок навігації
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
    ui->contentStackedWidget->setCurrentWidget(ui->ordersPage); // Переключаємо на сторінку замовлень
    loadAndDisplayOrders(); // Завантажуємо та відображаємо замовлення
}

// Слот для кнопки профілю в бічній панелі
void MainWindow::on_navProfileButton_clicked()
{
    qInfo() << "Navigating to profile page for customer ID:" << m_currentCustomerId;
    ui->contentStackedWidget->setCurrentWidget(ui->pageProfile);

    // Завантажуємо дані профілю при переході на сторінку
    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Неможливо завантажити профіль, оскільки користувач не визначений."));
        populateProfilePanel(CustomerProfileInfo()); // Показати помилку в полях
        return;
    }
    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
         populateProfilePanel(CustomerProfileInfo()); // Показати помилку в полях
         return;
    }

    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
    if (!profile.found) {
        QMessageBox::warning(this, tr("Профіль користувача"), tr("Не вдалося знайти інформацію для вашого профілю."));
    }
    populateProfilePanel(profile); // Заповнюємо сторінку профілю
}


// Налаштування анімації бокової панелі
void MainWindow::setupSidebarAnimation()
{
    m_sidebarAnimation = new QPropertyAnimation(ui->sidebarFrame, "maximumWidth", this);
    m_sidebarAnimation->setDuration(250); // Тривалість анімації в мс
    m_sidebarAnimation->setEasingCurve(QEasingCurve::InOutQuad); // Плавність анімації
}

// Функція для розгортання/згортання панелі
void MainWindow::toggleSidebar(bool expand)
{
    if (m_isSidebarExpanded == expand && m_sidebarAnimation->state() == QAbstractAnimation::Stopped) {
        return; // Вже в потрібному стані і анімація не йде
    }
     // Якщо анімація ще триває, зупиняємо її перед запуском нової
    if (m_sidebarAnimation->state() == QAbstractAnimation::Running) {
        m_sidebarAnimation->stop();
    }

    m_isSidebarExpanded = expand;

    // Оновлюємо текст кнопок
    for (auto it = m_buttonOriginalText.begin(); it != m_buttonOriginalText.end(); ++it) {
        QPushButton *button = it.key();
        const QString &originalText = it.value();
        if (expand) {
            button->setText(originalText);
            button->setToolTip(""); // Очистити підказку, коли текст видно
        } else {
            // Залишаємо тільки перший символ (іконку)
            button->setText(originalText.left(originalText.indexOf(' ') > 0 ? originalText.indexOf(' ') : 1));
             button->setToolTip(originalText.mid(originalText.indexOf(' ') + 1)); // Показати текст як підказку
        }
    }

    m_sidebarAnimation->setStartValue(ui->sidebarFrame->width());
    m_sidebarAnimation->setEndValue(expand ? m_expandedWidth : m_collapsedWidth);
    m_sidebarAnimation->start();
}


// Перехоплення подій для sidebarFrame
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->sidebarFrame) {
        if (event->type() == QEvent::Enter) {
            // Миша увійшла в область sidebarFrame
            toggleSidebar(true); // Розгорнути
            return true; // Подія оброблена
        } else if (event->type() == QEvent::Leave) {
            // Миша покинула область sidebarFrame
            toggleSidebar(false); // Згорнути
            return true; // Подія оброблена
        }
    }
    // Передаємо подію батьківському класу для стандартної обробки
    return QMainWindow::eventFilter(watched, event);
}

// Метод для створення віджету картки замовлення
QWidget* MainWindow::createOrderWidget(const OrderDisplayInfo &orderInfo)
{
    // Основний віджет-контейнер для замовлення (використовуємо QFrame)
    QFrame *orderFrame = new QFrame();
    orderFrame->setFrameShape(QFrame::StyledPanel);
    orderFrame->setFrameShadow(QFrame::Sunken); // Трохи інший стиль для відокремлення
    orderFrame->setLineWidth(1);
    orderFrame->setStyleSheet("QFrame { background-color: #f8f9fa; border-radius: 6px; border: 1px solid #dee2e6; }");
    orderFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); // Розширюється по ширині

    QVBoxLayout *mainLayout = new QVBoxLayout(orderFrame);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // --- Верхня частина: ID, Дата, Сума ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(20);

    QLabel *orderIdLabel = new QLabel(tr("Замовлення №%1").arg(orderInfo.orderId));
    orderIdLabel->setStyleSheet("font-weight: bold; font-size: 12pt; color: black;"); // Змінено колір на чорний

    QLabel *orderDateLabel = new QLabel(tr("Дата: %1").arg(QLocale::system().toString(orderInfo.orderDate, QLocale::ShortFormat)));
    orderDateLabel->setStyleSheet("color: black;"); // Змінено колір на чорний

    QLabel *totalAmountLabel = new QLabel(tr("Сума: %1 грн").arg(QLocale::system().toString(orderInfo.totalAmount, 'f', 2)));
    totalAmountLabel->setStyleSheet("font-weight: bold; color: black; font-size: 11pt;"); // Змінено колір на чорний

    headerLayout->addWidget(orderIdLabel);
    headerLayout->addWidget(orderDateLabel);
    headerLayout->addStretch(1); // Розтягувач, щоб притиснути суму вправо
    headerLayout->addWidget(totalAmountLabel);

    mainLayout->addLayout(headerLayout);

    // --- Роздільник ---
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("border-color: #e0e0e0;");
    mainLayout->addWidget(separator);

    // --- Деталі: Адреса, Оплата ---
    QFormLayout *detailsLayout = new QFormLayout();
    detailsLayout->setSpacing(8);
    detailsLayout->setLabelAlignment(Qt::AlignLeft); // Вирівнювання назв полів по лівому краю

    // Адреса доставки
    QLabel *addressLabel = new QLabel(tr("Адреса доставки:"));
    addressLabel->setStyleSheet("color: black; font-weight: bold;"); // Чорний жирний шрифт для назви поля
    QLabel *addressValueLabel = new QLabel(orderInfo.shippingAddress.isEmpty() ? tr("(не вказано)") : orderInfo.shippingAddress);
    addressValueLabel->setStyleSheet("color: black;"); // Чорний колір для значення
    addressValueLabel->setWordWrap(true); // Дозволяємо перенос тексту адреси
    detailsLayout->addRow(addressLabel, addressValueLabel);

    // Спосіб оплати
    QLabel *paymentLabel = new QLabel(tr("Спосіб оплати:"));
    paymentLabel->setStyleSheet("color: black; font-weight: bold;"); // Чорний жирний шрифт для назви поля
    QLabel *paymentValueLabel = new QLabel(orderInfo.paymentMethod.isEmpty() ? tr("(не вказано)") : orderInfo.paymentMethod);
    paymentValueLabel->setStyleSheet("color: black;"); // Чорний колір для значення
    detailsLayout->addRow(paymentLabel, paymentValueLabel);

    mainLayout->addLayout(detailsLayout);

    // --- Позиції замовлення (Таблиця) ---
    if (!orderInfo.items.isEmpty()) {
        QGroupBox *itemsGroup = new QGroupBox(tr("Товари в замовленні"));
        itemsGroup->setStyleSheet("QGroupBox { color: black; font-weight: bold; }"); // Додано стиль для групи
        QVBoxLayout *itemsLayout = new QVBoxLayout(itemsGroup);

        QTableWidget *itemsTable = new QTableWidget(orderInfo.items.size(), 3);
        itemsTable->setHorizontalHeaderLabels({tr("Назва книги"), tr("Кількість"), tr("Ціна за од.")});
        itemsTable->verticalHeader()->setVisible(false); // Сховати вертикальні заголовки
        itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Заборонити редагування
        itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        itemsTable->setAlternatingRowColors(true);
        itemsTable->horizontalHeader()->setStretchLastSection(false); // Не розтягувати останню колонку
        itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); // Розтягнути назву
        itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Кількість по вмісту
        itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Ціна по вмісту
        itemsTable->setStyleSheet("QTableWidget { border: 1px solid #dee2e6; gridline-color: #e9ecef; color: black; } QHeaderView::section { background-color: #f1f3f5; padding: 4px; border: none; border-bottom: 1px solid #dee2e6; color: black; } "); // Додано color: black

        for (int i = 0; i < orderInfo.items.size(); ++i) {
            const auto &item = orderInfo.items.at(i);
            itemsTable->setItem(i, 0, new QTableWidgetItem(item.bookTitle));
            QTableWidgetItem *quantityItem = new QTableWidgetItem(QString::number(item.quantity));
            quantityItem->setTextAlignment(Qt::AlignCenter);
            itemsTable->setItem(i, 1, quantityItem);
            QTableWidgetItem *priceItem = new QTableWidgetItem(QLocale::system().toString(item.pricePerUnit, 'f', 2) + tr(" грн"));
            priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            itemsTable->setItem(i, 2, priceItem);
        }
        itemsTable->resizeRowsToContents();
        // Встановлюємо висоту таблиці на основі вмісту + заголовок
        int tableHeight = itemsTable->horizontalHeader()->height();
        for(int i=0; i<itemsTable->rowCount(); ++i) {
            tableHeight += itemsTable->rowHeight(i);
        }
        // Обмежуємо максимальну висоту таблиці товарів, щоб уникнути надмірного розтягування картки
        itemsTable->setMaximumHeight(tableHeight + 5 < 300 ? tableHeight + 5 : 300); // Збільшено макс. висоту до 300px

        itemsLayout->addWidget(itemsTable);
        mainLayout->addWidget(itemsGroup);
    }

    // --- Статуси замовлення (Таблиця) ---
    if (!orderInfo.statuses.isEmpty()) {
        QGroupBox *statusGroup = new QGroupBox(tr("Історія статусів"));
        statusGroup->setStyleSheet("QGroupBox { color: black; font-weight: bold; }"); // Додано стиль для групи
        QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);

        QTableWidget *statusTable = new QTableWidget(orderInfo.statuses.size(), 3);
        statusTable->setHorizontalHeaderLabels({tr("Статус"), tr("Дата"), tr("Номер ТТН")});
        statusTable->verticalHeader()->setVisible(false);
        statusTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        statusTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        statusTable->setAlternatingRowColors(true);
        statusTable->horizontalHeader()->setStretchLastSection(false);
        statusTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        statusTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        statusTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        statusTable->setStyleSheet("QTableWidget { border: 1px solid #dee2e6; gridline-color: #e9ecef; color: black; } QHeaderView::section { background-color: #f1f3f5; padding: 4px; border: none; border-bottom: 1px solid #dee2e6; color: black; } "); // Додано color: black


        for (int i = 0; i < orderInfo.statuses.size(); ++i) {
            const auto &status = orderInfo.statuses.at(i);
            statusTable->setItem(i, 0, new QTableWidgetItem(status.status));
            statusTable->setItem(i, 1, new QTableWidgetItem(QLocale::system().toString(status.statusDate, QLocale::ShortFormat)));
            statusTable->setItem(i, 2, new QTableWidgetItem(status.trackingNumber.isEmpty() ? "-" : status.trackingNumber));
        }
        statusTable->resizeRowsToContents();
        // Встановлюємо висоту таблиці статусів
        int tableHeight = statusTable->horizontalHeader()->height();
        for(int i=0; i<statusTable->rowCount(); ++i) {
            tableHeight += statusTable->rowHeight(i);
        }
        // Обмежуємо максимальну висоту таблиці статусів
        statusTable->setMaximumHeight(tableHeight + 5 < 250 ? tableHeight + 5 : 250); // Збільшено макс. висоту до 250px

        statusLayout->addWidget(statusTable);
        mainLayout->addWidget(statusGroup);
    }

    orderFrame->setLayout(mainLayout);
    return orderFrame;
}

// Метод для відображення списку замовлень
void MainWindow::displayOrders(const QList<OrderDisplayInfo> &orders)
{
    if (!ui->ordersContentLayout) {
        qWarning() << "displayOrders: ordersContentLayout is null!";
        // Можна показати помилку в statusBar
        ui->statusBar->showMessage(tr("Помилка інтерфейсу: Не вдалося відобразити замовлення."), 5000);
        return;
    }

    // Очищаємо layout від попередніх замовлень та спейсера
    clearLayout(ui->ordersContentLayout);

    if (orders.isEmpty()) {
        QLabel *noOrdersLabel = new QLabel(tr("У вас ще немає замовлень."), ui->ordersContainerWidget);
        noOrdersLabel->setAlignment(Qt::AlignCenter);
        noOrdersLabel->setStyleSheet("font-style: italic; color: black; padding: 20px;"); // Змінено колір на чорний
        ui->ordersContentLayout->addWidget(noOrdersLabel);
        // Додаємо спейсер знизу, щоб мітка була по центру вертикально
        ui->ordersContentLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    } else {
        for (const OrderDisplayInfo &orderInfo : orders) {
            QWidget *orderCard = createOrderWidget(orderInfo);
            if (orderCard) {
                ui->ordersContentLayout->addWidget(orderCard);
            }
        }
        // Додаємо спейсер в кінці, щоб притиснути картки вгору
        ui->ordersContentLayout->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding)); // Зменшено висоту спейсера
    }

    // Оновлюємо геометрію контейнера
    ui->ordersContainerWidget->updateGeometry();
    // Переконуємось, що ScrollArea оновилась, якщо вміст змінився
    QCoreApplication::processEvents(); // Даємо можливість обробити події перед прокруткою
    ui->ordersScrollArea->ensureVisible(0,0); // Прокручуємо до верху
}

// Метод для завантаження та відображення замовлень
void MainWindow::loadAndDisplayOrders()
{
    qInfo() << "Завантаження замовлень для customer ID:" << m_currentCustomerId;
    if (!m_dbManager) {
        qCritical() << "loadAndDisplayOrders: DatabaseManager is null!";
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        displayOrders({}); // Показати порожній список з помилкою
        return;
    }
    if (m_currentCustomerId <= 0) {
        qWarning() << "loadAndDisplayOrders: Invalid customer ID:" << m_currentCustomerId;
        QMessageBox::warning(this, tr("Помилка"), tr("Неможливо завантажити замовлення, користувач не визначений."));
        displayOrders({}); // Показати порожній список з помилкою
        return;
    }

    QList<OrderDisplayInfo> orders = m_dbManager->getCustomerOrdersForDisplay(m_currentCustomerId);
    qInfo() << "Завантажено" << orders.size() << "замовлень.";
    displayOrders(orders); // Відображаємо отримані замовлення

    if (m_dbManager->lastError().isValid()) {
         ui->statusBar->showMessage(tr("Помилка при завантаженні замовлень: %1").arg(m_dbManager->lastError().text()), 5000);
    } else if (!orders.isEmpty()) {
         ui->statusBar->showMessage(tr("Замовлення успішно завантажено."), 3000);
    } else {
         // Якщо помилки не було, але замовлень 0, показуємо відповідне повідомлення
         ui->statusBar->showMessage(tr("У вас ще немає замовлень."), 3000);
    }
}


// Заповнення полів сторінки профілю даними
void MainWindow::populateProfilePanel(const CustomerProfileInfo &profileInfo)
{
    // Перевіряємо, чи вказівники на QLabel існують (важливо після змін в UI)
    if (!ui->profileFirstNameLabel || !ui->profileLastNameLabel || !ui->profileEmailLabel ||
        !ui->profilePhoneLabel || !ui->profileAddressLabel || !ui->profileJoinDateLabel ||
        !ui->profileLoyaltyLabel || !ui->profilePointsLabel)
    {
        qWarning() << "populateProfilePanel: One or more profile labels are null!";
        // Не показуємо QMessageBox тут, щоб не заважати користувачу
        // Просто виходимо або встановлюємо текст помилки
        if(ui->pageProfile) { // Спробуємо показати помилку на самій сторінці
             clearLayout(ui->profilePageLayout); // Очистимо, щоб не було старих даних
             QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося знайти поля для відображення профілю."), ui->pageProfile);
             errorLabel->setAlignment(Qt::AlignCenter);
             errorLabel->setWordWrap(true);
             ui->profilePageLayout->addWidget(errorLabel);
        }
        return;
    }

     // Перевіряємо, чи дані взагалі були знайдені
    if (!profileInfo.found || profileInfo.customerId <= 0) {
        // Заповнюємо поля текстом про помилку або відсутність даних
        const QString errorText = tr("(Помилка завантаження або дані відсутні)");
        ui->profileFirstNameLabel->setText(errorText);
        ui->profileLastNameLabel->setText(errorText);
        ui->profileEmailLabel->setText(errorText);
        ui->profilePhoneLabel->setText(errorText);
        ui->profileAddressLabel->setText(errorText);
        ui->profileJoinDateLabel->setText(errorText);
        ui->profileLoyaltyLabel->setText(errorText);
        ui->profilePointsLabel->setText("-");
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з mainwindow.ui (всередині pageProfile)
    ui->profileFirstNameLabel->setText(profileInfo.firstName.isEmpty() ? tr("(не вказано)") : profileInfo.firstName);
    ui->profileLastNameLabel->setText(profileInfo.lastName.isEmpty() ? tr("(не вказано)") : profileInfo.lastName);
    ui->profileEmailLabel->setText(profileInfo.email); // Email має бути завжди
    ui->profilePhoneLabel->setText(profileInfo.phone.isEmpty() ? tr("(не вказано)") : profileInfo.phone);
    ui->profileAddressLabel->setText(profileInfo.address.isEmpty() ? tr("(не вказано)") : profileInfo.address);
    ui->profileJoinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->profileLoyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->profilePointsLabel->setText(QString::number(profileInfo.loyaltyPoints));
}

// --- Кінець реалізації слотів та функцій ---
