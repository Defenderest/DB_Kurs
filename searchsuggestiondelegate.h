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
#include "datatypes.h" // Для SearchSuggestionInfo та ролей

// Визначимо кастомні ролі для передачі даних через модель
namespace SearchSuggestionRoles {
    const int TypeRole = Qt::UserRole + 1;      // SearchSuggestionInfo::SuggestionType
    const int IdRole = Qt::UserRole + 2;        // int (bookId або authorId)
    const int ImagePathRole = Qt::UserRole + 3; // QString
    const int DisplayTextRole = Qt::DisplayRole; // Використовуємо стандартну роль для тексту
}

class SearchSuggestionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SearchSuggestionDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int m_imageSize = 40; // Розмір зображення в пікселях
    int m_padding = 5;    // Відступи
};

#endif // SEARCHSUGGESTIONDELEGATE_H
