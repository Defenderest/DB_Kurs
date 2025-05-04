-- name: GetAllBooksForDisplay
SELECT
    b.book_id,
    b.title,
    b.price,
    b.cover_image_path,
    b.stock_quantity,
    b.genre,
    COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
    STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
FROM book b
LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
LEFT JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN author a ON ba.author_id = a.author_id
GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre, p.name
ORDER BY b.title;

-- name: GetFilteredBooksForDisplayBase
SELECT DISTINCT -- Використовуємо DISTINCT, оскільки JOIN з авторами може дублювати книги
    b.book_id,
    b.title,
    b.price,
    b.cover_image_path,
    b.stock_quantity,
    b.genre,
    b.language, -- Додано мову
    COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
    STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
FROM book b
LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
LEFT JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN author a ON ba.author_id = a.author_id
-- WHERE clause will be added dynamically in C++ code
-- GROUP BY and ORDER BY will be added dynamically in C++ code

-- name: GetAllDistinctGenres
SELECT DISTINCT genre FROM book WHERE genre IS NOT NULL AND genre != '' ORDER BY genre;

-- name: GetAllDistinctLanguages
SELECT DISTINCT language FROM book WHERE language IS NOT NULL AND language != '' ORDER BY language;

-- name: GetBookDetailsById
SELECT
    b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity,
    b.genre, b.description, b.publication_date, b.isbn, b.page_count, b.language,
    COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
    STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
FROM book b
LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
LEFT JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN author a ON ba.author_id = a.author_id
WHERE b.book_id = :bookId
GROUP BY b.book_id, p.name, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre, b.description, b.publication_date, b.isbn, b.page_count, b.language
LIMIT 1;

-- name: GetBookDisplayInfoById
SELECT
    b.book_id,
    b.title,
    b.price,
    b.cover_image_path,
    b.stock_quantity,
    b.genre,
    STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
FROM book b
LEFT JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN author a ON ba.author_id = a.author_id
WHERE b.book_id = :bookId
GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre
LIMIT 1;

-- name: GetBooksByGenre
SELECT
    b.book_id,
    b.title,
    b.price,
    b.cover_image_path,
    b.stock_quantity,
    b.genre,
    COALESCE(p.name, 'Невідомий видавець') AS publisher_name,
    STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
FROM book b
LEFT JOIN publisher p ON b.publisher_id = p.publisher_id
LEFT JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN author a ON ba.author_id = a.author_id
WHERE b.genre = :genre
GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre, p.name
ORDER BY b.publication_date DESC, b.title
LIMIT :limit;

-- name: GetSearchSuggestions
SELECT 'book' AS type, book_id AS id, title AS display_text, cover_image_path AS image_path, price
FROM book
WHERE LOWER(title) LIKE LOWER(:prefix) || '%'
UNION ALL
SELECT 'author' AS type, author_id AS id, first_name || ' ' || last_name AS display_text, image_path, 0.0 AS price
FROM author
WHERE LOWER(first_name || ' ' || last_name) LIKE LOWER(:prefix) || '%'
ORDER BY display_text
LIMIT :total_limit;

-- name: GetSimilarBooksByGenre
SELECT
    b.book_id,
    b.title,
    b.price,
    b.cover_image_path,
    b.stock_quantity,
    b.genre,
    STRING_AGG(DISTINCT a.first_name || ' ' || a.last_name, ', ') AS authors
FROM book b
LEFT JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN author a ON ba.author_id = a.author_id
WHERE b.genre = :genre AND b.book_id != :currentBookId
GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre
ORDER BY RANDOM()
LIMIT :limit;
