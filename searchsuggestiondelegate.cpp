#include "searchsuggestiondelegate.h"

SearchSuggestionDelegate::SearchSuggestionDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void SearchSuggestionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    // Отримуємо дані з моделі, використовуючи кастомні ролі
    QString displayText = index.data(SearchSuggestionRoles::DisplayTextRole).toString();
    QString imagePath = index.data(SearchSuggestionRoles::ImagePathRole).toString();
    SearchSuggestionInfo::SuggestionType type = static_cast<SearchSuggestionInfo::SuggestionType>(index.data(SearchSuggestionRoles::TypeRole).toInt());
    double price = index.data(SearchSuggestionRoles::PriceRole).toDouble();
    // int id = index.data(SearchSuggestionRoles::IdRole).toInt(); // ID може знадобитися для ToolTip або іншого

    // --- Малювання фону ---
    // Прибираємо явне малювання фону. Стилі QListView::item подбають про фон та заокруглення.
    // if (option.state & QStyle::State_Selected) {
    //     painter->fillRect(option.rect, option.palette.highlight());
    // } else {
    //     // painter->fillRect(option.rect, option.palette.base());
    // }
    // Замість цього, можна викликати базовий метод для малювання фону та виділення,
    // але це може перекрити наші кастомні елементи. Краще покластися на стилі.
    // QStyledItemDelegate::paint(painter, option, QModelIndex()); // Не передаємо index, щоб не малювати стандартний вміст

    // --- Малювання зображення ---
    QRect imageRect(option.rect.left() + m_padding,
                    option.rect.top() + (option.rect.height() - m_imageSize) / 2, // Центруємо вертикально
                    m_imageSize,
                    m_imageSize);

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        // Якщо зображення немає, малюємо плейсхолдер (простий прямокутник)
        painter->fillRect(imageRect, Qt::lightGray);
        // Можна додати іконку за замовчуванням залежно від типу
        // QIcon defaultIcon = (type == SearchSuggestionInfo::Book) ? QApplication::style()->standardIcon(QStyle::SP_FileIcon) : QApplication::style()->standardIcon(QStyle::SP_UserIcon);
        // defaultIcon.paint(painter, imageRect);
    } else {
        // Масштабуємо зображення під квадрат, зберігаючи пропорції
        pixmap = pixmap.scaled(imageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // Малюємо зображення
        painter->drawPixmap(imageRect.left() + (m_imageSize - pixmap.width()) / 2, // Центруємо горизонтально в межах imageRect
                           imageRect.top() + (m_imageSize - pixmap.height()) / 2, // Центруємо вертикально в межах imageRect
                           pixmap);
    }

    // --- Малювання тексту ---
    QRect textRect = option.rect;
    textRect.setLeft(imageRect.right() + m_padding * 2); // Відступ від зображення
    textRect.setRight(textRect.right() - m_padding); // Відступ справа

    // --- Визначення області для ціни (справа) ---
    QRect priceRect = option.rect;
    priceRect.setLeft(priceRect.right() - m_padding - 60); // Резервуємо ~60px для ціни + відступ
    priceRect.setRight(priceRect.right() - m_padding); // Відступ справа

    // --- Коригування області для основного тексту ---
    // Зменшуємо ширину textRect, щоб не накладатися на ціну
    if (type == SearchSuggestionInfo::Book && price > 0) {
        textRect.setRight(priceRect.left() - m_padding); // Відступ між текстом та ціною
    }

    // --- Малювання основного тексту ---
    QColor defaultTextColor = option.palette.text().color();
    QColor selectedTextColor = option.palette.highlightedText().color();
    bool isSelected = option.state & QStyle::State_Selected;

    qDebug() << "Painting item:" << displayText
             << "Selected:" << isSelected
             << "Default Text Color:" << defaultTextColor.name(QColor::HexRgb)
             << "Selected Text Color:" << selectedTextColor.name(QColor::HexRgb);

    // Спочатку встановлюємо колір пера за замовчуванням (для невибраного стану)
    painter->setPen(defaultTextColor);

    // Якщо елемент вибрано, змінюємо колір пера на колір виділеного тексту
    if (isSelected) {
        painter->setPen(selectedTextColor);
    }

    qDebug() << "  -> Pen color set to:" << painter->pen().color().name(QColor::HexRgb);

    // Використовуємо FontMetrics для обрізання тексту з "..." якщо він не влазить
    QFontMetrics fm(option.font);
    QString elidedText = fm.elidedText(displayText, Qt::ElideRight, textRect.width());

    // Малюємо основний текст, вирівнюючи по вертикалі
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);

    // --- Малювання ціни (тільки для книг) ---
    if (type == SearchSuggestionInfo::Book && price > 0) {
        QString priceText = QString::number(price, 'f', 2) + tr(" грн");
        // Встановлюємо колір ціни (можна зробити іншим, наприклад, сірим)
        // Використовуємо той самий колір, що й для основного тексту, щоб він змінювався при виділенні
        // painter->setPen(QColor("#555")); // Приклад іншого кольору
        // Малюємо ціну, вирівнюючи по правому краю та по вертикалі
        painter->drawText(priceRect, Qt::AlignVCenter | Qt::AlignRight, priceText);
    }


    painter->restore();
}

QSize SearchSuggestionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Базова висота визначається розміром зображення та відступами
    int height = m_imageSize + m_padding * 2;
    // Базова ширина (можна взяти стандартну або розрахувати)
    QSize baseSize = QStyledItemDelegate::sizeHint(option, index);
    // Повертаємо розмір з нашою висотою та стандартною шириною (або більшою, якщо потрібно)
    return QSize(baseSize.width(), qMax(height, baseSize.height()));
}
