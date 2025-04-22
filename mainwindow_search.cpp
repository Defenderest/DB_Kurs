#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QDebug>

// Налаштування автодоповнення для глобального пошуку
void MainWindow::setupSearchCompleter()
{
    if (!ui->globalSearchLineEdit) {
        qWarning() << "setupSearchCompleter: globalSearchLineEdit is null!";
        return;
    }

    m_searchSuggestionModel = new QStringListModel(this); // Модель для пропозицій
    m_searchCompleter = new QCompleter(m_searchSuggestionModel, this); // Комплітер

    m_searchCompleter->setCaseSensitivity(Qt::CaseInsensitive); // Нечутливий до регістру
    m_searchCompleter->setCompletionMode(QCompleter::PopupCompletion); // Випадаючий список
    m_searchCompleter->setFilterMode(Qt::MatchStartsWith); // Пропозиції, що починаються з введеного тексту
    // m_searchCompleter->setPopup(ui->globalSearchLineEdit->findChild<QListView*>()); // Використовуємо стандартний popup - findChild може бути ненадійним, краще залишити стандартний popup комплітера

    ui->globalSearchLineEdit->setCompleter(m_searchCompleter);

    // Підключаємо сигнал зміни тексту до слота оновлення пропозицій
    connect(ui->globalSearchLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateSearchSuggestions);

    qInfo() << "Search completer setup complete for globalSearchLineEdit.";
}

// Слот для оновлення пропозицій пошуку при зміні тексту
void MainWindow::updateSearchSuggestions(const QString &text)
{
    if (!m_dbManager || !m_searchSuggestionModel) {
        return; // Немає менеджера БД або моделі
    }

    // Отримуємо пропозиції, тільки якщо текст достатньо довгий
    if (text.length() < 2) { // Мінімальна довжина для пошуку (можна змінити)
        m_searchSuggestionModel->setStringList({}); // Очищаємо модель, якщо текст короткий
        return;
    }

    QStringList suggestions = m_dbManager->getSearchSuggestions(text);
    m_searchSuggestionModel->setStringList(suggestions); // Оновлюємо модель пропозиціями

    // qInfo() << "Updated search suggestions for text:" << text << "Count:" << suggestions.count();
}
