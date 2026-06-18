"""Примитивный микросервис хэширования паролей.

Намеренно НЕ использует классические хэши (hashlib и т.п.) — вместо этого
своя простая итеративная KDF. Это учебный, не криптостойкий вариант.

Формат хранимой строки: salt_hex:iterations:hash_hex
"""
import os

from flask import Flask, jsonify, request

app = Flask(__name__)

DEFAULT_ITERATIONS = 100_000
HASH_LEN = 32


def generate_salt(length: int = 16) -> bytes:
    return os.urandom(length)


def simple_kdf(password: str, salt: bytes, iterations: int) -> bytes:
    """Простая (не классическая) KDF: детерминированно перемешивает
    пароль и соль за `iterations` проходов, возвращает HASH_LEN байт."""
    data = password.encode("utf-8") + salt
    if not data:
        data = b"\x00"
    state = bytearray(HASH_LEN)
    for i in range(HASH_LEN):
        state[i] = (salt[i % len(salt)] + i) & 0xFF if salt else i
    for it in range(iterations):
        for i in range(HASH_LEN):
            b = state[i]
            d = data[(i + it) % len(data)]
            b = (b * 31 + d + it) & 0xFF       # смешивание
            b ^= state[(i + 1) % HASH_LEN]      # связь соседних байт
            b = ((b << 3) | (b >> 5)) & 0xFF    # циклический сдвиг влево на 3
            state[i] = b
    return bytes(state)


def hash_password(password: str, iterations: int = DEFAULT_ITERATIONS) -> str:
    salt = generate_salt()
    hash_value = simple_kdf(password, salt, iterations)
    # Храним в формате salt_hex:iterations:hash_hex
    return f"{salt.hex()}:{iterations}:{hash_value.hex()}"


def verify_password(password: str, stored: str) -> bool:
    try:
        salt_hex, iters_str, hash_hex = stored.split(":")
        salt = bytes.fromhex(salt_hex)
        iterations = int(iters_str)
        expected_hash = simple_kdf(password, salt, iterations)
        # В реальности сравнение должно быть константным по времени, здесь просто ==
        return expected_hash.hex() == hash_hex
    except Exception:
        return False


@app.get("/health")
def health():
    return jsonify(status="ok", service="hasher")


@app.post("/hash")
def hash_endpoint():
    data = request.get_json(silent=True) or {}
    password = data.get("password")
    if not isinstance(password, str) or password == "":
        return jsonify(error="password is required"), 400
    try:
        iterations = int(data.get("iterations", DEFAULT_ITERATIONS))
    except (TypeError, ValueError):
        return jsonify(error="iterations must be an integer"), 400

    stored = hash_password(password, iterations)
    salt_hex, iters_str, hash_hex = stored.split(":")
    return jsonify(
        stored=stored,
        salt=salt_hex,
        iterations=int(iters_str),
        hash=hash_hex,
    )


@app.post("/verify")
def verify_endpoint():
    data = request.get_json(silent=True) or {}
    password = data.get("password")
    stored = data.get("stored")
    if not isinstance(password, str) or not isinstance(stored, str):
        return jsonify(error="password and stored are required"), 400
    return jsonify(valid=verify_password(password, stored))


if __name__ == "__main__":
    port = int(os.environ.get("PORT", "5005"))
    app.run(host="0.0.0.0", port=port, threaded=True)
