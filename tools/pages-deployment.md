# Публикация сайта на https://precizer.github.io/

Этот репозиторий теперь может автоматически синхронизировать содержимое Jekyll-сайта в репозиторий **precizer/precizer.github.io**, чтобы страница открывалась по адресу `https://precizer.github.io/` без суффикса `/precizer/`.

## Подготовка одноимённого репозитория
1. Создайте (или переименуйте существующий) репозиторий `precizer.github.io` в организации/аккаунте `precizer`.
2. Включите GitHub Pages для ветки `main` и корня репозитория.

## Настройка секрета для деплоя
В репозитории `precizer` создайте секрет `USER_SITE_DEPLOY_KEY` с деплой-ключом, который имеет права записи в `precizer/precizer.github.io` (см. [Deploy keys](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/managing-deploy-keys#deploy-keys)).

## Запуск публикации
После пуша в `main` или ручного запуска workflow `Publish user GitHub Pages site` содержимое `_config.yml`, `README.md`, `README.ru.md` и каталога `img` будет скопировано в ветку `main` репозитория `precizer.github.io`.
