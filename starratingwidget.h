#ifndef STARRATINGWIDGET_H
#define STARRATINGWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QPolygonF>

class StarRatingWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int rating READ rating WRITE setRating NOTIFY ratingChanged)
    Q_PROPERTY(int maxRating READ maxRating WRITE setMaxRating)
    Q_PROPERTY(QColor starColor READ starColor WRITE setStarColor)
    Q_PROPERTY(QColor emptyStarColor READ emptyStarColor WRITE setEmptyStarColor)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)

public:
    explicit StarRatingWidget(QWidget *parent = nullptr);

    int rating() const;
    void setRating(int rating);

    int maxRating() const;
    void setMaxRating(int maxRating);

    QColor starColor() const;
    void setStarColor(const QColor &color);

    QColor emptyStarColor() const;
    void setEmptyStarColor(const QColor &color);

    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void ratingChanged(int rating);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void paintStar(QPainter *painter, const QRect &rect, bool filled);
    int starAtPosition(int x);

    int m_rating;
    int m_maxRating;
    QColor m_starColor;
    QColor m_emptyStarColor;
    bool m_readOnly;
    int m_hoverRating;
    QVector<QPolygonF> m_stars;
    int m_starSize;
};

#endif // STARRATINGWIDGET_H

