# Precizer
Крошечное, высокопроизводительное приложение для проверки целостности файлов

«По-настоящему хорошая программа всегда поместится на дискету. Есть надежда, что кто-то всё ещё помнит, что это такое… Речь идёт не о дискетах, а о качественных программах!»© :-D

<p width="100%" height="100%">
<img width="20%" src="img/micrometer_0.svg">
</p>

## АВТОР
Автор приложения [Денис Владимирович Разумовский](https://github.com/dennisrazumovsky)

## LICENSE
This program is distributed under the [CC0 (Creative Commons Share Alike) license](https://creativecommons.org/publicdomain/zero/1.0/). The author is not responsible for any use of the source code or the entire program. Anyone who uses the code or the program uses it at their own risk and responsibility.

Использование программы, исходных текстов или частей исходного кода категорически запрещено на территории рашстского террористического государства, захваченного оккупировавшей власть авторитарной диктатурой.

## КРАТКО

### Обзор

**precizer** — крошечное и быстрое консольное приложение полностью написанное на чистом Си. Предназначено для проверки целостности и сравнения файлов. Особенно полезно для проверки результатов синхронизации. Программа рекурсивно обходит каталоги и создает базу данных файлов и их контрольных сумм с последующим быстрым сравнением.

**precizer** предназначен как для работы на embedded платформах, так и с файловыми системами гигантского размера на базе кластерных мейнфреймов. С помощью программы можно найти ошибки синхронизации, сравнивая данные с файлами и их контрольными суммами из разных источников. Также **precizer** можно использовать для исследования исторических изменений путем сравнения баз данных из одних и тех же источников но за разное время.

### Простой пример

Допустим, есть две машины у которых в /mnt1 и в /mnt2 соответственно, примонтированы диски большого объёма с идентичным содержимым. Стои́т задача побайтно проверить действительно ли содержимое абсолютно идентично или есть различия.

1. Запустить программу на первой машине с hostname, например «host1»:

```sh
precizer --progress /mnt1
```

В результате работы программы будут рекурсивно исследованы все директории начиная с /mnt1 и создана база данных host1.db в текущей директории. Параметр _--progress_ визуализирует прогресс и покажет объем пространства и количество исследуемых файлов.

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
* Какие файлы отсутствуют на «host1» но при этом присутствуют на «host2» и наоборот.
* Для каких файлов, присутствующих на обеих хостах, контрольные суммы НЕ совпадают.

### Относительные пути в базе данных для сравнения

Следует обратить внимание, что **precizer** записывает в базу данных только относительные пути. Файл  

```
/mnt1/abc/def/aaa.txt
```

из приведённого примера будет записан в базу данных как  

```
abc/def/aaa.txt
```

без _/mnt1_. То же самое произойдёт с файлом

```
/mnt2/abc/def/aaa.txt
```

Смысл в том, что несмотря на разные точки монтирования и разные источники файлы можно будет сравнить между собой под одинаковыми именами

```
abc/def/aaa.txt
```

и с соответствующими контрольными суммами.

## ТЕХНИЧЕСКИЕ ПОДРОБНОСТИ

Рассмотрим сценарий, когда имеется основное дисковое хранилище и его копия. Например, это может быть хранилище датацентра и его Disaster Recovery копия. Периодически происходит синхронизация с основного хранилища на резервное, но по причине огромных объёмов данных, скорее всего, синхронизация происходит не побайтно, а за счёт вычисления изменений среди метаданных файлов на файловой системе. В таких случаях учитывается размер файла и время модификации, но изменившееся содержимое байт за байтом не исследуется. В этом есть смысл, потому что между основным датацентром и резервным Disaster Recovery центром, как правило, хорошие каналы связи, но полная побайтовая синхронизация может занять нецелесообразно много времени. Такие инструменты как rsync позволяют производить синхронизацию по обеим методикам: как с учётом изменившихся файлов так и побайтово, но у них есть один серьёзный недостаток — **состояние не сохраняется между сессиями**. Что это значит рассмотрим детально:

* Даны: сервер «A» и сервер «B» (основной датацентр и резервный Disaster Recovery)
* На сервере «A» изменились некоторые файлы.
* Алгоритм rsync их определил за счёт изменившегося размера и времени модификации файла и синхронизировал на сервер «B».
* Во время синхронизации между основным датацентром и Disaster Recovery происходили многократные сбои связи.
* Для проверки целостности данных (эквивалентности сохранённых файлов на «A» и «B» байт в байт) обычно используют тот же rsync только с включением побайтного сравнения. Для этого:
  * **rsync** запускается на сервере «A» в режиме _--checksum_ и во время одного сеанса пытается подсчитать контрольные суммы последовательно сначала на «A», и затем на «B».
  * Этот процесс занимает **неимоверно большое время для огромных дисковых массивов**
  * Так как rsync не позволяет сохранять состояние уже подсчитанных контрольных сумм между сеансами, то возникает целый ряд технических сложностей. А именно:
    * В случае разрыва соединения **rsуnc** завершает сеанс и в следующий запуск **всё нужно начинать сначала!** С учётом огромных объёмов побайтовая проверка данных на полную идентичность таким образом превращается в нереализуемую задачу.
  * Причиной неидентичности бинарного содержимого файлов могут стать так же сбои на уровне дисковой подсистемы. В таких случаях с помощью метаданных файловой системы невозможно будет определить есть ли разница между содержимым внутри файлов на серверах «A» и «B» или нет.
  * Со временем ошибки накапливаются и появляется угроза получить **неконсистентную копию системы «A» на системе «B»**, что сводит на нет все усилия и затраты по поддержанию Disaster Recovery. При этом стандартные утилиты не обладают возможностями проверок и технический персонал даже не будет знать о накопившихся проблемах с неэквивалентным содержанием дисковых массивов на Disaster Recovery центре.
* Для устранения вышеописанных недостатков создана программа **precizer**. Программа позволяет **выявить какие именно файлы отличаются между «A» и «B»** для проведения повторной синхронизации с устранением отличий. Программа работает **максимально быстро** (практически на грани аппаратных возможностей) за счёт того, что написана на чистом Си и использует современные алгоритмы, оптимизированные под высокую производительность. Программа предназначена для работы как с мелкими файлами, так и с объёмами данных, измеряемыми петабайтами, и не ограничена этими цифрами.
* Название программы **precizer** происходит от слова precision (точность) и означает что-то, что увеличивает точность.
* Программа с высокой точностью исследует содержимое директорий, субдиректорий и **подсчитывает контрольные суммы** для каждого встреченного файла, при этом **сохраняя метаинформацию** о всех файлах в **SQLite базе** (обычный бинарный файл).
* **precizer устойчива к сбоям** и умеет продолжить работу с того момента, где была прервана. Например, если программа была остановлена нажатием **Ctrl+C** в момент анализа файла петабайтного объёма, то при повторном запуске она **НЕ будет анализировать его с самого начала**, а продолжит именно с того момента, на котором была прервана и о котором уже есть запись в базе данных. Это позволит сэкономить ресурсы, время и нервы системных администраторов.
* Работа этой программы **может быть прервана в любой момент любым способом** и это **безопасно** как для исследуемых данных, так и для БД, созданной самой программой.
* В случае умышленной или случайной остановки работы программы **можно не беспокоиться о результатах сбоя**. Результат работы будет полностью сохранён и повторно использован при следующих запусках.
* Для подсчёта контрольных сумм используется **надёжный и быстрый алгоритм SHA512**, полностью исключающий коллизии даже в случае анализа единичного файла петабайтного объёма. Если есть два полностью идентичных файла огромного объёма, различающихся только в один байт, то **алгоритм SHA512 это отразит** и контрольные суммы будут различаться, что **не может быть гарантировано в случае использования более простых хеш-функций** типа SHA1 или CRC32.
* Алгоритмы программы **precizer** разработаны так, что очень просто **поддерживать актуальность** содержащихся данных в созданной базе с путями к файлам и их контрольными суммами **без пересчёта всего с самого начала**. Достаточно запустить программу с параметром _--update_, чтобы в БД попали новые файлы или была удалена информация о стёртых с диска файлах. Для тех файлов, которые подверглись модификациям и их размеры изменились, будет пересчитана контрольная сумма SHA512 и обновлённая записана в БД.
* Можно указать опцию, при которой при обновлении БД будет учитываться не только размер изменившихся файлов, но так же и время создания или изменения файлов. Это значит, что изменения любой метаинформации о файле приведёт к пересчёту контрольной суммы SHA512 с последующим обновлением данных о файле в БД. Например, если у файла изменилась **ctime**, но не изменился размер, то контрольная сумма для такого файла **НЕ будет пересчитана** при указании только одного параметра --update.
* **precizer может служить инструментом контроля безопасности**, определяя последствия вторжения за счёт выявления несанкционированно изменённых файлов, у которых могло быть модифицировано содержимое, но метаданные остаться прежними.
* Программа **никогда не меняет, не удаляет, не перемещает и не копирует** ни файлы, ни исследуемые директории. Всё, что она делает: **составляет списки файлов, контрольные суммы содержимого этих файлов и актуализирует их в базе данных**. Все изменения происходят исключительно в границах базы данных.
* Производительность программы в основном упирается в производительность дисковой подсистемы. Каждый файл считывается побайтно, и для каждого файла формируется своя контрольная сумма с использованием SHA512.
* Программа работает **очень быстро** благодаря библиотекам **SQLite** и **FTS** ([man 3 fts](https://man7.org/linux/man-pages/man3/fts.3.html)).
* Разбор параметров строки реализован через библиотеку **ARGP**.
* Для регулярных выражений выбрана библиотека **PCRE2**.
* Программа **безопасна для случаев с огромным количеством файлов, директорий и поддиректорий** любой вложенности. Благодаря библиотеке **FTS** рекурсия не используется, поэтому не произойдёт переполнения стека даже в случае большой вложенности файлов.
* За счёт своей компактности и переносимости кода программа может использоваться даже на специализированных устройствах типа NAS или любых embedded или IoT устройствах.

## ВОПРОСЫ И БАГРЕПОРТЫ

* Подсказка --help сделана максимально подробной специально для помощи пользователям, не обладающих специализированными техническими знаниями.
* Обратиться к автору можно:  
  * [Через форму GitHub Дискуссий](https://github.com/precizer/precizer/discussions).
  * Там же на GitHub можно [опубликовать багрепорт](https://github.com/precizer/precizer/issues/new).
* При возникновении сложностей при использовании программы можно задать вопрос на ru.stackoverflow.com используя тег **precizer**. Автор следит за такими вопросами и будет рад помочь в решении проблем любой сложности.

## СБОРКА И УСТАНОВКА

### Готовое портируемое решение

Полностью готовое к работе решение [можно скачать по этой ссылке](https://github.com/precizer/precizer/releases).

#### Технические подробности portable сборки

Готовая сборка представляет из себя исполнимый статически слинкованный бинарный файл в формате ELF, который может быть запущен сразу практически на любом дистрибутиве x64 Linux. Файл собран CI/CD автосборкой ресурса GitHub, затем сжат с помощью [UPX (архиватора исполняемых файлов)](https://upx.github.io). После этого самораспаковывающийся сжатый бинарный файл вкладывается в zip архив для удобного скачивания. Для использования его достаточно распаковать из zip и запустить.

### Distributives Packaging

* Автор был рад подготовить автоматическую сборку средствами GitHub Workflows и будет в дальнейшем поддерживать новые версии.

* Автор НЕ готов самостоятельно готовить и поддерживать в будущем опакечивание программы **precizer** под _все_ существующие дистрибутивы операционных систем.

* Если Вы горите желанием создать пакет под любой дистрибутив и столкнулись с непреодолимыми трудностями по адаптации кода программы, то именно в этом случае автор будет очень рад оказать всю необходимую помощь в поддержке инициативы и оптимизации кода программы под конкретный дистрибутив или пакетный менеджер. Как связаться с автором описано в разделе [«Вопросы и багрепорты»](#ВОПРОСЫ-И-БАГРЕПОРТЫ)

### Самостоятельная сборка

Результатом сборки будет статически слинкованный бинарный исполняемый файл в формате ELF без каких либо динамических зависимостей. Этот файл представляет из себя всю программу целиком и может быть запущен практически на любом современном дистрибутиве Linux.

В программу интегрированы почти все используемые библиотеки и по умолчанию программа собирается как статический исполняемый файл. Это сделано для увеличения переносимости и уменьшению зависимостей. Благодаря вышеописанному программу можно легко собрать на большинстве современных платформ выполнив несколько команд:

1. Install build and compile tools on Linux

#### Arch Linux

```sh
sudo pacman -S --noconfirm base-devel
```
#### Debian/Ubuntu Linux

```sh
sudo apt -y install build-essential
```

#### Alpine Linux

```sh
sudo apk add --update build-base fts-dev argp-standalone
```

2. Get source code

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
```

3. Build

```sh
make
```

4. Скопируйте получившийся исполняемый файл **precizer** в любое место, прописанное в системной переменной $PATH для быстрого вызова.

5. Clean everything

```sh
# Clean
make clean

# Clean with building libraries
make cleanall
```

6. Update

```sh
git pull
make

# Перейти к пункту 4.
```

### Сборка portable

Повторить пункты 1. и 2. Вместо пункта 3 выполнить:

```sh
make portable
```

### Сборка с помощью Docker

Если есть причины не устанавливать дополнительные пакеты для сборки приложения, то можно воспользоваться подготовленным решением на базе Docker.

Для сборки достаточно в системе уже иметь установленный docker.

Результатом работы простой команды `make docker`:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make docker
```

станет то, что в текущей директории будет создан бинарный исполняемый файл precizer. Его можно запускать на месте или скопировать в директории из списка $PATH

Если утилита make не установлена, то для сборки приложения в контейнере достаточно выполнить следующие действия:

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
docker build -t precizer .
docker create --name precizer precizer
docker cp precizer:/precizer/precizer precizer
docker rm -f precizer
```

В результате в текущей директории появится статически слинкованный бинарный файл

Если есть проблемы с исполняемым файлом, то можно попробовать повысить его переносимость между разными системами.

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make docker-portable
```
или

```sh
git clone https://github.com/precizer/precizer.git
cd precizer
docker build --build-arg OS=ubuntu:18.04 --build-arg BUILD=portable -t precizer .
docker create --name precizer precizer
docker cp precizer:/precizer/precizer precizer
docker rm -f precizer
```

## ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ
### Тесты
Для проверки возможностей программы можно использовать наборы тестов из директории tests/examples/ в исходном коде программы

Запуск тестов:  
```sh
git clone https://github.com/precizer/precizer.git
cd precizer
make debug
cd tests/
make debug
./testitall
```

### Пример 1
Добавить файлы в две базы данных и сравнить их между собой:

```sh
precizer --progress --database=database1.db tests/examples/diffs/diff1

precizer --progress --database=database2.db tests/examples/diffs/diff2

precizer --compare database1.db database2.db
```
<sup>The comparison of database1.db and database2.db databases is starting…  
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

### Пример 2
Актуализация базы данных

Попробуем использовать предыдущий пример ещё раз. Первая попытка. Сообщение с предупреждением.

```sh
precizer --progress --database=database1.db tests/examples/diffs/diff1
```

<sub>The database database1.db was previously created and already contains data with files and their checksums. Use the --update option only when you are certain that the database needs to be updated and when file information (including changes, deletions, and additions) should be synchronized with the database.  
ERROR: The precizer process terminated unexpectedly due to an error
</sub>

Должен быть добавлен параметр **--update** параметр необходим для защиты базы данных от потери информации из-за случайного запуска.

```sh
precizer --update --progress --database=database1.db tests/examples/diffs/diff1
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
echo -n "  " >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt

# Add a new file
touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt

# Remove a file
rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt

```

и запустим **precizer** ещё раз, но уже с параметром --update:

```sh
precizer --update --progress --database=database1.db tests/examples/diffs/diff1
```

<sub>Primary database file name: database1.db  
Starting database file database1.db integrity check…  
Database database1.db has been verified and is in good condition  
File system traversal initiated to calculate file count and storage usage  
Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
The **--update** option has been used, so the information about files will be updated against the database database1.db  
File traversal started  
**These files have been added or changed and those changes will be reflected against the DB database1.db:**  
1/AAA/BCB/CCC/a.txt changed size & ctime & mtime rehashed  
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

При каждом запуске **precizer** обходит файловую систему после этого проверяя, есть ли запись об определенном файле в базе данных или нет. Другими словами, приоритет для программы имеет состояние файловой системы на диске.

Обход каталогов **precizer** работает очень похоже на работу rsync, поскольку использует похожий алгоритм.

Стоит обратить внимание, что **precizer** не будет пересчитывать контрольные суммы SHA512 для файлов, которые уже были записаны в базу данных и для которых метаданные файла остаются прежними (такие как размер и время последнего доступ atime). Если указан аргумент --watch-timestamps то помимо размера будут учитываться время создания и время модификации (mtime и ctime).

Любые новые файлы, удаленные файлы или те файлы, которые изменились между запусками приложения, будут обработаны и, соответственно, все изменения будут отражены в базе данных если указан параметр _--update_.

### Пример 3
Использование режима _--silent_ При включении этого режима программа ничего не выводит на экран. Это имеет смысл при использовании **precizer** в скриптах.

Добавим параметр **--silent** к предыдущему примеру:

```sh
precizer --silent --update --progress --database=database1.db tests/examples/diffs/diff1
```

В результате на экране ничего не отобразится.

### Пример 4
Дополнительная информация в режиме _--verbose_ Может быть полезна для отладки.

Добавим параметр **--verbose** к предыдущему примеру:

```sh
precizer --verbose --update --progress --database=database1.db tests/examples/diffs/diff1
```
<sub>2025-01-28 09:55:59:820 src/parse_arguments.c:442:parse_arguments:Configuration: rational_logger_mode=VERBOSE  
paths=tests/examples/diffs/diff1; database=database1.db; db_file_name=database1.db; verbose=yes; maxdepth=-1; silent=no; force=no; update=yes; watch-timestamps=no; progress=yes; compare=no, db-clean-ignored=no, dry-run=no, check-level=FULL, rational_logger_mode=VERBOSE  
2025-01-28 09:55:59:820 src/parse_arguments.c:558:parse_arguments:Arguments parsed  
2025-01-28 09:55:59:820 src/detect_paths.c:025:detect_paths:Checking directory paths provided as arguments  
2025-01-28 09:55:59:820 src/file_availability.c:034:file_availability:Verify that the path tests/examples/diffs/diff1 exists  
2025-01-28 09:55:59:820 src/file_availability.c:053:file_availability:The path tests/examples/diffs/diff1 is exists and it is a directory  
2025-01-28 09:55:59:821 src/detect_paths.c:036:detect_paths:Paths detected  
2025-01-28 09:55:59:821 src/init_signals.c:034:init_signals:Set signal SIGUSR2 OK:pid:604770  
2025-01-28 09:55:59:821 src/init_signals.c:043:init_signals:Set signal SIGINT OK:pid:604770  
2025-01-28 09:55:59:821 src/init_signals.c:052:init_signals:Set signal SIGTERM OK:pid:604770  
2025-01-28 09:55:59:821 src/init_signals.c:055:init_signals:Signals initialized  
2025-01-28 09:55:59:821 src/determine_running_dir.c:018:determine_running_dir:Current directory: /tmp  
2025-01-28 09:55:59:821 src/db_determine_name.c:099:db_determine_name:Primary database file name: database1.db  
2025-01-28 09:55:59:821 src/db_determine_name.c:105:db_determine_name:Primary database file path: database1.db  
2025-01-28 09:55:59:821 src/db_determine_name.c:109:db_determine_name:DB name determined  
2025-01-28 09:55:59:821 src/file_availability.c:034:file_availability:Verify that the path . exists  
2025-01-28 09:55:59:821 src/file_availability.c:053:file_availability:The path . is exists and it is a directory  
2025-01-28 09:55:59:821 src/file_availability.c:034:file_availability:Verify that the path database1.db exists  
2025-01-28 09:55:59:821 src/file_availability.c:044:file_availability:The path database1.db is exists and it is a file  
2025-01-28 09:55:59:821 src/db_determine_mode.c:128:db_determine_mode:Final value for config->sqlite_open_flag: SQLITE_OPEN_READWRITE  
2025-01-28 09:55:59:821 src/db_determine_mode.c:129:db_determine_mode:Final value for config->db_initialize_tables: false  
2025-01-28 09:55:59:821 src/db_determine_mode.c:131:db_determine_mode:DB mode determined  
2025-01-28 09:55:59:821 src/db_test.c:061:db_test:Starting database file database1.db integrity check…  
2025-01-28 09:55:59:821 src/db_test.c:082:db_test:The database verification level has been set to FULL  
2025-01-28 09:55:59:821 src/db_test.c:126:db_test:Database database1.db has been verified and is in good condition  
2025-01-28 09:55:59:822 src/db_get_version.c:087:db_get_version:Version number 1 found in database  
2025-01-28 09:55:59:822 src/db_check_version.c:032:db_check_version:The database1.db database file is version 1  
2025-01-28 09:55:59:822 src/db_check_version.c:061:db_check_version:The database database1.db is on version 1 and does not require any upgrades  
2025-01-28 09:55:59:822 src/db_init.c:030:db_init:Successfully opened database database1.db  
2025-01-28 09:55:59:822 src/db_init.c:118:db_init:The primary database and tables have NOT been initialized  
2025-01-28 09:55:59:822 src/db_init.c:150:db_init:The primary database named database1.db is ready for operations  
2025-01-28 09:55:59:822 src/db_init.c:167:db_init:The in-memory runtime_paths_id database successfully attached to the primary database database1.db  
2025-01-28 09:55:59:822 src/db_init.c:174:db_init:Database initialization process completed  
2025-01-28 09:55:59:822 src/db_compare.c:136:db_compare:Database comparison mode is not enabled. Skipping comparison  
2025-01-28 09:55:59:822 src/db_contains_data.c:086:db_contains_data:The database database1.db has already been created previously  
2025-01-28 09:55:59:822 src/db_validate_paths.c:192:db_validate_paths:The paths written against the database and the paths passed as arguments are completely identical  
2025-01-28 09:55:59:822 src/file_list.c:143:file_list:File system traversal initiated to calculate file count and storage usage  
2025-01-28 09:55:59:823 src/file_list.c:038:show_status:Total size: 43B, total items: 58, dirs: 46, files: 12, symlnks: 0  
2025-01-28 09:55:59:825 src/db_get_version.c:087:db_get_version:Version number 1 found in database  
2025-01-28 09:55:59:825 src/db_consider_vacuum_primary.c:025:db_consider_vacuum_primary:No changes were made. The primary database doesn't require vacuuming  
2025-01-28 09:55:59:825 src/status_of_changes.c:049:status_of_changes:**The database file database1.db has NOT been modified since the program was launched**  
2025-01-28 09:55:59:825 src/exit_status.c:027:exit_status:The precizer completed its execution without any issues  
</sub>

### Пример 5
Исследование без рекурсии с помощью параметра _--maxdepth_

```sh
tree tests/examples/4

tests/examples/4
├── AAA
│   ├── BBB
│   │   ├── CCC
│   │   │   └── a.txt
│   │   └── uuu.txt
│   └── tttt.txt
└── sss.txt

3 directories, 4 files
```

Параметр _--maxdepth_ со значением _=0_ полностью отключает рекурсию.

```sh
precizer --maxdepth=0 tests/examples/4
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

Чтобы проверить и протестировать регулярные выражения PCRE2 можно использовать ресурс https://regex101.com/

Для понимания как выглядит относительный путь достаточно запустить сканирование директорий без опции _--ignore_ и посмотреть, как терминал будет отображать относительные пути, записываемые в базу данных:

```sh
% tree -L 3 tests/examples/diffs

tests/examples/diffs
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
precizer --ignore="diff1/1/.*" tests/examples/diffs
```

В этом примере начальный путь сканирования — ./tests/examples/diffs, а сформированный путь для игнорирования — ./tests/examples/diffs/diff2/1/ со всеми подкаталогами (/*).

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

Повторим тот же пример, но без опции _--ignore_, чтобы добавить три ранее проигнорированных файла:

```sh
precizer --update tests/examples/diffs
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

Несколько регулярных выражений для игнорирования можно использовать с помощью опций _--ignore_ указав их одновременно.

База данных будет очищена от упоминаний файлов, соответствующих регулярным выражениям из аргументов --ignore: "diff1/1/.\*" и "diff2/1/.\*"

Параметр _--db-clean-ignored_ должен быть указан дополнительно чтобы удалить из базы данных упоминание файлов, соответствующих регулярным выражениям, переданным через опции _--ignore_.

На файловой системе не было изменений, но игнорируемые файлы будут удалены из БД.

```sh
# Обновить базу данных, удалив информацию о тех файлах, которые были указаны как игнорируемые:

precizer \
    --update \
    --db-clean-ignored \
    --ignore="diff1/1/.*" \
    --ignore="diff2/1/.*" \
    tests/examples/diffs
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

### Example 8

Использование параметров _--ignore_ вместе с _--include_

```sh
# Удалим старую базу данных и создадим новую, наполним ее данными:

rm -i "${HOST}.db"

precizer tests/examples/diffs
```
Усложним задачу с использованием регулярных выражений.

Регулярные выражения PCRE2 для относительных путей, которые необходимо включить. Включаем указанные относительные пути, даже если они были исключены с помощью одного или нескольких параметров --ignore. Несколько регулярных выражений могут быть указаны с помощью --include

Чтобы проверить и протестировать регулярные выражения PCRE2 можно использовать ресурс https://regex101.com/

DB будет очищена от упоминаний файлов, соответствующих регулярным выражениям из аргументов --ignore: "^.\*/path2/.\*" и "diff2/.\*" но пути, соответствующие шаблонам из _--include_ останутся в базе данных.

Параметр _--db-clean-ignored_ должен быть указан дополнительно чтобы удалить из базы данных упоминание файлов, соответствующих регулярным выражениям, переданным через опции _--ignore_.

```sh

# Обновить базу данных, удалив информацию о тех файлах, которые были указаны как игнорируемые за исключением шаблонов путей из _--include_

precizer --update --db-clean-ignored \
	--ignore="^.*/path2/.*" \
	--ignore="diff2/.*" \
	--include="diff2/1/AAA/ZAW/A/b/c/.*" \
	--include="diff2/path1/AAA/ZAW/.*" \
	tests/examples/diffs
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
