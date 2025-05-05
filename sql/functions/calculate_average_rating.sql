-- Name: CreateCalculateAverageRatingFunction
-- Description: Creates or replaces a function to calculate the average rating for a given book_id, ignoring ratings of 0.
CREATE OR REPLACE FUNCTION calculate_average_book_rating(book_id_param INT)
RETURNS NUMERIC AS $$
DECLARE
    avg_rating NUMERIC;
BEGIN
    SELECT COALESCE(AVG(rating), 0.0) -- Return 0.0 if no ratings > 0 exist
    INTO avg_rating
    FROM comment
    WHERE book_id = book_id_param AND rating > 0; -- Only consider ratings greater than 0

    RETURN avg_rating;
END;
$$ LANGUAGE plpgsql;
