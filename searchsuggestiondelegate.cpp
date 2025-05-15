#include "searchsuggestiondelegate.h"

SearchSuggestionDelegate::SearchSuggestionDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void SearchSuggestionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QString displayText = index.data(SearchSuggestionRoles::DisplayTextRole).toString();
    QString imagePath = index.data(SearchSuggestionRoles::ImagePathRole).toString();
    SearchSuggestionInfo::SuggestionType type = static_cast<SearchSuggestionInfo::SuggestionType>(index.data(SearchSuggestionRoles::TypeRole).toInt());
    double price = index.data(SearchSuggestionRoles::PriceRole).toDouble();

    QRect imageRect(option.rect.left() + m_padding,
                    option.rect.top() + (option.rect.height() - m_imageSize) / 2,
                    m_imageSize,
                    m_imageSize);

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        painter->fillRect(imageRect, Qt::lightGray);
    } else {
        pixmap = pixmap.scaled(imageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter->drawPixmap(imageRect.left() + (m_imageSize - pixmap.width()) / 2,
                           imageRect.top() + (m_imageSize - pixmap.height()) / 2,
                           pixmap);
    }

    QRect textRect = option.rect;
    textRect.setLeft(imageRect.right() + m_padding * 2);
    textRect.setRight(textRect.right() - m_padding);

    QRect priceRect = option.rect;
    priceRect.setLeft(priceRect.right() - m_padding - 60);
    priceRect.setRight(priceRect.right() - m_padding);

    if (type == SearchSuggestionInfo::Book && price > 0) {
        textRect.setRight(priceRect.left() - m_padding);
    }

    bool isSelected = option.state & QStyle::State_Selected;

    if (isSelected) {
        painter->setPen(Qt::white);
    } else {
        painter->setPen(Qt::black);
    }

    QFontMetrics fm(option.font);
    QString elidedText = fm.elidedText(displayText, Qt::ElideRight, textRect.width());

    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);

    if (type == SearchSuggestionInfo::Book && price > 0) {
        QString priceText = QString::number(price, 'f', 2) + tr(" грн");
        painter->drawText(priceRect, Qt::AlignVCenter | Qt::AlignRight, priceText);
    }


    painter->restore();
}

QSize SearchSuggestionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int height = m_imageSize + m_padding * 2;
    QSize baseSize = QStyledItemDelegate::sizeHint(option, index);
    return QSize(baseSize.width(), qMax(height, baseSize.height()));
}
