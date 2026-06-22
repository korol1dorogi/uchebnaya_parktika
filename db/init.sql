
-- Пользователи
CREATE TABLE IF NOT EXISTS users (
    uuid     UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    login    TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    salt     TEXT NOT NULL
);

-- Хокку
CREATE TABLE IF NOT EXISTS haikus (
    id         UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    text       TEXT NOT NULL,
    user_uuid  UUID NULL REFERENCES users(uuid) ON DELETE SET NULL,  -- NULL = аноним
    comment    TEXT NULL DEFAULT NULL,                                -- заполняет пользователь
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_haikus_user ON haikus (user_uuid);
CREATE INDEX IF NOT EXISTS idx_haikus_created ON haikus (created_at DESC);
