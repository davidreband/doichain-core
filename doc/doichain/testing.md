# Testing Doichain

What exists, what it covers, how to run it, and what the tests found. See
[README.md](README.md) for the port itself.

## Running

Unit tests. Run them through `ctest`, not by invoking the binary with several
suites at once: each suite needs its own process, because the test fixtures
register command line arguments into the global `ArgsManager` and a second
fixture in the same process trips an assert. `ctest` already runs one process
per suite.

    ctest --test-dir build -j4
    ctest --test-dir build -R name          # the name suites only

Functional tests.

    python3 build/test/functional/test_runner.py --jobs=6
    python3 build/test/functional/test_runner.py name_doi.py

A shared regtest chain is cached in `build/test/cache` on first use. Delete that
directory if chain parameters change, otherwise the cache no longer matches.

Two traps worth knowing. Boost.Test catches `SIGABRT` and continues with the
next case, so one failing assert cascades into a long list of unrelated
failures — always fix the first one and re-run before reading the rest. And
after editing a source file, check that the test binary was actually relinked;
an interrupted build can leave a stale binary that silently tests the old code.

## Inherited from Namecoin

The name machinery arrives with a substantial suite, and it is worth running:
it tells you whether the Doichain changes broke the Namecoin layer underneath.

| | |
|---|---|
| Unit | 27 cases in `name_tests.cpp`, `name_mempool_tests.cpp`, `name_applications_tests.cpp` |
| Functional | 25 files, about 3500 lines: registration, expiration, pending, reorg, raw transactions, wallet, multisig, segwit, PSBT, encodings |

All of them pass.

## New: `name_doi.py`

`name_doi` had no test of any kind for six years. `test/functional/name_doi.py`
is the first, with seven scenarios:

1. **Registration.** `name_doi` on an unused name enters the mempool, appears in
   `name_pending` as `name_doi`, and after mining is visible through
   `name_show` and `name_list`.
2. **Update.** A second operation spending the previous DOI output replaces the
   value.
3. **Registration without a name input.** Verifies that the transaction really
   has no name input among its inputs, and that it is accepted. This is the one
   consensus divergence from Namecoin, so the test pins it down explicitly.
4. **The `d/` namespace is refused.** `name_doi` on a `d/` name does not enter
   the mempool and `sendrawtransaction` rejects it with
   `txn-mempool-name-error`, while the classic `name_new` plus
   `name_firstupdate` workflow on `d/` keeps working. Note the wallet still
   returns a txid for the rejected transaction, because it records what it
   built; the check is that the node did not accept it.
5. **Transaction decoding.** `getrawtransaction` with verbosity and
   `decoderawtransaction` both report `"op": "name_doi"`. Without the
   `OP_NAME_DOI` case in `NameOpToUniv` the default branch asserts and takes
   the node down, so this test guards a crash, not a formatting detail.
6. **Pending operations.** Two operations chain onto each other, the second
   spending the first one's name output, and a third is refused by
   `DEFAULT_NAME_CHAIN_LIMIT`, which is 2 for Doichain.
7. **Reorg.** A block carrying a `name_doi` is reorged out, the name disappears
   from the chain, and mining on the node that still holds the transaction
   brings it back.

The test runs with `-txindex`, matching how the production nodes are configured
and allowing it to inspect the inputs of confirmed transactions.

## What these tests found

Writing them surfaced three defects in `name_doi`, all regressions of the port
relative to the 0.20 production code, and all invisible without tests.

**No chain limit and no mempool awareness.** The port had rewritten `name_doi`
to consult only the confirmed chainstate. The 0.20 code checks
`pendingNameChainLength` and builds on `lastNameOutput`, exactly as
`name_update` still does.

**Double counting in `pendingChainLength`.** `registersDoi()` reads the same
`mapNameDois` whose size is then added again, so every pending DOI counted
twice. Namecoin's equivalent `registersName()` reads `mapNameRegs`, a different
map from `updates`, which is why the bug is specific to DOI.

**A pending name input was rejected.** A `NAME_DOI` update whose input was
still in the mempool failed with `tx-nameupdate-nonexistant`, because the name
is not in the database until the first operation is mined. `NAME_UPDATE`
handles this by returning early when the input coin sits at `MEMPOOL_HEIGHT`;
`NAME_DOI` now does the same. This was what prevented DOI operations from
chaining at all.

Separately, the `d/` namespace check had never worked. It compared the name
against `EncodeNameForMessage()`, which wraps the name in single quotes, so
`rfind("d/", 0) == 0` never matched. It now compares raw bytes. This is mempool
policy rather than consensus, so names already on chain are unaffected — only
what the node accepts from now on.

## Adapting inherited tests to Doichain

Several upstream suites carry Namecoin constants. Where a failure was a stale
constant it was updated; where it pointed at something real it was left alone.

| Suite | What it was |
|---|---|
| `TestChain100Setup` | Namecoin's regtest chain tip. One assert, and it aborted about fifteen suites before they ran |
| `rpcnames_tests` | expected HKDF salts, recomputed for the Doichain info string |
| `util_tests` | message signatures, regenerated for `"Doichain Signed Message:\n"` |
| `script_standard_tests` | 8 BIP341 taproot vectors, re-encoded for the `dc` prefix |
| `net_peer_connection_tests` | Namecoin's default port 8334 |
| `auxpow_tests` | assumed a strict chain id; the check now follows `fStrictChainId` |
| `validation_chainstate*`, `validation_tests` | the regtest assumeutxo block hash |
| **`pow_tests`** | **left failing on purpose** — see README, it reports a real property of `powLimit` |

Two of those turned out to be defects rather than constants. `DEPLOYMENT_TAPROOT`
shared bit 28 with `DEPLOYMENT_TESTDUMMY` on mainnet, a collision introduced by
the 29.x chainparams patch; upstream and Doichain's own other networks use bit 2.
And the regtest assumeutxo entry kept Namecoin's block hash — telling detail: the
UTXO set hash and transaction count matched exactly, because the snapshot
contents do not depend on chain parameters, only the block identifier does.

The functional framework needed work before it would start at all: 40 hardcoded
`ncrt1...` regtest addresses had to be decoded and re-encoded for the `dcrt`
prefix, since a bech32 checksum covers the human-readable part, plus three
places where the prefix is a constant.

## Verifying against the real chain

Unit and functional tests run on regtest and cannot show that historical
mainnet blocks still validate. That needs the real chain.

    doichaind -datadir=<copy> -reindex          # revalidate local block files
    doichaind -datadir=<copy>                   # then let it sync the rest

Acceptance is equivalence with a production node: same height, same best block
hash. Last run, at height 430881:

    ours: 5d561d4c32af68a3135f66c7edbae5f9ac6320409ab5b1b517ea5298b731232f
    prod: 5d561d4c32af68a3135f66c7edbae5f9ac6320409ab5b1b517ea5298b731232f

Zero consensus errors across all 430881 blocks, with the proof-of-work
difficulty check enabled. The node connected to the live network and downloaded
from 0.20 peers, which also demonstrates P2P compatibility: network magic,
protocol version and block relay.

Run this before every release. It is the only check that covers the historical
chain, and regtest cannot substitute for it.

## Gaps

There are no unit tests for `OP_NAME_DOI` at the `CNameScript` level, and no
functional coverage of `name_doi` through PSBT, multisig or `namerawtransaction`,
all of which the Namecoin operations do have. Worth adding.
