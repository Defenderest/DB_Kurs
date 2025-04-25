#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QPushButton>
#include <QDebug>
#include <QGridLayout> // Для displayAuthors
#include <QSpacerItem> // Для displayAuthors

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
    viewBooksButton->setStyleSheet("QPushButton { background-color: #0078d4; color: white; border: none; border-radius: 8px; padding: 6px; font-size: 9pt; } QPushButton:hover { background-color: #106ebe; }"); // Збільшено border-radius
    viewBooksButton->setToolTip(tr("Переглянути книги автора %1 %2").arg(authorInfo.firstName, authorInfo.lastName));
    // connect(viewBooksButton, &QPushButton::clicked, this, [this, authorInfo](){ /* логіка перегляду книг автора */ });
    cardLayout->addWidget(viewBooksButton);

    // --- Додавання обробки кліків ---
    // Встановлюємо authorId як динамічну властивість для легкого доступу в eventFilter
    cardFrame->setProperty("authorId", authorInfo.authorId);
    // Встановлюємо фільтр подій на сам фрейм картки
    cardFrame->installEventFilter(this);
    // Змінюємо курсор при наведенні, щоб показати клікабельність
    cardFrame->setCursor(Qt::PointingHandCursor);


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
