# Kubernetes (minikube)

Манифесты для запуска стека (praktika + hasher + postgres) в кластере.
Код приложения менять не нужно: сервисы называются `db` и `hasher` — так же,
как в `docker-compose`, поэтому DNS внутри кластера совпадает с `DB_HOST`/
`HASHER_URL`.

## Состав

- `namespace.yaml` — namespace `praktika`
- `configmap.yaml` — несекретные настройки (DB_HOST/PORT/NAME/USER, HASHER_URL)
- `secret.example.yaml` — шаблон секрета (реальный создаём из `.env`, в git не коммитим)
- `postgres.yaml` — Deployment + PVC + Service `db`
- `hasher.yaml` — Deployment + Service `hasher`
- `praktika.yaml` — Deployment (+ initContainer ожидания БД, resource requests) + Service NodePort
- `hpa.yaml` — HorizontalPodAutoscaler (автоскейл praktika по CPU)

## Запуск

Требуется запущенный minikube (`minikube start --driver=docker`) и собранные
образы (`docker compose build`).

```bash
# 1. Загрузить локальные образы в кластер minikube
minikube image load praktika:latest
minikube image load praktika-hasher:latest
minikube image load postgres:16-alpine

# 2. Namespace и конфиг
kubectl apply -f k8s/namespace.yaml
kubectl apply -f k8s/configmap.yaml

# 3. Секрет из .env (НЕ коммитится) — даёт ключи GROQ_API_KEY и DB_PASSWORD
kubectl -n praktika create secret generic praktika-secrets --from-env-file=.env

# 4. Схема БД как ConfigMap (монтируется в /docker-entrypoint-initdb.d)
kubectl -n praktika create configmap db-init --from-file=init.sql=db/init.sql

# 5. Рабочие нагрузки
kubectl apply -f k8s/postgres.yaml
kubectl apply -f k8s/hasher.yaml
kubectl apply -f k8s/praktika.yaml

# 6. Статус
kubectl -n praktika get pods,svc

# 7. Доступ с хоста (Docker-драйвер: через port-forward или minikube service)
kubectl -n praktika port-forward svc/praktika 8082:8081
#   -> http://127.0.0.1:8082/
# либо:
minikube service praktika -n praktika --url
```

## Автоскейлинг (HPA)

HPA сам меняет число реплик praktika по средней загрузке CPU
(min 1, max 5, цель 50% от `resources.requests.cpu`). Нужен metrics-server
и заданные requests (они уже есть в `praktika.yaml`).

```bash
# 1. метрики
minikube addons enable metrics-server

# 2. применить HPA
kubectl apply -f k8s/hpa.yaml
kubectl -n praktika get hpa            # TARGETS: cpu: N%/50%

# 3. нагрузка (60 параллельных циклов запросов изнутри кластера)
kubectl -n praktika run loadgen --image=busybox --restart=Never -- /bin/sh -c \
  'for i in $(seq 1 60); do (while true; do wget -q -O /dev/null http://praktika:8081/api/hello; done) & done; sleep 600'

# 4. наблюдать рост реплик
kubectl -n praktika get hpa,pods -w

# 5. снять нагрузку -> через ~минуту реплики схлопнутся обратно к 1
kubectl -n praktika delete pod loadgen
```

## Удаление

```bash
kubectl delete namespace praktika      # всё, включая PVC с данными
```
