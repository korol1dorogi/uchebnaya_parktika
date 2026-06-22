# Kubernetes с нуля (minikube) — на примере этого проекта

Гайд: как пользоваться кластером от установки minikube до обновления и отладки.
Минимальный «маршрут» — в конце файла.

## 0. Мысленная модель (отличие от docker compose)

Kubernetes (k8s) — оркестратор: ты декларативно описываешь *желаемое
состояние*, а кластер постоянно следит, чтобы реальное состояние ему
соответствовало (упал под → поднимет новый). minikube — локальный кластер из
одного узла для учёбы.

| docker compose | Kubernetes |
|---|---|
| сервис | **Deployment** (управляет подами) + **Service** (сеть) |
| контейнер | живёт внутри **Pod** (минимальная единица, 1+ контейнеров) |
| `depends_on` | **initContainer** / **probes** |
| `volumes:` | **PVC** (PersistentVolumeClaim) |
| `.env` | **ConfigMap** (несекретное) + **Secret** (секреты) |
| `ports: 8082:8081` | **Service** (NodePort) + `port-forward` |

Инструменты:
- **minikube** — управляет самим кластером (старт/стоп).
- **kubectl** — «пульт» к кластеру (создать/посмотреть/удалить объекты), основной инструмент.

## 1. Установка и запуск кластера

```bash
winget install Kubernetes.minikube      # один раз
minikube start --driver=docker          # поднять кластер (через Docker)
```
`minikube start` сам настраивает `kubectl`. Проверка:
```bash
kubectl config current-context          # -> minikube
kubectl get nodes                        # узел minikube Ready
minikube status
```

## 2. Ключевой нюанс с образами

У minikube **свой Docker-демон**. Локально собранные образы кластер не видит,
поэтому их надо «занести»:
```bash
docker compose build
minikube image load praktika:latest
minikube image load praktika-hasher:latest
minikube image load postgres:16-alpine
```
В манифесте — `imagePullPolicy: IfNotPresent` (брать локальный, не из реестра).

## 3. Из чего состоит описание (наши манифесты в `k8s/`)

Каждый объект YAML: `apiVersion`, `kind`, `metadata`, `spec`.

- **Namespace** (`namespace.yaml`) — изолятор для ресурсов (`praktika`).
- **ConfigMap** (`configmap.yaml`) — несекретные переменные (`DB_HOST=db`, ...).
- **Secret** — секреты (`GROQ_API_KEY`, `DB_PASSWORD`), из `.env`, в git не кладём.
- **PVC + Deployment + Service** Postgres (`postgres.yaml`) — БД с постоянным диском.
- **Deployment + Service** для `hasher` и `praktika`.
- **probes** — k8s проверяет здоровье (`/api/health`); **initContainer** `wait-for-db`
  ждёт БД (замена `depends_on`).

Связь по сети — по **имени Service**, не по IP. Service'ы названы `db` и `hasher`,
как `DB_HOST`/`HASHER_URL` в коде, поэтому код не менялся.

## 4. Применение (создание объектов)

`kubectl apply -f <файл>` = «приведи кластер к этому описанию».
```bash
kubectl apply -f k8s/namespace.yaml
kubectl apply -f k8s/configmap.yaml
kubectl -n praktika create secret generic praktika-secrets --from-env-file=.env
kubectl -n praktika create configmap db-init --from-file=init.sql=db/init.sql
kubectl apply -f k8s/postgres.yaml
kubectl apply -f k8s/hasher.yaml
kubectl apply -f k8s/praktika.yaml
```
`-n praktika` = «в namespace praktika». Сделать namespace по умолчанию:
```bash
kubectl config set-context --current --namespace=praktika
```

## 5. Смотреть и отлаживать (ежедневные команды)

```bash
kubectl -n praktika get pods              # статусы подов
kubectl -n praktika get pods,svc          # + сервисы
kubectl -n praktika describe pod <имя>    # подробности и события
kubectl -n praktika logs <под>            # логи (как docker logs)
kubectl -n praktika logs -f deploy/praktika
kubectl -n praktika exec -it <под> -- sh  # зайти внутрь
kubectl -n praktika get events --sort-by=.lastTimestamp
```
Статусы: `Running` — ок; `Pending` — не запланировался; `ImagePullBackOff` —
не нашёл образ (забыл `image load`); `CrashLoopBackOff` — контейнер падает (смотри `logs`).

## 6. Доступ к приложению с хоста

На Docker-драйвере NodePort напрямую недоступен:
```bash
kubectl -n praktika port-forward svc/praktika 8082:8081
# http://127.0.0.1:8082/
# либо:
minikube service praktika -n praktika --url
```

## 7. Цикл обновления после правок кода

Под не обновится сам при том же теге образа. Нужно:
```bash
docker compose build praktika
minikube image load praktika:latest
kubectl -n praktika rollout restart deploy/praktika
kubectl -n praktika rollout status deploy/praktika
```
(В проде: пуш образа с новым тегом в реестр + смена тега в манифесте.)

## 8. Self-healing (k8s сам поднимает упавшее)

Deployment держит заданное число реплик. Если под удалить/уронить — контроллер
сразу создаёт новый:
```bash
kubectl -n praktika get pods
kubectl -n praktika delete pod <под-praktika>      # «роняем»
kubectl -n praktika get pods                        # старый Terminating, новый ContainerCreating
kubectl -n praktika rollout status deploy/praktika  # снова Ready
```
Так же отработают liveness-probe (перезапуск зависшего контейнера) и пересоздание
пода при падении узла.

## 9. Остановка и очистка

```bash
kubectl delete namespace praktika    # удалить наши объекты (вкл. PVC с данными)
minikube stop                        # остановить кластер, сохранив состояние
minikube start                       # снова поднять
minikube delete                      # снести кластер целиком
```

## Минимальный маршрут с нуля

1. `winget install Kubernetes.minikube`
2. `minikube start --driver=docker`
3. `docker compose build` → `minikube image load <образы>`
4. `kubectl apply -f k8s/...` (+ создать secret и db-init)
5. `kubectl -n praktika get pods` (ждём `Running`)
6. `kubectl -n praktika port-forward svc/praktika 8082:8081` → браузер
7. правки → пересборка → `image load` → `rollout restart`
8. `minikube stop` когда закончил
