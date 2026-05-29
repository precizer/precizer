# Вклад в `precizer`

В этом документе описано, как вносить изменения в код, тесты и документацию.

Ищете, чем заняться? Загляните в Issues: [https://github.com/precizer/precizer/issues](https://github.com/precizer/precizer/issues)
Там публикуются баги, задачи и запросы на новые фичи — под разный уровень вовлечённости.

## С чего начать

* Баг-репорты и запросы фич: [https://github.com/precizer/precizer/issues/new](https://github.com/precizer/precizer/issues/new)
* Технические обсуждения: [https://github.com/precizer/precizer/discussions](https://github.com/precizer/precizer/discussions)
* Pull request’ы приветствуются для: кода, тестов, документации и улучшений сборки

Несколько правил игры:

* Один pull request — одно логическое изменение.
* Для всего, что не совсем тривиально, сначала согласуйте идею и рамки в issue/обсуждении, и только потом пишите код.
* Если меняется поведение во время выполнения, обновляйте тесты **и** пользовательскую документацию в том же pull request’е.

## Разработка с AI-ассистентами

Работа над кодом с помощью AI ассистентов всячески приветствуется! Это не только помогает защититься от банальных ошибок, сделанных случайно, но и позволяет избавиться от рутины печатания текстов в пользу превращения программирования в творческий процесс по "творению" если не миров, то кода. Хе хе :-)

Как местному демиургу Вам нельзя позволять ассистенту управлять Вами и принимать решения вместо Вас, поэтому созданный код обязательно должен быть проверен вручную.

Пожалуйста, не используйте слабые AI модели для программирования.

## Локальное окружение

### Зависимости по сценариям

Ниже — матрица зависимостей для четырёх типовых сценариев.

Источники: `Makefile`, `tests/Makefile`, `.docker/Dockerfile.*`, `.github/workflows/precizer.yml`.
Подробности по пакетам для конкретных дистрибутивов (AlmaLinux, Alpine, Arch, Debian, Gentoo, Rocky, Ubuntu) — в `.docker/Dockerfile.<distro>`.

#### 1. Статическая сборка (`make portable` или `make production`)

Нужно:

* компилятор: `gcc` (или `clang` при `make clang`)
* сборка: `make`
* заголовки regex-библиотеки: `libpcre2-dev`
* упаковщик исполняемого файла, который используется в сборке: `upx-ucl`
* `llvm` рекомендуется для дальнейших сценариев с санитайзерами/отладкой

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc clang make libpcre2-dev upx-ucl llvm llvm-dev
```

Примечание: для `portable/production` `sqlite3` собирается из `libs/sqlite3`, системный пакет `libsqlite3-dev` для этих статических таргетов не нужен.

#### 2. Динамическая сборка (`make dynamic-production`)

Дополнительно нужно:

* системные dev-библиотеки для `sqlite3` и `pcre2`

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc make libpcre2-dev libsqlite3-dev upx-ucl
```

#### 3. Запуск тестов (`make tests`) с санитайзерами

Нужно:

* зависимости из пунктов 1 и 2
* тулчейн санитайзеров (`ASan`/`UBSan`) и `llvm-symbolizer`

Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y gcc make libpcre2-dev libsqlite3-dev llvm llvm-dev upx-ucl
```

#### 4. Статический анализ и инструменты (`cppcheck` и связанные таргеты)

Минимум для `make cppcheck`:

```sh
sudo apt-get update
sudo apt-get install -y cppcheck
```

Базовый набор диагностик из комментариев `Makefile`:

```sh
sudo apt-get install -y cloc valgrind clang-tools cppcheck
```

Расширенный набор для дополнительных таргетов (`make analyze`, `make perf`, `make sparse-analyzer`, `make splint`, `make doc`, `make spellcheck`):

```sh
sudo apt-get install -y valgrind cppcheck clang-20 clang-tools-20 sparse splint doxygen cloc gource
sudo apt-get install -y linux-tools-common linux-tools-generic linux-tools-$(uname -r)
```

Примечание: `make clang-analyzer` сейчас использует имена `clang-20` и `scan-build-20` из `Makefile`. Если на вашей системе пакеты называются иначе, подстройте окружение соответствующим образом.

`make spellcheck` использует `typos` из Cargo (`~/.cargo/bin/typos`):

```sh
cargo install typos-cli
```

### Клонирование и сборка

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make production
./precizer --version
```

Варианты сборки:

* `make portable` — статически линкованный переносимый бинарник (Linux)
* `make production` — статический бинарник, оптимизированный под локальный CPU
* `make dynamic-production` — динамически линкованный бинарник, оптимизированный под локальный CPU

Поведение режимов сборки и технические различия подробно описаны в `README.md`, раздел [Building with Docker](README.md#building-with-docker).

Очистка (рекурсивно удаляет `.builds`):

```sh
make purge
```

## Стиль кода

* Стандарт языка: `C2x`.
* В сборке включены строгие предупреждения и `-Werror`; новый код должен компилироваться без предупреждений.
* В изменяемых файлах придерживайтесь существующих паттернов именования и структуры.
* Форматируйте только то, что затронули:

```sh
make format
cd libs && make format
cd tests && make format
```

## Тестирование

Минимум перед открытием pull request’а:

```sh
make tests
```

Куда добавлять тесты:

* основной тестовый контур: `tests/`
* исходники тестов: `tests/src/` (шаблон имени: `testXXXX.c`)
* шаблоны ожидаемого вывода: `tests/templates/`
* файловые фикстуры: `tests/fixtures/`

В документе [TESTING](TESTING.ru.md) есть краткое описание тестового фреймворка: dual-path прогоны (in-process и black-box CLI), контракты вывода и состояния, санитайзеры, отчёты покрытия, а также рекомендации, чего лучше избегать при написании тестов.

## Коммиты и Pull Request’ы

* Создайте рабочую ветку от `main`.
* Пишите понятные сообщения коммитов в повелительном наклонении.
* Не коммитьте артефакты сборки и временные файлы (`.builds/`, `precizer`, временные `.db` и т. п.).

В описании pull request’а укажите:

1. какую проблему решаете;
2. точный объём изменений;
3. какие команды проверки выполнялись (например, `make tests`);
4. известные ограничения и идеи для дальнейшей работы.

Если меняется поведение CLI, обновляйте `README.md` в том же pull request’е.

## Лицензия

Отправляя изменения, контрибьюторы соглашаются, что вклад распространяется на условиях лицензирования репозитория:

* `COPYING`
* `README.md`, раздел `COPYING`
