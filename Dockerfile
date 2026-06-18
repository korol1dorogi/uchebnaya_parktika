# ---- Стадия сборки ----
# Официальный образ Drogon: в нём уже установлен фреймворк и тулчейн.
FROM drogonframework/drogon AS build

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --parallel "$(nproc)"

# ---- Рантайм-стадия ----
# Та же база, что и сборочная => все нужные библиотеки уже присутствуют.
FROM drogonframework/drogon

# curl нужен для HEALTHCHECK ниже
RUN apt-get update \
 && apt-get install -y --no-install-recommends curl \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/praktika ./praktika
COPY config.json ./config.json

EXPOSE 8081

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -fsS http://127.0.0.1:8081/api/health || exit 1

CMD ["./praktika"]
