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
#include <QLineEdit>        // Додано для QLineEdit у профілі
#include <QCompleter>       // Додано для автодоповнення
#include <QStringListModel> // Додано для моделі автодоповнення
#include <QListView>        // Додано для QListView (використовується в автодоповненні)
#include <QMouseEvent>      // Додано для подій миші
#include <QTextEdit>        // Додано для QTextEdit (опис книги)

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

    // Підключаємо зміну комбо-боксу статусу замовлень до перезавантаження списку
    connect(ui->orderStatusComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::loadAndDisplayOrders);

    // Підключаємо кнопки редагування та збереження профілю
    connect(ui->editProfileButton, &QPushButton::clicked, this, &MainWindow::on_editProfileButton_clicked);
    connect(ui->saveProfileButton, &QPushButton::clicked, this, &MainWindow::on_saveProfileButton_clicked);

    // Видалено з'єднання для кнопки профілю з хедера (якщо воно було)

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
    // loadAndDisplayOrders();

    // Встановлюємо початковий стан сторінки профілю (не в режимі редагування)
    setProfileEditingEnabled(false);

    // Налаштовуємо автодоповнення для глобального пошуку
    setupSearchCompleter();

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


    // --- Додавання обробки кліків ---
    // Встановлюємо bookId як динамічну властивість для легкого доступу в eventFilter
    cardFrame->setProperty("bookId", bookInfo.bookId);
    // Встановлюємо фільтр подій на сам фрейм картки
    cardFrame->installEventFilter(this);
    // Змінюємо курсор при наведенні, щоб показати клікабельність
    cardFrame->setCursor(Qt::PointingHandCursor);


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
    setProfileEditingEnabled(false); // Переконуємось, що режим редагування вимкнено при переході
}

// Слот для кнопки редагування профілю
void MainWindow::on_editProfileButton_clicked()
{
    setProfileEditingEnabled(true);
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
    // --- Обробка кліків на картках книг ---
    // Перевіряємо, чи об'єкт є QFrame (картка книги) і чи має властивість bookId
    if (qobject_cast<QFrame*>(watched) && watched->property("bookId").isValid()) {
        if (event->type() == QEvent::MouseButtonPress) {
            // Переконуємось, що це ліва кнопка миші
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                int bookId = watched->property("bookId").toInt();
                qInfo() << "Book card clicked, bookId:" << bookId;
                showBookDetails(bookId); // Викликаємо слот для показу деталей
                return true; // Подія оброблена
            }
        }
    }

    // Передаємо подію батьківському класу для стандартної обробки
    return QMainWindow::eventFilter(watched, event);
}


// Слот для відображення сторінки з деталями книги
void MainWindow::showBookDetails(int bookId)
{
    qInfo() << "Attempting to show details for book ID:" << bookId;
    if (bookId <= 0) {
        qWarning() << "Invalid book ID received:" << bookId;
        QMessageBox::warning(this, tr("Помилка"), tr("Некоректний ідентифікатор книги."));
        return;
    }
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        return;
    }
    if (!ui->bookDetailsPage) {
         QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Сторінка деталей книги не знайдена."));
         return;
    }

    // Отримуємо деталі книги з бази даних
    BookDetailsInfo bookDetails = m_dbManager->getBookDetails(bookId);

    if (!bookDetails.found) {
        QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію для книги з ID %1.").arg(bookId));
        return;
    }

    // Заповнюємо сторінку даними
    populateBookDetailsPage(bookDetails);

    // Переключаємо StackedWidget на сторінку деталей
    ui->contentStackedWidget->setCurrentWidget(ui->bookDetailsPage);
}

// Допоміжна функція для створення віджету коментаря (оновлений дизайн)
QWidget* MainWindow::createCommentWidget(const CommentDisplayInfo &commentInfo)
{
    // Основний контейнер коментаря
    QFrame *commentFrame = new QFrame();
    commentFrame->setObjectName("commentFrame"); // Для стилізації
    commentFrame->setFrameShape(QFrame::StyledPanel);
    commentFrame->setFrameShadow(QFrame::Plain); // Використовуємо тінь через стиль
    commentFrame->setLineWidth(0); // Рамка через стиль
    // Стиль з тінню, заокругленням та відступами
    commentFrame->setStyleSheet(R"(
        QFrame#commentFrame {
            background-color: #ffffff; /* Білий фон */
            border: 1px solid #e9ecef; /* Світло-сіра рамка */
            border-radius: 8px; /* Більше заокруглення */
            padding: 15px; /* Збільшені відступи */
            margin-bottom: 10px; /* Відступ між коментарями */
            /* Можна додати тінь, але це може вплинути на продуктивність */
            /* box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05); */
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(commentFrame);
    mainLayout->setSpacing(8); // Збільшений відступ між елементами
    mainLayout->setContentsMargins(0, 0, 0, 0); // Відступи керуються padding у стилі фрейму

    // --- Верхній рядок: Автор та Дата ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    // Ім'я автора (виділено)
    QLabel *authorLabel = new QLabel(commentInfo.authorName);
    authorLabel->setStyleSheet("font-weight: 600; font-size: 11pt; color: #343a40;"); // Жирний, трохи більший

    // Дата (менш помітна, праворуч)
    QLabel *dateLabel = new QLabel(QLocale::system().toString(commentInfo.commentDate, QLocale::ShortFormat));
    dateLabel->setStyleSheet("color: #868e96; font-size: 9pt;"); // Світліший сірий, менший шрифт
    dateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(authorLabel);
    headerLayout->addStretch(1); // Розтягувач між автором та датою
    headerLayout->addWidget(dateLabel);
    mainLayout->addLayout(headerLayout);

    // --- Рядок рейтингу (якщо є) ---
    if (commentInfo.rating > 0) {
        QLabel *ratingLabel = new QLabel();
        QString stars;
        for (int i = 0; i < 5; ++i) {
            // Використовуємо заповнену та порожню зірку
            stars += (i < commentInfo.rating) ? "★" : "☆";
        }
        ratingLabel->setText(stars);
        // Стиль для зірок (жовтий/золотий колір)
        ratingLabel->setStyleSheet("color: #ffc107; font-size: 13pt; margin-top: 2px; margin-bottom: 4px;");
        mainLayout->addWidget(ratingLabel);
    } else {
        // Можна додати невеликий відступ, якщо немає рейтингу, щоб вирівняти текст
        mainLayout->addSpacing(5);
    }

    // --- Текст коментаря ---
    QLabel *commentTextLabel = new QLabel(commentInfo.commentText);
    commentTextLabel->setWordWrap(true); // Перенесення слів обов'язкове
    commentTextLabel->setStyleSheet("color: #495057; font-size: 10pt; line-height: 1.5;"); // Стандартний текст, міжрядковий інтервал
    mainLayout->addWidget(commentTextLabel);

    // Встановлюємо layout для фрейму
    commentFrame->setLayout(mainLayout);
    return commentFrame;
}


// Заповнення сторінки деталей книги даними
void MainWindow::populateBookDetailsPage(const BookDetailsInfo &details)
{
    // Перевірка існування віджетів на сторінці деталей (використовуємо нове ім'я bookDetailDescriptionLabel)
    if (!ui->bookDetailCoverLabel || !ui->bookDetailTitleLabel || !ui->bookDetailAuthorLabel ||
        !ui->bookDetailGenreLabel || !ui->bookDetailPublisherLabel || !ui->bookDetailYearLabel ||
        !ui->bookDetailPagesLabel || !ui->bookDetailIsbnLabel || !ui->bookDetailPriceLabel ||
        !ui->bookDetailDescriptionLabel || !ui->bookDetailAddToCartButton) // Змінено QTextEdit на QLabel
    {
        qWarning() << "populateBookDetailsPage: One or more detail page widgets are null!";
        // Можна показати повідомлення про помилку на самій сторінці
        if(ui->bookDetailsPageLayout) {
            clearLayout(ui->bookDetailsPageLayout);
            QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося відобразити деталі книги."), ui->bookDetailsPage);
            ui->bookDetailsPageLayout->addWidget(errorLabel);
        }
        return;
    }

    // 1. Обкладинка
    QPixmap coverPixmap(details.coverImagePath);
    if (coverPixmap.isNull() || details.coverImagePath.isEmpty()) {
        ui->bookDetailCoverLabel->setText(tr("Немає\nобкладинки"));
        ui->bookDetailCoverLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: #555; border: 1px solid #ccc; border-radius: 4px; }");
    } else {
        ui->bookDetailCoverLabel->setPixmap(coverPixmap.scaled(ui->bookDetailCoverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->bookDetailCoverLabel->setStyleSheet("QLabel { background-color: transparent; border: 1px solid #ccc; border-radius: 4px; }"); // Забираємо фон
    }

    // 2. Текстові поля
    ui->bookDetailTitleLabel->setText(details.title.isEmpty() ? tr("(Без назви)") : details.title);
    ui->bookDetailAuthorLabel->setText(details.authors.isEmpty() ? tr("(Автор невідомий)") : details.authors);
    ui->bookDetailGenreLabel->setText(tr("Жанр: %1").arg(details.genre.isEmpty() ? "-" : details.genre));
    ui->bookDetailPublisherLabel->setText(tr("Видавництво: %1").arg(details.publisherName.isEmpty() ? "-" : details.publisherName));
    ui->bookDetailYearLabel->setText(tr("Рік видання: %1").arg(details.publicationDate.isValid() ? QString::number(details.publicationDate.year()) : "-"));
    ui->bookDetailPagesLabel->setText(tr("Сторінок: %1").arg(details.pageCount > 0 ? QString::number(details.pageCount) : "-"));
    ui->bookDetailIsbnLabel->setText(tr("ISBN: %1").arg(details.isbn.isEmpty() ? "-" : details.isbn));
    ui->bookDetailPriceLabel->setText(QString::number(details.price, 'f', 2) + tr(" грн"));
    ui->bookDetailDescriptionLabel->setText(details.description.isEmpty() ? tr("(Опис відсутній)") : details.description); // Використовуємо setText для QLabel

    // 3. Кнопка "Додати в кошик" (можна додати логіку або сховати, якщо немає в наявності)
    ui->bookDetailAddToCartButton->setEnabled(details.stockQuantity > 0);
    ui->bookDetailAddToCartButton->setToolTip(details.stockQuantity > 0 ? tr("Додати '%1' до кошика").arg(details.title) : tr("Немає в наявності"));
    // TODO: Підключити сигнал кнопки до слота додавання в кошик

    // 4. Рейтинг (поки що просто текст)
    // TODO: Розрахувати середній рейтинг з details.comments
    QString ratingText = tr("Рейтинг: (ще не розраховано)");
    if (!details.comments.isEmpty()) {
        double totalRating = 0;
        int ratedCount = 0;
        for(const auto& comment : details.comments) {
            if (comment.rating > 0) {
                totalRating += comment.rating;
                ratedCount++;
            }
        }
        if (ratedCount > 0) {
            double avgRating = totalRating / ratedCount;
            QString stars;
            int fullStars = qRound(avgRating);
            for(int i=0; i<5; ++i) stars += (i < fullStars ? "⭐" : "☆");
            ratingText = tr("Середній рейтинг: %1 (%2 відгуків)").arg(stars).arg(ratedCount);
        } else {
            ratingText = tr("Рейтинг: (ще немає оцінок)");
        }
    }
    ui->bookDetailRatingLabel->setText(ratingText);


    // 5. Коментарі
    // Очищаємо попередні коментарі та спейсер
    clearLayout(ui->commentsListLayout); // Використовуємо правильний layout

    if (details.comments.isEmpty()) {
        QLabel *noCommentsLabel = new QLabel(tr("Відгуків ще немає. Будьте першим!"));
        noCommentsLabel->setAlignment(Qt::AlignCenter);
        noCommentsLabel->setStyleSheet("color: #6c757d; font-style: italic; padding: 20px;");
        ui->commentsListLayout->addWidget(noCommentsLabel);
        // Додаємо спейсер, щоб мітка була по центру, якщо немає коментарів
        ui->commentsListLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    } else {
        for (const CommentDisplayInfo &commentInfo : details.comments) {
            QWidget *commentWidget = createCommentWidget(commentInfo);
            if (commentWidget) {
                ui->commentsListLayout->addWidget(commentWidget);
            }
        }
        // Додаємо спейсер в кінці, щоб притиснути коментарі вгору
        ui->commentsListLayout->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    // Оновлюємо геометрію контейнера коментарів
    ui->commentsContainerWidget->updateGeometry();


    qInfo() << "Book details page populated for:" << details.title;
}


// Налаштування автодоповнення для глобального пошуку
void MainWindow::setupSearchCompleter()
{
    if (!ui->globalSearchLineEdit) {
        qWarning() << "setupSearchCompleter: globalSearchLineEdit is null!";
        return;
    }

    m_searchSuggestionModel = new QStringListModel(this); // Модель для пропозицій
    m_searchCompleter = new QCompleter(m_searchSuggestionModel, this); // Комплітер

    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive); // Нечутливий до регістру
    m_searchCompleter->setCompletionMode(QCompleter::PopupCompletion); // Випадаючий список
    m_searchCompleter->setFilterMode(Qt::MatchStartsWith); // Пропозиції, що починаються з введеного тексту
    // m_searchCompleter->setPopup(ui->globalSearchLineEdit->findChild<QListView*>()); // Використовуємо стандартний popup - findChild може бути ненадійним, краще залишити стандартний popup комплітера

    ui->globalSearchLineEdit->setCompleter(m_searchCompleter);

    // Підключаємо сигнал зміни тексту до слота оновлення пропозицій
    connect(ui->globalSearchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchSuggestions);

    qInfo() << "Search completer setup complete for globalSearchLineEdit.";
}

// Слот для оновлення пропозицій пошуку при зміні тексту
void MainWindow::updateSearchSuggestions(const QString &text)
{
    if (!m_dbManager || !m_searchSuggestionModel) {
        return; // Немає менеджера БД або моделі
    }

    // Отримуємо пропозиції, тільки якщо текст достатньо довгий
    if (text.length() < 2) { // Мінімальна довжина для пошуку (можна змінити)
        m_searchSuggestionModel->setStringList({}); // Очищаємо модель, якщо текст короткий
        return;
    }

    QStringList suggestions = m_dbManager->getSearchSuggestions(text);
    m_searchSuggestionModel->setStringList(suggestions); // Оновлюємо модель пропозиціями

    // qInfo() << "Updated search suggestions for text:" << text << "Count:" << suggestions.count();
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

    // Форматуємо дату більш явно і перевіряємо валідність
    QString dateString = orderInfo.orderDate.isValid()
                         ? QLocale::system().toString(orderInfo.orderDate, "dd.MM.yyyy hh:mm")
                         : tr("(невідома дата)");
    QLabel *orderDateLabel = new QLabel(tr("Дата: %1").arg(dateString));
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

    // Отримуємо всі замовлення
    QList<OrderDisplayInfo> allOrders = m_dbManager->getCustomerOrdersForDisplay(m_currentCustomerId);
    qInfo() << "Завантажено" << allOrders.size() << "замовлень.";

    // Отримуємо вибраний статус для фільтрації
    QString statusFilter = ui->orderStatusComboBox->currentText();
    QList<OrderDisplayInfo> filteredOrders;

    if (statusFilter == tr("Всі статуси")) {
        filteredOrders = allOrders; // Показуємо всі, якщо вибрано "Всі статуси"
    } else {
        // Фільтруємо замовлення за останнім статусом
        for (const OrderDisplayInfo &order : allOrders) {
            if (!order.statuses.isEmpty()) {
                // Останній статус - це останній елемент у списку, оскільки вони відсортовані за датою в БД
                const OrderStatusDisplayInfo &lastStatus = order.statuses.last();
                if (lastStatus.status == statusFilter) {
                    filteredOrders.append(order);
                }
            }
            // Якщо у замовлення немає статусів, воно не потрапить у фільтрований список (крім "Всі статуси")
        }
        qInfo() << "Відфільтровано" << filteredOrders.size() << "замовлень за статусом:" << statusFilter;
    }


    displayOrders(filteredOrders); // Відображаємо відфільтровані замовлення

    if (m_dbManager->lastError().isValid()) {
         ui->statusBar->showMessage(tr("Помилка при завантаженні замовлень: %1").arg(m_dbManager->lastError().text()), 5000);
    } else if (!filteredOrders.isEmpty()) { // Використовуємо відфільтрований список
         ui->statusBar->showMessage(tr("Замовлення успішно завантажено."), 3000);
    } else {
         // Якщо помилки не було, але замовлень 0 (після фільтрації), показуємо відповідне повідомлення
         ui->statusBar->showMessage(tr("У вас ще немає замовлень."), 3000);
    }
}


// Заповнення полів сторінки профілю даними
void MainWindow::populateProfilePanel(const CustomerProfileInfo &profileInfo)
{
    // Перевіряємо, чи вказівники на віджети існують (використовуємо нові LineEdit)
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profileEmailLabel ||
        !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit || !ui->profileJoinDateLabel ||
        !ui->profileLoyaltyLabel || !ui->profilePointsLabel)
    {
        qWarning() << "populateProfilePanel: One or more profile widgets are null!";
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
        // Заповнюємо поля текстом про помилку або відсутність даних (використовуємо нові LineEdit)
        const QString errorText = tr("(Помилка завантаження або дані відсутні)");
        ui->profileFirstNameLineEdit->setText("");
        ui->profileFirstNameLineEdit->setPlaceholderText(errorText);
        ui->profileFirstNameLineEdit->setEnabled(false);
        ui->profileLastNameLineEdit->setText("");
        ui->profileLastNameLineEdit->setPlaceholderText(errorText);
        ui->profileLastNameLineEdit->setEnabled(false);
        ui->profileEmailLabel->setText(errorText);
        ui->profilePhoneLineEdit->setText("");
        ui->profilePhoneLineEdit->setPlaceholderText(errorText);
        ui->profilePhoneLineEdit->setEnabled(false);
        ui->profileAddressLineEdit->setText("");
        ui->profileAddressLineEdit->setPlaceholderText(errorText);
        ui->profileAddressLineEdit->setEnabled(false);
        ui->profileJoinDateLabel->setText(errorText);
        ui->profileLoyaltyLabel->setText(errorText);
        ui->profilePointsLabel->setText("-");
        ui->saveProfileButton->setEnabled(false); // Блокуємо кнопку збереження
        return;
    }

    // Заповнюємо поля, використовуючи імена віджетів з mainwindow.ui (використовуємо нові LineEdit)
    ui->profileFirstNameLineEdit->setText(profileInfo.firstName);
    ui->profileFirstNameLineEdit->setPlaceholderText(tr("Введіть ім'я"));
    ui->profileFirstNameLineEdit->setEnabled(true);
    ui->profileLastNameLineEdit->setText(profileInfo.lastName);
    ui->profileLastNameLineEdit->setPlaceholderText(tr("Введіть прізвище"));
    ui->profileLastNameLineEdit->setEnabled(true);
    ui->profileEmailLabel->setText(profileInfo.email); // Email залишається QLabel
    ui->profilePhoneLineEdit->setText(profileInfo.phone);
    ui->profilePhoneLineEdit->setPlaceholderText(tr("Введіть номер телефону"));
    ui->profilePhoneLineEdit->setEnabled(true);
    ui->profileAddressLineEdit->setText(profileInfo.address);
    ui->profileAddressLineEdit->setPlaceholderText(tr("Введіть адресу"));
    ui->profileAddressLineEdit->setEnabled(true);
    ui->profileJoinDateLabel->setText(profileInfo.joinDate.isValid() ? profileInfo.joinDate.toString("dd.MM.yyyy") : tr("(невідомо)"));
    ui->profileLoyaltyLabel->setText(profileInfo.loyaltyProgram ? tr("Так") : tr("Ні"));
    ui->profilePointsLabel->setText(QString::number(profileInfo.loyaltyPoints));

    // Розблоковуємо кнопку збереження
    ui->saveProfileButton->setEnabled(true);

    // Поля, які не редагуються (Email, Дата реєстрації, Лояльність), можна зробити візуально неактивними
    ui->profileEmailLabel->setEnabled(false);
    ui->profileJoinDateLabel->setEnabled(false);
    ui->profileLoyaltyLabel->setEnabled(false);
    ui->profilePointsLabel->setEnabled(false);

    // Встановлюємо початковий стан редагування (зазвичай false)
    // setProfileEditingEnabled(false); // Перенесено в on_navProfileButton_clicked та конструктор
}


// Функція для ввімкнення/вимкнення режиму редагування профілю
void MainWindow::setProfileEditingEnabled(bool enabled)
{
    // Перевірка існування віджетів
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profilePhoneLineEdit ||
        !ui->profileAddressLineEdit || !ui->editProfileButton || !ui->saveProfileButton)
    {
        qWarning() << "setProfileEditingEnabled: One or more profile widgets are null!";
        return;
    }

    // Вмикаємо/вимикаємо редагування полів
    ui->profileFirstNameLineEdit->setReadOnly(!enabled);
    ui->profileLastNameLineEdit->setReadOnly(!enabled);
    ui->profilePhoneLineEdit->setReadOnly(!enabled);
    ui->profileAddressLineEdit->setReadOnly(!enabled);

    // Показуємо/ховаємо кнопки
    ui->editProfileButton->setVisible(!enabled);
    ui->saveProfileButton->setVisible(enabled);

    // Змінюємо стиль полів для візуального розрізнення (опціонально)
    QString lineEditStyle = enabled
        ? "QLineEdit { background-color: white; border: 1px solid #86b7fe; }" // Стиль при редагуванні
        : "QLineEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; }"; // Стиль при читанні
    ui->profileFirstNameLineEdit->setStyleSheet(lineEditStyle);
    ui->profileLastNameLineEdit->setStyleSheet(lineEditStyle);
    ui->profilePhoneLineEdit->setStyleSheet(lineEditStyle);
    ui->profileAddressLineEdit->setStyleSheet(lineEditStyle);

    // Встановлюємо фокус на перше поле при ввімкненні редагування
    if (enabled) {
        ui->profileFirstNameLineEdit->setFocus();
    }
}


// Слот для кнопки збереження змін у профілі
void MainWindow::on_saveProfileButton_clicked()
{
    qInfo() << "Attempting to save profile changes for customer ID:" << m_currentCustomerId;

    if (m_currentCustomerId <= 0) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Неможливо зберегти зміни, користувач не визначений."));
        return;
    }
    if (!m_dbManager) {
         QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних. Неможливо зберегти зміни."));
         return;
    }
    // Перевіряємо вказівники на нові LineEdit
    if (!ui->profileFirstNameLineEdit || !ui->profileLastNameLineEdit || !ui->profilePhoneLineEdit || !ui->profileAddressLineEdit) {
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося знайти одне або декілька полів профілю."));
        return;
    }

    // Отримуємо нові значення з полів
    QString newFirstName = ui->profileFirstNameLineEdit->text().trimmed();
    QString newLastName = ui->profileLastNameLineEdit->text().trimmed();
    QString newPhoneNumber = ui->profilePhoneLineEdit->text().trimmed();
    QString newAddress = ui->profileAddressLineEdit->text().trimmed();

    // Валідація (приклад)
    if (newFirstName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Ім'я не може бути порожнім."));
        ui->profileFirstNameLineEdit->setFocus();
        return;
    }
    if (newLastName.isEmpty()) {
        QMessageBox::warning(this, tr("Збереження профілю"), tr("Прізвище не може бути порожнім."));
        ui->profileLastNameLineEdit->setFocus();
        return;
    }
    // TODO: Додати валідацію номера телефону та адреси

    // Викликаємо методи DatabaseManager для оновлення
    bool nameSuccess = m_dbManager->updateCustomerName(m_currentCustomerId, newFirstName, newLastName);
    bool phoneSuccess = m_dbManager->updateCustomerPhone(m_currentCustomerId, newPhoneNumber);
    bool addressSuccess = m_dbManager->updateCustomerAddress(m_currentCustomerId, newAddress);

    if (nameSuccess && phoneSuccess && addressSuccess) {
        ui->statusBar->showMessage(tr("Дані профілю успішно оновлено!"), 5000);
        qInfo() << "Profile data updated successfully for customer ID:" << m_currentCustomerId;
        setProfileEditingEnabled(false); // Вимикаємо режим редагування після успішного збереження
        // Можна перезавантажити дані, щоб переконатися, що все відображається коректно
        // CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
        // populateProfilePanel(profile); // Це оновить поля, але знову вимкне редагування
    } else {
        QString errorMessage = tr("Не вдалося оновити дані профілю:\n");
        if (!nameSuccess) errorMessage += tr("- Помилка оновлення імені/прізвища.\n");
        if (!phoneSuccess) errorMessage += tr("- Помилка оновлення телефону.\n");
        if (!addressSuccess) errorMessage += tr("- Помилка оновлення адреси.\n");
        errorMessage += tr("\nПеревірте журнал помилок.");
        QMessageBox::critical(this, tr("Помилка збереження"), errorMessage);
        qWarning() << "Failed to update profile data for customer ID:" << m_currentCustomerId << "Error:" << m_dbManager->lastError().text();
    }
}


// --- Кінець реалізації слотів та функцій ---
