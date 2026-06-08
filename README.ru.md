[<img src=".html/img/i18n-icon.svg"> Link to the English language README page](README.md)

# Precizer — проверка целостности данных для файловых систем любого масштаба

Крошечное, высокопроизводительное приложение для проверки целостности файлов

«По-настоящему хорошая программа всегда поместится на дискету. Есть надежда, что кто-то всё ещё помнит, что это такое… Речь идёт не о дискетах, а о качественных программах!»<sup>©</sup> :-D

<p width="100%" height="100%"><img width="20%" src=".html/img/micrometer_0.svg"></p>

## Continuous integration and automation

### Гибридный интеграционно-системный набор тестов

* In-process integration tests
* Out-of-process CLI system tests
* Unit tests

#### Test coverage:

<a href="https://precizer.github.io/code_coverage_report/"><img src=".html/img/test-coverage-total.svg" height="20" alt="Total completed tests" /><br>
<img src=".html/img/test-coverage-lines.svg" height="20" alt="Lines of code covered by tests" /><br>
<img src=".html/img/test-coverage-functions.svg" height="20" alt="Functions covered by tests" /><br>
<img src=".html/img/test-coverage-branches.svg" height="20" alt="Code branches covered by tests" /></a>

### Automated builds:

[![precizer build & testing](https://github.com/precizer/precizer/actions/workflows/precizer.yml/badge.svg)](https://github.com/precizer/precizer/actions/workflows/precizer.yml)

### Security:

[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/12159/badge)](https://www.bestpractices.dev/projects/12159)  
[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/precizer/precizer/badge)](https://scorecard.dev/viewer/?uri=github.com/precizer/precizer)

## КРАТКО

### Обзор

**precizer** — крошечное и быстрое консольное приложение, полностью написанное на чистом Си. Предназначено для проверки целостности и сравнения файлов. Особенно полезно для проверки результатов синхронизации. Программа обходит дерево каталогов и создаёт базу данных файлов и их контрольных сумм с последующим быстрым сравнением.

**precizer** предназначается как для работы на embedded-платформах, так и для файловых систем гигантского размера в кластерных мейнфрейм-окружениях. С помощью программы можно находить ошибки синхронизации, сравнивая файлы и их контрольные суммы из разных источников. Также **precizer** можно использовать для исследования исторических изменений, сравнивая базы данных, полученные из одного и того же источника в разные моменты времени.

### Простой пример

Допустим, есть две машины, у которых в `/mnt1` и `/mnt2` соответственно примонтированы диски большого объёма с идентичным содержимым. Стоит задача побайтно проверить, действительно ли содержимое абсолютно идентично или есть различия.

1. Запустить программу на первой машине с hostname, например «host1»:

```sh
precizer --progress /mnt1
```

В результате работы программы будут исследованы все директории, начиная с `/mnt1`, и в текущей директории будет создана база данных host1.db. Параметр `--progress` визуализирует прогресс и покажет объём пространства и количество исследуемых файлов.

2. Запустить программу на второй машине с hostname, например «host2»:

```sh
precizer --progress /mnt2
```

В результате будет создана база данных host2.db в текущей директории.

3. Скопировать файлы с базами данных host1.db и host2.db на одну из машин и запустить программу с соответствующими параметрами для сравнения:

```sh
precizer --compare host1.db host2.db
```

На экран будет выведена следующая информация:
* Какие файлы отсутствуют на «host1», но при этом присутствуют на «host2» и наоборот.
* Для каких файлов, присутствующих на обоих хостах, контрольные суммы НЕ совпадают.

### Относительные пути в базе данных для сравнения

Следует обратить внимание, что **precizer** записывает в базу данных только относительные пути. Файл

```
/mnt1/abc/def/aaa.txt
```

из приведённого примера будет записан в базу данных как

```
abc/def/aaa.txt
```

без `/mnt1`. То же самое произойдёт с файлом

```
/mnt2/abc/def/aaa.txt
```

Смысл в том, что, несмотря на разные точки монтирования и разные источники, файлы можно будет сравнить между собой под одинаковыми именами

```
abc/def/aaa.txt
```

и с соответствующими контрольными суммами.

## [DOWNLOAD](https://github.com/precizer/precizer/releases/latest/)

Скачать [https://github.com/precizer/precizer/releases/latest/](https://github.com/precizer/precizer/releases/latest/) исполняемые файлы для:

* Linux x86_64 [precizer_linux_x86_64_portable.zip](https://github.com/precizer/precizer/releases/latest/download/precizer_linux_x86_64_portable.zip)
* Linux arm aarch64 [precizer_linux_aarch64_portable.zip](https://github.com/precizer/precizer/releases/latest/download/precizer_linux_aarch64_portable.zip)
* macOS arm64 [precizer_macos_arm64.zip](https://github.com/precizer/precizer/releases/latest/download/precizer_macos_arm64.zip)

Пакеты содержат переносимые исполняемые бинарные файлы в архиве zip (portable‑версии).

### Скачать, распаковать и запустить
Универсальный способ для автоматизации обновлений на новые версии

```sh
# Automation for downloading and unarchiving new versions

# Download
wget -O precizer.zip -q "https://github.com/precizer/precizer/releases/latest/download/precizer_$(uname -s | tr '[:upper:]' '[:lower:]' | sed 's/darwin/macos/')_$(uname -m | sed 's/amd64/x86_64/')$( [ "$(uname -s)" = "Linux" ] && echo '_portable' ).zip"

# Extract the archive
unzip -jqo precizer.zip '*/precizer' -d ./

# Run
./precizer --version
```

### Технические детали portable сборки

* Готовая Linux сборка представляет собой один исполняемый, статически слинкованный бинарный файл в формате ELF, не привязанный к какому-либо определённому дистрибутиву. Файл может быть запущен сразу, практически на любом дистрибутиве Linux и не требует использования внешних, динамически подгружаемых библиотек.

* Файл собран CI/CD автосборкой ресурса GitHub, затем сжат с помощью [UPX (архиватора исполняемых файлов)](https://upx.github.io). После этого самораспаковывающийся сжатый бинарный файл вкладывается в ZIP-архив для удобного скачивания. Для использования достаточно распаковать файл из архива и запустить.

* На macOS статическая линковка не поддерживается, поэтому для запуска скачанного приложения нужно озаботиться наличием в системе таких библиотек, как sqlite3, pcre2, argp и fts.

## ИСТОРИЯ ИЗМЕНЕНИЙ

Список изменений по версиям можно найти в отдельном файле [CHANGELOG](CHANGELOG.md)

## ТЕХНИЧЕСКИЕ ПОДРОБНОСТИ АЛГОРИТМОВ РАБОТЫ

Рассмотрим сценарий, когда имеется основное дисковое хранилище и его копия.
Например, это может быть хранилище датацентра и его Disaster Recovery‑копия.
Периодически происходит синхронизация с основного хранилища на резервное, но по причине огромных объёмов данных, скорее всего, синхронизация происходит не побайтно, а за счёт вычисления изменений среди метаданных файлов на файловой системе.
В таких случаях учитывается размер файла и время модификации, но изменившееся содержимое байт за байтом не исследуется.
В этом есть смысл, потому что между основным датацентром и резервным Disaster Recovery‑центром, как правило, хорошие каналы связи, но полная побайтовая синхронизация может занять нецелесообразно много времени.
Такие инструменты, как `rsync`, позволяют производить синхронизацию по обеим методикам: как с учётом изменившихся файлов, так и побайтово, но у них есть один серьёзный недостаток — *состояние не сохраняется между сессиями*. Что это значит, рассмотрим детально:

* Даны: сервер «A» и сервер «B» (основной датацентр и резервный Disaster Recovery‑центр)
* На сервере «A» изменились некоторые файлы.
* Алгоритм `rsync` их определил за счёт изменившегося размера и времени модификации файла и синхронизировал на сервер «B».
* Во время синхронизации между основным датацентром и Disaster Recovery‑центром происходили многократные сбои связи.
* Для проверки целостности данных (эквивалентности сохранённых файлов на «A» и «B» байт в байт) обычно используют тот же `rsync` только с включением побайтного сравнения. Для этого:
  * `rsync` запускается на сервере «A» в режиме `--checksum` и во время одного сеанса пытается подсчитать контрольные суммы последовательно: сначала на «A», а затем на «B».
  * Этот процесс занимает неимоверно много времени для огромных дисковых массивов.
  * Так как `rsync` не позволяет сохранять состояние уже подсчитанных контрольных сумм между сеансами, возникает целый ряд технических сложностей. А именно:
    * В случае разрыва соединения `rsync` завершает сеанс и в следующий запуск всё нужно начинать сначала! С учётом огромных объёмов побайтовая проверка данных на полную идентичность таким образом превращается в нереализуемую задачу.
  * Причиной неидентичности бинарного содержимого файлов могут стать также сбои на уровне дисковой подсистемы. В таких случаях с помощью метаданных файловой системы невозможно будет определить, есть ли разница между содержимым внутри файлов на серверах «A» и «B», или нет.
  * Со временем ошибки накапливаются и появляется угроза получить неконсистентную копию системы «A» на системе «B», что сводит на нет все усилия и затраты по поддержанию Disaster Recovery‑центра. При этом стандартные утилиты не обладают возможностями проверок и технический персонал даже не будет знать о накопившихся проблемах с неэквивалентным содержанием дисковых массивов на Disaster Recovery‑центре.
* Для устранения вышеописанных недостатков создана программа precizer. Программа позволяет выявить, какие именно файлы отличаются между «A» и «B» для проведения повторной синхронизации с устранением отличий. Программа работает максимально быстро (практически на грани аппаратных возможностей) за счёт того, что написана на чистом Си и использует современные алгоритмы, оптимизированные под высокую производительность. Программа предназначена для работы как с мелкими файлами, так и с объёмами данных, измеряемыми петабайтами, и не ограничена этими цифрами.
* Название программы precizer происходит от слова precision (точность) и означает что-то, что увеличивает точность.
* Программа с высокой точностью исследует содержимое директорий, субдиректорий и подсчитывает контрольные суммы для каждого встреченного файла, при этом сохраняя метаинформацию о всех файлах в SQLite базе (обычный бинарный файл).
* precizer устойчива к сбоям и умеет продолжить работу с того момента, когда была прервана. Например, если программа была остановлена нажатием Ctrl+C в момент анализа файла петабайтного объёма, то при повторном запуске она НЕ будет анализировать его с самого начала, а продолжит именно с того момента, на котором была прервана и о котором уже есть запись в базе данных. Это позволит сэкономить ресурсы, время и нервы системных администраторов.
* Работа этой программы может быть прервана в любой момент любым способом, и это безопасно как для исследуемых данных, так и для БД, созданной самой программой.
* В случае умышленной или случайной остановки работы программы можно не беспокоиться о результатах сбоя. Результат работы будет полностью сохранён и повторно использован при следующих запусках.
* Для подсчёта контрольных сумм используется криптографически стойкий, надёжный и быстрый алгоритм SHA512 с очень высокой практической устойчивостью к коллизиям. Если два больших файла различаются хотя бы на один байт, SHA512 с подавляющей вероятностью отразит это в разных контрольных суммах; в отличие от CRC32 и устаревшего SHA1, этот алгоритм предназначен именно для надёжной проверки целостности данных
* Алгоритмы программы precizer разработаны так, что очень просто поддерживать актуальность содержащихся данных в созданной базе с путями к файлам и их контрольными суммами без пересчёта всего с самого начала. Достаточно запустить программу с параметром `--update`, чтобы в БД попали новые файлы или была удалена информация о стёртых с диска. Для тех файлов, которые подверглись модификациям и чьи размеры изменились, будет пересчитана контрольная сумма SHA512, и обновлённая контрольная сумма будет записана в БД.
* При `--update` записи об отсутствующих файлах удаляются, но записи о недоступных файлах (например, из-за прав доступа) по умолчанию сохраняются. Такая защита нужна потому, что права могут временно меняться (владелец, ACL, проблемы монтирования), и при удалении записей в этот момент можно незаметно потерять корректную историю в БД. Использование `--db-drop-inaccessible` вместе с `--update` оправдано только если нужно действительно удалить эти записи из БД.
* При включённом `--progress` предупреждения и ошибки, накопленные в течение сессии, выводятся единым блоком перед завершением программы, чтобы важные сообщения (например, об отсутствии доступа к файлам) не терялись на фоне второстепенных логов.
* Опция `--quiet-ignored` отключает вывод строк о файлах, отфильтрованных через `--ignore` и `--include`. Это помогает не засорять логи программы лишними сообщенями, когда регулярные выражения для игнорирования уже отлажены и стабильны в работе; остальные предупреждения и ошибки продолжают выводиться.
* Можно указать опцию, при которой при обновлении БД будет учитываться не только размер изменившихся файлов, но также и время создания или изменения файлов. Это значит, что изменения любой метаинформации о файле приведут к пересчёту контрольной суммы SHA512 с последующим обновлением данных о файле в БД. Например, если у файла изменилась ctime, но не изменился размер, то контрольная сумма для такого файла НЕ будет пересчитана при указании только одного параметра `--update`. Чтобы контрольная сумма для таких файлов была пересчитана, необходимо добавить `--watch-timestamps`. Эта опция не включена по умолчанию, потому что ctime (как и mtime) могут меняться часто, например, командами `chmod` или `chown`, при том, что само содержимое файла остаётся прежним.
* precizer может служить инструментом контроля безопасности, определяя последствия вторжения за счёт выявления несанкционированно изменённых файлов, у которых могло быть модифицировано содержимое, но метаданные могли остаться прежними.
* Безопасность:
  * Программа никогда не меняет, не удаляет, не перемещает и не копирует ни файлы, ни исследуемые директории.
  * Программа составляет списки файлов, контрольные суммы содержимого этих файлов и сохраняет их в локальной базе данных. Все изменения происходят исключительно в границах базы данных.
  * База данных не хранит содержимое файлов, только их относительные пути, контрольные суммы и такие метаданные, как размер и даты ctime и mtime.
  * Программа не открывает сокеты.
  * Не передаёт данные куда-либо.
  * Не требует привилегированных прав для исполнения и не использует SUID-бит или какие-либо другие небезопасные биты.
  * В программе нет ничего, что может эксплуатироваться для повышения привилегий или других нарушений безопасности.
* Производительность программы в основном упирается в производительность дисковой подсистемы. Каждый файл считывается побайтно, и для каждого файла формируется своя контрольная сумма с использованием алгоритма SHA512.
* Программа работает очень быстро благодаря библиотекам SQLite и FTS ([man 3 fts](https://man7.org/linux/man-pages/man3/fts.3.html)).
* Разбор параметров строки реализован через библиотеку ARGP.
* Для регулярных выражений выбрана библиотека PCRE2.
* Программа безопасна для случаев с огромным количеством файлов, директорий и поддиректорий любой вложенности. Благодаря библиотеке FTS рекурсия не используется, поэтому не произойдёт переполнения стека даже в случае большой вложенности файлов.
* За счёт своей компактности и переносимости кода программа может использоваться даже на специализированных устройствах типа NAS, а также на embedded- или IoT-устройствах.
* Если любопытно узнать, что содержится внутри базы данных, сформированной **precizer**, то можно воспользоваться удобным GUI [DB Browser for SQLite](https://sqlitebrowser.org).

## ВОПРОСЫ И БАГРЕПОРТЫ

* Подсказка `--help` сделана максимально подробной специально для помощи пользователям, не обладающим специализированными техническими знаниями.
* Обратиться к автору можно:
  * [Через форму GitHub Дискуссий](https://github.com/precizer/precizer/discussions).
  * Там же на GitHub можно [опубликовать багрепорт или feature request](https://github.com/precizer/precizer/issues/new).

## УЧАСТИЕ В ПРОЕКТЕ

Участие в развитии проекта приветствуется. Начните с [CONTRIBUTING](CONTRIBUTING.ru.md): там описаны рабочий процесс, зависимости, шаги проверки и требования к pull request. Актуальные запросы на усовершенствования можно выбрать в списке [Issues](https://github.com/precizer/precizer/issues) по уровню и интересу.

## СБОРКА И УСТАНОВКА

### Пакетирование для дистрибутивов

* Автор был рад подготовить автоматическую сборку средствами GitHub CI Workflows и будет в дальнейшем поддерживать новые версии.
* Автор **НЕ** готов самостоятельно создавать и поддерживать в будущем пакетирование программы **precizer** под _все_ существующие дистрибутивы операционных систем.
* Если Вы горите желанием создать пакет под любой дистрибутив и столкнулись с непреодолимыми трудностями по адаптации кода программы, то именно в этом случае автор будет очень рад оказать всю необходимую помощь в поддержке инициативы и оптимизации кода программы под конкретный дистрибутив или пакетный менеджер. Как связаться с автором, описано в разделе [«Вопросы и багрепорты»](#ВОПРОСЫ-И-БАГРЕПОРТЫ).

### Сборка с помощью Docker

Сборка программы уже предусмотрена с использованием Docker. При этом подготовлено несколько тюнингованных платформ, которые могут быть выбраны в качестве дистрибутива для сборки программы. Среди успешно работающих дистрибутивов:

* Almalinux
* Alpine
* Arch
* Debian
* Gentoo
* Rocky
* Ubuntu

Ознакомиться с подробностями настроек и устанавливаемыми библиотеками можно в соответствующих Docker‑файлах в поддиректории проекта `.docker/`

Для сборки достаточно указать команду `make`, ключевое слово `docker`, дистрибутив `debian` (например), а затем цель `dynamic-production` или другую.

Пример сборки в контейнере:

```sh
make docker-gentoo-production
```

Команда соберёт программу в режиме production, используя Docker‑контейнер с дистрибутивом Gentoo.

```sh
make docker-ubuntu-production
```

Команда соберёт программу в том же режиме `production`, но уже используя дистрибутив Ubuntu.

В результате запуска в директории проекта появится исполняемый файл **precizer**, который был собран внутри контейнера. При этом явным преимуществом использования Docker является то, что нет необходимости в систему устанавливать много инструментов для сборки и тестирования, библиотек и их зависимостей. Достаточно запустить Docker и получить результат — исполняемый файл. Дальше нужно определиться с типом этого исполняемого файла. В случае сомнений лучше остановить выбор на `make portable` как на наиболее универсальном варианте. Описания всех доступных вариантов сборки представлены ниже.

### Самостоятельная сборка

#### Подготовка

```sh
git clone --depth=1 https://github.com/precizer/precizer.git
cd precizer
```

#### Портируемый бинарный файл

```sh
make portable
```

Результатом сборки будет один статически слинкованный, самораспаковывающийся сжатый UPX файл в формате ELF, без каких-либо динамических зависимостей. Этот файл представляет собой всю программу целиком и может быть запущен практически на любом современном дистрибутиве Linux. Достаточно этот файл скопировать на любую платформу в рамках архитектуры x64/arm/etc.

Программа оптимизирована под **максимальную переносимость**.

Особенности компиляции и линковки: `-static -O2 -mtune=generic`

Альтернатива с использованием докера:
```sh
make docker-ubuntu-portable
```
или замените `-ubuntu-` на любой из вышеперечисленных дистрибутивов

#### Единый бинарный файл с оптимизацией под локальный CPU

```sh
make production
```

Результатом сборки будет заточенный под локальный CPU, статически слинкованный, самораспаковывающийся, сжатый UPX файл в формате ELF, не нуждающийся в отдельных библиотеках. Этот файл представляет собой всю программу целиком, может быть запущен на локальном компьютере и будет использовать все возможности CPU по-максимуму.

Программа оптимизирована под **предельно возможную производительность на локальном железе**.

Особенности компиляции и линковки: `-static -O3 -march=native`

Альтернатива с использованием докера:
```sh
make docker-ubuntu-production
```
или замените `-ubuntu-` на любой из вышеперечисленных дистрибутивов

#### Исполняемый файл с динамически подгружаемыми библиотеками и оптимизацией под локальный CPU

```sh
make dynamic-production
```

Результатом сборки будет исполняемый файл размером около **50 килобайт** в формате ELF. Он будет заточен под локальный CPU, динамически слинкован с установленными в системе библиотеками, самораспаковывающийся, сжатый UPX. Этот файл может быть успешно собран и запущен на локальном компьютере, если в системе были предварительно установлены такие библиотеки, как sqlite3, pcre2, argp и fts.

Файл оптимизирован под **максимальную производительность и минимальный размер**.

Особенности компиляции: `-O3 -march=native`

Альтернатива с использованием докера:
```sh
make docker-ubuntu-dynamic-production
```
или замените `-ubuntu-` на любой из вышеперечисленных дистрибутивов

#### Тесты

Для проверки возможностей программы можно использовать наборы тестов из директории tests/fixtures/ в исходном коде программы

Запуск тестов:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make tests
```

#### Установка

Просто скопируйте получившийся исполняемый файл **precizer** в любое место, прописанное в системной переменной $PATH для быстрого вызова.

#### Зависимости для сборки на определённых OS

Install build and compile tools on Linux

#### Arch Linux

```sh
sudo pacman -S --noconfirm base-devel gcc-libs sqlite pcre2 upx
```

#### Ubuntu/Debian Linux

```sh
sudo apt -y install gcc make libpcre2-dev libsqlite3-dev upx-ucl
```

#### Alpine Linux

```sh
sudo apk add --update build-base pcre2-dev pcre2-static fts-dev argp-standalone sqlite-dev upx
```

#### Almalinux/Rocky Linux

```sh
sudo dnf -y install gcc make sqlite sqlite-devel glibc-devel pcre2 pcre2-devel upx pcre2-static glibc-static
```

#### Gentoo Linux

```sh
echo "dev-libs/libpcre2 static-libs" >> /etc/portage/package.use/libpcre2;
emerge dev-libs/libpcre2 app-arch/upx
```

#### Очистка от старых сборок и артефактов

##### Быстрое удаление всей директории с артефактами

```sh
make purge
```

## ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ

### Пример 1

Добавить файлы в две базы данных и сравнить их между собой:

```sh
precizer --progress --database=database1.db tests/fixtures/diffs/diff1

precizer --progress --database=database2.db tests/fixtures/diffs/diff2

precizer --compare database1.db database2.db
```

<sub>The comparison of database1.db and database2.db databases is starting…  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
Starting database file database2.db integrity check…  
Database database2.db has been verified and is in good condition  
**These files are no longer in the database1.db but still exist in the database2.db**  
path1/AAA/BCB/CCC/b.txt  
**These files are no longer in the database2.db but still exist in the database1.db**  
path2/AAA/ZAW/D/e/f/b_file.txt  
**The SHA512 checksums of these files do not match between database1.db and database2.db**  
2/AAA/BBB/CZC/a.txt  
3/AAA/BBB/CCC/a.txt  
4/AAA/BBB/CCC/a.txt  
path1/AAA/ZAW/D/e/f/b_file.txt  
path2/AAA/BCB/CCC/a.txt  
Comparison of database1.db and database2.db databases is complete  
The precizer completed its execution without any issues  
</sub>

В режиме `--compare` параметры `--ignore` и `--include` ограничивают ту область относительных путей, по которой строится отчёт сравнения. Если фильтры скрывают часть различий, итоговые сообщения об идентичности относятся только к оставшейся отфильтрованной области. Пути, возвращённые через `--include`, снова участвуют и в списках различий, и в итоговых сводках

### Пример 2
Актуализация базы данных

Попробуем использовать предыдущий пример ещё раз. Первая попытка. Сообщение с предупреждением.

```sh
precizer --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>The database database1.db was previously created and already contains data with files and their checksums. Use the `--update` option only when you are certain that the database needs to be updated and when file information (including changes, deletions, and additions) should be synchronized with the database.  
ERROR: The precizer process terminated unexpectedly due to an error  
</sub>

Должен быть добавлен параметр **--update**. Этот параметр необходим для защиты базы данных от потери информации из-за случайного запуска.

```sh
precizer --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 45B, total items: 58, dirs: 46, files: 12, symlnks: 0  
**The database file database1.db has NOT been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

Внесём некоторые изменения:

```sh
# Modify a file
echo -n "  " >> tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt

# Add a new file
touch tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/c.txt

# Remove a file
rm tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt

```

и запустим **precizer** ещё раз, но уже с параметром `--update`:

```sh
precizer --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
The **--update** option has been used, so the information about files will be updated against the database database1.db  
File traversal started  
**These files have been added or changed and those changes will be reflected against the DB database1.db:**  
1/AAA/BCB/CCC/a.txt changed lsize & ctime & mtime rehashed  
1/AAA/BCB/CCC/c.txt added  
File traversal complete  
Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
**These files are no longer exist or ignored and will be deleted against the DB database1.db:**  
path2/AAA/ZAW/D/e/f/b_file.txt  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file database1.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

Метки изменений в выводе означают:
- `lsize` — логический размер файла в байтах (`st_size`)
- `asize` — размер, выделенный на диске, в байтах (`st_blocks * 512`)
- `ctime` — время изменения метаданных/статуса
- `mtime` — время изменения содержимого файла

При каждом запуске **precizer** обходит файловую систему, после этого проверяя, есть ли запись об определённом файле в базе данных или нет. Другими словами, приоритет для программы имеет состояние файловой системы на диске.

Обход каталогов **precizer** работает очень похоже на работу `rsync`, поскольку использует похожий алгоритм.

Стоит обратить внимание, что **precizer** не будет пересчитывать контрольные суммы SHA512 для файлов, которые уже были записаны в базу данных и для которых метаданные файла остаются прежними (такие, как размер и время последнего доступа — atime). Если указан аргумент `--watch-timestamps`, то помимо размера будут учитываться время создания и время модификации (mtime и ctime).

Любые новые файлы, удалённые файлы или те файлы, которые изменились между запусками приложения, будут обработаны и, соответственно, все изменения будут отражены в базе данных, если указан параметр `--update`.

### Пример 3

Использование режима `--silent`. При включении этого режима программа ничего не выводит на экран. Это имеет смысл при использовании **precizer** в скриптах.

Исключение составляет режим `--compare`: вместе с `--silent` остаётся только вывод результатов сравнения. Пути с различиями печатаются напрямую, а заголовки категорий сохраняются только тогда, когда одновременно активны несколько категорий сравнения.

Добавим параметр **--silent** к предыдущему примеру:

```sh
precizer --silent --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

В результате на экране ничего не отобразится.

### Пример 4
Дополнительная информация в режиме `--verbose` может быть полезна для отладки.

Добавим параметр **--verbose** к предыдущему примеру:

```sh
precizer --verbose --update --progress --database=database1.db tests/fixtures/diffs/diff1
```

<sub>2025-01-25 09:55:59:820 src/parse_arguments.c:442:parse_arguments:Configuration: rational_logger_mode=VERBOSE  
paths=tests/fixtures/diffs/diff1; database=database1.db; db_file_name=database1.db; verbose=yes; maxdepth=-1; silent=no; force=no; update=yes; watch-timestamps=no; progress=yes; compare=no, db-drop-ignored=no, dry-run=no, check-level=FULL, rational_logger_mode=VERBOSE  
2025-01-25 09:55:59:820 src/parse_arguments.c:558:parse_arguments:Arguments parsed  
2025-01-25 09:55:59:820 src/detect_paths.c:025:detect_paths:Checking directory paths provided as arguments  
2025-01-25 09:55:59:820 src/file_availability.c:034:file_availability:Verify that the path tests/fixtures/diffs/diff1 exists  
2025-01-25 09:55:59:820 src/file_availability.c:053:file_availability:The path tests/fixtures/diffs/diff1 is exists and it is a directory  
2025-01-25 09:55:59:821 src/detect_paths.c:036:detect_paths:Paths detected  
2025-01-25 09:55:59:821 src/init_signals.c:034:init_signals:Set signal SIGUSR2 OK:pid:604770  
2025-01-25 09:55:59:821 src/init_signals.c:043:init_signals:Set signal SIGINT OK:pid:604770  
2025-01-25 09:55:59:821 src/init_signals.c:052:init_signals:Set signal SIGTERM OK:pid:604770  
2025-01-25 09:55:59:821 src/init_signals.c:055:init_signals:Signals initialized  
2025-01-25 09:55:59:821 src/determine_running_dir.c:018:determine_running_dir:Current directory: /tmp  
2025-01-25 09:55:59:821 src/db_determine_name.c:099:db_determine_name:Primary database file name: database1.db  
2025-01-25 09:55:59:821 src/db_determine_name.c:105:db_determine_name:Primary database file path: database1.db  
2025-01-25 09:55:59:821 src/db_determine_name.c:109:db_determine_name:DB name determined  
2025-01-25 09:55:59:821 src/file_availability.c:034:file_availability:Verify that the path . exists  
2025-01-25 09:55:59:821 src/file_availability.c:053:file_availability:The path . is exists and it is a directory  
2025-01-25 09:55:59:821 src/file_availability.c:034:file_availability:Verify that the path database1.db exists  
2025-01-25 09:55:59:821 src/file_availability.c:044:file_availability:The path database1.db is exists and it is a file  
2025-01-25 09:55:59:821 src/db_determine_mode.c:128:db_determine_mode:Final value for config->sqlite_open_flag: SQLITE_OPEN_READWRITE  
2025-01-25 09:55:59:821 src/db_determine_mode.c:129:db_determine_mode:Final value for config->db_initialize_tables: false  
2025-01-25 09:55:59:821 src/db_determine_mode.c:131:db_determine_mode:DB mode determined  
2025-01-25 09:55:59:821 src/db_test.c:061:db_test:Starting database file database1.db integrity check…  
2025-01-25 09:55:59:821 src/db_test.c:082:db_test:The database verification level has been set to FULL  
2025-01-25 09:55:59:821 src/db_test.c:126:db_test:Database database1.db has been verified and is in good condition  
2025-01-25 09:55:59:822 src/db_get_version.c:087:db_get_version:Version number 1 found in database  
2025-01-25 09:55:59:822 src/db_check_version.c:032:db_check_version:The database1.db database file is version 1  
2025-01-25 09:55:59:822 src/db_check_version.c:061:db_check_version:The database database1.db is on version 1 and does not require any upgrades  
2025-01-25 09:55:59:822 src/db_init.c:030:db_init:Successfully opened database database1.db  
2025-01-25 09:55:59:822 src/db_init.c:118:db_init:The primary database and tables have NOT been initialized  
2025-01-25 09:55:59:822 src/db_init.c:150:db_init:The primary database named database1.db is ready for operations  
2025-01-25 09:55:59:822 src/db_init.c:167:db_init:The in-memory runtime_paths_id database successfully attached to the primary database database1.db  
2025-01-25 09:55:59:822 src/db_init.c:174:db_init:Database initialization process completed  
2025-01-25 09:55:59:822 src/db_compare.c:136:db_compare:Database comparison mode is not enabled. Skipping comparison  
2025-01-25 09:55:59:822 src/db_contains_data.c:086:db_contains_data:The database database1.db has already been created previously  
2025-01-25 09:55:59:822 src/db_validate_paths.c:192:db_validate_paths:The paths written against the database and the paths passed as arguments are completely identical  
2025-01-25 09:55:59:822 src/file_list.c:143:file_list:File system traversal initiated to calculate file count and storage usage  
2025-01-25 09:55:59:823 src/file_list.c:038:show_status:Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
2025-01-25 09:55:59:825 src/db_get_version.c:087:db_get_version:Version number 1 found in database  
2025-01-25 09:55:59:825 src/db_consider_vacuum_primary.c:025:db_consider_vacuum_primary:No changes were made. The primary database doesn't require vacuuming  
2025-01-25 09:55:59:825 src/status_of_changes.c:049:status_of_changes:**The database file database1.db has NOT been modified since the program was launched**  
2025-01-25 09:55:59:825 src/exit_status.c:027:exit_status:The precizer completed its execution without any issues  
</sub>

### Пример 5
Исследование без рекурсии с помощью параметра `--maxdepth`

```sh
tree tests/fixtures/4

tests/fixtures/4
├── AAA
│   ├── BBB
│   │   ├── CCC
│   │   │   └── a.txt
│   │   └── uuu.txt
│   └── tttt.txt
└── sss.txt

3 directories, 4 files
```

Параметр `--maxdepth` со значением `=0` полностью отключает рекурсию.

```sh
precizer --maxdepth=0 tests/fixtures/4
```

<sub>Primary database file name: myhost.db  
The path myhost.db doesn't exist or it is not a file  
The primary DB file not yet exists. Brand new database will be created  
Recursion depth limited to: 0  
File traversal started  
**These files will be added against the myhost.db database:**  
sss.txt  
File traversal complete  
Total size: 2B, total items: 5, dirs: 4, files: 1, symlnks: 0  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database myhost.db has been modified since the last check (files were added, removed, or updated)**  
The precizer completed its execution without any issues  
</sub>

### Пример 6

Пример пути, который следует игнорировать. Для указания шаблона игнорирования файлов или каталогов можно использовать регулярные выражения PCRE2. Внимание! Все пути в регулярном выражении должны быть указаны как **относительные**.

Чтобы проверить и протестировать регулярные выражения PCRE2, можно использовать ресурс https://regex101.com

Для понимания, как выглядит относительный путь, достаточно запустить сканирование директорий без опции `--ignore` и посмотреть, как терминал будет отображать относительные пути, записываемые в базу данных:

```sh
% tree -L 3 tests/fixtures/diffs

tests/fixtures/diffs
├── diff1
│   ├── 1
│   │   └── AAA
│   ├── 2
│   │   └── AAA
│   ├── 3
│   │   └── AAA
│   ├── 4
│   │   └── AAA
│   ├── path1
│   │   └── AAA
│   └── path2
│       └── AAA
└── diff2
    ├── 1
    │   └── AAA
    ├── 2
    │   └── AAA
    ├── 3
    │   └── AAA
    ├── 4
    │   └── AAA
    ├── path1
    │   └── AAA
    └── path2
        └── AAA

26 directories, 0 files
```

```sh
precizer --ignore="^diff1/1/.*" tests/fixtures/diffs
```

В этом примере начальный путь сканирования — `./tests/fixtures/diffs`, а сформированный путь для игнорирования — `./tests/fixtures/diffs/diff1/1/` со всеми подкаталогами (`/*`).

<sub>Primary database file name: myhost.db  
The path myhost.db doesn't exist or it is not a file  
The primary DB file not yet exists. Brand new database will be created  
File traversal started  
**These files will be added against the myhost.db database:**  
diff1/1/AAA/BCB/CCC/a.txt **ignored & not added**  
diff1/1/AAA/ZAW/A/b/c/a_file.txt **ignored & not added**  
diff1/1/AAA/ZAW/D/e/f/b_file.txt **ignored & not added**  
diff1/2/AAA/BBB/CZC/a.txt  
diff1/3/AAA/BBB/CCC/a.txt  
diff1/4/AAA/BBB/CCC/a.txt  
diff1/path1/AAA/BCB/CCC/a.txt  
diff1/path1/AAA/ZAW/A/b/c/a_file.txt  
diff1/path1/AAA/ZAW/D/e/f/b_file.txt  
diff1/path2/AAA/BCB/CCC/a.txt  
diff1/path2/AAA/ZAW/A/b/c/a_file.txt  
diff1/path2/AAA/ZAW/D/e/f/b_file.txt  
diff2/1/AAA/BCB/CCC/a.txt  
diff2/1/AAA/ZAW/A/b/c/a_file.txt  
diff2/1/AAA/ZAW/D/e/f/b_file.txt  
diff2/2/AAA/BBB/CZC/a.txt  
diff2/3/AAA/BBB/CCC/a.txt  
diff2/4/AAA/BBB/CCC/a.txt  
diff2/path1/AAA/BCB/CCC/a.txt  
diff2/path1/AAA/BCB/CCC/b.txt  
diff2/path1/AAA/ZAW/A/b/c/a_file.txt  
diff2/path1/AAA/ZAW/D/e/f/b_file.txt  
diff2/path2/AAA/BCB/CCC/a.txt  
diff2/path2/AAA/ZAW/A/b/c/a_file.txt  
File traversal complete  
Total size: 97B, total items: 114, dirs: 90, files: 24, symlnks: 0  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database myhost.db has been modified since the last check (files were added, removed, or updated)**  
The precizer completed its execution without any issues  
Enjoy your life!  
</sub>

Повторим тот же пример, но без опции `--ignore`, чтобы добавить три ранее проигнорированных файла:

```sh
precizer --update tests/fixtures/diffs
```

<sub>Primary database file name: myhost.db  
Starting database file myhost.db integrity check…  
Database myhost.db has been verified and is in good condition  
The **--update** option has been used, so the information about files will be updated against the database myhost.db  
File traversal started  
**These files have been added or changed and those changes will be reflected against the DB myhost.db:**  
diff1/1/AAA/BCB/CCC/a.txt add  
diff1/1/AAA/ZAW/A/b/c/a_file.txt add  
diff1/1/AAA/ZAW/D/e/f/b_file.txt add  
File traversal complete  
Total size: 97B, total items: 114, dirs: 90, files: 24, symlnks: 0  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file myhost.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

### Пример 7

Продолжение предыдущего примера [Пример 6](#пример-6).

Несколько регулярных выражений для игнорирования можно указать одновременно, повторив опцию `--ignore` несколько раз.

База данных будет очищена от упоминаний файлов, соответствующих регулярным выражениям из аргументов `--ignore`: "diff1/1/.\*" и "diff2/1/.\*"

Параметр `--db-drop-ignored` должен быть указан дополнительно, чтобы удалить из базы данных упоминание файлов, соответствующих регулярным выражениям, переданным через опции `--ignore`.

На файловой системе не было изменений, но игнорируемые файлы будут удалены из БД.

```sh
# Обновить базу данных, удалив информацию о тех файлах, которые были указаны как игнорируемые:

precizer \
    --update \
    --db-drop-ignored \
    --ignore="^diff1/1/.*" \
    --ignore="^diff2/1/.*" \
    tests/fixtures/diffs
```

<sub>Primary database file name: myhost.db  
Starting database file myhost.db integrity check…  
Database myhost.db has been verified and is in good condition  
The **--update** option has been used, so the information about files will be deleted against the database myhost.db  
**These files are no longer exist or ignored and will be deleted against the DB myhost.db:**  
diff1/1/AAA/BCB/CCC/a.txt **clean ignored**  
diff1/1/AAA/ZAW/A/b/c/a_file.txt **clean ignored**  
diff1/1/AAA/ZAW/D/e/f/b_file.txt **clean ignored**  
diff2/1/AAA/BCB/CCC/a.txt **clean ignored**  
diff2/1/AAA/ZAW/A/b/c/a_file.txt **clean ignored**  
diff2/1/AAA/ZAW/D/e/f/b_file.txt **clean ignored**  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file myhost.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

### Пример 8

Использование параметров `--ignore` вместе с `--include`

```sh

# Удалим старую базу данных и создадим новую, наполним её данными:

rm -i "${HOST}.db"

precizer tests/fixtures/diffs
```

Усложним задачу с использованием регулярных выражений.

Регулярные выражения PCRE2 для относительных путей, которые необходимо включить. Включаем указанные относительные пути, даже если они были исключены с помощью одного или нескольких параметров `--ignore`. Несколько регулярных выражений могут быть указаны с помощью `--include`

Чтобы проверить и протестировать регулярные выражения PCRE2, можно использовать ресурс https://regex101.com

DB будет очищена от упоминаний файлов, соответствующих регулярным выражениям из аргументов `--ignore`: "^.\*/path2/.\*" и "diff2/.\*", но опция `--include` оставит в базе данных пути, соответствующие заданным шаблонам.

Параметр `--db-drop-ignored` должен быть указан дополнительно, чтобы удалить из базы данных упоминание файлов, соответствующих регулярным выражениям, переданным через опции `--ignore`.

```sh

# Обновить базу данных, удалив информацию о тех файлах, которые были указаны как игнорируемые за исключением шаблонов путей из --include

precizer --update \
	--progress \
	--ignore="^.*/path2/.*" \
	--ignore="^diff2/.*" \
	--include="^diff2/1/AAA/ZAW/A/b/c/.*" \
	--include="^diff2/path1/AAA/ZAW/.*" \
	--include="^diff1/path2/AAA/ZAW/A/b/c/a_file\..*" \
	--db-drop-ignored \
	tests/fixtures/diffs
```

<sub>Primary database file name: myhost.db  
Starting database file myhost.db integrity check…  
Database myhost.db has been verified and is in good condition  
The **--update** option has been used, so the information about files will be deleted against the database myhost.db  
**These files are no longer exist or ignored and will be deleted against the DB myhost.db:**  
diff1/path2/AAA/BCB/CCC/a.txt clean ignored  
diff1/path2/AAA/ZAW/A/b/c/a_file.txt clean ignored  
diff1/path2/AAA/ZAW/D/e/f/b_file.txt clean ignored  
diff2/1/AAA/BCB/CCC/a.txt clean ignored  
diff2/1/AAA/ZAW/D/e/f/b_file.txt clean ignored  
diff2/2/AAA/BBB/CZC/a.txt clean ignored  
diff2/3/AAA/BBB/CCC/a.txt clean ignored  
diff2/4/AAA/BBB/CCC/a.txt clean ignored  
diff2/path1/AAA/BCB/CCC/a.txt clean ignored  
diff2/path1/AAA/BCB/CCC/b.txt clean ignored  
diff2/path2/AAA/BCB/CCC/a.txt clean ignored  
diff2/path2/AAA/ZAW/A/b/c/a_file.txt clean ignored  
Start vacuuming the primary database…  
The primary database has been vacuumed  
**The database file myhost.db has been modified since the program was launched**  
The precizer completed its execution without any issues  
</sub>

#### Те же фильтры в режиме `--compare`

Та же комбинация `--ignore` и `--include` работает и при сравнении двух баз данных. Фильтры определяют не только то, какие строки будут напечатаны, но и то, какая часть относительных путей считается входящей в отчёт сравнения. Поэтому итоговые сводки и сообщения об идентичности относятся только к этой отфильтрованной области

```sh
# Продолжая Пример 1, вернём в отчёт только один путь из скрытой группы различий

precizer --compare \
	--ignore="^(?:2|3|4)/.*" \
	--ignore="^path1/.*" \
	--ignore="^path2/.*" \
	--include="^2/AAA/BBB/CZC/a\.txt$" \
	database1.db database2.db
```

<sub>The comparison of database1.db and database2.db databases is starting…  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
Starting database file database2.db integrity check…  
Database database2.db has been verified and is in good condition  
**The SHA512 checksums of these files do not match between database1.db and database2.db**  
2/AAA/BBB/CZC/a.txt  
Comparison of database1.db and database2.db databases is complete  
The precizer completed its execution without any issues  
</sub>

В этом примере все различия из путей `2/`, `3/`, `4/`, `path1/` и `path2/` сначала выводятся из области отчёта сравнения, а затем `--include` возвращает обратно только `2/AAA/BBB/CZC/a.txt`. В результате в отчёте снова появится именно этот путь, а остальные скрытые различия останутся вне итоговых списков и сводок

### Пример 9
Защита неизменяемых архивов с помощью `--lock-checksum`

Опция `--lock-checksum` нужна для архивных каталогов или файлов, содержимое которых не должно переписываться. Она принимает регулярные выражения PCRE2 для **относительных** путей (тот же формат, что и у `--ignore`). Пути, совпавшие с любым шаблоном, записываются в БД один раз. После этого их контрольные суммы не пересчитываются даже при `--update`. Любое дальнейшее изменение размера или временных меток (при использовании режима `--watch-timestamps`), исчезновение файла с диска, потеря доступа на чтение или неожиданный сбой проверки доступа считается порчей данных и выводится соответствующее сообщение об ошибке вместо обновления записи. Для исчезнувшего или недоступного заблокированного файла запись в БД сохраняется, чтобы нарушение не терялось между последующими запусками. Защита заблокированного пути имеет приоритет над `--ignore`, `--db-drop-ignored` и `--db-drop-inaccessible`: совпадение с этими фильтрами не должно молча удалять такую запись из БД. Та же защита сохраняется и в ситуации, когда `--include` возвращает только часть ранее скрытого поддерева. Несколько шаблонов могут быть указаны повторением добавления опции `--lock-checksum`:

```sh
precizer \
  --lock-checksum="^archive/2024/.*" \
  --lock-checksum="^snapshots/monthly/.*" \
  /mnt/storage
```

В последующих запусках необходимо сохранять те же шаблоны блокировки при актуализации базы:

```sh
precizer \
  --update \
  --lock-checksum="^archive/2024/.*" \
  --lock-checksum="^snapshots/monthly/.*" \
  /mnt/storage
```

Файлы вне шаблонов обновляются в обычном порядке. Для заблокированных через `--lock-checksum` записей любое расхождение сразу становится заметным и код возврата при завершении precizer будет больше нуля и это можно использовать в скриптах.

### Пример 10
Глубокая проверка заблокированных данных с помощью `--rehash-locked`

Опция `--rehash-locked` используется только вместе с `--lock-checksum`. При её включении каждый файл, попавший под шаблон блокировки и уже присутствующий в базе данных, перечитывается побайтно при каждой актуализации базы данных через `--update`. Для такого файла заново вычисляется контрольная сумма SHA512 и выполняется сравнение с сохранённым значением. Это позволяет выполнить дополнительный аудит неизменяемых архивов ценой увеличенного дискового ввода-вывода. Наличие или отсутствие параметра `--watch-timestamps` не влияет на работу `--rehash-locked`. Если пересчитанная сумма и размер совпадают с записью в БД, файл считается консистентным; если при этом на диске отличаются ctime/mtime, новые значения сохраняются в базе данных. Если заблокированный путь одновременно совпадает с `--ignore`, параметр `--rehash-locked` всё равно обязан пройти по этому пути и выполнить проверку, а не пропустить её.

```sh
precizer --update \
  --lock-checksum="^archive/2024/.*" \
  --rehash-locked \
  /mnt/storage
```

Чтобы увидеть взаимодействие параметров командной строки `--lock-checksum`, `--watch-timestamps` и `--rehash-locked`, рассмотрим несколько случаев:

1. **Размер файла отличается** Если размер, записанный в базе, не совпадает с размером на диске, файл будет помечен как «нарушение заблокированной контрольной суммы» вне зависимости от `--watch-timestamps` и `--rehash-locked`. Пересчитывать контрольную сумму бессмысленно: при изменившемся размере она всё равно не совпадёт.
2. **Совпадает размер файла, не указаны ни `--watch-timestamps`, ни `--rehash-locked`** поэтому остальные значения, такие как SHA512 и временные метки не учитываются и файл считается полностью консистентным, программа завершится со статусом `SUCCESS`.
3. **Совпадают размер и временные метки; указана `--watch-timestamps` и не указана опция `--rehash-locked`** Файл считается полностью консистентным, он не появится в выводе, а статус завершения программы будет `SUCCESS`.
4. **Размер совпадает, временные метки различаются; указана `--watch-timestamps` и не указана `--rehash-locked`** Файл будет отмечен как «нарушение заблокированной контрольной суммы» только из-за различающихся временных меток, и программа завершится со статусом `WARNING`.
5. **Размер совпадает, указан параметр `--rehash-locked`** Для проверки учитываются только контрольная сумма и размер, записанные в базе данных. Если они совпадают, файл считается консистентным. Если на диске изменились временные метки, то новые значения ctime/mtime будут сохранены в базе данных независимо от того был ли указан `--watch-timestamps` или нет.

Подробные примеры поведения в сложных ситуациях собраны в тесте №30 (`tests/src/test0030.c`). Там проверяются удаления заблокированных файлов, потеря доступа, сбои проверки доступа и взаимодействие с `--ignore`, `--include`, `--db-drop-ignored` и `--db-drop-inaccessible`.

Практический сценарий: выполнять ежедневное быстрое сканирование без `--rehash-locked` (и при необходимости без `--watch-timestamps`), чтобы поддерживать БД в актуальном состоянии, а реже запускать углублённый аудит с `--rehash-locked`, который гарантированно сверяет контрольные суммы «замороженных» данных.

### Пример 11
Сброс записей о недоступных файлах с помощью `--db-drop-inaccessible`

По умолчанию, если файл недоступен из-за прав доступа, его запись в базе данных сохраняется при `--update`, чтобы избежать случайной потери данных. Для удаления таких записей используется дополнительный параметр командной строки `--db-drop-inaccessible`:

```sh
precizer --update --db-drop-inaccessible /mnt/storage
```

<sub>drop due to inaccessible archive/secret.bin</sub>

Важно: `--db-drop-inaccessible` относится не к удалённым файлам, а к файлам, которые сейчас нельзя прочитать или для которых не удалось надёжно проверить доступ. Например, могли измениться права, владелец, ACL, или файловая система временно вернула ошибку доступа. По умолчанию такие записи остаются в БД, чтобы временная проблема с доступом не стёрла полезную историю. Если вы уверены, что такие записи больше не нужны, добавьте `--db-drop-inaccessible`.

Если же файл или один из каталогов в его пути вообще не виден в файловой системе, `precizer` считает удалённой обычную запись, не защищённую через `--lock-checksum`, и при `--update` удалит её из БД без дополнительных опций. Программа не может сама отличить настоящее удаление от ситуации, когда точка монтирования существует, но нужный том не подключился и поэтому каталог выглядит пустым. Перед обновлением БД для внешних, сетевых или съёмных томов сначала убедитесь, что нужный том действительно смонтирован. Для важных архивных путей используйте `--lock-checksum`: тогда исчезнувший или недоступный файл будет показан как предупреждение, а запись в БД сохранится.

## УСТРАНЕНИЕ НЕПОЛАДОК (ТРАБЛШУТИНГ)

### Медленный проход по файлам, медленный расчёт контрольных сумм, медленная запись в БД. Или решение проблемы под названием «ВСЁ МЕДЛЕННО»

Чтобы выяснить причину, попробуйте запустить `precizer` в режиме `--dry-run` или `--dry-run=with-checksums`.

`--dry-run` рекурсивно проходит по **Файловой Системе**. В этом режиме, кроме обхода дерева файлов, больше ничего не выполняется. Можно добавить `--progress` — тогда добавится подсчёт занимаемого файлами места и количества файлов, но записи в БД не будет. В этом режиме можно проверить доступность ФС и частично — аппаратные возможности. Если медленно даже с `--dry-run`, то маловероятно, что причина в самой программе `precizer`.

Режим `--dry-run=with-checksums` отличается от `--dry-run` только тем, что каждый встреченный файл будет полностью (побайтово) считан с диска, и для него будет рассчитана контрольная сумма. Это значительно более ресурсоёмкий процесс, практически идентичный реальной работе программы. Режим полностью раскрывает аппаратный потенциал, но при этом не делает записей в БД. Если при этом указать `--progress`, то после прохода по файловой системе будет показан статус: количество байт, которые прошли через хеширование, а так же средняя скорость хеширования в B/s Показатель скорости хеширования можно сравнивать с результатами сторонних бенчмарков для поиска «бутылочного горлышка».

Возможна ситуация, когда даже с `--dry-run=with-checksums` программа работает очень быстро, но в режиме реальной работы (без Dry-Run) наблюдается заметная деградация производительности, особенно при массовых добавлениях или изменениях в БД. В этой ситуации нужно проверить ФС, **на которой расположен файл базы данных**. Именно там может скрываться причина.

База данных SQLite открывается программой `precizer` с такими параметрами, чтобы максимально сохранить уже записанное и всеми силами противостоять повреждению файла БД. Однако это предполагает больше дисковых I/O-операций и больше синхронизаций ФС с блочным устройством. На практике SQLite действительно очень быстрая БД и обычно не является «слабым звеном». Но если ФС с компрессией, сетевая или просто медленная, это может ощутимо отразиться на производительности программы в целом.

Например, при массовых добавлениях `precizer` ждёт, когда информация о файле будет записана в БД и транзакция будет завершена, и только потом переходит к следующему файлу. В такой ситуации для решения проблем производительности имеет смысл попробовать временно разместить файл `.db`, например, на `tmpfs`, — это может значительно увеличить общую производительность и понять где именно кроется проблема.

### Если по-прежнему всё медленно в любых режимах, то имеет смысл проверить:

#### Целостность файловой системы

Сделайте системные проверки ФС, на которой делаются слепки контрольных сумм файлов, а также той ФС, где хранится и обновляется файл базы данных `.db`. Наблюдаемые просадки производительности могут быть связаны с логическими проблемами на уровне ФС.

#### Аппаратные возможности:

##### Скорость диска

Файлы считываются полностью и побайтово. Производительность дисковой подсистемы прямо пропорциональна скорости работы `precizer`. Программа работает поверх ФС как с более высоким слоем абстракции, а не с блочными устройствами напрямую. Любые логические ошибки ФС имеют значение.

Падение производительности может сильно зависеть от выбранной ФС. Например, скорость обработки файлов из смонтированной точки NFS может упираться в производительность сети и заметно отличаться от скорости доступа к ФС с отключённым `atime` на локальном диске NVMe (`atime` — access time, метка «время последнего доступа» к файлу). Используйте сторонние утилиты для проверки производительности файловой подсистемы.

##### Производительность CPU

Расчёт контрольных сумм — это чистая математика и достаточно трудоёмкий процесс. Современные процессоры справляются с этим незаметно быстро, но имеет смысл убедиться, что производительность не деградирует из-за разделяемых ресурсов виртуальных CPU в случае контейнеризации или виртуализации. Используйте сторонние системы мониторинга и бенчмарки для проверки производительности CPU.

### Матрица диагностики узких мест

| Шаг | Режим/команда                       | Что проверяем                                                       | Если здесь медленно                                                 | Что делать дальше                                                                                |
| --- | ----------------------------------- | ------------------------------------------------------------------- | ------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| 1   | precizer --dry-run                  | Доступность ФС, скорость обхода дерева файлов, базовая скорость I/O | Вероятнее всего проблема вне `precizer`: ФС/диск/сеть/нагрузка      | Проверить ФС и подсистему хранения, состояние монтирования, нагрузку системы                     |
| 2   | precizer --dry-run=with-checksums   | Реальную скорость чтения файлов + вычисления checksum (без БД)      | Узкое место: чтение данных (диск/сеть/ФС) или CPU (реже)            | Проверить скорость диска/сети, параметры ФС, ограничение ресурсов CPU (виртуализация/контейнеры) |
| 3   | Реальный режим (без Dry-Run)        | Влияние записи в БД (SQLite) и транзакций                           | Часто проблема в ФС, где лежит `.db` (медленная/сетевая/компрессия) | Проверить ФС под `.db`, попробовать временно перенести `.db` на быстрый носитель или `tmpfs`     |

## АЛЬТЕРНАТИВЫ

Альтернативы программе **precizer** с открытыми архитектурами и кодом. Больше альтернатив (актуально): [alternativeto.net](https://alternativeto.net/software/precizer-verify-file-checksums-at-scale/)

* **AIDE** — [aide.github.io](https://aide.github.io/)
  * Платформы/архитектуры: Linux/*BSD/macOS (x86_64, arm64 и др.)
  * Написана на: C, поддерживается
    * Можно выбирать/комбинировать разные хэш-алгоритмы и контролировать больше «не-хэш» атрибутов (ACL/xattr/SELinux и т.п.) на уровне правил.
    * База/снимок — текстовый формат (опционально gzip), а не SQLite: хуже подходит для сложных выборок/аналитики по данным БД.
    * Нет «докачки» прерванного хэширования внутри огромного файла: обычно перескан начинается заново.

* **Samhain** — [www.la-samhna.de/samhain](https://www.la-samhna.de/samhain/)
  * Платформы/архитектуры: Linux/Unix/POSIX, Windows (x86_64, arm64 и др.)
  * Написана на: C, поддерживается
    * Встроенная модель «агент + центральный сервер» (централизованный сбор/контроль), что выходит за рамки «одна локальная БД».
    * Упор на tamper-resistance (подписи/криптозащита части артефактов).
    * Сильно более сложное развёртывание/сопровождение (агенты/сервер/ключи), если нужна просто быстрая сверка двух деревьев через SQLite.

* **OSSEC** — [www.ossec.net](https://www.ossec.net/)
  * Платформы/архитектуры: Linux, Windows, macOS, *BSD (x86_64, arm64 и др.)
  * Написана на: C, поддерживается
    * Событийная/агентская HIDS-платформа: реально-временные алерты, правила, корреляция, реакция — это другой класс задач, чем «снимок+сверка».
    * Централизованная архитектура из коробки.
    * Не заточен под хранение «снимка файлов» именно в SQLite и под возобновление прерванного вычисления хэша внутри большого файла.

* **Open Source Tripwire** — [github.com/Tripwire/tripwire-open-source](https://github.com/Tripwire/tripwire-open-source)
  * Платформы/архитектуры: POSIX-подобные ОС (Linux/macOS/*BSD/Solaris и т.п.), Windows через Cygwin (x86_64, arm64 и др.)
  * Написана на: C++, последние изменения 8 лет назад
    * Развитый policy-язык и практика подписания policy/конфигов/базы (цепочка доверия вокруг baseline).
    * Нет SQLite-БД как «первичного формата данных» и нет возобновления прерванного хэширования внутри большого файла.

* **integrit** — [github.com/integrit/integrit](https://github.com/integrit/integrit)
  * Платформы/архитектуры: Linux/*BSD (x86_64, arm64 и др.)
  * Написана на: C, последние изменения 2 года назад
    * Минимализм и малая зависимость от внешних библиотек (если вам важнее «маленькая утилита», чем SQL-модель данных).
    * Не использует SQLite и, как следствие, хуже подходит для «жирных» выборок/сравнений/аналитики по снимкам.

* **mtree (NetBSD mtree)** — [man.netbsd.org/mtree.8](https://man.netbsd.org/mtree.8)
  * Платформы/архитектуры: *BSD, Linux (x86_64, arm64 и др.)
  * Написана на: C, последние изменения 18 лет назад
    * Спецификация дерева (spec) — удобна в сценариях «эталонная раскладка/права/владельцы», где нужен именно декларативный формат.
    * Очень старые изменения (практически «заморожено»).
    * Формат/workflow «spec-файл», а не SQLite-снимок + быстрые SQL-сверки; нет возобновления прерванного хэширования внутри большого файла.

* **hashdeep (md5deep/sha*deep)** — [github.com/jessek/hashdeep](https://github.com/jessek/hashdeep)
  * Платформы/архитектуры: Linux, Windows, macOS (x86_64, arm64 и др.)
  * Написана на: C++/C, последние изменения 9 лет назад
    * Выбор семейства хэшей/форматов вывода. Удобно, когда нужно соответствовать внешним требованиям/стандартам.
    * Нет SQLite-снимка «как продукта»: результаты — это в основном файлы отчётов/листинги, а не БД для быстрых сравнений.

* **hashit** — [github.com/boyter/hashit](https://github.com/boyter/hashit)
  * Платформы/архитектуры: Linux, Windows, macOS (x86_64, arm64 и др.)
  * Написана на: Go, поддерживается
    * Умеет считать несколько разных хэшей для одного файла за один проход.
    * Нет SQLite-снимка и механизмов «обновления базы».

* **RHash** — [github.com/rhash/RHash](https://github.com/rhash/RHash)
  * Платформы/архитектуры: Linux, Windows, macOS, *BSD (x86_64, arm64 и др.)
  * Написана на: C, поддерживается
    * Очень широкий набор алгоритмов/выходных форматов (включая магнет-ссылки и т.п.), если требуется совместимость с внешними экосистемами.
    * Не делает SQLite-снимок дерева как первичную сущность (скорее «посчитал/проверил хэши», чем «веду БД снимков»).

* **rsync** — [rsync.samba.org](https://rsync.samba.org/)
  * Платформы/архитектуры: Linux, *BSD, macOS (x86_64, arm64 и др.), Windows через Cygwin/MSYS2
  * Написана на: C, поддерживается
    * Это одновременно и перенос/синхронизация, и проверка: можно сразу «исправлять расхождения», а не только выявлять их.
    * Контрольные суммы не сохраняются между запусками: при обрывах/повторных проверках пересчёт начинается заново

* **rclone** — [rclone.org](https://rclone.org/)
  * Платформы/архитектуры: Linux, Windows, macOS, *BSD (amd64/arm/arm64 и др.)
  * Написана на: Go, поддерживается
    * Ориентирован на удалённые/облачные хранилища: S3/Drive/etc.
    * Не делает локальный SQLite-снимок дерева для быстрых офлайн-сверок A↔B.
    * Проверка целостности часто зависит от возможностей конкретного backend’а (какие хэши/метаданные он вообще отдаёт).

* **QuickHash GUI** — [www.quickhash-gui.org](https://www.quickhash-gui.org/)
  * Платформы/архитектуры: Linux, Windows, macOS (x86_64; macOS arm64)
  * Написана на: FreePascal (Lazarus), последние изменения 2 года назад
    * GUI-only ориентированный инструмент и «комбайн» под разные носители/артефакты (например, сценарии форензики), где CLI-снапшоты не всегда удобны.
    * Не использует SQLite-снимок дерева как ядро workflow.

* **restic** — [restic.net](https://restic.net/)
  * Платформы/архитектуры: Linux, Windows, macOS, *BSD (x86_64, arm64 и др.)
  * Написана на: Go, поддерживается
    * Репозиторий бэкапов со снапшотами, дедупликацией и шифрованием — если цель «хранить историю и переносить её», это сильнее, чем просто сравнить два дерева.
    * Другой подход: chunk/dedup-модель вместо «SQLite-снимок дерева + сверка»; для задачи чистой сверки A↔B может быть избыточен.
    * Не дает «прямой» модели сравнения двух локальных деревьев через одну SQL-БД

## АВТОР
Автор приложения [Денис Владимирович Разумовский](https://github.com/dennisrazumovsky)

## COPYING
This program is distributed under the GNU General Public License v3.0 (GPLv3) as provided in the top-level `COPYING` file. The author is not responsible for any use of the source code or the entire program. Anyone who uses the code or the program uses it at their own risk and responsibility

### Ограничение использования на территории рашистского террористического геообразования, захваченного оккупировавшей власть авторитарной диктатурой

- Разрешено: исключительно личное некоммерческое использование частными лицами.
- Запрещено: любое использование, прямо или косвенно приводящее к уплате налогов, сборов, взносов или иных обязательных платежей в местные бюджеты на указанной территории (включая НДС, налог на прибыль, НДФЛ при исполнении обязанностей налогового агента, страховые взносы, таможенные пошлины и т.п.).
- Также запрещено использование структурами, которые по недоразумению именуют себя государственными органами, государственными компаниями, бюджетными учреждениями и аффилированными организациями.
- Коммерческая эксплуатация, возмездное распространение, платная поддержка и интеграция — запрещены, если осуществляются на указанной территории или для её резидентов и влекут уплату обязательных платежей.
- Запрет распространяется на использование самой программы, её исходного кода полностью или частично.
- Цель условия — исключить прямое и косвенное финансирование войны в Украине.
