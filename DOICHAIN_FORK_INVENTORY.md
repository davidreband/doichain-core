# Инвентаризация изменений Doichain относительно Namecoin Core

Документ составлен по факту анализа git-истории и диффов репозитория (дата анализа: 2026-08-07).
Все утверждения проверены по коду; ссылки на файлы/строки указывают на ветку `doichain-nc29-migration`,
если не сказано иное.

---

## 1. Топология репозитория

| Ветка / объект | Что это | База (upstream) | Своих коммитов | Диффстат |
|---|---|---|---|---|
| `master` (`b74273e682`, 2022-11-30) | **Старый (production) Doichain**, v0.20.1.11 | Namecoin `3e69dd1013` (2020-09-21, эпоха Bitcoin Core 0.20 / nc0.21) | **120** | 179 файлов, +8197 / −1284 |
| `upstream-29.x` | указывает на тот же коммит, что `master` — **имя вводит в заблуждение**, никакого upstream 29.x там нет | — | — | — |
| `patches/` (120 файлов) | Те же 120 коммитов `master`, выгруженные через `git format-patch` | — | — | — |
| `doichain-current-backup` = `doichain-updated-from-upstream` (`0025bf7677`) | Попытка мёрджа upstream в старый Doichain, брошена | — | — | — |
| `updated-from-upstream` | Чистая копия namecoin master | — | — | — |
| **`doichain-nc29-migration`** (текущая) | **Новая миграция**, v29.20.1.11 | Namecoin **`upstream/29.x` @ `d00111caac`** (2025-08-18) | **47** | 151 файл, +1243 / −893 |

> Состояние upstream на 2026-08-07: актуальный релиз namecoin — **nc31.1** (2026-07-13,
> Bitcoin Core 31.1), ветка `29.x` закрыта на nc29.1 (2025-09-08). Текущая ветка отстаёт
> на 4325 коммитов от nc31.1. Обоснование смены цели миграции — `DOICHAIN_UPGRADE_STRATEGY.md` §1.0.

Ключевой факт: `doichain-nc29-migration` — это **не** продолжение `master`. Она форкнута
напрямую от namecoin 29.x, и изменения Doichain на неё перенесены заново, вручную.
Общий предок с `master` — точка форка 2020 года. То есть «потерять изменения» здесь можно
только одним способом: не заметить что-то из 120 старых коммитов при переносе.

---

## 2. Слои изменений Doichain

### Слой A. Ребрендинг (механический, ~85 % объёма диффа)

Namecoin → Doichain, NMC → DOI, порты, имена бинарников, GUI, иконки, локали, man-страницы,
`namecoind.pid` → `doichaind.pid`, `namecoinconsensus.*` → `doichainconsensus.*`.

Идентификаторы кода, которые были переименованы (важно — это ломает мёрдж с upstream **везде**):

| Namecoin | Doichain | Где |
|---|---|---|
| `CTransaction::IsNamecoin()` | `IsDoichain()` | [transaction.h:365](src/primitives/transaction.h#L365) |
| `CMutableTransaction::SetNamecoin()` | `SetDoichain()` | [transaction.h:436](src/primitives/transaction.h#L436) |
| `NAMECOIN_VERSION` (`0x7100`) | `DOICHAIN_VERSION` (значение **то же**) | [transaction.h:303](src/primitives/transaction.h#L303) |
| `TxAllowedForNamecoin()` | `TxAllowedForDoichain()` | node/miner |

Статус: перенесено в nc29-ветку полностью (коммиты `97c877992f`, `940fe763fe`, `4f946f677e`, `7abf686271`, `6ed3581c65` и др.).

> **Замечание по стратегии:** переименование `IsNamecoin`→`IsDoichain` даёт ~40 точек конфликта
> при каждом обновлении upstream и **не несёт функциональной ценности**. См. стратегию, п. 3.

### Слой B. Сетевые и консенсус-параметры

Файл: `src/kernel/chainparams.cpp` (в старом — `src/chainparams.cpp`).

| Параметр | Namecoin | Doichain | Статус в nc29 |
|---|---|---|---|
| Genesis mainnet | `000000000062b72c…` | `000006fdd8b4d786fd9bdde5bae9486c464e3aa4336c5f8415dfdd3fc1679134` | ✅ [chainparams.cpp:176](src/kernel/chainparams.cpp#L176) |
| Genesis timestamp | «…choose what comes next…» | «USA überraschen Europa mit einem Gesetz zur Online-Durchsuchung» | ✅ |
| Genesis testnet | Bitcoin-строка | «Mark Zuckerberg und Apple-Chef Tim Cook…», hash `0000cd7572b3…` | ✅ |
| `nDefaultPort` | 8334 / 18334 | **8338** / **18338** | ✅ |
| `pchMessageStart` | `f9 be b4 fe` | **`f8 b2 b2 ff`** / testnet `fc ba b2 fb` | ✅ |
| `powLimit` mainnet | `00000000ffff…` | **`0000ffff…`** (в 65536× легче) | ✅ |
| `nAuxpowChainId` | 0x0001 | **0x0002** (testnet 0x0003) | ✅ |
| `nAuxpowStartHeight` | 19200 | **1** | ✅ |
| `fStrictChainId` | true | **false** | ✅ |
| `nLegacyBlocksBefore` | 19200 | **1** | ✅ |
| `BIP16Height` | 475000 | **0** | ✅ |
| `BIP34Height` | 250000 | **100000000** (де-факто выключен) | ✅ |
| `BIP65/66Height` | 335000/250000 | **130000** | ✅ |
| `CSV/SegwitHeight` | 475000 | **216500** | ✅ |
| base58 префиксы | 52 / 13 / 180 | **те же** (совпадение с Namecoin) | ✅ |
| `bech32_hrp` | `nc` | **`dc`** | ✅ |
| DNS seeds | nmc.seed.quisquis.de … | **dnsseed.doichain.org**, **seed.doi.works** | ✅ |
| `nMinimumChainWork` | реальный | **`…0001`** — проверка отключена, реальное значение закомментировано на [строке 146](src/kernel/chainparams.cpp#L146) | ⚠️ временно отключено |
| `mapHistoricBugs` | заполнена багами Namecoin | **пуста** (`assert(mapHistoricBugs.empty())`) | ✅ (в nc29 сделано аккуратнее, чем в старом `master`, где механизм был вырезан целиком) |

Константы имён (`MAX_NAME_LENGTH=255`, `MAX_VALUE_LENGTH=1023`, `NAME_LOCKED_AMOUNT=0.01`,
`MIN_FIRSTUPDATE_DEPTH=12`) — **не менялись** относительно Namecoin.

### Слой C. Протокол: операция `OP_NAME_DOI`

Ядро форка. `OP_NAME_DOI = OP_10` (в обоих версиях: [script.h:99](src/script/script.h#L99)).
Скрипт: `OP_NAME_DOI <name> <value> OP_2DROP OP_DROP <addr>` — 2 аргумента, как `NAME_UPDATE`.

| Файл | Что добавлено | Статус в nc29 |
|---|---|---|
| `src/script/script.h` | опкод `OP_NAME_DOI=OP_10` | ✅ |
| `src/script/names.h` | `isDoiRegistration()`, DOI в `isNameOp/isAnyUpdate/getOpName/getOpValue`, объявление `buildNameDOI` | ✅ [names.h](src/script/names.h) |
| `src/script/names.cpp` | парсинг (2 аргумента), `GetPrefix()`, `buildNameDOI()` | ✅ (в nc29 **чище**, чем в старом: использует `GetPrefix()`, а не дублирует код) |
| `src/rpc/names.cpp` | `name_doi` в `name_pending` и `namerawtransaction` | ✅ |
| `src/wallet/rpc/walletnames.cpp` | RPC-команда **`name_doi`** ([:1098](src/wallet/rpc/walletnames.cpp#L1098)) | ✅ |
| `src/wallet/rpc/wallet.cpp`, `src/rpc/client.cpp` | регистрация команды | ✅ |
| `src/names/main.cpp` | ветка `OP_NAME_DOI` в `CheckNameTransaction` | ⚠️ перенесено с проблемами (см. §4) |
| `src/txdb.cpp` | `ValidateNameDB`: DOI-имя может встречаться в UTXO-сете **несколько раз** | ✅ [txdb.cpp:385](src/txdb.cpp#L385) |
| `src/validation.cpp` | `MemPoolAccept::PreChecks` учитывает DOI | ✅ |
| **`src/core_write.cpp`** | `NameOpToUniv`: вывод `"op":"name_doi"` | ❌ **НЕ ПЕРЕНЕСЕНО — блокер, см. §4.1** |

**Семантика DOI (реконструирована из кода, нигде формально не описана):**

* DOI-имя пишется в ту же name-БД, что и обычные имена (`isAnyUpdate()` возвращает `true` для `OP_NAME_DOI`),
  подчиняется тем же правилам истечения.
* Имена, начинающиеся с `d/`, **запрещены** для `name_doi` — это разделение доменного
  namespace Namecoin и DOI-namespace ([names/mempool.cpp:439](src/names/mempool.cpp#L439)).
* `name_doi` **без** name-входа = регистрация. Разрешена даже если имя уже существует,
  но только до высоты **170000** (хардкод в консенсусе).
* `name_doi` **с** name-входом = обновление; вход обязан быть тоже `OP_NAME_DOI`.
* Одно и то же DOI-имя может иметь несколько UTXO — поэтому в `ValidateNameDB` снята
  проверка дублей.

### Слой D. Мемпул

| Изменение | Файл | Статус |
|---|---|---|
| `mapNameDois` — `map<valtype, set<Txid>>`, отдельный индекс DOI | [names/mempool.h:81](src/names/mempool.h#L81) | ✅ |
| `registersDoi()` | [names/mempool.h:123](src/names/mempool.h#L123) | ✅ |
| `isNameDoi()` в записи мемпула | [kernel/mempool_entry.h:222](src/kernel/mempool_entry.h#L222) (в старом — `txmempool.h`) | ✅ |
| `addUnchecked` / `remove` / `clear` / `check` / `pendingChainLength` / `lastNameOutput` для DOI | `names/mempool.cpp` | ✅ |
| `checkTx`: запрет `d/`-имён в `name_doi` | [names/mempool.cpp:439](src/names/mempool.cpp#L439) | ✅ |
| `DEFAULT_NAME_CHAIN_LIMIT` 1 → **2** | [names/mempool.h:31](src/names/mempool.h#L31) | ✅ |
| `removeConflicts` для DOI | — | ❌ **не реализовано** (открытый TODO ещё со старой версии) |

### Слой E. Ослабленные / отключённые проверки консенсуса

Это самый рискованный слой. Все пункты присутствуют **и в старом `master`, и в текущей ветке**.

| # | Что отключено | Где сейчас | Риск |
|---|---|---|---|
| E1 | **Проверка сложности PoW** `block.nBits != GetNextWorkRequired(...)` закомментирована | [validation.cpp:4335](src/validation.cpp#L4335) | **Проверено 2026-08-07 — не является правилом сети, подлежит восстановлению.** См. ниже |
| E2 | `nMinimumChainWork` = 1 | [chainparams.cpp:150](src/kernel/chainparams.cpp#L150) | Высокий: снимается защита от подмены цепи при первичной синхронизации |
| E3 | `NAME_FIRSTUPDATE` больше не требует входа `NAME_NEW` (`assert` + `state.Invalid` закомментированы) | [names/main.cpp:352](src/names/main.cpp#L352) | Высокий: снят анти-front-running commit/reveal Namecoin |
| E4 | `tx-nameupdate-without-name-input` закомментирована с TODO про блок 29966 | [names/main.cpp:203](src/names/main.cpp#L203) | Высокий |
| E5 | Удалена проверка `SCRIPT_VERIFY_NAMES_LONG_SALT` (rand ≥ 20 байт) | `names/main.cpp` | Средний |
| E6 | `CNameScript::getNameOp()` в `default` возвращает `op` вместо `assert(false)` | [script/names.h:99](src/script/names.h#L99) | Средний: тихо ломает инварианты во всех `switch` по name-операциям |
| E7 | Хардкод высоты `170000` как правила консенсуса для перезаписи DOI | [names/main.cpp:307](src/names/main.cpp#L307) | Средний: недокументированный форк-хайт |

> Каждый из этих пунктов — отступление от Namecoin, вынужденное или ошибочное. Разбираться
> с каждым нужно отдельно: часть может оказаться нужной для синхронизации существующей цепи,
> часть — незакрытой отладкой (как выяснилось про E1). Всё, что останется, обязано быть
> оформлено явными правилами консенсуса, а не закомментированным кодом.

#### E1 разобран: проверка сложности отключена напрасно

Проверено 2026-08-07 дважды. Сначала разбором заголовков из локальной копии цепи
(высоты 0–320864, 159 точек ретаргета), затем — по всей цепи через RPC работающей
прод-ноды `itchy-jellyfish-89` (`116.203.34.154`), высота 430878, **213 точек ретаргета**.
Для каждой точки вычислено ожидаемое значение `nBits` по двум формулам и сопоставлено
с фактическим:

| Правило | Расхождений из 213 (вся цепь) |
|---|---|
| `nBlocksBack = 2015` (формула Bitcoin) | **193** |
| `nBlocksBack = 2016` (правило Namecoin, реализованное в `pow.cpp`) | **0** |

Локальная проверка на 159 точках дала тот же результат (142 против 0), то есть
два независимых источника данных сходятся.

Дополнительно: блоков вне точек ретаргета с отличающимся `nBits` — **0**, то есть ветка
`return pindexLast->nBits` тоже отрабатывает без исключений.

Гипотеза о том, что `nAuxpowStartHeight = 1` (вместо 19200 у Namecoin) сдвигает окно
ретаргета и ломает валидацию, **не подтвердилась**: цепь Doichain построена ровно по тому
правилу, которое реализовано в коде. Проверка `block.nBits != GetNextWorkRequired(...)`
на проверенном участке проходит без единого отказа.

**Вывод: E1 — не правило сети Doichain, а незакрытая отладка. Проверку следует восстановить.**
Причина проблем, из-за которых её когда-то закомментировали, лежит в другом месте.
Вопрос закрыт полностью: покрыта вся цепь до текущей высоты, непроверенных участков нет.

### Слой F. Кошелёк и RPC

| Изменение | Статус |
|---|---|
| RPC `name_doi <name> <value> [options]` | ✅ [walletnames.cpp:1098](src/wallet/rpc/walletnames.cpp#L1098) |
| `CachedTxGetAmounts`: метка `"doi: …"` | ⚠️ перенесено, но **код недостижим** — `isAnyUpdate()` уже `true` для DOI, ветка `doi:` никогда не выполнится ([receive.cpp:283](src/wallet/receive.cpp#L283)) |
| `CreateTransaction`: `SetDoichain()` для name-скриптов | ✅ |
| Множество `LogPrintf`-отладки в `DummySignTx` / `CalculateMaximumSignedTxSize` (старый `master`) | ✅ не перенесено (и хорошо) |

### Слой G. Сборка, GUI, тесты

* CMake-конфигурация под Doichain (имена таргетов `doichaind`, `doichain-cli`, `doichain-qt`, `test_doichain`) — ✅ перенесено.
* GUI: логотип, брендинг, единицы (DOI/mDOI/µDOI) — ✅ перенесено (`823ed94037`, `634fb498d9`, `18c03e2a5f`, `7455c087b6`).
* Версия: `CLIENT_VERSION 29.20.1.11` ([CMakeLists.txt:30](CMakeLists.txt#L30)) — унаследована схема нумерации старого Doichain (0.20.1.11).
* Функциональные и unit-тесты для `name_doi`: **отсутствуют полностью** (`grep name_doi test/functional src/test` → 0 совпадений) — и в старой, и в новой версии.

---

## 3. Что из старого Doichain НЕ перенесено (и правильно)

Из 120 старых коммитов примерно **60** — это чистая отладка, которую переносить не нужно:
`0038…0099` — сплошные «added logging», «improved logging», «deactivated EvalScript for
scriptPubKey», `printf` внутри `getNameOp()`, ручные отступы и мусор от мёрдж-конфликтов
(например, `{	{` в `script/names.cpp` старой версии). Всё это в nc29-ветке отсутствует —
это улучшение, а не потеря.

Отдельно: старый `master` **вырезал** `CChainParams::IsHistoricBug` / `mapHistoricBugs`.
Ветка nc29 вместо этого оставила механизм upstream и объявила карту пустой — так конфликтов
с namecoin не будет.

---

## 4. Gap-анализ: что осталось доделать в `doichain-nc29-migration`

### 4.1 Блокер: `core_write.cpp` не знает про `OP_NAME_DOI`

[core_write.cpp](src/core_write.cpp), `NameOpToUniv()` — в `switch` нет ветки `OP_NAME_DOI`,
`default:` = `assert(false)`. `isNameOp()` для DOI возвращает `true`, значит функция будет
вызвана. Следствие: **любой** вызов `decoderawtransaction`, `getrawtransaction verbose`,
`gettransaction`, REST `/rest/tx/…` над транзакцией `name_doi` роняет узел по assert
(в Bitcoin Core assert активен в release-сборках).

В старом `master` эта ветка была ([master:src/core_write.cpp](src/core_write.cpp)) — при
переносе её потеряли. Правка на 8 строк.

### 4.2 Отладочный вывод в горячем пути консенсуса

`LogPrintf("CheckNameTransaction: Checking Inputs - 0\n")`, `"… Step 2/3/4/5/6"` —
[names/main.cpp:108,133,167,220,256,362](src/names/main.cpp#L108). Выполняется для каждой
транзакции при валидации блока и при приёме в мемпул, без фильтра по категории лога.
Убить или перевести на `LogDebug(BCLog::NAMES, …)`.

### 4.3 `#define error(...)` в `names/main.cpp`

[names/main.cpp:28](src/names/main.cpp#L28) — макрос-заглушка вместо удалённого в upstream
хелпера `error()`. Это откат миграции upstream на `LogError`, и он делает файл минным полем
для любого будущего ребейза (макрос без скобок вокруг имени, действует на весь TU).
В том же файле часть функций уже переписана на `LogError` — код в двух стилях одновременно.

### 4.4 Откат upstream-рефакторинга `CNameMemPool::check()`

Сигнатура возвращена к старой namecoin-версии
`check(ChainstateManager&, const CCoinsView&)` вместо upstream
`check(const CCoinsViewCache& tip, int64_t spendheight)` — [names/mempool.h:190](src/names/mempool.h#L190).
Это не Doichain-фича, а отменённое изменение upstream: заново вычисляет высоту через
`chainman.BlockIndex()`. При обновлении на nc30 конфликт гарантирован.

### 4.5 Прочее

* `#include <key_io.h>` / `<script/solver.h>` посреди файла [names/main.cpp:90](src/names/main.cpp#L90).
* Избыточные условия `isAnyUpdate() || isDoiRegistration()` (первое уже включает второе) — txdb.cpp, validation.cpp, names/main.cpp.
* Мёртвая ветка `"doi: "` в [wallet/receive.cpp:283](src/wallet/receive.cpp#L283).
* Ноль тестов на `name_doi`.
* `MIGRATION_PLAN.md` содержит **выдуманные факты**: RPC-команд `name_update_doi`,
  `name_transfer_doi`, `name_list_doi` в коде не существует и никогда не существовало
  (проверено по `master` и по текущей ветке). Документ следует пометить как устаревший.

---

## 5. Сводка: полнота переноса

| Слой | Перенесено |
|---|---|
| A. Ребрендинг | 100 % |
| B. Сетевые/консенсус-параметры | ~100 % (кроме временно отключённого `nMinimumChainWork`) |
| C. Протокол `OP_NAME_DOI` | ~95 % — не хватает `core_write.cpp` |
| D. Мемпул | 100 % (кроме `removeConflicts`, которого не было и в оригинале) |
| E. Ослабленные проверки | 100 % (перенесены **вместе с рисками**) |
| F. Кошелёк/RPC | 100 % |
| G. Сборка/GUI | 100 % |
| Тесты | 0 % (их не было) |

**Вывод: функционально ничего существенного не потеряно.** Основная работа теперь не в
«переносе», а в приведении перенесённого в состояние, пригодное для сопровождения и для
следующих обновлений upstream. См. `DOICHAIN_UPGRADE_STRATEGY.md`.
