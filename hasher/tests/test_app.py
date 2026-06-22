"""Юнит-тесты микросервиса хэширования (pytest)."""
import os
import sys

# чтобы импортировать app.py из родительской папки hasher/
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import app as hasher  # noqa: E402


# ---- чистые функции ----

def test_simple_kdf_deterministic():
    salt = b"\x01\x02\x03\x04"
    a = hasher.simple_kdf("pw", salt, 100)
    b = hasher.simple_kdf("pw", salt, 100)
    assert a == b                      # одинаковый вход -> одинаковый выход
    assert len(a) == hasher.HASH_LEN   # фиксированная длина


def test_simple_kdf_varies():
    salt = b"\x01\x02\x03\x04"
    other_salt = b"\x09\x08\x07\x06"
    assert hasher.simple_kdf("pw", salt, 100) != hasher.simple_kdf("pw2", salt, 100)
    assert hasher.simple_kdf("pw", salt, 100) != hasher.simple_kdf("pw", other_salt, 100)
    assert hasher.simple_kdf("pw", salt, 100) != hasher.simple_kdf("pw", salt, 101)


def test_hash_password_format():
    stored = hasher.hash_password("secret", iterations=500)
    salt_hex, iters, hash_hex = stored.split(":")
    assert int(iters) == 500
    bytes.fromhex(salt_hex)   # корректный hex
    bytes.fromhex(hash_hex)


def test_verify_roundtrip():
    stored = hasher.hash_password("secret", iterations=1000)
    assert hasher.verify_password("secret", stored) is True
    assert hasher.verify_password("wrong", stored) is False


def test_verify_bad_input():
    assert hasher.verify_password("x", "не-формат") is False
    assert hasher.verify_password("x", "") is False


def test_salt_is_random():
    # две хэш-строки одного пароля различаются за счёт случайной соли
    assert hasher.hash_password("p", 100) != hasher.hash_password("p", 100)


# ---- HTTP-эндпоинты (через test_client Flask) ----

def make_client():
    hasher.app.testing = True
    return hasher.app.test_client()


def test_health_endpoint():
    r = make_client().get("/health")
    assert r.status_code == 200
    assert r.get_json()["status"] == "ok"


def test_hash_and_verify_endpoints():
    c = make_client()
    r = c.post("/hash", json={"password": "p", "iterations": 1000})
    assert r.status_code == 200
    data = r.get_json()
    assert "stored" in data and "salt" in data and data["iterations"] == 1000

    ok = c.post("/verify", json={"password": "p", "stored": data["stored"]})
    assert ok.get_json()["valid"] is True

    bad = c.post("/verify", json={"password": "nope", "stored": data["stored"]})
    assert bad.get_json()["valid"] is False


def test_hash_requires_password():
    r = make_client().post("/hash", json={})
    assert r.status_code == 400
