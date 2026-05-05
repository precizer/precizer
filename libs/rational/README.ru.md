[<img src="../../.html/img/i18n-icon.svg"> Link to the English language README page](README.md)

# librational — единые статусы возврата для C-кода

`librational` — небольшая внутренняя библиотека с общими флагами, макросами возврата, логированием и вспомогательными функциями. Самая заметная часть библиотеки — тип `Return`, который позволяет функции вернуть не один плоский код, а несколько независимых признаков в одном значении.

Такой возврат удобен, когда нужно отдельно понимать:

* была ли техническая ошибка внутри функции;
* успешно ли функция завершила свою работу;
* какой логический ответ функция получила, если она что-то проверяла;
* есть ли глобальный контекст, который должен повлиять на последующие возвраты.

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

### Бинарный слой

Бинарный слой нужен для ответов в стиле «да/нет», но без смешивания с техническим успехом или техническим сбоем.

| Флаг | Значение |
|---|---|
| `YES` | локальный ответ функции-проверки: да, условие выполнено |
| `NO` | локальный ответ функции-проверки: нет, условие не выполнено |
| `BOOLEAN` | маска бинарных флагов: `YES | NO` |

Важно: `YES` и `NO` — это не значения C-типа `bool`, а битовые флаги внутри `Return`. Поэтому `NO` не равен `0`. Его можно проверять через маску точно так же, как остальные флаги: `if(NO & status)`.

Пример: функция проверяет, доступен ли путь для чтения. Если проверка выполнена штатно и путь доступен, она может вернуть `SUCCESS | YES`. Если проверка выполнена штатно, но путь недоступен, она может вернуть `SUCCESS | NO`.

Простое правило: `YES` и `NO` хорошо подходят функциям, которые по смыслу отвечают на вопрос. Например: `is_*`, `has_*`, `can_*`, `check_*`, `validate_*`. Вызывающий код обычно разбирает такой ответ сразу после вызова. Если текущая функция уже решила, что делать дальше, бинарный ответ считается обработанным. Возвращайте `YES` или `NO` выше только тогда, когда текущая функция сама тоже обещает ответить «да» или «нет».

### Глобальный слой

Глобальный слой используется для состояния процесса, которое может быть установлено вне текущей функции. Например, обработчик сигнала может установить `global_return_status` в `HALTED`, чтобы последующие возвраты знали, что программа должна остановиться.

| Флаг | Значение |
|---|---|
| `INFO` | информационный результат. Например, приложение вывело `--help`, `--version` или другую справку и штатно завершило работу |
| `WARNING` | предупреждение, которое должно быть видно за пределами одной функции |
| `HALTED` | процесс остановлен или должен остановиться. Например, пользователь нажал Ctrl+C, и обработчик сигнала попросил завершить работу |
| `GLOBAL` | маска флагов, которым разрешено переходить из `global_return_status` в обычный возврат |

Из `global_return_status` в результат функции попадают только биты `GLOBAL`. Бинарные ответы `YES` и `NO` не протекают из глобального статуса между функциями.

## Нормализация

Перед возвратом статус проходит через `normalize_return_status()`. Нормализация убирает противоречивые комбинации:

* если установлен `CRITICAL`, удаляется `SUCCESS`;
* если установлен `NO`, удаляется `YES`;
* `global_return_status` также нормализуется и сохраняется обратно;
* после подмешивания `GLOBAL` статус нормализуется ещё раз.

Это значит, что код может случайно собрать `SUCCESS | FAILURE`, но наружу такая комбинация не должна выйти как «и успех, и сбой». Проблемный технический флаг сильнее `SUCCESS`.

То же правило действует для бинарного слоя: если где-то получилось `YES | NO`, остаётся `NO`. Если хотя бы одна проверка ответила «нет», итоговый бинарный ответ уже не может быть «да».

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

Сначала проверяйте технический слой. Только после этого разбирайте бинарный ответ. В примере ниже `path_is_readable()` возвращает `YES` или `NO`, потому что её смысл — ответить на вопрос «доступен ли файл для чтения?». Функция `print_file_if_readable()` уже не отвечает на такой вопрос. Она только решает, печатать файл или нет, поэтому не возвращает наследованные `YES` или `NO` дальше.

В примере впервые используется `run(...)`. Коротко: `run(print_file(path))` вызывает `print_file(path)`, подмешивает её `Return` в локальный `status` и нормализует результат. Если `print_file()` вернёт техническую ошибку, текущий `status` тоже станет проблемным. Подробное описание есть ниже в разделе [«Цепочки вызовов: run() и call()»](#цепочки-вызовов-run-и-call).

```c
Return print_file_if_readable(const char *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	Return readable = path_is_readable(path);

	if(CRITICAL & readable)
	{
		/* The check itself failed.
		   This function cannot continue either */
		provide(FAILURE);
	}

	if(YES & readable)
	{
		/* YES is used only as a local decision.
		   The file is readable, so this function may print it */
		/* run() merges print_file() return status into local status */
		run(print_file(path));

	} else {
		/* In this example NO is not interesting as a returned answer.
		   It only means there is nothing to print */
	}

	provide(status);
}
```

Для короткой проверки технического успеха можно использовать `TRIUMPH`. Логика остаётся такой же: технический сбой превращается в `FAILURE` текущей функции, а `YES` используется только для локального решения.

```c
Return result = path_is_readable(path);

if((TRIUMPH & result) == 0)
{
	/* There is no successful or graceful technical outcome */
	return(FAILURE);
}

if(YES & result)
{
	/* The condition is true, so the caller may do the useful work */
	return(print_file(path));
}

/* NO is handled here as "nothing to do", not as a returned answer */
return(SUCCESS);
```

Главное правило: не воспринимайте `NO` как технический сбой. Это отрицательный логический ответ. Технический сбой проверяется через `CRITICAL`, `FAILURE`, `WARNING` или отсутствие ожидаемого `TRIUMPH`. Не протаскивайте `YES` и `NO` по цепочке автоматически. Если функция получила бинарный ответ и на его основании уже выбрала действие, для этой функции бинарный ответ обработан.

## Последовательные проверки через status

Один и тот же алгоритм можно написать несколькими способами. Допустим, функция должна скопировать две строки через `strdup()`. Сама `strdup()` выделяет память и возвращает `NULL` при сбое. Если первая строка уже скопирована, а вторая не скопировалась, первую копию нужно освободить перед выходом.

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

## Практические правила

1. Обычная функция начинает работу с `Return status = SUCCESS`.
2. Возврат из функции выполняется через `provide(status)` или `deliver(status)`.
3. `FAILURE` используется только для технических ошибок внутри функции.
4. `YES` и `NO` используются только для локального логического ответа функции-проверки.
5. `YES` и `NO` возвращаются наружу только тогда, когда сама текущая функция по смыслу является проверкой и должна ответить «да» или «нет». Это локальные флаги, которые не нужно автоматически наследовать по цепочке из одной функции в другую.
6. В вызывающем коде сначала анализируется технический слой, а потом бинарный ответ.
7. `run()` используется для рабочих шагов, которые можно пропустить после ошибки или остановки.
8. `call()` используется для очистки и обязательных финальных действий.
9. Если статус может содержать несколько битов, точное сравнение со значением одного флага не используется. Для составных возвратов применяются битовые маски.
