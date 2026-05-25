[Link to the English language README page](README.md)

# librational — помогатор для C-кода

`librational` — небольшая встраиваемая библиотека с общими флагами, обработкой возврата, логированием, форматированием значений и вспомогательными функциями для C-кода. Самая заметная часть библиотеки — тип `Return`, который позволяет функции вернуть не один плоский код, а несколько независимых признаков в одном значении.

Кроме работы с `Return`, библиотека умеет выводить сообщения через общий слой отчетов и логирования, получать текущее время, форматировать числа с разделителями тысяч, переводить размер в байтах в человеко-читаемый вид и превращать длительность в наносекундах в строку с годами, месяцами, неделями, днями, часами и более мелкими единицами.

Тип `Return` удобен, когда нужно отдельно понимать:

* была ли техническая ошибка внутри функции;
* успешно ли функция завершила свою работу;
* какой логический ответ функция получила, если она что-то проверяла;
* есть ли глобальный контекст, который должен повлиять на последующие возвраты.

## Содержание

1. [Основная идея](#основная-идея)
2. [Быстрый старт](#быстрый-старт)
3. [Вспомогательное форматирование](#вспомогательное-форматирование)
4. [Вспомогательные функции времени](#вспомогательные-функции-времени)
5. [Текстовое представление статусов](#текстовое-представление-статусов)
6. [Отчеты и логирование](#отчеты-и-логирование)
7. [Слои статусов](#слои-статусов)
8. [Базовая функция](#базовая-функция)
9. [Функция-проверка с YES и NO](#функция-проверка-с-yes-и-no)
10. [Как правильно разбирать результат](#как-правильно-разбирать-результат)
11. [Последовательные проверки через status](#последовательные-проверки-через-status)
12. [Цепочки вызовов: run() и call()](#цепочки-вызовов-run-и-call)
13. [Глобальный статус](#глобальный-статус)
14. [Технические подробности](#технические-подробности)
15. [Практические правила](#практические-правила)

## Основная идея

Обычный `int`-код возврата часто смешивает разные смыслы. Например, `access()` возвращает `0`, если файл доступен, и `-1`, если файл недоступен или произошла ошибка. В простом API этого достаточно, но в большом приложении полезно различать:

* функция сама отработала корректно, но проверка дала отрицательный ответ;
* функция столкнулась с внутренней технической проблемой;
* программа уже находится в глобальном состоянии остановки или предупреждения.

`Return` решает это через битовые флаги. Один возврат может выглядеть так:

```c
SUCCESS | YES
```

Это значит: функция завершилась без внутренней ошибки, а логический ответ проверки положительный.

Другой пример:

```c
SUCCESS | NO
```

Это значит: функция завершилась без внутренней ошибки, но логический ответ проверки отрицательный. Например, файл не доступен, запись не найдена или условие не выполнено.

## Быстрый старт

Обычная функция начинается с локального `status` и выходит через `provide(status)`.

```c
Return load_settings(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	provide(status);
}
```

Функция-проверка возвращает обычный технический результат вместе с локальным ответом `YES` или `NO`.

```c
Return path_is_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status |= YES; /* or NO */

	provide(status);
}
```

Вызывающий код читает такой ответ через `ask(...)`.

```c
if(ask(path_is_readable(path)))
{
	run(print_file(path));
}
```

Короткая шпаргалка:

| Нужно | Использовать |
|---|---|
| вернуть статус из обычной функции | `provide(status)` |
| вернуть статус без TRACE-лога | `deliver(status)` |
| выполнить рабочий шаг, который можно пропустить после сбоя | `run(func())` |
| выполнить очистку или обязательное финальное действие | `call(func())` |
| разобрать `YES` или `NO` | `ask(check_func())` |
| вручную продолжить только при штатном статусе | `if(TRIUMPH & status)` |
| остановить цикл после любого `SKIP`-статуса | `if((SKIP & status) == 0)` |

Не путать:

* `NO` не равен `false` или `0`;
* `NO` не является `FAILURE`;
* `YES` и `NO` не передаются через `run()` и `call()`;
* `YES` и `NO` не возвращаются выше, если текущая функция сама не является функцией-проверкой.

## Вспомогательное форматирование

`librational` также содержит небольшие функции для подготовки значений к человеко-читаемому выводу. Они полезны в логах, отчетах, тестовых сообщениях и CLI-выводе, где важнее получить понятную строку, чем каждый раз заново собирать форматирование в вызывающем коде.

Числовой макрос `form(value,buffer,buffer_size)` выбирает подходящую функцию форматирования по типу аргумента. Вещественные значения форматируются с запятой как разделителем тысяч, точкой как десятичным разделителем, округлением до 9 дробных знаков и удалением лишних нулей справа. Целые значения форматируются с группировкой по три цифры. Например:

```c
char text[FORM_OUTPUT_BUFFER_SIZE];

printf("%s\n",form(1234567.125L,text,sizeof(text))); /* 1,234,567.125 */
printf("%s\n",form((int)-12345,text,sizeof(text)));  /* -12,345 */
```

Для явного вызова доступны reentrant-функции `form_real_r()`, `form_intmax_r()` и `form_uintmax_r()`. Они пишут результат в буфер вызывающего кода. Если буфер для `form_real_r()` слишком мал, функция сначала уменьшает дробную точность, а если значение всё равно не помещается, записывает пустую строку. Функции форматирования целых чисел записывают пустую строку, если целое значение невозможно поместить полностью.

Функция `itoa(value,buffer,base)` преобразует `int` в строку в системе счисления от 2 до 36. В base 10 отрицательные числа выводятся со знаком `-`. В других системах счисления отрицательные значения выводятся как unsigned bit pattern, что удобно для шестнадцатеричных и двоичных дампов. Буфер должен быть достаточно большим: функция не получает его размер и не может сама защититься от переполнения. При недопустимом основании функция записывает пустую строку, выставляет `errno = EINVAL` и возвращает `buffer`; при `NULL`-буфере выставляет `errno = EINVAL` и возвращает `NULL`.

```c
char number[33];

printf("%s\n",itoa(255,number,16));  /* FF */
printf("%s\n",itoa(-789,number,10)); /* -789 */
```

Функции `bkbmbgbtbpbeb()` и `bkbmbgbtbpbeb_r()` переводят количество байтов в строку с двоичными единицами `B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB` и `EiB`. Режим `FULL_VIEW` показывает все ненулевые единицы, а `MAJOR_VIEW` оставляет только самую крупную:

```c
printf("%s\n",bkbmbgbtbpbeb(1536,FULL_VIEW));  /* 1KiB 512B */
printf("%s\n",bkbmbgbtbpbeb(1536,MAJOR_VIEW)); /* 1KiB */
```

Функции `form_date()` и `form_date_r()` переводят длительность в наносекундах в строку с единицами времени. В `FULL_VIEW` выводятся все ненулевые части, а в `MAJOR_VIEW` только самая крупная часть:

```c
printf("%s\n",form_date(3600000000001LL,FULL_VIEW));  /* 1h 1ns */
printf("%s\n",form_date(3600000000001LL,MAJOR_VIEW)); /* 1h */
```

Функции без суффикса `_r` возвращают указатель на внутренний статический буфер. Это удобно для короткого вывода, но следующий вызов той же функции перезапишет предыдущую строку. Функции с суффиксом `_r` принимают буфер вызывающего кода и подходят для случаев, где нужно сохранить несколько результатов одновременно или избежать общего статического состояния.

## Вспомогательные функции времени

`librational` содержит несколько простых функций для получения времени и форматирования отметок времени. Они нужны для логов, тестов и измерений, где не хочется каждый раз писать одинаковую обвязку вокруг системных часов.

`cur_time_ms()` возвращает количество миллисекунд с начала Unix epoch по системным календарным часам. `cur_time_ns()` возвращает количество наносекунд с начала Unix epoch по `CLOCK_REALTIME`. Эти значения привязаны к реальному календарному времени и могут измениться скачком, если системные часы были скорректированы.

`cur_time_monotonic_ns()` возвращает наносекунды из монотонного источника времени, когда он доступен на целевой платформе. Это значение не является календарной датой: оно подходит для измерения интервалов между двумя событиями. Если монотонные часы недоступны во время сборки, имя прозрачно заменяется на `cur_time_ns()`.

`seconds_to_ISOdate(seconds)` превращает Unix-время в секундах в локальную строку вида `YYYY-MM-DD HH:MM:SS`. Функция возвращает указатель на внутренний статический буфер, поэтому следующий вызов перезапишет предыдущий результат. Чтобы получить текущее время, передайте `time(NULL)`; значение `0` означает саму Unix epoch, а не “сейчас”.

```c
printf("%lld\n",cur_time_ms());
printf("%s\n",seconds_to_ISOdate(time(NULL)));
```

## Текстовое представление статусов

`show_status(status)` переводит `Return` в короткую строку для логов, отладочных сообщений и тестов. Нулевой статус `OK` выводится как `OK`, а известные флаги объединяются через `|`, например `SUCCESS|YES` или `FAILURE|WARNING`. Если в значении нет известных флагов, функция возвращает `UNKNOWN`.

Для составных статусов функция использует внутренний статический буфер. Сохраняйте строку в свой буфер, если нужно пережить следующий вызов `show_status()`.

```c
Return status = SUCCESS | YES;
printf("%s\n",show_status(status)); /* SUCCESS|YES */
```

## Отчеты и логирование

Для диагностических сообщений есть два низкоуровневых помощника. `serp(prefix)` печатает в `stderr` сообщение с текущим `errno`, именем файла и именем функции. `report(format,...)` печатает форматированное сообщение об ошибке с файлом, функцией, строкой исходного кода и расшифровкой `errno`. Эти помощники рассчитаны на аварийные пути и не требуют выделения памяти в куче.

```c
errno = EINVAL;
serp("Invalid input");
report("Failed to process item %d",item_id);
```

Основной логгер вызывается через `slog(level,format,...)`. Макрос автоматически добавляет файл, строку и имя функции, а вывод управляется глобальным атомарным режимом `rational_logger_mode`.

Основные режимы:

* `REGULAR` — обычные сообщения
* `VERBOSE` — подробные сообщения с временной меткой и местом вызова
* `TESTING` — сообщения для тестового вывода
* `ERROR` — сообщения об ошибках
* `SILENT` — подавить обычный вывод
* `UNDECOR` — вывести только текст сообщения без префиксов логгера
* `REMEMBER` — передать готовую строку в необязательный обработчик `rational_remember()`
* `VISIBLE_IN_SILENT` — разрешить конкретному сообщению появиться даже в `SILENT`

`rational_reconvert(mode)` возвращает человеко-читаемую строку с именами флагов режима, например `REGULAR | VERBOSE`. `rational_convert(NAME)` — простой macro, который превращает имя другого macro в строку.

```c
rational_logger_mode = REGULAR | VERBOSE;
slog(REGULAR,"Started\n");
slog(VERBOSE,"Detailed value: %d\n",value);
```

## Слои статусов

### Технический слой

Технический слой говорит о том, что произошло с самой функцией.

| Флаг | Значение |
|---|---|
| `SUCCESS` | функция завершилась без внутренней ошибки |
| `FAILURE` | внутри функции произошла техническая ошибка: нехватка памяти, повреждённое внутреннее состояние, невозможность продолжать работу |
| `WARNING` | операция завершилась, но есть важная проблема, о которой должен узнать вызывающий код |
| `DONOTHING` | действие осознанно пропущено и это не ошибка |
| `CRITICAL` | маска проблемных технических флагов: `WARNING | FAILURE` |
| `TRIUMPH` | маска штатных и управляемых исходов: `SUCCESS | HALTED | DONOTHING | INFO` |

`FAILURE` стоит использовать только для проблем, которые не являются обычной логикой приложения. Например: не удалось выделить память, нарушено ожидаемое состояние внутренней структуры, библиотечная функция получила некорректный дескриптор.

Статус может содержать несколько флагов сразу, поэтому его обычно проверяют через битовые маски.

```c
if(CRITICAL & status)
{
	/* Technical problem: WARNING or FAILURE is present */
}

if(WARNING & status)
{
	/* WARNING is present, even if other flags are present too */
}
```

### Бинарный слой

Бинарный слой нужен для ответов в стиле «да/нет», но без смешивания с техническим успехом или техническим сбоем.

| Флаг | Значение |
|---|---|
| `YES` | локальный ответ функции-проверки: да, условие выполнено |
| `NO` | локальный ответ функции-проверки: нет, условие не выполнено |
| `BOOLEAN` | маска бинарных флагов: `YES | NO` |

Важно: `YES` и `NO` — это не значения C-типа `bool`, а битовые флаги внутри `Return`. Поэтому `NO` не равен `0`, и код вида `if(path_is_readable(path))` не является правильным способом прочитать ответ.

Пример: функция проверяет, доступен ли путь для чтения. Если проверка выполнена штатно и путь доступен, она может вернуть `SUCCESS | YES`. Если проверка выполнена штатно, но путь недоступен, она может вернуть `SUCCESS | NO`.

Простое правило: `YES` и `NO` хорошо подходят функциям, которые по смыслу отвечают на вопрос. Например: `is_*`, `has_*`, `can_*`, `check_*`, `validate_*`. Такой ответ читается через `ask(...)`, который возвращает обычный C-результат `true` или `false`.

`YES` и `NO` не предназначены для автоматического наследования через цепочку обычных функций. Возвращайте их выше только тогда, когда текущая функция сама тоже обещает ответить «да» или «нет».

### Глобальный слой

Глобальный слой используется для состояния процесса, которое может быть установлено вне текущей функции. Например, обработчик сигнала может установить `global_return_status` в `HALTED`, чтобы последующие возвраты знали, что программа должна остановиться.

| Флаг | Значение |
|---|---|
| `INFO` | информационный результат. Например, приложение вывело `--help`, `--version` или другую справку и штатно завершило работу |
| `WARNING` | предупреждение, которое должно быть видно за пределами одной функции |
| `HALTED` | процесс остановлен или должен остановиться. Например, пользователь нажал Ctrl+C, и обработчик сигнала попросил завершить работу |
| `GLOBAL` | маска флагов, которым разрешено переходить из `global_return_status` в обычный возврат |

Из `global_return_status` в результат функции попадают только биты `GLOBAL`. Бинарные ответы `YES` и `NO` не протекают из глобального статуса между функциями.

## Базовая функция

Обычная функция создаёт локальный `status`, меняет его по ходу работы и завершает выполнение через `provide(status)`.

```c
#include "rational.h"

Return load_settings(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Some internal operation failed.
	   This is a technical failure, not a normal business result */
	if(settings_storage_is_broken())
	{
		status = FAILURE;
	}

	provide(status);
}
```

`provide()` делает три вещи:

* нормализует локальный статус;
* учитывает разрешённые глобальные флаги из `global_return_status`;
* пишет TRACE-лог, если итоговый статус содержит `CRITICAL`.

Если TRACE-лог на выходе не нужен, используется `deliver(status)`.

```c
Return quiet_cleanup(void)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Cleanup may be called from paths where extra TRACE output is noisy */
	release_temporary_buffers();

	deliver(status);
}
```

## Функция-проверка с YES и NO

Бинарные флаги удобны для функций, которые проверяют условие. В этом примере отсутствие доступа к файлу не считается внутренним сбоем функции. Функция смогла выполнить проверку, поэтому технический слой остаётся `SUCCESS`.

```c
#include <unistd.h>

#include "rational.h"

Return path_is_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(path == NULL)
	{
		/* NULL input is a caller or program error.
		   The function cannot perform a meaningful check */
		status = FAILURE;
	}

	/* Run the logical check only while the technical status is still successful */
	if(TRIUMPH & status)
	{
		if(access(path,R_OK) == 0)
		{
			/* The check ran normally and the answer is positive */
			status |= YES;

		} else {
			/* The check ran normally and the answer is negative */
			status |= NO;
		}
	}

	provide(status);
}
```

Здесь важна форма `if(TRIUMPH & status)`. Она позволяет сначала выставить технический статус, а потом выполнять следующий шаг только если предыдущие шаги не сломались. Такой стиль помогает избежать двух частых проблем:

* каскадной вложенности `if` внутри `if`, когда с каждым уровнем отступов код становится менее читаемым;
* необходимости использовать `goto` для перехода к общей очистке ресурсов в конце функции.

Подробный разбор этого приёма есть ниже в разделе [«Последовательные проверки через status»](#последовательные-проверки-через-status).

## Как правильно разбирать результат

### Что возвращает функция-проверка

Функция-проверка возвращает обычный `Return`. Если сама проверка выполнена без технического сбоя, к `SUCCESS` добавляется ответ `YES` или `NO`.

В примере ниже `path_is_readable()` возвращает `YES` или `NO`, потому что её смысл — ответить на вопрос «доступен ли файл для чтения?». Функция `print_file_if_readable()` уже не отвечает на такой вопрос. Она только решает, печатать файл или нет, поэтому не возвращает наследованные `YES` или `NO` дальше.

Главное правило: не воспринимайте `NO` как технический сбой. Это отрицательный логический ответ. Если функция-проверка вернула техническую ошибку, `ask(...)` добавит эту ошибку в локальный `status` и вернёт `false`. Если функция-проверка вернула штатный `NO`, `ask(...)` тоже вернёт `false`, но локальный `status` останется технически успешным.

### Что делает ask(...)

Функции-проверки вызываются через `ask(...)`. Макрос `ask(...)` принимает выражение, которое возвращает `Return`, проверяет техническую часть результата, разбирает ответ `YES` или `NO`, очищает бинарные флаги из локального `status` и возвращает обычный C-результат `true` или `false`.

Для `ask(...)` в текущей области видимости должен быть локальный `Return status`. Именно в него `ask(...)` переносит техническую часть возврата функции-проверки.

Поведение `ask(...)` можно читать так:

* если функция-проверка вернула штатный `YES`, `ask(...)` возвращает `true`, а локальный `status` остаётся технически успешным;
* если функция-проверка вернула штатный `NO`, `ask(...)` возвращает `false`, а локальный `status` тоже остаётся технически успешным;
* если функция-проверка вернула техническую ошибку, `ask(...)` возвращает `false` и добавляет эту ошибку в локальный `status`;
* после `ask(...)` бинарный ответ считается обработанным и больше не должен переходить дальше в обычные `run()`, `call()`, `provide()` или `deliver()`.

`ask(...)` предназначен для настоящего возврата функции-проверки, а не для вручную собранной маски в вызывающем коде. Правильная функция-проверка сама выставляет `YES` или `NO` и выходит через `provide(status)` или `deliver(status)`.

В примере впервые используется `run(...)`. Коротко: `run(print_file(path))` вызывает `print_file(path)`, подмешивает её `Return` в локальный `status` и нормализует результат. Если `print_file()` вернёт техническую ошибку, текущий `status` тоже станет проблемным. Подробное описание есть ниже в разделе [«Цепочки вызовов: run() и call()»](#цепочки-вызовов-run-и-call).

### Допустимые формы вызова

#### Пример 1: прямой вызов функции-проверки

```c
Return print_file_if_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(ask(path_is_readable(path)))
	{
		/* path_is_readable() returned a technical success and YES.
		   ask() consumed YES and returned true.
		   The local status no longer carries the binary answer */

		/* print_file() is a regular Return function.
		   run() may merge its technical result into local status */
		run(print_file(path));

	} else {
		/* ask() returned false in one of two cases.
		   Case 1: path_is_readable() returned SUCCESS | NO.
		   The file is not readable, but local status is still technically successful.
		   Case 2: path_is_readable() returned FAILURE.
		   The local status now contains the technical failure */
	}

	provide(status);
}
```

#### Пример 2: отдельная переменная для результата проверки

`ask(...)` можно использовать с уже сохранённым `Return`, если это удобнее для читаемости или отладки. В этом варианте результат функции-проверки сохраняется отдельно, а локальный `status` получает техническую часть только в момент вызова `ask(readable)`.

```c
Return print_file_if_readable_later(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	Return readable = path_is_readable(path);

	/* readable contains the full Return from the check function.
	   ask(readable) consumes the YES or NO answer.
	   Technical FAILURE, if present, is copied into local status */
	if(ask(readable))
	{
		/* The answer was YES and the technical layer was successful */
		run(print_file(path));
	}

	provide(status);
}
```

Если результат сохранён в отдельную переменную, его нужно разобрать рядом с вызовом. Язык C не даёт библиотеке автоматически понять, что отдельная локальная переменная была забыта и больше никогда не будет использована.

#### Пример 3: разбор ответа, уже лежащего в status

Иногда удобно сначала записать полный возврат функции-проверки прямо в локальный `status`, а потом разобрать его через `ask(status)`. Это допустимо, но важно понимать разницу: присваивание `status = path_is_readable(path)` заменяет всё, что было в `status` раньше. Такой стиль подходит только тогда, когда это именно желаемое поведение.

```c
Return print_file_if_readable_resetting_status(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* This assignment replaces the previous local status.
	   After the call, status temporarily contains both the technical layer
	   and the binary YES or NO answer from path_is_readable() */
	status = path_is_readable(path);

	if(ask(status))
	{
		/* ask(status) consumed YES and left only the technical layer.
		   The condition is true only for a technically successful YES */
		run(print_file(path));

	} else {
		/* For SUCCESS | NO, ask(status) returns false and leaves status successful.
		   For FAILURE, ask(status) returns false and leaves status critical */
	}

	provide(status);
}
```

### Что считается ошибкой

`run(...)` и `call(...)` не предназначены для функций-проверок с `YES` или `NO`. Если случайно написать `run(path_is_readable(path))`, библиотека выведет ошибку и переведёт локальный `status` в `FAILURE`. То же произойдёт, если функция получила бинарный ответ в локальный `status` и попыталась выйти через `provide(status)` или `deliver(status)`, не обработав этот ответ через `ask(...)`.

Правильный алгоритм выглядит так:

```c
if(ask(path_is_readable(path)))
{
	run(print_file(path));
}
```

Неправильный алгоритм выглядит так:

```c
run(path_is_readable(path));
```

Во втором случае функция-проверка вернула не обычный рабочий статус, а ответ на вопрос «да» или «нет». `run()` не умеет принимать такие ответы, потому что иначе `YES` или `NO` начали бы случайно наследоваться по цепочке обычных вызовов. Поэтому библиотека сообщает об ошибке и переводит локальный `status` в `FAILURE`.

Если результат функции-проверки нужно намеренно полностью отбросить, можно использовать явное приведение к `void`.

```c
(void)path_is_readable(path);
```

Такой код сознательно игнорирует весь `Return`: и техническую часть, и ответ `YES` или `NO`. Библиотека не будет пытаться восстановить уже отброшенный результат.

## Последовательные проверки через status

Один и тот же алгоритм можно написать несколькими способами. Допустим, функция должна скопировать две строки через `strdup()`. Сама `strdup()` выделяет память и возвращает `NULL` при сбое. Если первая строка уже скопирована, а вторая не скопировалась, первую копию нужно освободить перед выходом.

| Подход | Что получается |
|---|---|
| вложенные `if` | работает, но код быстро уходит вправо и становится труднее для чтения |
| `goto cleanup` | решает общую очистку, но добавляет запрещённый стиль переходов |
| последовательный `status` | оставляет код плоским, читабельным и с понятной финальной очисткой |

Первый вариант рабочий, но быстро превращается в лестницу вложенных условий. Чем больше шагов, тем глубже код уезжает вправо.

```c
Return copy_pair_nested(char **first_out, char **second_out, const char *first, const char *second)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *first_copy = NULL;
	char *second_copy = NULL;

	if((first_out == NULL) || (second_out == NULL) || (first == NULL) || (second == NULL))
	{
		status = FAILURE;

	} else {
		first_copy = strdup(first);

		if(first_copy == NULL)
		{
			status = FAILURE;

		} else {
			second_copy = strdup(second);

			if(second_copy == NULL)
			{
				free(first_copy);
				status = FAILURE;

			} else {
				*first_out = first_copy;
				*second_out = second_copy;
			}
		}
	}

	provide(status);
}
```

Второй вариант часто встречается в C-коде: все сбои прыгают в общий блок очистки через `goto`. Это решает проблему освобождения памяти, но добавляет отдельный механизм переходов. В современном коде любых проектов `goto` считается плохой практикой и должен быть полностью исключён. Именно поэтому в `librational` используется альтернативный механизм последовательных проверок через `status`.

```c
Return copy_pair_goto(char **first_out, char **second_out, const char *first, const char *second)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *first_copy = NULL;
	char *second_copy = NULL;

	if((first_out == NULL) || (second_out == NULL) || (first == NULL) || (second == NULL))
	{
		status = FAILURE;
		goto cleanup;
	}

	first_copy = strdup(first);
	if(first_copy == NULL)
	{
		status = FAILURE;
		goto cleanup;
	}

	second_copy = strdup(second);
	if(second_copy == NULL)
	{
		status = FAILURE;
		goto cleanup;
	}

	*first_out = first_copy;
	*second_out = second_copy;
	first_copy = NULL;
	second_copy = NULL;

cleanup:
	free(second_copy);
	free(first_copy);
	provide(status);
}
```

Третий вариант использует тот же принцип, что и `path_is_readable()`: каждый следующий шаг запускается только если текущий `status` всё ещё содержит штатный технический результат. Временные указатели всегда очищаются в конце. Если всё прошло успешно, владение памятью передаётся наружу, а локальные указатели обнуляются, чтобы `free()` ничего не удалил.

```c
Return copy_pair_status(char **first_out, char **second_out, const char *first, const char *second)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *first_copy = NULL;
	char *second_copy = NULL;

	if((first_out == NULL) || (second_out == NULL) || (first == NULL) || (second == NULL))
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		first_copy = strdup(first);

		if(first_copy == NULL)
		{
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		second_copy = strdup(second);

		if(second_copy == NULL)
		{
			status = FAILURE;
		}
	}

	if(TRIUMPH & status)
	{
		*first_out = first_copy;
		*second_out = second_copy;
		first_copy = NULL;
		second_copy = NULL;
	}

	free(second_copy);
	free(first_copy);
	provide(status);
}
```

Такой код остаётся плоским, читается сверху вниз и не требует отдельного аварийного выхода. Вся логика строится вокруг одного локального `status`: если шаг сломался, следующие рабочие шаги просто не выполняются, а финальная очистка всё равно находится в одном понятном месте.

## Цепочки вызовов: run() и call()

`run(func)` нужен для обычных рабочих шагов, которые имеют смысл только пока функция продолжает выполняться штатно. Его можно читать так: «если раньше не было ошибки, предупреждения, остановки или другого статуса, запрещающего продолжать цепочку, выполни следующий шаг».

После вызова `run()` добавляет результат возврата из `func` в локальный `status` и нормализует его. Поэтому следующая строка с `run()` уже увидит обновлённое состояние и при необходимости будет пропущена.

`call(func)` нужен для действий, которые должны выполниться в любом случае. Он НЕ проверяет локальный `status` перед запуском `func` и НЕ решает, можно ли продолжать рабочую цепочку, а просто выполняет `func`. Но после выполнения возврат `func` всё равно добавляется в локальный `status` и влияет на дальнейший итог работы вызвавшей функции. Поэтому `call()` удобно использовать для очистки: освободить память, закрыть файл, удалить временный объект, вывести финальное сообщение.

Коротко: `run()` — для работы, которую можно пропустить после сбоя. `call()` — для обязательной уборки и завершающих действий с безусловным запуском `func`.

Единственное обязательное условие: `func` должна возвращать `Return`. Эти макросы берут возвращённый статус, добавляют его в локальный `status` и нормализуют результат. К сожалению, их нельзя напрямую использовать с функциями, которые возвращают `void`, `bool`, `int` или любой другой тип. Для таких функций нужна небольшая обёртка, которая сама вернёт `Return`.

Функции-проверки, которые возвращают `YES` или `NO`, не вызываются через `run()` или `call()`. Для них используется `ask(...)`, потому что бинарный ответ должен быть обработан сразу рядом с вызовом. Если такой ответ случайно попадёт в `run()` или `call()`, библиотека сообщит об ошибке и выставит `FAILURE`.

```c
Return process_file(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Work steps.
	   open_input() runs first.
	   If it returns FAILURE, WARNING, INFO, or HALTED, local status receives that flag.
	   These flags are SKIP statuses, so later run() calls below will not execute */
	run(open_input(path));
	run(read_input());
	run(write_output());

	/* Cleanup steps.
	   call() does not check local status before running the function.
	   Even if status is already FAILURE or another non-SUCCESS value,
	   both close_input() and close_output() will still be executed */
	call(close_input());
	call(close_output());

	provide(status);
}
```

Если `open_input()` вернёт `FAILURE`, следующие рабочие шаги `read_input()` и `write_output()` уже не запустятся. Но `close_input()` и `close_output()` всё равно выполнятся, потому что они вызваны через `call()`.

## Глобальный статус

Обычный `Return status` описывает состояние одной конкретной функции. Когда функция завершилась, её локальный `status` тоже закончился.

`global_return_status` нужен для событий уровня всей программы. Это такие события, которые произошли не обязательно внутри текущей функции, но должны быть видны всем следующим возвратам. Типичный пример — пользователь нажал Ctrl+C. Обработчик сигнала не знает, какая функция сейчас выполняется, но может выставить общий флаг `HALTED`.

После этого любая функция, которая завершится через `provide()` или `deliver()`, получит этот глобальный контекст в своём возврате. `run()` и `call()` тоже увидят его после нормализации статуса.

Важно: из `global_return_status` в обычный возврат попадают только флаги из `GLOBAL`: `INFO`, `WARNING`, `HALTED`. Локальные бинарные ответы `YES` и `NO` через глобальный статус не распространяются.

Самая короткая форма выглядит так:

```c
atomic_store(&global_return_status,HALTED);
```

После этого ближайший возврат через `provide()` или `deliver()` получит `HALTED` как глобальный контекст.

```c
#include <signal.h>
#include <stdatomic.h>

#include "rational.h"

static size_t processed_items = 0;

void handle_sigint(int signal_number)
{
	(void)signal_number;

	/* Ctrl+C requests controlled shutdown for future returns */
	atomic_store(&global_return_status,HALTED);
}

Return install_sigint_handler(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct sigaction action = {
		.sa_handler = handle_sigint
	};

	/* SIGINT is the signal usually sent by Ctrl+C */
	if(sigemptyset(&action.sa_mask) == -1)
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		if(sigaction(SIGINT,&action,NULL) == -1)
		{
			status = FAILURE;
		}
	}

	provide(status);
}

Return process_one_item(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Do one small unit of work.
	   The counter makes the example function perform a visible state change */
	processed_items++;

	/* If Ctrl+C was pressed earlier, provide() will merge HALTED from global_return_status */

	provide(status);
}

Return process_items(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Register Ctrl+C handler before the work loop starts.
	   If handler setup fails, status becomes FAILURE and the loop will not run */
	run(install_sigint_handler());

	while(((SKIP & status) == 0) && items_left())
	{
		/* Before Ctrl+C: process_one_item() returns SUCCESS and the loop continues.
		   After Ctrl+C: the OS calls handle_sigint(), which stores HALTED in global_return_status.
		   The next run() calls process_one_item(), normalizes status, merges HALTED into local status,
		   and the next loop check stops early because HALTED is a SKIP status */
		run(process_one_item());
	}

	/* Cleanup still runs even after HALTED was added to local status */
	call(close_items());

	provide(status);
}
```

## Технические подробности

Этот раздел описывает внутренние механизмы librational. В обычном пользовательском коде они скрыты за `provide()`, `deliver()`, `run()`, `call()` и `ask(...)`, поэтому напрямую их использовать не нужно.

Технические подробности оставлены в README для тех случаев, когда нужно доработать саму библиотеку, проверить её поведение или глубже разобраться в причинах конкретного возврата.

### Нормализация

Перед выходом из функции статус проходит через внутреннюю нормализацию. Нормализация убирает противоречивые комбинации:

* если установлен `CRITICAL`, удаляется `SUCCESS`;
* если установлен `NO`, удаляется `YES`;
* `global_return_status` также нормализуется и сохраняется обратно;
* после подмешивания `GLOBAL` статус нормализуется ещё раз.

Это значит, что код может случайно собрать `SUCCESS | FAILURE`, но наружу такая комбинация не должна выйти как «и успех, и сбой». Проблемный технический флаг сильнее `SUCCESS`.

То же правило действует для бинарного слоя: если где-то получилось `YES | NO`, остаётся `NO`. Если хотя бы одна проверка ответила «нет», итоговый бинарный ответ уже не может быть «да».

### Внутренний алгоритм YES/NO

Пользовательский код обычно видит только `YES`, `NO` и `ask(...)`. Внутри библиотека добавляет ещё один служебный шаг: она помечает бинарный ответ как ожидающий обработки. Для этого используется внутренний флаг `AWAITING`.

Этот флаг не нужен в обычном коде приложения. Он нужен самой библиотеке, чтобы отличить два разных состояния:

* функция-проверка только что вернула ответ `YES` или `NO`, и этот ответ ещё нужно разобрать через `ask(...)`;
* вызывающий код уже разобрал ответ, и дальше по цепочке должен идти только обычный технический `status`.

Алгоритм выглядит так:

```text
функция-проверка -> SUCCESS | YES или SUCCESS | NO
provide()/deliver() -> помечает ответ как ожидающий ask(...)
ask(...) -> читает YES/NO, возвращает true/false, очищает бинарные флаги
обычный код -> продолжает работу только с техническим status
```

Например, функция `path_is_readable()` из раздела [«Функция-проверка с YES и NO»](#функция-проверка-с-yes-и-no) сама выставляет `YES` или `NO`. Когда она выходит через `provide(status)`, библиотека понимает: это бинарный ответ функции-проверки, его нельзя случайно смешать с обычной цепочкой вызовов.

После этого вызывающий код должен сделать так:

```c
if(ask(path_is_readable(path)))
{
	run(print_file(path));
}
```

В этом варианте `ask(...)` выполняет всю обработку:

* проверяет, что функция действительно вернула ожидающий бинарный ответ;
* смотрит, какой ответ пришёл: `YES` или `NO`;
* возвращает обычный C-результат `true` или `false`;
* переносит технические ошибки в локальный `status`;
* очищает бинарные флаги, чтобы они не наследовались дальше.

Если `path_is_readable(path)` вернула `SUCCESS | YES`, условие будет истинным. Если она вернула `SUCCESS | NO`, условие будет ложным, но локальный `status` останется успешным. Если она вернула `FAILURE`, условие тоже будет ложным, а локальный `status` станет критическим.

Защитный механизм нужен для случаев, когда алгоритм построен неправильно. Например:

```c
run(path_is_readable(path));
```

Такой код пытается передать бинарный ответ в `run()`, хотя `run()` предназначен для обычных рабочих шагов. В этом случае библиотека сообщает об ошибке и переводит локальный `status` в `FAILURE`. То же правило действует для `call()`.

Ещё один защищаемый случай — выход из функции с необработанным бинарным ответом:

```c
status = path_is_readable(path);

provide(status);
```

Здесь функция получила ответ `YES` или `NO`, но не вызвала `ask(status)`. Поэтому `provide(status)` не выпускает такой результат как обычный возврат, сообщает об ошибке и возвращает `FAILURE`.

Правильный вариант для такого стиля показан в [третьем примере разбора результата](#пример-3-разбор-ответа-уже-лежащего-в-status):

```c
status = path_is_readable(path);

if(ask(status))
{
	run(print_file(path));
}
```

Явное приведение к `void` — отдельный случай:

```c
(void)path_is_readable(path);
```

Такой код намеренно отбрасывает весь возврат функции: и техническую часть, и `YES` или `NO`. После такого вызова библиотека уже не видит результат и не пытается его восстанавливать. Это ожидаемое поведение для случая, когда программист действительно хочет полностью игнорировать ответ.

## Практические правила

1. Обычная функция начинает работу с `Return status = SUCCESS`.
2. Возврат из функции выполняется через `provide(status)` или `deliver(status)`.
3. `FAILURE` используется только для технических ошибок внутри функции.
4. `YES` и `NO` используются только для локального логического ответа функции-проверки.
5. `YES` и `NO` возвращаются наружу только тогда, когда сама текущая функция по смыслу является проверкой и должна ответить «да» или «нет». Это локальные флаги, которые не нужно автоматически наследовать по цепочке из одной функции в другую.
6. В вызывающем коде функции-проверки разбираются через `ask(...)`.
7. `run()` используется для рабочих шагов, которые можно пропустить после ошибки или остановки.
8. `call()` используется для очистки и обязательных финальных действий.
9. `run()` и `call()` не используются для функций-проверок, которые возвращают `YES` или `NO`.
10. Если статус может содержать несколько битов, точное сравнение со значением одного флага не используется. Для составных возвратов применяются битовые маски.
