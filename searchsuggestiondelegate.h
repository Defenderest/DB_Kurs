#ifndef SEARCHSUGGESTIONDELEGATE_H
#define SEARCHSUGGESTIONDELEGATE_H

#include <QStyledItemDelegate>
#include <QPixmap>
#include <QPainter>
#include <QApplication> // Для доступу до палітри стилів
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QSize>
#include <QDebug>
#include "datatypes.h"

namespace SearchSuggestionRoles {
    const int TypeRole = Qt::UserRole + 1;
    const int IdRole = Qt::UserRole + 2;
    const int ImagePathRole = Qt::UserRole + 3;
    const int PriceRole = Qt::UserRole + 4;
    const int DisplayTextRole = Qt::DisplayRole;
}

class SearchSuggestionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SearchSuggestionDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int m_imageSize = 40;
    int m_padding = 5;
};

#endif // SEARCHSUGGESTIONDELEGATE_H
