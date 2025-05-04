-- name: GetAllAuthorsForDisplay
SELECT
    author_id,
    first_name,
    last_name,
    nationality,
    image_path
FROM author
ORDER BY last_name, first_name;

-- name: GetAuthorDetailsById
SELECT
    author_id, first_name, last_name, nationality, image_path, biography, birth_date
FROM author
WHERE author_id = :authorId
LIMIT 1;

-- name: GetAuthorBooksForDisplay
SELECT
    b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre,
    STRING_AGG(DISTINCT a_other.first_name || ' ' || a_other.last_name, ', ') AS authors -- Збираємо ВСІХ авторів книги
FROM book b
INNER JOIN book_author ba ON b.book_id = ba.book_id
LEFT JOIN book_author ba_other ON b.book_id = ba_other.book_id -- Ще один join для збору всіх авторів
LEFT JOIN author a_other ON ba_other.author_id = a_other.author_id
WHERE ba.author_id = :authorId -- Фільтруємо за ID потрібного автора
GROUP BY b.book_id, b.title, b.price, b.cover_image_path, b.stock_quantity, b.genre
ORDER BY b.title;
