#pragma once
#include "Audiobook.h"
#include <QWidget>
#include <QLabel>
#include <QPixmap>

class QLabel;
class QProgressBar;
class QPushButton;

class BookCard : public QWidget {
    Q_OBJECT
public:
    explicit BookCard(const Audiobook& book, QWidget* parent = nullptr);
    void updateBook(const Audiobook& book);
    const QString& bookId() const { return m_bookId; }

signals:
    void clicked(const QString& bookId);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void paintEvent(QPaintEvent*) override;

private:
    QString      m_bookId;
    QLabel*      m_cover;
    QLabel*      m_title;
    QLabel*      m_author;
    QLabel*      m_duration;
    QWidget*     m_progressBar;
    int          m_progress = 0;
    bool         m_hovered  = false;

    void setupUi(const Audiobook& book);
    void applyStyle();
};
