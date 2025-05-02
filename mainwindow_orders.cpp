#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLocale>
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication> // Для processEvents
#include <QScrollArea> // Для ensureVisible
#include <QComboBox> // Для loadAndDisplayOrders
#include <QDateEdit> // Для loadAndDisplayOrders
#include <QStatusBar> // Для loadAndDisplayOrders
#include <QPropertyAnimation> // Для анімації панелі деталей

// Слот для кнопки навігації "Замовлення"
void MainWindow::on_navOrdersButton_clicked()
{
    ui->contentStackedWidget->setCurrentWidget(ui->ordersPage); // Переключаємо на сторінку замовлень
    loadAndDisplayOrders(); // Завантажуємо та відображаємо замовлення
}

// Метод для створення віджету картки замовлення (Новий дизайн)
QWidget* MainWindow::createOrderWidget(const OrderDisplayInfo &orderInfo)
{
    // Основний віджет-контейнер для картки замовлення
    QFrame *orderCard = new QFrame();
    orderCard->setObjectName("orderCardWidget"); // Ім'я для застосування стилів з UI
    orderCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Розширюється по ширині, фіксована висота

    // Головний горизонтальний layout картки
    QHBoxLayout *mainLayout = new QHBoxLayout(orderCard);
    mainLayout->setSpacing(15); // Відступ між основними блоками
    mainLayout->setContentsMargins(0, 0, 0, 0); // Відступи керуються стилем QFrame#orderCardWidget

    // --- Ліва частина: ID та Дата ---
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4); // Менший відступ між ID та датою

    QLabel *idLabel = new QLabel(tr("Замовлення №%1").arg(orderInfo.orderId));
    idLabel->setObjectName("orderIdLabel"); // Ім'я для стилів

    // Використовуємо QLocale::ShortFormat і перевірку isValid()
    QString dateString = orderInfo.orderDate.isValid()
                         ? QLocale::system().toString(orderInfo.orderDate, QLocale::ShortFormat) // Дата і час відповідно до локалі
                         : tr("(невідома дата)");
    QLabel *dateLabel = new QLabel(dateString);
    dateLabel->setObjectName("orderDateLabel"); // Ім'я для стилів

    infoLayout->addWidget(idLabel);
    infoLayout->addWidget(dateLabel);
    infoLayout->addStretch(1); // Притискає ID та дату вгору

    mainLayout->addLayout(infoLayout, 1); // Додаємо ліву частину (з розтягуванням)

    // --- Центральна частина: Сума ---
    QLabel *totalLabel = new QLabel(QString::number(orderInfo.totalAmount, 'f', 2) + tr(" грн"));
    totalLabel->setObjectName("orderTotalLabel"); // Ім'я для стилів
    totalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // Вирівнювання по правому краю
    mainLayout->addWidget(totalLabel); // Додаємо суму

    // --- Права частина: Статус та Кнопка ---
    QVBoxLayout *statusLayout = new QVBoxLayout();
    statusLayout->setSpacing(8); // Відступ між статусом та кнопкою
    statusLayout->setAlignment(Qt::AlignRight); // Вирівнюємо вміст по правому краю

    QLabel *statusLabel = new QLabel();
    statusLabel->setObjectName("orderStatusLabel"); // Ім'я для стилів

    // Визначаємо текст та CSS-властивість статусу
    QString statusText = tr("Невідомо");
    QString statusCss = "unknown"; // Статус для CSS (має бути lowercase)
    if (!orderInfo.statuses.isEmpty()) {
        // Беремо останній статус
        statusText = orderInfo.statuses.last().status;
        // TODO: Перетворити статус з БД на відповідний CSS-клас (new, processing, shipped, delivered, cancelled)
        // Припускаємо, що вони співпадають після переведення в нижній регістр
        statusCss = statusText.toLower();
        // Приклад простого мапінгу (якщо назви відрізняються):
        // if (statusText == "В обробці") statusCss = "processing";
        // else if (statusText == "Відправлено") statusCss = "shipped";
        // ... і т.д.
    }
    statusLabel->setText(statusText);
    statusLabel->setProperty("status", statusCss); // Встановлюємо властивість для CSS
    statusLabel->ensurePolished(); // Застосовуємо стиль негайно

    // Кнопка "Деталі" (використовує стиль viewOrderDetailsButton з UI)
    QPushButton *detailsButton = new QPushButton(tr("Деталі"));
    detailsButton->setObjectName("viewOrderDetailsButton"); // Ім'я для стилів
    detailsButton->setCursor(Qt::PointingHandCursor);
    // Підключаємо сигнал кнопки до нового слота для показу деталей замовлення
    connect(detailsButton, &QPushButton::clicked, this, [this, orderId = orderInfo.orderId]() {
        showOrderDetails(orderId); // Викликаємо новий слот
    });
    detailsButton->setToolTip(tr("Переглянути деталі замовлення №%1").arg(orderInfo.orderId));

    statusLayout->addWidget(statusLabel, 0, Qt::AlignRight); // Додаємо статус
    statusLayout->addWidget(detailsButton, 0, Qt::AlignRight); // Додаємо кнопку

    mainLayout->addLayout(statusLayout); // Додаємо праву частину

    orderCard->setLayout(mainLayout);
    return orderCard;
}

// Метод для відображення списку замовлень (Новий дизайн)
void MainWindow::displayOrders(const QList<OrderDisplayInfo> &orders)
{
    // Перевіряємо наявність необхідних віджетів
    if (!ui->ordersContentLayout || !ui->emptyOrdersLabel || !ui->ordersScrollArea) {
        qWarning() << "displayOrders: Required widgets (ordersContentLayout, emptyOrdersLabel, or ordersScrollArea) are null!";
        ui->statusBar->showMessage(tr("Помилка інтерфейсу: Не вдалося відобразити замовлення."), 5000);
        return;
    }

    // Очищаємо layout від попередніх карток замовлень
    clearLayout(ui->ordersContentLayout);

    bool isEmpty = orders.isEmpty();

    // Показуємо/ховаємо мітку про порожній список та область прокрутки
    // Додаємо додаткову перевірку перед використанням, щоб уникнути виключення
    if (ui->emptyOrdersLabel) {
        ui->emptyOrdersLabel->setVisible(isEmpty);
    } else {
        qWarning() << "displayOrders: emptyOrdersLabel was null during the 'if' check.";
    }
    // Аналогічна перевірка для ordersScrollArea (про всяк випадок)
    if (ui->ordersScrollArea) {
        ui->ordersScrollArea->setVisible(!isEmpty); // Ховаємо ScrollArea, якщо список порожній
    } else {
         qWarning() << "displayOrders: ordersScrollArea is unexpectedly null right before setVisible()!";
    }


    if (!isEmpty) {
        // Додаємо картки для кожного замовлення
        for (const OrderDisplayInfo &orderInfo : orders) {
            QWidget *orderCard = createOrderWidget(orderInfo); // Використовуємо оновлену функцію
            if (orderCard) {
                ui->ordersContentLayout->addWidget(orderCard);
            }
        }
        // Додаємо розтягувач, щоб притиснути картки вгору, якщо їх мало
        ui->ordersContentLayout->addStretch(1);
    }
 
    // Оновлюємо геометрію контейнера, щоб ScrollArea знала розмір
    ui->ordersContainerWidget->adjustSize();
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

    // // Отримуємо вибраний статус та дату для фільтрації - ВІДЖЕТИ ВИДАЛЕНО З UI
    // QString statusFilter = ui->orderStatusComboBox->currentText();
    // QDate dateFilter = ui->orderDateEdit->date(); // Отримуємо дату з QDateEdit
    QList<OrderDisplayInfo> filteredOrders; // Список для відфільтрованих замовлень (зараз не використовується)

    // Наразі фільтрація не застосовується, оскільки віджети видалено.
    // Просто копіюємо всі замовлення до списку для відображення.
    filteredOrders = allOrders;

    /* // Старий код фільтрації (закоментовано)
    for (const OrderDisplayInfo &order : allOrders) {
        bool statusMatch = false;
        bool dateMatch = false;

        // Перевірка статусу
        if (statusFilter == tr("Всі статуси")) {
            statusMatch = true;
        } else if (!order.statuses.isEmpty()) {
            // Порівнюємо останній статус замовлення з вибраним фільтром
            // Важливо: Переконайтесь, що рядки статусу з БД точно відповідають рядкам у ComboBox
            if (order.statuses.last().status == statusFilter) {
                statusMatch = true;
            }
        }

        // Перевірка дати (замовлення має бути створене НЕ РАНІШЕ вибраної дати)
        // Порівнюємо тільки дати, ігноруючи час
        if (!dateFilter.isNull() && order.orderDate.isValid()) {
             if (order.orderDate.date() >= dateFilter) {
                 dateMatch = true;
             }
        } else {
             // Якщо дата не вибрана (або дата замовлення невалідна), вважаємо, що дата підходить
             dateMatch = true;
        }


        // Додаємо замовлення, якщо воно відповідає обом фільтрам
        if (statusMatch && dateMatch) {
            filteredOrders.append(order);
        }
    }
    */
    // qInfo() << "Відфільтровано" << filteredOrders.size() << "замовлень за статусом:" << statusFilter << "та датою від:" << dateFilter.toString(Qt::ISODate); // Закоментовано, бо фільтри не використовуються


    displayOrders(filteredOrders); // Відображаємо замовлення (наразі всі)

    if (m_dbManager->lastError().isValid()) {
         ui->statusBar->showMessage(tr("Помилка при завантаженні замовлень: %1").arg(m_dbManager->lastError().text()), 5000);
    } else if (!filteredOrders.isEmpty()) { // Використовуємо відфільтрований список
         ui->statusBar->showMessage(tr("Замовлення успішно завантажено."), 3000);
    } else {
         // Якщо помилки не було, але замовлень 0 (після фільтрації), показуємо відповідне повідомлення
         ui->statusBar->showMessage(tr("У вас ще немає замовлень."), 3000);
    }
}

// Видалено старий слот showOrderDetailsPlaceholder


// --- Логіка панелі деталей замовлення ---

void MainWindow::showOrderDetails(int orderId)
{
    qInfo() << "Attempting to show details panel for order ID:" << orderId;
    // Перевіряємо наявність панелі та анімації (вони ініціалізуються в конструкторі MainWindow)
    // Додаємо this-> для явного доступу до членів класу
    if (!this->m_orderDetailsPanel || !this->m_orderDetailsAnimation || !this->m_dbManager) {
        qWarning() << "Order details panel, animation, or DB manager is null. Cannot show details.";
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося ініціалізувати панель деталей замовлення."));
        return;
    }

    // Отримуємо дані замовлення // Додаємо this->
    OrderDisplayInfo orderInfo = this->m_dbManager->getOrderDetailsById(orderId);
    if (!orderInfo.found) {
        qWarning() << "Order details not found for ID:" << orderId;
        QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти деталі для замовлення #%1.").arg(orderId));
        return;
    }

    // Заповнюємо панель даними // Додаємо this->
    this->populateOrderDetailsPanel(orderInfo);

    // Запускаємо анімацію показу, якщо панель ще не видима // Додаємо this->
    if (!this->m_isOrderDetailsPanelVisible) {
        if (this->m_orderDetailsAnimation->state() == QAbstractAnimation::Running) { // Додаємо this->
            this->m_orderDetailsAnimation->stop(); // Додаємо this->
        }

        this->m_orderDetailsPanel->setVisible(true); // Додаємо this->
        this->m_orderDetailsAnimation->setStartValue(this->m_orderDetailsPanel->width()); // Додаємо this->
        this->m_orderDetailsAnimation->setEndValue(this->m_orderDetailsPanelWidth); // Додаємо this->

        // Оновлюємо стан ПІСЛЯ завершення анімації // Додаємо this->
        disconnect(this->m_orderDetailsAnimation, &QPropertyAnimation::finished, this, nullptr); // Видаляємо старі з'єднання
        connect(this->m_orderDetailsAnimation, &QPropertyAnimation::finished, this, [this]() { // Додаємо this->
            // Використовуємо this-> всередині лямбди
            if (!this->m_isOrderDetailsPanelVisible) { // Перевірка, щоб уникнути подвійного встановлення
                 this->m_isOrderDetailsPanelVisible = true;
                 qDebug() << "Order details panel animation finished (show). State set to visible.";
            }
        });

        this->m_orderDetailsAnimation->start(); // Додаємо this->
        qDebug() << "Starting order details panel show animation.";
    } else {
         qDebug() << "Order details panel already visible. Content updated.";
    }
}

void MainWindow::populateOrderDetailsPanel(const OrderDisplayInfo &orderInfo)
{
    // Перевіряємо наявність віджетів панелі (вони ініціалізуються в конструкторі MainWindow)
    // Додаємо this->
    if (!this->m_orderDetailsIdLabel || !this->m_orderDetailsDateLabel || !this->m_orderDetailsTotalLabel ||
        !this->m_orderDetailsShippingLabel || !this->m_orderDetailsPaymentLabel || !this->m_orderDetailsItemsLayout ||
        !this->m_orderDetailsStatusLayout)
    {
        qWarning() << "Cannot populate order details panel: one or more widgets are null.";
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Не вдалося знайти елементи панелі деталей замовлення."));
        return;
    }

    // Встановлюємо основні дані // Додаємо this->
    this->m_orderDetailsIdLabel->setText(tr("<b>Замовлення №:</b> %1").arg(orderInfo.orderId));
    // Додаємо перевірку isValid() і використовуємо QLocale::ShortFormat
    QString dateText = orderInfo.orderDate.isValid()
                       ? QLocale::system().toString(orderInfo.orderDate, QLocale::ShortFormat)
                       : tr("(невідома дата)");
    this->m_orderDetailsDateLabel->setText(tr("<b>Дата:</b> %1").arg(dateText));
    this->m_orderDetailsTotalLabel->setText(tr("<b>Сума:</b> %1 грн").arg(QString::number(orderInfo.totalAmount, 'f', 2)));
    this->m_orderDetailsShippingLabel->setText(tr("<b>Адреса доставки:</b><br>%1").arg(orderInfo.shippingAddress.isEmpty() ? tr("(не вказано)") : orderInfo.shippingAddress));
    this->m_orderDetailsPaymentLabel->setText(tr("<b>Оплата:</b> %1").arg(orderInfo.paymentMethod.isEmpty() ? tr("(не вказано)") : orderInfo.paymentMethod));

    // Очищаємо списки товарів та статусів // Додаємо this->
    this->clearLayout(this->m_orderDetailsItemsLayout); // Використовуємо this-> для виклику методу
    this->clearLayout(this->m_orderDetailsStatusLayout); // Використовуємо this-> для виклику методу

    // Додаємо товари // Додаємо this->
    if (orderInfo.items.isEmpty()) {
        this->m_orderDetailsItemsLayout->addWidget(new QLabel(tr("(Немає товарів у замовленні)")));
    } else {
        for (const auto &item : orderInfo.items) {
            QString itemText = tr("%1 (%2 шт.) - %3 грн/шт.")
                                   .arg(item.bookTitle)
                                   .arg(item.quantity)
                                   .arg(QString::number(item.pricePerUnit, 'f', 2));
            QLabel *itemLabel = new QLabel(itemText);
            itemLabel->setWordWrap(true);
            itemLabel->setProperty("class", "orderItemLabel"); // Для стилізації
            this->m_orderDetailsItemsLayout->addWidget(itemLabel);
        }
    }
    // Видаляємо старий розтягувач, якщо він є, перед додаванням нового
    if (QLayoutItem *item = this->m_orderDetailsItemsLayout->takeAt(this->m_orderDetailsItemsLayout->count() - 1)) {
        if (!item->spacerItem()) { // Якщо це не спейсер, повертаємо назад
            this->m_orderDetailsItemsLayout->addItem(item);
        } else {
            delete item; // Видаляємо спейсер
        }
    }

    // Додаємо історію статусів // Додаємо this->
    if (orderInfo.statuses.isEmpty()) {
        this->m_orderDetailsStatusLayout->addWidget(new QLabel(tr("(Історія статусів відсутня)")));
    } else {
        for (const auto &status : orderInfo.statuses) {
            QString statusText = tr("%1 - %2")
                                     .arg(QLocale::system().toString(status.statusDate, QLocale::ShortFormat)) // Форматуємо дату
                                     .arg(status.status);
            if (!status.trackingNumber.isEmpty()) {
                statusText += tr(" (Трек: %1)").arg(status.trackingNumber);
            }
            QLabel *statusLabel = new QLabel(statusText);
            statusLabel->setWordWrap(true);
            statusLabel->setProperty("class", "orderStatusHistoryLabel"); // Для стилізації
            this->m_orderDetailsStatusLayout->addWidget(statusLabel);
        }
    }
    // Видаляємо старий розтягувач, якщо він є, перед додаванням нового
    if (QLayoutItem *item = this->m_orderDetailsStatusLayout->takeAt(this->m_orderDetailsStatusLayout->count() - 1)) {
        if (!item->spacerItem()) { // Якщо це не спейсер, повертаємо назад
            this->m_orderDetailsStatusLayout->addItem(item);
        } else {
            delete item; // Видаляємо спейсер
        }
    }

    // Додаємо розтягувач в кінці layout'ів, щоб притиснути вміст вгору
    this->m_orderDetailsItemsLayout->addStretch(1);
    this->m_orderDetailsStatusLayout->addStretch(1);

    qInfo() << "Order details panel populated for order ID:" << orderInfo.orderId;
}


void MainWindow::hideOrderDetailsPanel()
{
    qDebug() << "Attempting to hide order details panel.";
    // Додаємо this->
    if (!this->m_orderDetailsPanel || !this->m_orderDetailsAnimation) {
        qWarning() << "Order details panel or animation is null. Cannot hide.";
        return;
    }

    // Додаємо this->
    if (!this->m_isOrderDetailsPanelVisible) {
        qDebug() << "Order details panel already hidden.";
        return; // Вже приховано
    }

    // Додаємо this->
    if (this->m_orderDetailsAnimation->state() == QAbstractAnimation::Running) {
        this->m_orderDetailsAnimation->stop();
    }

    // Додаємо this->
    this->m_orderDetailsAnimation->setStartValue(this->m_orderDetailsPanel->width());
    this->m_orderDetailsAnimation->setEndValue(0); // Цільова ширина 0

    // Оновлюємо стан та ховаємо віджет ПІСЛЯ завершення анімації // Додаємо this->
    disconnect(this->m_orderDetailsAnimation, &QPropertyAnimation::finished, this, nullptr); // Видаляємо старі з'єднання
    connect(this->m_orderDetailsAnimation, &QPropertyAnimation::finished, this, [this]() { // Додаємо this->
        // Використовуємо this-> всередині лямбди
        if (this->m_isOrderDetailsPanelVisible) { // Перевірка, щоб уникнути подвійного встановлення
            this->m_orderDetailsPanel->setVisible(false); // Ховаємо віджет
            this->m_isOrderDetailsPanelVisible = false;
            qDebug() << "Order details panel animation finished (hide). State set to hidden.";
        }
    });

    // Додаємо this->
    this->m_orderDetailsAnimation->start();
    qDebug() << "Starting order details panel hide animation.";
}

// --- Кінець логіки панелі деталей замовлення ---
