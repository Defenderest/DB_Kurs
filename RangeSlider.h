#ifndef RANGESLIDER_H
#define RANGESLIDER_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication> // For style metrics
#include <QStyleOptionSlider> // For style hints

// Простий діапазонний слайдер
class RangeSlider : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY rangeChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY rangeChanged)
    Q_PROPERTY(int lowerValue READ lowerValue WRITE setLowerValue NOTIFY lowerValueChanged)
    Q_PROPERTY(int upperValue READ upperValue WRITE setUpperValue NOTIFY upperValueChanged)

public:
    explicit RangeSlider(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

    int minimum() const { return m_minimum; }
    void setMinimum(int min);

    int maximum() const { return m_maximum; }
    void setMaximum(int max);

    int lowerValue() const { return m_lowerValue; }
    void setLowerValue(int lower);

    int upperValue() const { return m_upperValue; }
    void setUpperValue(int upper);

    void setRange(int min, int max);

signals:
    void lowerValueChanged(int lower);
    void upperValueChanged(int upper);
    void rangeChanged(int min, int max); // Сигнал зміни всього діапазону

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent* event) override; // Для оновлення стилю

private:
    enum Handle { NoHandle, LowerHandle, UpperHandle, BothHandles };

    // Геометрія та стан
    QRect grooveRect() const; // Область доріжки
    QRect handleRect(Handle handle) const; // Область ручки
    int positionToValue(int pos) const;
    int valueToPosition(int val) const;
    void updateHoverControl(const QPoint& pos); // Оновлення стану наведення
    void triggerAction(QAbstractSlider::SliderAction action, bool fast); // Для обробки кліків на доріжці

    // Властивості
    int m_minimum = 0;
    int m_maximum = 1000; // Значення за замовчуванням
    int m_lowerValue = 0;
    int m_upperValue = 1000;
    Qt::Orientation m_orientation;
    int m_handleWidth = 16; // Ширина ручки (можна налаштувати)
    int m_grooveHeight = 6; // Висота доріжки (зробимо тоншою)

    // Стан взаємодії
    Handle m_pressedControl = NoHandle;
    Handle m_hoverControl = NoHandle;
    int m_clickOffset = 0; // Зсув кліку відносно ручки
    int m_lastPressedValue = 0; // Для визначення зміни значення при відпусканні

    // Стилізація
    QStyleOptionSlider getStyleOption(Handle handle = NoHandle) const;
};

#endif // RANGESLIDER_H
