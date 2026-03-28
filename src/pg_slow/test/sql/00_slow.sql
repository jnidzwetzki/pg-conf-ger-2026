CREATE EXTENSION pg_slow;
CREATE TABLE data(value INT);

-- Test with positive values
INSERT INTO data(value) SELECT generate_series(0, 1000);

-- Test output
SELECT value, (value%2)::boolean, is_odd_slow(value), is_odd_fast(value) FROM data LIMIT 20;

-- Test differences
SELECT *, (value % 2)::boolean, is_odd_slow(value) FROM data WHERE (value % 2)::boolean <> is_odd_slow(value);

-- Test with negative values
TRUNCATE data;
INSERT INTO data(value) SELECT generate_series(-1000, 0);
SELECT value, (value%2)::boolean, is_odd_slow(value), is_odd_fast(value) FROM data LIMIT 20;
SELECT *, (value % 2)::boolean, is_odd_slow(value) FROM data WHERE (value % 2)::boolean <> is_odd_slow(value);
