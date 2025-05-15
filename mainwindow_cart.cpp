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
#include <QMap>
#include <QSpacerItem>
#include <QStatusBar>
#include <QLineEdit>
#include <QPainter>
#include <QIcon>
#include "checkoutdialog.h"

QWidget* MainWindow::createCartItemWidget(const CartItem &item, int bookId)
{
    QFrame *itemFrame = new QFrame();
    itemFrame->setObjectName("cartItemFrame");
    itemFrame->setFrameShape(QFrame::StyledPanel);
    itemFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *mainLayout = new QHBoxLayout(itemFrame);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *coverLabel = new QLabel();
    coverLabel->setObjectName("cartItemCoverLabel");
    coverLabel->setAlignment(Qt::AlignCenter);
    QPixmap coverPixmap(item.book.coverImagePath);
    if (coverPixmap.isNull() || item.book.coverImagePath.isEmpty()) {
        coverLabel->setText(tr("Фото"));
    } else {
        QSize labelSize = coverLabel->minimumSize();
        if (!labelSize.isValid() || labelSize.width() <= 0 || labelSize.height() <= 0) {
             labelSize = QSize(60, 85);
             qWarning() << "Cart item cover label size not set, using default:" << labelSize;
        }
        coverLabel->setPixmap(coverPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        coverLabel->setText("");
    }
    mainLayout->addWidget(coverLabel);

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
    infoLayout->addStretch(1);
    mainLayout->addLayout(infoLayout, 2);

    QLabel *priceLabel = new QLabel(QString::number(item.book.price, 'f', 2) + tr(" грн"));
    priceLabel->setObjectName("cartItemPriceLabel");
    priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mainLayout->addWidget(priceLabel, 1);

    QSpinBox *quantitySpinBox = new QSpinBox();
    quantitySpinBox->setObjectName("cartQuantitySpinBox");
    quantitySpinBox->setMinimum(1);
    quantitySpinBox->setMaximum(item.book.stockQuantity);
    quantitySpinBox->setValue(item.quantity);
    quantitySpinBox->setAlignment(Qt::AlignCenter);
    quantitySpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    quantitySpinBox->setProperty("bookId", bookId);
    connect(quantitySpinBox, &QSpinBox::valueChanged, this, [this, bookId](int newValue){
        updateCartItemQuantity(bookId, newValue);
    });
    mainLayout->addWidget(quantitySpinBox);

    QLabel *subtotalLabel = new QLabel(QString::number(item.book.price * item.quantity, 'f', 2) + tr(" грн"));
    subtotalLabel->setObjectName("cartItemSubtotalLabel");
    subtotalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    subtotalLabel->setMinimumWidth(80);
    mainLayout->addWidget(subtotalLabel, 1);
    m_cartSubtotalLabels.insert(bookId, subtotalLabel);

    QPushButton *removeButton = new QPushButton();
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

void MainWindow::on_addToCartButtonClicked(int bookId)
{
    qInfo() << "Add to cart button clicked for book ID:" << bookId;
    if (!m_dbManager) {
        QMessageBox::critical(this, tr("Помилка"), tr("Помилка доступу до бази даних."));
        return;
    }

    BookDisplayInfo bookInfo = m_dbManager->getBookDisplayInfoById(bookId);

    if (!bookInfo.found) {
         qWarning() << "Book with ID" << bookId << "not found for adding to cart.";
         QMessageBox::warning(this, tr("Помилка"), tr("Не вдалося знайти інформацію про книгу (ID: %1).").arg(bookId));
         return;
    }

    if (bookInfo.stockQuantity <= 0) {
        QMessageBox::information(this, tr("Немає в наявності"), tr("На жаль, книги '%1' зараз немає в наявності.").arg(bookInfo.title));
        return;
    }

    if (m_cartItems.contains(bookId)) {
        if (m_cartItems[bookId].quantity + 1 > bookInfo.stockQuantity) {
             QMessageBox::information(this, tr("Обмеження кількості"), tr("Ви не можете додати більше одиниць книги '%1', ніж є на складі (%2).").arg(bookInfo.title).arg(bookInfo.stockQuantity));
             return;
        }
        m_cartItems[bookId].quantity++;
        qInfo() << "Increased quantity for book ID" << bookId << "to" << m_cartItems[bookId].quantity;
    } else {
        CartItem newItem;
        newItem.book = bookInfo;
        newItem.quantity = 1;
        m_cartItems.insert(bookId, newItem);
        qInfo() << "Added new book ID" << bookId << "to cart.";
    }

    if (m_dbManager) {
        m_dbManager->addOrUpdateCartItem(m_currentCustomerId, bookId, m_cartItems[bookId].quantity);
    } else {
        qWarning() << "on_addToCartButtonClicked: DatabaseManager is null, cannot save cart item to DB.";
    }

    updateCartIcon();

    ui->statusBar->showMessage(tr("Книгу '%1' додано до кошика.").arg(bookInfo.title), 3000);

    if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
        populateCartPage();
    }
}

void MainWindow::on_cartButton_clicked()
{
    qInfo() << "Cart button clicked. Navigating to cart page.";
    if (!ui->cartPage) {
        qWarning() << "Cart page widget not found in UI!";
        QMessageBox::critical(this, tr("Помилка інтерфейсу"), tr("Сторінка кошика не знайдена."));
        return;
    }
    ui->contentStackedWidget->setCurrentWidget(ui->cartPage);
    populateCartPage();
}

void MainWindow::populateCartPage()
{
    qInfo() << "Populating cart page (new design)...";
    if (!ui->cartScrollArea || !ui->cartItemsContainerWidget || !ui->cartItemsLayout || !ui->cartTotalTextLabel || !ui->placeOrderButton || !ui->cartTotalsWidget) {
        qWarning() << "populateCartPage: One or more new cart page widgets are null!";
        if(ui->cartPage && ui->cartPage->layout()) {
             clearLayout(ui->cartPage->layout());
             QLabel *errorLabel = new QLabel(tr("Помилка інтерфейсу: Не вдалося відобразити кошик."), ui->cartPage);
             ui->cartPage->layout()->addWidget(errorLabel);
        }
        return;
    }

    clearLayout(ui->cartItemsLayout);
    m_cartSubtotalLabels.clear();

    QLabel* emptyCartLabel = ui->cartItemsContainerWidget->findChild<QLabel*>("emptyCartLabel");
    if(emptyCartLabel) {
        delete emptyCartLabel;
    }

    if (m_cartItems.isEmpty()) {
        qInfo() << "Cart is empty.";
        QLabel *noItemsLabel = new QLabel(tr("🛒\n\nВаш кошик порожній.\nЧас додати щось цікаве!"), ui->cartItemsContainerWidget);
        noItemsLabel->setObjectName("emptyCartLabel");
        noItemsLabel->setAlignment(Qt::AlignCenter);
        noItemsLabel->setWordWrap(true);
        ui->cartItemsLayout->addWidget(noItemsLabel);
        ui->cartItemsLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        ui->placeOrderButton->setEnabled(false);
        ui->cartTotalTextLabel->setText(tr("Загальна сума: 0.00 грн"));
        ui->cartTotalsWidget->setVisible(false);
        return;
    }

    ui->cartTotalsWidget->setVisible(true);

    for (auto it = m_cartItems.constBegin(); it != m_cartItems.constEnd(); ++it) {
        QWidget *itemWidget = createCartItemWidget(it.value(), it.key());
        if (itemWidget) {
            ui->cartItemsLayout->addWidget(itemWidget);
        }
    }

    ui->cartItemsLayout->addSpacerItem(new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    updateCartTotal();
    ui->placeOrderButton->setEnabled(true);
    qInfo() << "Cart page populated with" << m_cartItems.size() << "items.";

    ui->cartItemsContainerWidget->adjustSize();
}

void MainWindow::updateCartTotal()
{
    if (!ui->cartTotalTextLabel) return;

    double total = 0.0;
    for (const auto &item : m_cartItems) {
        total += item.book.price * item.quantity;
    }

    ui->cartTotalTextLabel->setText(tr("Загальна сума: %1 грн").arg(QString::number(total, 'f', 2)));
    qInfo() << "Cart total updated:" << total;
}

void MainWindow::updateCartIcon()
{
    if (!ui->cartButton || !m_cartBadgeLabel) {
        qWarning() << "updateCartIcon: cartButton or m_cartBadgeLabel is null!";
        return;
    }

    int totalItems = 0;
    for (const auto &item : m_cartItems) {
        totalItems += item.quantity;
    }

    const QString baseIconPath = "D:/projects/DB_Kurs/QtAPP/untitled/icons/cart.png";
    QIcon baseIcon(baseIconPath);
    if (!baseIcon.isNull()) {
        ui->cartButton->setIcon(baseIcon);
        if (ui->cartButton->iconSize().isEmpty()) {
             ui->cartButton->setIconSize(QSize(24, 24));
        }
    } else {
        qWarning() << "Failed to load base cart icon:" << baseIconPath;
        ui->cartButton->setText("?");
    }
    ui->cartButton->setText("");

    if (totalItems > 0) {
        m_cartBadgeLabel->setText(QString::number(totalItems));
        m_cartBadgeLabel->show();
        ui->cartButton->setToolTip(tr("Кошик (%1 товар(ів))").arg(totalItems));
        qInfo() << "Cart badge updated. Total items:" << totalItems;
    } else {
        m_cartBadgeLabel->hide();
        ui->cartButton->setToolTip(tr("Кошик"));
        qInfo() << "Cart is empty, badge hidden.";
    }
}

void MainWindow::updateCartItemQuantity(int bookId, int quantity)
{
    if (m_cartItems.contains(bookId)) {
        qInfo() << "Updating quantity for book ID" << bookId << "to" << quantity;

        int stockQuantity = m_cartItems[bookId].book.stockQuantity;
        if (quantity > stockQuantity) {
            qWarning() << "Attempted to set quantity" << quantity << "but only" << stockQuantity << "available for book ID" << bookId;
            quantity = stockQuantity;
        }
        if (quantity < 1) {
             qWarning() << "Attempted to set quantity less than 1 for book ID" << bookId;
             quantity = 1;
        }

        m_cartItems[bookId].quantity = quantity;

        QLabel *subtotalLabel = m_cartSubtotalLabels.value(bookId, nullptr);
        if (subtotalLabel) {
            double newSubtotal = m_cartItems[bookId].book.price * quantity;
            subtotalLabel->setText(QString::number(newSubtotal, 'f', 2) + tr(" грн"));
            qInfo() << "Updated subtotal label for book ID" << bookId;
        } else {
            qWarning() << "Could not find subtotal label for book ID" << bookId << "to update.";
            if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
                populateCartPage();
            }
        }

        updateCartTotal();
        updateCartIcon();

        if (m_dbManager) {
            m_dbManager->addOrUpdateCartItem(m_currentCustomerId, bookId, quantity);
        } else {
            qWarning() << "updateCartItemQuantity: DatabaseManager is null, cannot save cart item to DB.";
        }

    } else {
        qWarning() << "Attempted to update quantity for non-existent book ID in cart:" << bookId;
    }
}

void MainWindow::removeCartItem(int bookId)
{
     bool removedFromDb = false;
     if (m_dbManager) {
         removedFromDb = m_dbManager->removeCartItem(m_currentCustomerId, bookId);
         if (!removedFromDb) {
              qWarning() << "Failed to remove item (bookId:" << bookId << ") from DB cart for customerId:" << m_currentCustomerId;
         }
     } else {
         qWarning() << "removeCartItem: DatabaseManager is null, cannot remove item from DB cart.";
     }

     if (m_cartItems.contains(bookId)) {
         QString bookTitle = m_cartItems[bookId].book.title;
         m_cartItems.remove(bookId);
         qInfo() << "Removed book ID" << bookId << "from memory cart.";
         ui->statusBar->showMessage(tr("Книгу '%1' видалено з кошика.").arg(bookTitle), 3000);

         if (ui->contentStackedWidget->currentWidget() == ui->cartPage) {
            populateCartPage();
         }
         updateCartIcon();
     } else {
         qWarning() << "Attempted to remove non-existent book ID from cart:" << bookId;
     }
}

void MainWindow::on_placeOrderButton_clicked()
{
    qInfo() << "Place order button clicked. Opening checkout dialog...";
    if (m_cartItems.isEmpty()) {
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

    CustomerProfileInfo profile = m_dbManager->getCustomerProfileInfo(m_currentCustomerId);
    if (!profile.found) {
        QMessageBox::critical(this, tr("Помилка профілю"), tr("Не вдалося завантажити дані профілю користувача."));
        return;
    }

    double currentTotal = 0.0;
    for (const auto &item : m_cartItems) {
        currentTotal += item.book.price * item.quantity;
    }

    CheckoutDialog dialog(profile, currentTotal, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString finalAddress = dialog.getShippingAddress();
        QString finalPaymentMethod = dialog.getPaymentMethod();
        qInfo() << "Checkout confirmed. Address:" << finalAddress << "Payment:" << finalPaymentMethod;

        finalizeOrder(finalAddress, finalPaymentMethod);

    } else {
        qInfo() << "Checkout cancelled by user.";
    }
}

void MainWindow::finalizeOrder(const QString &shippingAddress, const QString &paymentMethod)
{
     qInfo() << "Finalizing order. Address:" << shippingAddress << "Payment:" << paymentMethod;

     if (m_cartItems.isEmpty() || !m_dbManager || m_currentCustomerId <= 0) {
         qWarning() << "Finalize order called with empty cart, no DB manager, or invalid customer ID.";
         QMessageBox::critical(this, tr("Помилка"), tr("Не вдалося завершити оформлення замовлення через внутрішню помилку."));
         return;
     }

     QMap<int, int> itemsMap;
     for (auto it = m_cartItems.constBegin(); it != m_cartItems.constEnd(); ++it) {
         itemsMap.insert(it.key(), it.value().quantity);
     }

     int newOrderId = -1;
     double orderTotal = m_dbManager->createOrder(m_currentCustomerId, itemsMap, shippingAddress, paymentMethod, newOrderId);

     if (orderTotal >= 0 && newOrderId > 0) {
         qInfo() << "Order" << newOrderId << "placed successfully for total" << orderTotal;

         int pointsToAdd = static_cast<int>(orderTotal / 10.0);
         if (pointsToAdd > 0) {
             qInfo() << "Adding" << pointsToAdd << "loyalty points for order total" << orderTotal;
             if (m_dbManager->addLoyaltyPoints(m_currentCustomerId, pointsToAdd)) {
                 qInfo() << "Loyalty points added successfully.";
                 ui->statusBar->showMessage(tr("Вам нараховано %1 бонусних балів!").arg(pointsToAdd), 4000);
             } else {
                 qWarning() << "Failed to add loyalty points for customer ID:" << m_currentCustomerId;
             }
         } else {
              qInfo() << "No loyalty points to add for order total" << orderTotal;
         }

         if (!m_dbManager->clearCart(m_currentCustomerId)) {
             qWarning() << "Failed to clear DB cart for customerId:" << m_currentCustomerId << "after placing order.";
         } else {
             qInfo() << "DB cart cleared successfully for customerId:" << m_currentCustomerId;
         }

         m_cartItems.clear();
         updateCartIcon();
         populateCartPage();
         on_navOrdersButton_clicked();

     } else {
         QMessageBox::critical(this, tr("Помилка оформлення"), tr("Не вдалося оформити замовлення. Можливо, деяких товарів вже немає в наявності. Перевірте журнал помилок або спробуйте пізніше."));
         qWarning() << "Failed to create order. DB Error:" << m_dbManager->lastError().text() << "Returned total:" << orderTotal;
         loadCartFromDatabase();
         populateCartPage();
     }
}
