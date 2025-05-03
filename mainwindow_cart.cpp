#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QSpinBox>
#include <QPushButton>
#include <QDebug>
#include <QMessageBox>
#include <QMap> // Для itemsMap у on_placeOrderButton_clicked
#include <QSpacerItem> // Для populateCartPage
#include <QStatusBar> // Для on_addToCartButtonClicked, removeCartItem
#include <QLineEdit> // Для on_placeOrderButton_clicked (profileAddressLineEdit)
#include <QPainter> // Для рисования значка на иконке
#include <QIcon>    // Для QIcon
#include "checkoutdialog.h" // Додано для діалогу оформлення

// --- Логіка кошика (Новий дизайн) ---

// Допоміжна функція для створення віджету одного товару в кошику
QWidget* MainWindow::createCartItemWidget(const CartItem &item, int bookId)
{
    // Основний фрейм для картки товару
    QFrame *itemFrame = new QFrame();
    itemFrame->setObjectName("cartItemFrame"); // Для стилізації
    itemFrame->setFrameShape(QFrame::StyledPanel); // Невидима рамка, керується стилем
    itemFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Розширюється по ширині, фіксована висота

    QHBoxLayout *mainLayout = new QHBoxLayout(itemFrame);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Відступи керуються стилем фрейму

    // 1. Обкладинка
    QLabel *coverLabel = new QLabel();
    coverLabel->setObjectName("cartItemCoverLabel");
    coverLabel->setAlignment(Qt::AlignCenter);
    QPixmap coverPixmap(item.book.coverImagePath);
    if (coverPixmap.isNull() || item.book.coverImagePath.isEmpty()) {
        coverLabel->setText(tr("Фото"));
        // Можна додати стиль для плейсхолдера, якщо потрібно
        // coverLabel->setStyleSheet("background-color: #e9ecef; color: #6c757d; border: 1px solid #dee2e6; border-radius: 4px;");
    } else {
        // Масштабуємо зображення до розміру QLabel (використовуємо minimumSize, визначений у стилі)
        // Переконуємось, що розмір QLabel встановлено (наприклад, через стиль або minimumSize)
        QSize labelSize = coverLabel->minimumSize(); // Беремо розмір з UI/стилю
        if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
             // Якщо розмір не встановлено, використовуємо типовий
             labelSize = QSize(60, 85);
             qWarning() << "Cart item cover label size not set, using default:" << labelSize;
        }
        coverLabel->setPixmap(coverPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        coverLabel->setText(""); // Прибираємо текст, якщо є зображення
    }
    mainLayout->addWidget(coverLabel);

    // 2. Інформація про книгу (Назва, Автор)
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    QLabel *titleLabel = new QLabel(item.book.title);
    titleLabel->setObjectName("cartItemTitleLabel");
    titleLabel->setWordWrap(true);
    QLabel *authorLabel = new QLabel(item.book.authors);
    authorLabel->setObjectName("cartItemAuthorLabel");
    authorLabel->setWordWrap(true);
    infoLayout->addWidget(titleLabel);
    infoLayout->addWidget(authorLabel);
    infoLayout->addStretch(1); // Притискає текст вгору
    mainLayout->addLayout(infoLayout, 2); // Даємо більше місця для назви/автора

    // 3. Ціна за одиницю
    QLabel *priceLabel = new QLabel(QString::number(item.book.price, 'f', 2) + tr(" грн"));
    priceLabel->setObjectName("cartItemPriceLabel");
    priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mainLayout->addWidget(priceLabel, 1); // Менше місця для ціни

    // 4. Кількість (SpinBox)
    QSpinBox *quantitySpinBox = new QSpinBox();
    quantitySpinBox->setObjectName("cartQuantitySpinBox");
    quantitySpinBox->setMinimum(1);
    quantitySpinBox->setMaximum(item.book.stockQuantity); // Обмеження по складу
    quantitySpinBox->setValue(item.quantity);
    quantitySpinBox->setAlignment(Qt::AlignCenter);
    quantitySpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows); // Повертаємо стандартні стрілки (це значення за замовчуванням)
    quantitySpinBox->setProperty("bookId", bookId); // Зберігаємо ID для слота
    connect(quantitySpinBox, &QSpinBox::valueChanged, this, [this, bookId](int newValue){
        updateCartItemQuantity(bookId, newValue);
    });
    mainLayout->addWidget(quantitySpinBox);

    // 5. Сума за позицію
    QLabel *subtotalLabel = new QLabel(QString::number(item.book.price * item.quantity, 'f', 2) + tr(" грн"));
    subtotalLabel->setObjectName("cartItemSubtotalLabel");
    subtotalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    subtotalLabel->setMinimumWidth(80); // Мінімальна ширина для суми
    mainLayout->addWidget(subtotalLabel, 1); // Менше місця для суми
    // Зберігаємо вказівник на мітку підсумку
    m_cartSubtotalLabels.insert(bookId, subtotalLabel);


    // 6. Кнопка видалення
    QPushButton *removeButton = new QPushButton(); // Використовуємо QPushButton для кращої стилізації
    removeButton->setObjectName("cartRemoveButton");
    removeButton->setToolTip(tr("Видалити '%1' з кошика").arg(item.book.title));
    removeButton->setCursor(Qt::PointingHandCursor);
    removeButton->setProperty("bookId", bookId);
    connect(removeButton, &QPushButton::clicked, this, [this, bookId](){
        removeCartItem(bookId);
    });
    mainLayout->addWidget(removeButton);

    itemFrame->setLayout(mainLayout);
    return itemFrame;
}


// Слот для кнопки "Додати в кошик" (залишається без змін)
void MainWindow::on_addToCartButtonClicked(int bookId)
{
    qInfo() << "Add to cart button clicked for book ID:" << bookId;
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        return;
    }

    // Отримуємо інформацію про книгу за допомогою нового методу
    BookDisplayInfo bookInfo = m_dbManager->getBookDisplayInfoById(bookId);

    if (!bookInfo.found) { // Перевіряємо прапорець found
         qWarning() << "Book with ID" << bookId << "not found for adding to cart.";
         QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію про книгу (ID: %1).").arg(bookId));
         return;
    }

    // Перевірка наявності на складі
    if (bookInfo.stockQuantity <= 0) {
        QMessageBox::information(this, tr("Немає в наявності"), tr("На жаль, книги '%1' зараз немає в наявності.").arg(bookInfo.title));
        return;
    }

    // Перевіряємо, чи товар вже є в кошику
    if (m_cartItems.contains(bookId)) {
        // Перевіряємо, чи не перевищить кількість наявну на складі
        if (m_cartItems[bookId].quantity + 1 > bookInfo.stockQuantity) {
             QMessageBox::information(this, tr("Обмеження кількості"), tr("Ви не можете додати більше одиниць книги '%1', ніж є на складі (%2).").arg(bookInfo.title).arg(bookInfo.stockQuantity));
             return;
        }
        // Збільшуємо кількість
        m_cartItems[bookId].quantity++;
        qInfo() << "Increased quantity for book ID" << bookId << "to" << m_cartItems[bookId].quantity;
    } else {
        // Додаємо новий товар
        CartItem newItem;
        newItem.book = bookInfo; // Зберігаємо інформацію про книгу
        newItem.quantity = 1;
        m_cartItems.insert(bookId, newItem);
        qInfo() << "Added new book ID" << bookId << "to cart.";
    }

    // --- Збереження в БД ---
    if (m_dbManager) {
        m_dbManager->addOrUpdateCartItem(m_currentCustomerId, bookId, m_cartItems[bookId].quantity);
    } else {
        qWarning() << "on_addToCartButtonClicked: DatabaseManager is null, cannot save cart item to DB.";
    }
    // --- Кінець збереження в БД ---


    // Оновлюємо іконку кошика
    updateCartIcon();

    // Показуємо повідомлення в статус барі
    ui->statusBar->showMessage(tr("Книгу '%1' додано до кошика.").arg(bookInfo.title), 3000);

    // Оновлюємо сторінку кошика, якщо вона відкрита
    if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
        populateCartPage();
    }
}

// Слот для кнопки кошика в хедері
void MainWindow::on_cartButton_clicked()
{
    qInfo() << "Cart button clicked. Navigating to cart page.";
    // Перевіряємо, чи існує сторінка кошика
    if (!ui->cartPage) {
        qWarning() << "Cart page widget not found in UI!";
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Сторінка кошика не знайдена."));
        return;
    }
    ui->contentStackedWidget->setCurrentWidget(ui->cartPage);
    populateCartPage(); // Заповнюємо/оновлюємо сторінку кошика
}

// Заповнення сторінки кошика (Новий дизайн)
void MainWindow::populateCartPage()
{
    qInfo() << "Populating cart page (new design)...";
    // Перевіряємо існування ключових віджетів нового дизайну
    if (!ui->cartScrollArea || !ui->cartItemsContainerWidget || !ui->cartItemsLayout || !ui->cartTotalTextLabel || !ui->placeOrderButton || !ui->cartTotalsWidget) {
        qWarning() << "populateCartPage: One or more new cart page widgets are null!";
        // Можна показати повідомлення про помилку
        if(ui->cartPage && ui->cartPage->layout()) {
             clearLayout(ui->cartPage->layout()); // Очистити, щоб не було старих елементів
             QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося відобразити кошик."), ui->cartPage);
             ui->cartPage->layout()->addWidget(errorLabel);
        }
        return;
    }

    // Очищаємо layout від попередніх елементів (карток товарів та спейсера/мітки)
    clearLayout(ui->cartItemsLayout);
    // Очищаємо мапу вказівників на мітки підсумків
    m_cartSubtotalLabels.clear();


    // Видаляємо мітку про порожній кошик, якщо вона була додана раніше
    QLabel* emptyCartLabel = ui->cartItemsContainerWidget->findChild<QLabel*>("emptyCartLabel");
    if(emptyCartLabel) {
        delete emptyCartLabel;
    }

    if (m_cartItems.isEmpty()) {
        qInfo() << "Cart is empty.";
        // Показуємо мітку про порожній кошик всередині контейнера
        QLabel *noItemsLabel = new QLabel(tr("🛒\n\nВаш кошик порожній.\nЧас додати щось цікаве!"), ui->cartItemsContainerWidget);
        noItemsLabel->setObjectName("emptyCartLabel"); // Для стилізації
        noItemsLabel->setAlignment(Qt::AlignCenter);
        noItemsLabel->setWordWrap(true);
        // Додаємо мітку безпосередньо в layout контейнера (не в cartItemsLayout)
        // Потрібно переконатися, що cartItemsContainerWidget має свій layout, якщо ми хочемо центрувати мітку
        // Простіше додати її в cartItemsLayout і потім спейсер
        ui->cartItemsLayout->addWidget(noItemsLabel);
        ui->cartItemsLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        ui->placeOrderButton->setEnabled(false); // Вимкнути кнопку замовлення
        ui->cartTotalTextLabel->setText(tr("Загальна сума: 0.00 грн")); // Скинути суму
        ui->cartTotalsWidget->setVisible(false); // Ховаємо блок з підсумками
        return;
    }

    // Показуємо блок з підсумками
    ui->cartTotalsWidget->setVisible(true);

    // Додаємо віджети для кожного товару
    for (auto it = m_cartItems.constBegin(); it != m_cartItems.constEnd(); ++it) {
        QWidget *itemWidget = createCartItemWidget(it.value(), it.key());
        if (itemWidget) {
            ui->cartItemsLayout->addWidget(itemWidget);
        }
    }

    // Додаємо спейсер в кінці, щоб притиснути картки вгору
    ui->cartItemsLayout->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    updateCartTotal(); // Оновлюємо загальну суму
    ui->placeOrderButton->setEnabled(true); // Увімкнути кнопку замовлення
    qInfo() << "Cart page populated with" << m_cartItems.size() << "items.";

    // Оновлюємо геометрію контейнера, щоб ScrollArea знала розмір вмісту
    ui->cartItemsContainerWidget->adjustSize();
}


// Оновлення загальної суми кошика (Новий дизайн)
void MainWindow::updateCartTotal()
{
    if (!ui->cartTotalTextLabel) return; // Перевірка

    double total = 0.0;
    // Рахуємо суму безпосередньо з m_cartItems
    for (const auto &item : m_cartItems) {
        total += item.book.price * item.quantity;
    }

    ui->cartTotalTextLabel->setText(tr("Загальна сума: %1 грн").arg(QString::number(total, 'f', 2)));
    qInfo() << "Cart total updated:" << total;
}

// Оновлення іконки кошика та значка кількості
void MainWindow::updateCartIcon()
{
    // Перевіряємо наявність кнопки та значка
    if (!ui->cartButton || !m_cartBadgeLabel) {
        qWarning() << "updateCartIcon: cartButton or m_cartBadgeLabel is null!";
        return;
    }

    int totalItems = 0;
    for (const auto &item : m_cartItems) {
        totalItems += item.quantity;
    }

    // 1. Встановлюємо базову іконку на кнопку
    const QString baseIconPath = "D:/projects/DB_Kurs/QtAPP/untitled/icons/cart.png";
    QIcon baseIcon(baseIconPath);
    if (!baseIcon.isNull()) {
        ui->cartButton->setIcon(baseIcon);
        // Переконуємось, що розмір іконки встановлено (можна зробити в конструкторі або тут)
        if (ui->cartButton->iconSize().isEmpty()) {
             ui->cartButton->setIconSize(QSize(24, 24)); // Встановлюємо розмір за замовчуванням
        }
    } else {
        qWarning() << "Failed to load base cart icon:" << baseIconPath;
        ui->cartButton->setText("?"); // Показати щось замість іконки
    }
    ui->cartButton->setText(""); // Текст кнопки завжди порожній

    // 2. Оновлюємо значок (badge)
    if (totalItems > 0) {
        m_cartBadgeLabel->setText(QString::number(totalItems));
        // Можна додати логіку для зміни розміру шрифта, якщо число велике
        // if (totalItems > 99) {
        //     QFont font = m_cartBadgeLabel->font();
        //     font.setPointSize(7); // Менший шрифт для 3+ цифр
        //     m_cartBadgeLabel->setFont(font);
        // } else {
        //     QFont font = m_cartBadgeLabel->font();
        //     font.setPointSize(9); // Стандартний шрифт
        //     m_cartBadgeLabel->setFont(font);
        // }
        m_cartBadgeLabel->show(); // Показуємо значок
        ui->cartButton->setToolTip(tr("Кошик (%1 товар(ів))").arg(totalItems));
        qInfo() << "Cart badge updated. Total items:" << totalItems;
    } else {
        m_cartBadgeLabel->hide(); // Ховаємо значок, якщо кошик порожній
        ui->cartButton->setToolTip(tr("Кошик"));
        qInfo() << "Cart is empty, badge hidden.";
    }
}

// Слот для зміни кількості товару в кошику (Новий дизайн - виправлено виліт)
void MainWindow::updateCartItemQuantity(int bookId, int quantity)
{
    if (m_cartItems.contains(bookId)) {
        qInfo() << "Updating quantity for book ID" << bookId << "to" << quantity;

        // Перевіряємо, чи кількість не перевищує доступну (SpinBox сам обмежує, але для безпеки)
        int stockQuantity = m_cartItems[bookId].book.stockQuantity;
        if (quantity > stockQuantity) {
            qWarning() << "Attempted to set quantity" << quantity << "but only" << stockQuantity << "available for book ID" << bookId;
            quantity = stockQuantity;
            // Потрібно знайти SpinBox і встановити йому правильне значення, якщо воно змінилося
            // Це може бути складно, простіше попередити користувача або дозволити SpinBox обробити це.
            // Наразі просто використовуємо скориговане значення.
        }
        if (quantity < 1) { // Мінімальна кількість - 1
             qWarning() << "Attempted to set quantity less than 1 for book ID" << bookId;
             quantity = 1;
        }


        // Оновлюємо кількість у внутрішній структурі даних
        m_cartItems[bookId].quantity = quantity;

        // Оновлюємо мітку підсумку для цього товару
        QLabel *subtotalLabel = m_cartSubtotalLabels.value(bookId, nullptr);
        if (subtotalLabel) {
            double newSubtotal = m_cartItems[bookId].book.price * quantity;
            subtotalLabel->setText(QString::number(newSubtotal, 'f', 2) + tr(" грн"));
            qInfo() << "Updated subtotal label for book ID" << bookId;
        } else {
            qWarning() << "Could not find subtotal label for book ID" << bookId << "to update.";
            // Якщо мітки немає, можливо, варто перезавантажити кошик для консистентності
            if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
                populateCartPage();
            }
        }

        // Оновлюємо загальну суму та іконку кошика
        // Оновлюємо загальну суму та іконку кошика
        updateCartTotal();
        updateCartIcon();

        // --- Збереження в БД ---
        if (m_dbManager) {
            m_dbManager->addOrUpdateCartItem(m_currentCustomerId, bookId, quantity);
        } else {
            qWarning() << "updateCartItemQuantity: DatabaseManager is null, cannot save cart item to DB.";
        }
        // --- Кінець збереження в БД ---

    } else {
        qWarning() << "Attempted to update quantity for non-existent book ID in cart:" << bookId;
    }
}

// Слот для видалення товару з кошика (Новий дизайн)
void MainWindow::removeCartItem(int bookId)
{
     // --- Видалення з БД (робимо ДО видалення з m_cartItems) ---
     bool removedFromDb = false;
     if (m_dbManager) {
         removedFromDb = m_dbManager->removeCartItem(m_currentCustomerId, bookId);
         // Ми продовжуємо, навіть якщо видалення з БД не вдалося,
         // щоб користувач бачив зміни в UI, але логуємо помилку.
         if (!removedFromDb) {
              qWarning() << "Failed to remove item (bookId:" << bookId << ") from DB cart for customerId:" << m_currentCustomerId;
         }
     } else {
         qWarning() << "removeCartItem: DatabaseManager is null, cannot remove item from DB cart.";
     }
     // --- Кінець видалення з БД ---

     if (m_cartItems.contains(bookId)) {
         QString bookTitle = m_cartItems[bookId].book.title; // Зберігаємо назву для повідомлення
         m_cartItems.remove(bookId); // Видаляємо з пам'яті
         qInfo() << "Removed book ID" << bookId << "from memory cart.";
         ui->statusBar->showMessage(tr("Книгу '%1' видалено з кошика.").arg(bookTitle), 3000);

         // Перезаповнюємо сторінку кошика, щоб видалити віджет
         if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
            populateCartPage();
         }
         // updateCartTotal(); // populateCartPage вже викликає updateCartTotal
         updateCartIcon(); // Оновлюємо іконку
     } else {
         qWarning() << "Attempted to remove non-existent book ID from cart:" << bookId;
     }
}

// Слот для кнопки "Оформити замовлення" - Тепер відкриває діалог
void MainWindow::on_placeOrderButton_clicked()
{
    qInfo() << "Place order button clicked. Opening checkout dialog...";
    if (m_cartItems.isEmpty()) {
        QMessageBox::information(this, tr("Порожній кошик"), tr("Ваш кошик порожній. Додайте товари перед оформленням замовлення."));
        return;
    }
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних. Неможливо оформити замовлення."));
        return;
    }
     if (m_currentCustomerId <= 0) {
        QMessageBox::critical(this, tr("Помилка"), tr("Неможливо оформити замовлення, користувач не визначений."));
        return;
    }

    // --- Отримання профілю користувача для діалогу ---
    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
    if (!profile.found) {
        QMessageBox::critical(this, tr("Помилка профілю"), tr("Не вдалося завантажити дані профілю користувача."));
        return;
    }

    // --- Розрахунок загальної суми для діалогу ---
    double currentTotal = 0.0;
    for (const auto &item : m_cartItems) {
        currentTotal += item.book.price * item.quantity;
    }

    // --- Створення та показ діалогу ---
    CheckoutDialog dialog(profile, currentTotal, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Користувач підтвердив, отримуємо дані з діалогу
        QString finalAddress = dialog.getShippingAddress();
        QString finalPaymentMethod = dialog.getPaymentMethod();
        qInfo() << "Checkout confirmed. Address:" << finalAddress << "Payment:" << finalPaymentMethod;

        // Викликаємо новий метод для фіналізації замовлення
        finalizeOrder(finalAddress, finalPaymentMethod);

    } else {
        // Користувач скасував
        qInfo() << "Checkout cancelled by user.";
    }
}

// Новий слот для фіналізації замовлення після підтвердження в діалозі
void MainWindow::finalizeOrder(const QString &shippingAddress, const QString &paymentMethod)
{
     qInfo() << "Finalizing order. Address:" << shippingAddress << "Payment:" << paymentMethod;

     if (m_cartItems.isEmpty() || !m_dbManager || m_currentCustomerId <= 0) {
         qWarning() << "Finalize order called with empty cart, no DB manager, or invalid customer ID.";
         QMessageBox::critical(this, tr("Помилка"), tr("Не вдалося завершити оформлення замовлення через внутрішню помилку."));
         return;
     }

     // Готуємо дані для createOrder (bookId -> quantity)
     QMap<int, int> itemsMap;
     for (auto it = m_cartItems.constBegin(); it != m_cartItems.constEnd(); ++it) {
         itemsMap.insert(it.key(), it.value().quantity);
     }

     // --- Виклик методу DatabaseManager для створення замовлення ---
     int newOrderId = -1;
     // Викликаємо оновлений createOrder, який повертає double
     double orderTotal = m_dbManager->createOrder(m_currentCustomerId, itemsMap, shippingAddress, paymentMethod, newOrderId);

     if (orderTotal >= 0 && newOrderId > 0) { // Перевіряємо, чи не повернулася помилка (-1.0)
         QMessageBox::information(this, tr("Замовлення оформлено"), tr("Ваше замовлення №%1 на суму %2 грн успішно оформлено!").arg(newOrderId).arg(QString::number(orderTotal, 'f', 2)));

         // --- Нарахування бонусних балів ---
         // Приклад: 1 бал за кожні 10 грн
         int pointsToAdd = static_cast<int>(orderTotal / 10.0);
         if (pointsToAdd > 0) {
             qInfo() << "Adding" << pointsToAdd << "loyalty points for order total" << orderTotal;
             if (m_dbManager->addLoyaltyPoints(m_currentCustomerId, pointsToAdd)) {
                 qInfo() << "Loyalty points added successfully.";
                 // Можна показати повідомлення користувачу про бали
                 ui->statusBar->showMessage(tr("Вам нараховано %1 бонусних балів!").arg(pointsToAdd), 4000);
             } else {
                 qWarning() << "Failed to add loyalty points for customer ID:" << m_currentCustomerId;
                 // Не критично для замовлення, але варто залогувати
             }
         } else {
              qInfo() << "No loyalty points to add for order total" << orderTotal;
         }
         // --- Кінець нарахування бонусних балів ---


         // --- Очищення кошика в БД ---
         if (!m_dbManager->clearCart(m_currentCustomerId)) {
             qWarning() << "Failed to clear DB cart for customerId:" << m_currentCustomerId << "after placing order.";
         } else {
             qInfo() << "DB cart cleared successfully for customerId:" << m_currentCustomerId;
         }
         // --- Кінець очищення кошика в БД ---

         m_cartItems.clear(); // Очищаємо кошик в пам'яті
         updateCartIcon(); // Оновлюємо іконку
         populateCartPage(); // Оновлюємо сторінку кошика (стане порожньою)
         // Можна перейти на сторінку замовлень
         on_navOrdersButton_clicked();

     } else {
         QMessageBox::critical(this, tr("Помилка оформлення"), tr("Не вдалося оформити замовлення. Можливо, деяких товарів вже немає в наявності. Перевірте журнал помилок або спробуйте пізніше."));
         qWarning() << "Failed to create order. DB Error:" << m_dbManager->lastError().text() << "Returned total:" << orderTotal;
         // Важливо: Не очищаємо кошик, якщо замовлення не створено!
         // Можливо, варто оновити кошик, щоб показати актуальну кількість товарів, якщо помилка була через наявність
         loadCartFromDatabase(); // Перезавантажуємо кошик з БД, щоб побачити актуальні дані
         populateCartPage();
     }
}


// --- Кінець логіки кошика ---
