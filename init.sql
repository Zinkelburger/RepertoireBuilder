-- init.sql
CREATE TABLE IF NOT EXISTS lichess (
    id SERIAL PRIMARY KEY,
    fen TEXT NOT NULL UNIQUE,
    white_wins INTEGER NOT NULL,
    black_wins INTEGER NOT NULL,
    draws INTEGER NOT NULL,
    children_moves TEXT[] NOT NULL
);
