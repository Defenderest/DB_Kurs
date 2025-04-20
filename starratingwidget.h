#ifndef STARRATINGWIDGET_H
#define STARRATINGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVector>
#include <QMouseEvent> // Потрібно для обробки подій миші

QT_BEGIN_NAMESPACE
class QHBoxLayout;
QT_END_NAMESPACE                                                                                                                                                                                                                                                                                                                                                                                                            class StarRatingWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StarRatingWidget(QWidget *parent = nullptr);

    int rating() const; // Повертає поточний вибраний рейтинг (0-5)
    void setRating(int rating); // Встановлює рейтинг програмно

    QSize sizeHint() const override; // Рекомендований розмір

signals:
    void ratingChanged(int rating); // Сигнал про зміну рейтингу користувачем

protected:
    // Перевизначені обробники подій миші
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override; // Коли миша покидає віджет

private:
    void updateStars(int hoverRating = -1); // Оновлює вигляд зірок (-1 означає без підсвітки наведення)
    int starAtPosition(int x); // Визначає, над якою зіркою знаходиться курсор

    int m_currentRating = 0; // Поточний вибраний рейтинг (0-5)
    int m_hoverRating = -1;  // Рейтинг під курсором миші (-1 якщо миша не над зірками)
    QVector<QLabel*> m_starLabels; // Вектор для зберігання міток зірок

    const int MAX_RATING = 5; // Максимальна кількість зірок
    const QString STAR_EMPTY = "☆"; // Символ порожньої зірки
    const QString STAR_FULL = "★";  // Символ заповненої зірки
    const QString STAR_COLOR_EMPTY = "#cccccc"; // Колір порожньої зірки (світло-сірий)
    const QString STAR_COLOR_HOVER = "#ffcc00"; // Колір при наведенні (жовтий)
    const QString STAR_COLOR_SELECTED = "#ffaa00"; // Колір вибраної зірки (темніший жовтий/оранжевий)
    const int STAR_SIZE = 20; // Розмір шрифту зірок
};

#endif // STARRATINGWIDGET_H

