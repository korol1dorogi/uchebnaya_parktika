# Docker, Docker Compose и Kubernetes — что это и как пользоваться

Краткий практический гайд по трём инструментам, на которых построена
инфраструктура проекта. Для каждого: суть и смысл, как пользоваться, таблица
основных команд и пример настройки.

---

# 1. Docker

## Что это и в чём смысл

**Docker** — платформа контейнеризации. **Контейнер** — это изолированный
процесс с собственной файловой системой, в который упаковано приложение
вместе со всеми зависимостями (библиотеки, рантайм, конфиг).

- **Образ (image)** — неизменяемый шаблон (рецепт + все слои файловой системы).
- **Контейнер (container)** — запущенный экземпляр образа.
- **Dockerfile** — текстовый «рецепт» сборки образа.
- **Слои (layers)** — образ собирается послойно, слои кэшируются (быстрая пересборка).
- **Реестр (registry)** — хранилище образов (Docker Hub, GHCR).
- **Том (volume)** — постоянные данные, переживающие пересоздание контейнера.

**Смысл:** «работает у меня → работает везде». Одинаковое окружение на машине
разработчика, в CI и в проде; изоляция приложений друг от друга; быстрый старт
(легче виртуальной машины — общее ядро ОС). Решает проблему «а у меня работало».

## Как пользоваться (рабочий цикл)

1. Пишешь **Dockerfile** (как собрать образ).
2. `docker build` → получаешь **образ**.
3. `docker run` → запускаешь **контейнер** из образа.
4. Управляешь: `ps`, `logs`, `exec`, `stop`, `rm`.

## Основные команды

| Команда | Что делает |
|---|---|
| `docker build -t имя:тег .` | собрать образ из Dockerfile в текущей папке |
| `docker images` | список локальных образов |
| `docker run -d -p 8080:80 имя` | запустить контейнер (в фоне, проброс порта хост:контейнер) |
| `docker run -it имя sh` | запустить интерактивно с shell |
| `docker ps` / `docker ps -a` | запущенные / все контейнеры |
| `docker logs -f <контейнер>` | смотреть логи (следить) |
| `docker exec -it <контейнер> sh` | выполнить команду / зайти внутрь |
| `docker stop` / `start` / `rm <контейнер>` | остановить / запустить / удалить |
| `docker rmi <образ>` | удалить образ |
| `docker pull` / `push <образ>` | скачать / выгрузить образ из реестра |
| `docker system prune -a` | удалить неиспользуемые образы/контейнеры (очистка) |

## Пример настройки (Dockerfile проекта)

Multi-stage сборка: в первой стадии собираем бинарник, во второй — кладём
только его в чистый образ (меньше размер, нет тулчейна в проде).

```dockerfile
# ---- стадия сборки ----
FROM drogonframework/drogon AS build
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --parallel "$(nproc)"

# ---- рантайм-стадия ----
FROM drogonframework/drogon
WORKDIR /app
COPY --from=build /src/build/praktika ./praktika
COPY config.json ./config.json
EXPOSE 8081
CMD ["./praktika"]
```

Использование:
```bash
docker build -t praktika:latest .
docker run --rm -t -p 8082:8081 praktika:latest
# открыть http://127.0.0.1:8082/api/health
```

---

# 2. Docker Compose

## Что это и в чём смысл

**Docker Compose** — инструмент для описания и запуска **многоконтейнерных**
приложений одним YAML-файлом (`docker-compose.yml`) и одной командой.

Вместо того чтобы вручную делать несколько `docker run`, прокидывать сети,
тома и переменные, ты декларативно описываешь все сервисы — и поднимаешь весь
стек целиком.

- **service** — один контейнер с его настройками (образ/сборка, порты, env, тома).
- Compose создаёт **общую сеть**, где сервисы видят друг друга **по имени**
  (например, `praktika` обращается к `db` и `hasher` просто по этим именам).
- **`.env`** — файл с переменными, который Compose подхватывает автоматически.

**Смысл:** наш проект — это praktika + hasher + PostgreSQL + сеть + тома + env.
Compose поднимает всё это одной командой, связывает и пробрасывает порты.
Идеально для локальной разработки и простого деплоя.

## Как пользоваться

1. Описываешь сервисы в `docker-compose.yml`.
2. `docker compose up -d --build` — собрать образы и поднять весь стек.
3. `docker compose logs` / `ps` — наблюдать.
4. `docker compose down` — остановить и убрать.

## Основные команды

| Команда | Что делает |
|---|---|
| `docker compose up -d --build` | собрать образы и поднять весь стек в фоне |
| `docker compose down` | остановить и удалить контейнеры и сеть |
| `docker compose down -v` | то же + удалить тома (то есть **данные БД**) |
| `docker compose ps` | статус сервисов |
| `docker compose logs -f [сервис]` | логи всех / конкретного сервиса |
| `docker compose build [сервис]` | пересобрать образы |
| `docker compose restart [сервис]` | перезапустить |
| `docker compose exec <сервис> sh` | shell внутри сервиса |
| `docker compose pull` | скачать образы сервисов |

## Пример настройки (фрагмент docker-compose.yml проекта)

```yaml
services:
  db:                                    # сервис PostgreSQL
    image: postgres:16-alpine
    environment:
      POSTGRES_DB: praktika
      POSTGRES_USER: praktika
      POSTGRES_PASSWORD: ${DB_PASSWORD}  # из .env
    volumes:
      - db_data:/var/lib/postgresql/data # постоянные данные

  hasher:                                # Python-микросервис
    build: ./hasher

  praktika:                              # основной сервис
    build: .
    depends_on: [db, hasher]             # порядок старта
    ports:
      - "8082:8081"                      # хост:контейнер
    environment:
      - DB_HOST=db                       # обращение к сервису db по имени
      - HASHER_URL=http://hasher:5005

volumes:
  db_data:
```

Использование:
```bash
cp .env.example .env          # задать GROQ_API_KEY и DB_PASSWORD
docker compose up -d --build  # поднять весь стек
docker compose logs -f praktika
docker compose down           # остановить
```

---

# 3. Kubernetes (кубер)

## Что это и в чём смысл

**Kubernetes (k8s)** — оркестратор контейнеров. Ты декларативно описываешь
*желаемое состояние* (сколько реплик, какие сервисы, какие ресурсы), а кластер
через **контроллеры** постоянно приводит реальность к этому состоянию.

Что даёт сверх Compose:
- **Self-healing** — упал контейнер/узел → кластер сам поднимает новый.
- **Масштабирование** — горизонтально нарастить число реплик.
- **Rolling-update** — обновление без простоя.
- **Service discovery + балансировка** — стабильные адреса и распределение нагрузки.

**Смысл:** Compose хорош для одной машины и разработки. Kubernetes — для
продакшена: много узлов, отказоустойчивость, масштабирование, нулевой простой.
**minikube** — локальный кластер из одного узла для обучения.

Ключевые объекты:

| Объект | Назначение |
|---|---|
| **Pod** | минимальная единица — один (или несколько) контейнеров |
| **Deployment** | управляет подами: число реплик, self-healing, обновления |
| **Service** | стабильное имя/IP + балансировка на поды (ClusterIP — внутри, NodePort — наружу) |
| **ConfigMap / Secret** | несекретные / секретные настройки (отдаются в под как env) |
| **PVC** | постоянный диск (данные переживают пересоздание пода) |
| **Namespace** | изоляция группы ресурсов |
| **probes** | readiness (готов к трафику) и liveness (жив ли, иначе рестарт) |

## Как пользоваться (с minikube)

1. `minikube start` — поднять кластер.
2. Собрать образы и **занести** их в кластер (`minikube image load`).
3. Описать объекты в YAML, применить `kubectl apply -f`.
4. Смотреть `kubectl get/describe/logs`, достучаться `port-forward`.

## Основные команды

| Команда | Что делает |
|---|---|
| `minikube start --driver=docker` | поднять локальный кластер |
| `minikube image load имя:тег` | занести локальный образ в кластер |
| `kubectl apply -f файл.yaml` | создать/обновить объекты из манифеста |
| `kubectl get pods,svc -n praktika` | список подов и сервисов в namespace |
| `kubectl describe pod <под> -n praktika` | подробности и события (отладка) |
| `kubectl logs -f <под> -n praktika` | логи пода |
| `kubectl exec -it <под> -n praktika -- sh` | shell внутри пода |
| `kubectl rollout restart deploy/<имя> -n praktika` | перезапуск после нового образа |
| `kubectl scale deploy/<имя> --replicas=3 -n praktika` | масштабировать |
| `kubectl port-forward svc/<сервис> 8082:8081 -n praktika` | проброс порта на хост |
| `kubectl delete -f файл.yaml` / `kubectl delete ns praktika` | удалить объекты / namespace |
| `minikube stop` / `minikube delete` | остановить / снести кластер |

## Пример настройки (Deployment + Service)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: praktika
  namespace: praktika
spec:
  replicas: 1                      # желаемое число подов
  selector:
    matchLabels: { app: praktika }
  template:
    metadata:
      labels: { app: praktika }
    spec:
      containers:
        - name: praktika
          image: praktika:latest
          imagePullPolicy: IfNotPresent   # брать локальный образ (из image load)
          ports:
            - containerPort: 8081
          readinessProbe:                  # не слать трафик, пока не готов
            httpGet: { path: /api/health, port: 8081 }
---
apiVersion: v1
kind: Service
metadata:
  name: praktika
  namespace: praktika
spec:
  type: NodePort                    # доступ снаружи кластера
  selector: { app: praktika }       # на какие поды направлять
  ports:
    - port: 8081
      targetPort: 8081
      nodePort: 30081
```

Использование:
```bash
minikube start --driver=docker
docker compose build                      # собрать образы
minikube image load praktika:latest       # занести в кластер
kubectl apply -f k8s/namespace.yaml
kubectl apply -f k8s/praktika.yaml
kubectl -n praktika get pods              # ждём Running
kubectl -n praktika port-forward svc/praktika 8082:8081
# открыть http://127.0.0.1:8082/
```

---

> Полные манифесты проекта — в каталоге `k8s/` (см. также `k8s/docs.md`),
> рабочий `docker-compose.yml` и `Dockerfile` — в корне репозитория.
