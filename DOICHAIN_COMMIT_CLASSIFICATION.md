# Классификация 128 коммитов Doichain для переноса

Ветка-источник: `master` (`b74273e682`), база — точка форка от Namecoin `3e69dd1013` (21.09.2020).
Рабочая ветка: **`doichain/base-0.20`** (создана от `master`).
Дата: 2026-08-07.

Цель: свести 128 коммитов к 12 тематическим, чтобы их можно было ребейзить по лестнице
релизов Namecoin (см. [ARBEITSPLAN-Doichain-Update.md](ARBEITSPLAN-Doichain-Update.md)).

## Итог

| Категория | Коммитов | Целевой тематический коммит |
|---|---|---|
| C1 | 7 | `chainparams: Doichain networks` — genesis, порты, magic, seeds, форк-высоты |
| C2 | 3 | `consensus: Doichain consensus divergences` — ослабленные проверки, оформить флагами |
| C3 | 4 | `script: add OP_NAME_DOI` |
| C4 | 9 | `names: DOI registration/update rules` |
| C5 | 12 | `names: track DOI ops in mempool` |
| C6 | 5 | `txdb/validation: allow duplicate DOI names in UTXO set` |
| C7 | 1 | `rpc: name_doi and DOI in tx decoding` |
| C8 | 9 | `wallet: name_doi RPC` |
| C9 | 2 | `build/qt/doc: Doichain branding` |
| C10 | 1 | версия — при переходе на CMake переписывается заново |
| C11 | 2 | `qt: Doichain branding` |
| C12 | 2 | `doc: Doichain documentation` |
| MERGE | 6 | merge-коммиты, не переносятся |
| **DROP** | **65** | **отладка и эксперименты — выбрасываются** |

**65 из 128 коммитов (51 %) — чистая отладка**: «added logging», «improved logging»,
`printf` внутри `getNameOp()`, «deactivated EvalScript for scriptPubKey», «changed test address»,
«added a fix address to test destination address», «disabled nLockTime for testing».
Ни один из них не несёт функциональности; все ломают при ребейзе файлы, которые апстрим
активно меняет (`script/interpreter.cpp`, `wallet/wallet.cpp`, `script/sign.cpp`).

Отдельно в DROP отправлены четыре коммита, устаревшие по другой причине:

* `e417bcd134` «Fixed Compile for Deprecated Lib Boost error» и `b640a1d574` «Update names.cpp
  fix for compile error» — починка сборки под Boost и компилятор 2022 года; на новом апстриме
  не нужны;
* `0a1fb820dd` «fixed creating mac dmg via make deploy» — autotools, с nc29 неактуально;
* `2647dba735` «fixed compilation errors» — правит то, что сломали соседние отладочные коммиты.

## Требуют решения при сборке тематических коммитов

**C2 — правила консенсуса.** Три коммита (`bb42d3c571` «removed check for not existing inputs»,
`35a7097d40` «disabled checks», `1cb04be5a1` «removed historic namecoin bug») плюс правка
`ContextualCheckBlockHeader` из базового коммита.

Отключённая проверка сложности PoW присутствует **и в прод-теге `dc0.20.1.10`**
(`src/validation.cpp:3591`), а не только в экспериментальной ветке 29. Однако проверка
2026-08-07 по 159 точкам ретаргета показала: цепь Doichain полностью соответствует правилу,
реализованному в `pow.cpp` (0 расхождений), а формула Bitcoin даёт 142 расхождения.
**Это незакрытая отладка, а не правило сети — при сборке C2 проверку восстановить.**
Подробности — [DOICHAIN_FORK_INVENTORY.md](DOICHAIN_FORK_INVENTORY.md), раздел «E1 разобран».

Остальные отступления (E3–E7) нужно разобрать так же поштучно, и то, что останется, перевести
из закомментированного кода в явные флаги `Consensus::Params` — иначе при первом же ребейзе
они восстановятся в апстрим-виде.

**C3/C4/C5/C6/C7/C8 — базовый коммит `add0c04fc1`** «implemented name_doi op» трогает 15 файлов
сразу и должен быть разрезан по этим шести категориям.

**C6 — `35b7313d94`** «disabled check for duplicated in UTXO set for DoiRegistration» —
ключевое отличие модели данных Doichain: одно DOI-имя может иметь несколько UTXO.
Не отладка, переносить обязательно.

**C5 — `538b09f2a8`** «filtering classic namecoin d/ transaction away from name_doi» —
разделение namespace `d/` (Namecoin) и DOI. Тоже обязательное правило.

## Полная таблица

### C1 — chainparams

| Коммит | Тема | Файлы |
|---|---|---|
| `1bc2a7b835` | changed genesis for mainnet and testnet | src/chainparams.cpp src/chainparamsbase.cpp src/chainparamsseeds.h  |
| `b35e34ae2d` | changed genesis one more time | src/chainparams.cpp  |
| `771c068ed5` | fixed regtest genesis | src/chainparams.cpp  |
| `e7412a15bc` | fixed testnet params | src/chainparams.cpp  |
| `dfb2f68c45` | added dnsseed.doichain.org | src/chainparams.cpp  |
| `4389bbb1c8` | Chainparams testnet | src/chainparams.cpp  |
| `c116b757a9` | Change Segwit Activation Height | src/chainparams.cpp  |

### C2 — правила консенсуса

| Коммит | Тема | Файлы |
|---|---|---|
| `bb42d3c571` | removed check for not existing inputs | src/names/main.cpp  |
| `35a7097d40` | disabled checks | src/names/main.cpp  |
| `1cb04be5a1` | removed historic namecoin bug in doichain blockchain | src/validation.cpp  |

### C3 — script: OP_NAME_DOI

| Коммит | Тема | Файлы |
|---|---|---|
| `add0c04fc1` | implemented name_doi op | src/core_write.cpp src/names/common.cpp src/names/main.cpp src/names/main.h (+11) |
| `e1e7bec8b2` | fixed to allow name_doi ops | src/script/names.h src/validation.cpp  |
| `1a326f35fb` | added fixed problem with inconsistent db when creating block after name_doi | src/script/names.h  |
| `85e5b22b76` | changed buildNameDOI script | src/script/names.cpp  |

### C4 — names: правила DOI

| Коммит | Тема | Файлы |
|---|---|---|
| `2066f20c3f` | modifications to access correct name_doi ops | src/names/main.cpp src/script/names.h  |
| `94dbe8dbed` | fixed to allow name_doi ops | src/names/main.cpp  |
| `b99fa287a7` | added checks for NAME_DOI operations | src/names/main.cpp  |
| `c45c439239` | minor improvements | src/names/main.cpp src/wallet/rpcnames.cpp  |
| `f75b79fdfc` | removed problem with new name_DOI | src/names/main.cpp  |
| `617b2ac92f` | removed problem with new name_DOI | src/names/main.cpp  |
| `5c31115408` | if name_doi nameId is already used and it is and inputs or no old name inputs throw an error | .settings/language.settings.xml .vscode/c_cpp_properties.json DOICHAIN_CHANGES.md src/consensus/tx_verify.cpp (+3) |
| `8ff85f8ff2` | fixed name_doi on same nameId | src/names/main.cpp src/wallet/rpcnames.cpp  |
| `95e209cfba` | fixed name_doi overwrite bug so old stransactions are still accepted | src/names/main.cpp  |

### C5 — мемпул DOI

| Коммит | Тема | Файлы |
|---|---|---|
| `b20bd546b0` | src/names/mempool.cpp | src/names/mempool.cpp  |
| `41737c5e5a` | removed assert and added possibility ot update a doi in mempool | src/names/mempool.cpp  |
| `59d4372fa1` | changed napNameDois to map with set (like udpates | src/names/mempool.cpp src/names/mempool.h  |
| `fa87268cb1` | fixed map problems in tx chechks for doi | src/names/mempool.cpp src/names/mempool.h  |
| `d139d3da8b` | fixed map problems in tx chechks for doi | src/names/mempool.cpp src/names/mempool.h  |
| `ab3a7ab673` | removed typo | src/names/mempool.cpp  |
| `538b09f2a8` | filtering classic namecoin d/ transaction away from name_doi - we don't want to interfear here. | src/names/mempool.cpp  |
| `3a7ec91ca6` | changed DEFAULT_NAME_CHAIN_LIMIT to 2 | src/consensus/tx_verify.cpp src/names/mempool.h src/wallet/rpcnames.cpp  |
| `c7f8f743f6` | fixed vector with char to string conversion | src/names/mempool.cpp  |
| `9f3b8d76f1` | fixed check on d/ names in name_doi operations | src/names/mempool.cpp  |
| `14b03c98f4` | finding possible previous output of a name_doi op tx | src/names/mempool.cpp  |
| `8f7dc12586` | update pendingChainLength so it finds dois | src/names/mempool.cpp  |

### C6 — txdb/validation

| Коммит | Тема | Файлы |
|---|---|---|
| `86d9c6e9b5` | problem with name in UTXO set but not in DB | src/consensus/tx_verify.cpp src/names/main.cpp src/names/main.h src/names/mempool.cpp (+5) |
| `7df34fa714` | now deciding to create an output without or with a specified input from old data | src/txdb.cpp  |
| `c5f7a9ea6c` | src/txdb.cpp | src/txdb.cpp  |
| `35b7313d94` | disabled check for duplicated in UTXO set for DoiRegistration | src/txdb.cpp  |
| `06dc0dfff7` | re-enabled check on expiry | .cproject .project .pydevproject .settings/language.settings.xml (+2) |

### C7 — RPC

| Коммит | Тема | Файлы |
|---|---|---|
| `e6e0a12289` | added missing name_doi op | src/rpc/names.cpp  |

### C8 — кошелёк

| Коммит | Тема | Файлы |
|---|---|---|
| `c6cf243db2` | buildNmaeDOI for updates and new name_doi registrations. | DOICHAIN_CHANGES.md src/wallet/rpcnames.cpp  |
| `d0f1869a1f` | getting oldData from coinsTip | src/wallet/rpcnames.cpp  |
| `34b6652279` | oldData now checked put always creating transaction without inputs | src/wallet/rpcnames.cpp  |
| `9c9b2b3578` | removed locktime for newly created transactions which might prevent the signing of an spending transaction | src/wallet/rpcnames.cpp src/wallet/wallet.cpp  |
| `bca5aa4847` | changed address of namescript to old address when name_doi is an update to an old one | src/wallet/rpcnames.cpp  |
| `ff3d406fd7` | fixed output.nameOp doi: instead of update: | src/names/main.cpp src/wallet/wallet.cpp  |
| `756a0139a5` | changed way to find inputs | src/wallet/rpcnames.cpp  |
| `b9df744df4` | unified address in name output | src/wallet/rpcnames.cpp  |
| `b3f34d53dc` | re-enabled burning storage fee | src/wallet/rpcnames.cpp  |

### C9 — ребрендинг

| Коммит | Тема | Файлы |
|---|---|---|
| `6d675c32d2` | renamed namecoin to doichain | .gitignore Changes Makefile.am README.md (+126) |
| `367decbf54` | renamed namecoin to doichain in man | doc/man/{namecoin-cli.1 doc/man/{namecoin-qt.1 doc/man/{namecoin-tx.1 doc/man/{namecoin-wallet.1 (+1) |

### C10 — сборка/версия

| Коммит | Тема | Файлы |
|---|---|---|
| `a4c83bfc5d` | Version angepasst | configure.ac  |

### C11 — Qt

| Коммит | Тема | Файлы |
|---|---|---|
| `dfc8e98cdb` | qt fixes | src/qt/forms/coincontroldialog.ui src/qt/forms/openuridialog.ui src/qt/forms/overviewpage.ui src/qt/forms/sendcoinsdialog.ui (+22) |
| `9c4a782394` | Coin units rebranded | src/qt/bitcoinunits.cpp  |

### C12 — документация и IDE

| Коммит | Тема | Файлы |
|---|---|---|
| `6fffd436d5` | added changed fiels | DOICHAIN_CHANGES.md  |
| `4f4e755319` | excluded vscode settings | .gitignore  |

### MERGE — merge-коммиты (не переносятся)

| Коммит | Тема | Файлы |
|---|---|---|
| `236eed63c6` | Merge pull request #1 from davidreband/master | src/chainparams.cpp  |
| `853b820a5b` | Merge pull request #5 from mrheat/patch-1 | src/chainparams.cpp  |
| `b3346b350d` | Merge pull request #12 from mrheat/master | src/qt/bitcoinunits.cpp src/qt/forms/coincontroldialog.ui src/qt/forms/openuridialog.ui src/qt/forms/overviewpage.ui (+23) |
| `b61ce3218a` | Merge pull request #14 from mrheat/master | src/coins.cpp src/coins.h src/names/main.cpp src/rpc/names.cpp (+2) |
| `859d54bf00` | Merge pull request #18 from mrheat/patch-3 | src/rpc/names.cpp  |
| `b74273e682` | Merge pull request #2 from Doichain/master | configure.ac src/chainparams.cpp src/coins.cpp src/coins.h (+31) |

### DROP — отладка и эксперименты (65 шт., не переносятся)

| Коммит | Тема | Файлы |
|---|---|---|
| `f72c07cd76` | added Log | src/script/names.h  |
| `c7b5c7cb98` | added util.h | src/script/names.h  |
| `79fca2819e` | added log | src/script/names.h  |
| `47c688e556` | added loggings to undestand the problem | src/names/main.cpp  |
| `c9fa0de9d2` | added logging when OP_NAME_DOI comes from getNameOp | src/names/main.cpp  |
| `3357bde380` | not clear why CheckTxInputs exists here adding debug logs | src/consensus/tx_verify.cpp  |
| `b5b6f96a6e` | not clear why CheckTxInputs exists here adding debug logs | src/consensus/tx_verify.cpp  |
| `0942a62650` | added debug logs to identify problem | src/names/mempool.cpp  |
| `8a2cc75f6a` | minor loging improvements | src/txdb.cpp  |
| `37da6c21a5` | fixed logging | src/txdb.cpp  |
| `d161a02d4e` | fixed logging, minor improvement | src/wallet/rpcnames.cpp  |
| `5a05aebe7b` | added loging for finding lastNameOutputs | src/names/mempool.cpp  |
| `817818d72d` | added loging for finding lastNameOutputs | src/txdb.cpp src/wallet/rpcnames.cpp  |
| `3e8caef8d5` | fixed logging | src/wallet/rpcnames.cpp  |
| `7fbd880913` | fixed logging | src/wallet/rpcnames.cpp  |
| `0060052e30` | commented out previously commented out functino | src/wallet/rpcnames.cpp  |
| `388bbd70a3` | added logging for signing tx | src/wallet/wallet.cpp  |
| `636e7acd19` | logging to see inputs for signing | src/wallet/rpcnames.cpp src/wallet/wallet.cpp  |
| `5c0df944ff` | fixed LogPrintf | src/wallet/wallet.cpp  |
| `c08d0de640` | improved logging | src/wallet/wallet.cpp  |
| `00913ff20a` | moved CTxIn to original position | src/wallet/rpcnames.cpp  |
| `8cffcb1e11` | improved logging to find reason for not signing | src/wallet/rpcnames.cpp src/wallet/wallet.cpp  |
| `b0485ae55c` | adding logging | src/script/sign.cpp  |
| `bb37b23e55` | improved logging | src/script/sign.cpp  |
| `58d653788a` | improvement of debugging | src/wallet/rpcnames.cpp  |
| `2cc2d75c47` | getName test | src/wallet/rpcnames.cpp  |
| `28361498ec` | added debugging to understand signature problem | src/script/sign.cpp  |
| `78888befea` | added another logging | src/script/sign.cpp  |
| `115497ef3d` | added logging after EvalScript | src/script/interpreter.cpp  |
| `5487427d49` | added logging after EvalScript | src/script/interpreter.cpp  |
| `3435e90b7f` | added logging | src/script/interpreter.cpp  |
| `95247667bb` | deactivated EvalScript for scriptPubKey | src/script/interpreter.cpp  |
| `3f8b878228` | deactivated EvalScript for scriptPubKey | src/script/interpreter.cpp  |
| `bebc47277f` | inserted loggings inbetween the op code eval | src/script/interpreter.cpp  |
| `4529a3ada3` | inserted logging inbetween the op code eval | src/script/interpreter.cpp  |
| `d17f3c3a54` | inserted logging inbetween the op code eval | src/script/interpreter.cpp  |
| `a0f42e3e3f` | inserted logging inbetween the op code eval | src/script/interpreter.cpp  |
| `33955db911` | added logging into ops | src/script/interpreter.cpp  |
| `2a83d970de` | added logging into ops | src/script/interpreter.cpp src/wallet/wallet.cpp  |
| `6bea1e782e` | improved debugging inside OP_EQUAL | src/script/interpreter.cpp  |
| `f5c3130c9e` | logging contents of first valtype | src/script/interpreter.cpp  |
| `c64db6bf50` | logging contents of second valtype | src/script/interpreter.cpp  |
| `ccb8a28bab` | logging contents of second valtype | src/script/interpreter.cpp  |
| `d40072083a` | logging contents of second valtype | src/script/interpreter.cpp  |
| `5b41ba09d7` | test if we can create name without txIn | src/wallet/rpcnames.cpp  |
| `83f41b6efb` | getting old data | src/wallet/rpcnames.cpp  |
| `e606ea6b28` | getting old data | src/wallet/rpcnames.cpp  |
| `070297c4d8` | setting nullptr again so we can add tx | src/wallet/rpcnames.cpp  |
| `45e6027245` | reenabled txIn | src/wallet/rpcnames.cpp  |
| `29d04de055` | reenabled txIn | src/wallet/rpcnames.cpp  |
| `864e1d88c5` | added a fix address to test destination address for name input | src/wallet/rpcnames.cpp  |
| `9b29db026d` | disabled nLockTime for testing | src/wallet/wallet.cpp  |
| `e0ccbfffa0` | improved logging | src/script/interpreter.cpp  |
| `b97c3adc83` | improved logging | src/script/interpreter.cpp  |
| `0f043bfcf9` | improved logging | src/wallet/rpcnames.cpp  |
| `e0a2f12d02` | changed test address | src/wallet/rpcnames.cpp  |
| `30c05a64c1` | removed loggings | src/script/interpreter.cpp  |
| `453c24a522` | logging intx as string | src/script/interpreter.cpp src/wallet/rpcnames.cpp  |
| `38ebfcd87b` | improved logging and clean up | src/script/interpreter.cpp src/wallet/rpcnames.cpp  |
| `2647dba735` | fixed compilation errors | src/names/main.cpp src/wallet/rpcnames.cpp  |
| `c10d7ad3e8` | removed debugging | src/script/sign.cpp  |
| `0a1fb820dd` | fixed creating mac dmg via make deploy | contrib/macdeploy/fancy.plist contrib/macdeploy/macdeployqtplus  |
| `4238d5dbcd` | removed debug logging | src/names/mempool.cpp  |
| `e417bcd134` | Fixed Compile for Deprecated Lib Boost error / all platforms | src/coins.cpp src/coins.h src/names/main.cpp src/rpc/names.cpp (+2) |
| `b640a1d574` | Update names.cpp fix for compile error | src/rpc/names.cpp  |

