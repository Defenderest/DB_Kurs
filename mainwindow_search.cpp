#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QLineEdit>
#include <QCompleter>
#include <QStandardItemModel> // Змінено з QStringListModel
#include <QStandardItem>      // Додано для створення елементів моделі
#include <QDebug>
#include <QListView>          // Додано для доступу до popup view
#include "searchsuggestiondelegate.h" // Додано включення делегата
#include <QMessageBox>        // Додано для QMessageBox

// Налаштування автодоповнення для глобального пошуку
void MainWindow::setupSearchCompleter()
{
    if (!ui->globalSearchLineEdit) {
        qWarning() << "setupSearchCompleter: globalSearchLineEdit is null!";
        return;
    }

    // Використовуємо QStandardItemModel замість QStringListModel
    m_searchSuggestionModel = new QStandardItemModel(this);
    m_searchCompleter = new QCompleter(m_searchSuggestionModel, this);

    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_searchCompleter->setCompletionMode(QCompleter::PopupCompletion);
    // Важливо: FilterMode більше не потрібен, оскільки ми самі фільтруємо в updateSearchSuggestions
    // m_searchCompleter->setFilterMode(Qt::MatchStartsWith); // Видалено або закоментовано

    // Створюємо та встановлюємо наш кастомний делегат
    m_searchDelegate = new SearchSuggestionDelegate(this);
    if (m_searchCompleter->popup()) {
        m_searchCompleter->popup()->setItemDelegate(m_searchDelegate);
        // Налаштування вигляду popup (опціонально)
        m_searchCompleter->popup()->setStyleSheet(R"(
            QListView {
                background-color: white; /* Змінено фон на білий */
                border: 1px solid #ced4da;
                border-radius: 4px;
                padding: 2px; /* Невеликий внутрішній відступ */
            }
            QListView::item {
                padding: 5px; /* Відступ для кожного елемента */
                border-radius: 3px; /* Невелике заокруглення для елементів */
                /* color: black; - Видалено, колір керується делегатом */
            }
            QListView::item:selected {
                background-color: #e9ecef; /* Світло-сірий фон для вибраного */
                /* color: black; - Видалено, колір керується делегатом */
            }
        )");
        // Логування стилю та палітри видалено
    } else {
        qWarning() << "setupSearchCompleter: Completer popup is null! Cannot set delegate.";
    }


    ui->globalSearchLineEdit->setCompleter(m_searchCompleter);

    // Підключаємо сигнал зміни тексту до слота оновлення пропозицій
    connect(ui->globalSearchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchSuggestions);

    // Підключаємо сигнал активації елемента (вибір зі списку) до нашого слота
    connect(m_searchCompleter, QOverload<const QModelIndex &>::of(&QCompleter::activated),
            this, &MainWindow::onSearchSuggestionActivated);


    qInfo() << "Search completer setup complete for globalSearchLineEdit.";
}

// Слот для оновлення пропозицій пошуку при зміні тексту
void MainWindow::updateSearchSuggestions(const QString &text)
{
    if (!m_dbManager || !m_searchSuggestionModel) {
        qWarning() << "updateSearchSuggestions: dbManager or searchSuggestionModel is null!";
        return; // Немає менеджера БД або моделі
    }

    // Отримуємо пропозиції, якщо текст НЕ порожній (шукаємо з першої літери)
    if (text.isEmpty()) {
        m_searchSuggestionModel->clear(); // Очищаємо модель, якщо текст порожній
        m_searchCompleter->popup()->hide(); // Ховаємо popup
        return;
    }

    // Отримуємо розширені пропозиції
    QList<SearchSuggestionInfo> suggestions = m_dbManager->getSearchSuggestions(text);

    // Очищаємо модель перед заповненням новими даними
    m_searchSuggestionModel->clear();

    // Заповнюємо модель даними
    for (const SearchSuggestionInfo &suggestion : suggestions) {
        QStandardItem *item = new QStandardItem();
        item->setData(suggestion.displayText, SearchSuggestionRoles::DisplayTextRole); // Основний текст
        item->setData(QVariant::fromValue(suggestion.type), SearchSuggestionRoles::TypeRole); // Тип (enum)
        item->setData(suggestion.id, SearchSuggestionRoles::IdRole); // ID
        item->setData(suggestion.imagePath, SearchSuggestionRoles::ImagePathRole); // Шлях до зображення
        item->setData(suggestion.price, SearchSuggestionRoles::PriceRole); // Додаємо ціну

        // Додаємо ToolTip для додаткової інформації (опціонально)
        item->setToolTip(QString("Тип: %1\nID: %2%3")
                         .arg(suggestion.type == SearchSuggestionInfo::Book ? tr("Книга") : tr("Автор"))
                         .arg(suggestion.id)
                         // Додаємо ціну до ToolTip, якщо це книга
                         .arg(suggestion.type == SearchSuggestionInfo::Book ? QString("\nЦіна: %1 грн").arg(suggestion.price, 0, 'f', 2) : QString())
                         );
        // Видалено дубльовані рядки .arg(...)

        m_searchSuggestionModel->appendRow(item);
    }

    // Показуємо або ховаємо popup залежно від наявності пропозицій
    if (m_searchSuggestionModel->rowCount() > 0) {
        // Якщо комплітер не активний, активуємо його
        if (!m_searchCompleter->popup()->isVisible()) {
             m_searchCompleter->complete();
        }
    } else {
        m_searchCompleter->popup()->hide();
    }

    qInfo() << "Updated search suggestions for text:" << text << "Count:" << suggestions.count();
}


// Слот для обробки вибору пропозиції зі списку
void MainWindow::onSearchSuggestionActivated(const QModelIndex &index)
{
    if (!index.isValid()) {
        qWarning() << "Invalid index activated in search completer.";
        return;
    }

    // Отримуємо дані з вибраного елемента моделі
    SearchSuggestionInfo::SuggestionType type = static_cast<SearchSuggestionInfo::SuggestionType>(index.data(SearchSuggestionRoles::TypeRole).toInt());
    int id = index.data(SearchSuggestionRoles::IdRole).toInt();
    QString displayText = index.data(SearchSuggestionRoles::DisplayTextRole).toString();

    qInfo() << "Search suggestion activated:" << displayText << "Type:" << type << "ID:" << id;

    // Виконуємо дію залежно від типу
    if (type == SearchSuggestionInfo::Book) {
        showBookDetails(id); // Переходимо на сторінку деталей книги
    } else if (type == SearchSuggestionInfo::Author) {
        // Переходимо на сторінку деталей автора
        showAuthorDetails(id);
    }

    // Опціонально: очистити поле пошуку після вибору
    // ui->globalSearchLineEdit->clear();
}
