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
* Если меняется поведение во время выполнения, обновляйте тесты **и пользовательскую документацию** в том же pull request’е.

## Разработка с AI-ассистентами

Работа над кодом с помощью AI ассистентов всячески приветствуется! Это не только помогает защититься от банальных ошибок, сделанных случайно, но и позволяет избавиться от рутины печатания текстов в пользу превращения программирования в творческий процесс по "творению" если не миров, то кода. Хе хе :-)

Как демиургу Вам нельзя позволять ассистенту управлять Вами и принимать решения вместо Вас, поэтому созданный код обязательно должен быть проверен вручную.

Пожалуйста, не используйте слабые AI модели для программирования.

## Локальное окружение

### Зависимости по сценариям

* Пакеты для сборки приложения перечислены по операционным системам в основной документации в разделе [«Самостоятельная сборка»](README.ru.md#самостоятельная-сборка)
* Пакеты, необходимые для запуска приложения, собранного с динамическими системными библиотеками, перечислены в подразделе [«Системные библиотеки для запуска приложения»](README.ru.md#системные-библиотеки-для-запуска-приложения)
* Команды установки пакетов для поддерживаемых дистрибутивов приведены в соответствующих Dockerfile в каталоге [`.docker/`](.docker/)
* Пакеты для тестов перечислены в подразделе [«Системные пакеты для тестирования»](#системные-пакеты-для-тестирования)

#### Статический анализ и дополнительные инструменты

Цель `make cppcheck` использует `bear` для создания `compile_commands.json`, после чего запускает `cppcheck`. В Ubuntu и Debian требуются следующие пакеты:

```sh
sudo apt-get update
sudo apt-get install -y bear cppcheck
```

Дополнительные цели анализа, измерения производительности и подготовки документации используют Clang Static Analyzer, Valgrind, Sparse, Splint, Doxygen, Cloc и Gource:

```sh
sudo apt-get install -y clang clang-tools valgrind sparse splint doxygen cloc gource
sudo apt-get install -y linux-tools-common linux-tools-generic linux-tools-$(uname -r)
```

Цель `make clang-analyzer` автоматически выбирает старшую доступную в `PATH` версию `clang` и соответствующую версию `scan-build`. Если исполняемые файлы с номером версии не найдены, используются команды `clang` и `scan-build` без суффикса.

`make spellcheck` использует `typos` из Cargo (`~/.cargo/bin/typos`):

```sh
cargo install typos-cli
```

### Системные пакеты для тестирования

Тестовый набор проверяет отдельные функции, работу приложения через командную строку и результаты обработки файлов из `tests/fixtures/`. SQLite и криптографическая библиотека Monocypher входят в исходный код проекта. Monocypher используется как независимый эталон для проверки SHA512, вычисленного внутренней библиотекой. Отдельные системные пакеты SQLite и внешние криптографические пакеты для запуска тестов не требуются.

Следующие команды устанавливают зависимости для `make tests-debug` и, кроме Alpine Linux, для `make tests` с санитайзерами.

#### Arch Linux

```sh
sudo pacman -S --needed base-devel pcre2 llvm zip unzip
```

#### Ubuntu/Debian Linux

```sh
sudo apt update
sudo apt -y install gcc make libpcre2-dev llvm libubsan1 zip unzip
```

#### Alpine Linux

```sh
sudo apk add --no-cache build-base pcre2-dev pcre2-static fts-dev argp-standalone zip unzip
```

На Alpine Linux санитайзерный режим не поддерживается. Тесты запускаются командой `make tests-debug`.

#### Fedora Linux

```sh
sudo dnf -y install gcc make llvm libasan libubsan glibc-devel glibc-static pcre2-devel pcre2-static zip unzip
```

#### AlmaLinux 10 / Rocky Linux 10

Сборки C2x и тесты с санитайзерами используют системный GCC.

```sh
sudo dnf -y install dnf-plugins-core
sudo dnf config-manager --set-enabled crb
sudo dnf -y install gcc make llvm libasan libubsan glibc-devel glibc-static pcre2-devel pcre2-static zip unzip
```

#### Gentoo Linux

Для PCRE2 требуется поддержка статических библиотек:

```sh
echo "dev-libs/libpcre2 static-libs" | sudo tee /etc/portage/package.use/libpcre2
sudo emerge llvm-core/clang dev-libs/libpcre2 app-arch/zip app-arch/unzip
```

#### macOS

Архиватор `zip` и программа распаковки `unzip` входят в macOS. Для тестовой сборки установите инструменты командной строки Xcode и библиотеки из Homebrew:

```sh
xcode-select --install
brew install llvm pcre2 argp-standalone
```

На macOS используется динамическая сборка с санитайзерами:

```sh
make tests
```

### Клонирование и сборка

Пример сборки и распаковки архива на Linux x86_64 после установки зависимостей:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make production
unzip precizer.zip '*/precizer'
"./v$(make version)/precizer" --version
```

Доступные режимы, команды, назначение получаемых исполняемых файлов и технические различия сборки подробно описаны в основной документации, в подразделе [«Варианты сборки с помощью Make»](README.ru.md#варианты-сборки-с-помощью-make).

Удаление сборочных файлов в `.builds/` с сохранением готовых ZIP-архивов в корне проекта:

```sh
make purge
```

## Стиль кода

* Стандарт языка: `C2x`.
* В сборке включены строгие предупреждения и `-Werror`; новый код должен компилироваться без предупреждений.
* В изменяемых файлах придерживайтесь существующих паттернов именования и структуры.
* Для глобального форматирования всего кода используйте:

```sh
make format && (cd libs && make format) && (cd tests && make format)
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
