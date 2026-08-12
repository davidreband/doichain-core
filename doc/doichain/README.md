# Doichain on top of Namecoin Core 31.1

This directory documents what Doichain changes in Namecoin Core, why, and what
to be careful with. It describes the state of the `doichain/31.x` branch, which
carries the port from the 2020-era 0.20 code base to Namecoin Core 31.1.

Read [testing.md](testing.md) for the test suite and how to run it.

## Status

| | |
|---|---|
| Base | Namecoin Core `nc31.1` (Bitcoin Core 31.1) |
| Doichain commits on top | 13 |
| Unit tests | 150 of 151 suites pass; `pow_tests` fails on purpose, see below |
| Functional name tests | 26 of 26 pass, including the new `name_doi.py` |
| Mainnet validation | full chain to height 430881 revalidated, tip hash identical to the production 0.20 node |

## How the branch is organised

The branch is a queue of thematic commits on top of an untouched upstream tag.
Keep it that way: updating to a newer Namecoin release should be a rebase of
these commits, not a merge.

    chainparams: Doichain network parameters
    script: add the OP_NAME_DOI operation
    names: validation rules for DOI operations
    names: track DOI operations in the mempool
    txdb, validation: allow several UTXOs per DOI name
    rpc: name_doi in raw transactions, pending list and tx decoding
    wallet: name_doi RPC command
    build: compile against nc31.1
    build, doc, qt: Doichain branding
    names: allow a pending DOI operation as the name input
    names: fix the d/ namespace check for name_doi
    test: functional tests for name_doi, and two fixes they found
    test: adapt the remaining inherited suites, and two chainparams fixes

Two rules that make the next update cheap:

* **Do not rename upstream identifiers.** `IsNamecoin()`, `SetNamecoin()`,
  `NAMECOIN_VERSION` and `TxAllowedForNamecoin()` keep their upstream names.
  They are internal, invisible to users, and live in the files upstream
  rewrites most often. Renaming them produced compile errors twice during this
  port and buys nothing.
* **Do not rewrite copyright headers.** Attribution for files written by the
  Bitcoin and Namecoin developers stays as it is.

## What Doichain changes

### Network parameters

`src/kernel/chainparams.cpp`. Own genesis blocks, network magic `f8b2b2ff`,
ports 8338/18338, bech32 prefixes `dc`/`td`/`dcrt`, DNS seeds
`dnsseed.doichain.org` and `seed.doi.works`, and its own fork heights
(CSV/Segwit at 216500).

Merge mining differs from Namecoin: chain id `0x0002`, auxpow from height 1,
and `fStrictChainId = false`. The last one means a parent block carrying
Doichain's own chain id is accepted; Namecoin rejects that.

`mapHistoricBugs` is empty. Doichain has none of Namecoin's historic bugs, so
the upstream mechanism is kept but never fires.

### The name_doi operation

`OP_NAME_DOI` (= `OP_10`) carries a name and a value, like `NAME_UPDATE`:

    OP_NAME_DOI <name> <value> OP_2DROP OP_DROP <address script>

Names registered this way use the `e/` prefix in practice. The `d/` namespace
belongs to the classic Namecoin workflow and is refused for `name_doi` by
mempool policy.

Two properties differ from Namecoin names:

* **A DOI may be registered without spending a name input.** This is the only
  consensus rule where Doichain diverges. Mainnet block 29966
  (`1173d2615de4aba9785646bc414040e622cc04869593f006872b9013e1b1201b`) carries
  three such registrations, so enforcing the Namecoin rule would make the
  existing chain unsyncable. The relaxation is tied to the operation, not to
  the network: `NAME_UPDATE` and `NAME_FIRSTUPDATE` still require a name input
  on every chain.
* **One DOI name may occupy several UTXOs**, so `ValidateNameDB` does not treat
  a repeated name in the UTXO set as a duplicate.

### Where the code lives

| Area | Files |
|---|---|
| Opcode and script | `src/script/script.h`, `src/script/names.{h,cpp}` |
| Consensus rules | `src/names/main.cpp` |
| Mempool tracking | `src/names/mempool.{h,cpp}`, `src/kernel/mempool_entry.h` |
| Database | `src/txdb.cpp`, `src/validation.cpp` |
| RPC | `src/rpc/names.cpp`, `src/core_io.cpp` |
| Wallet RPC | `src/wallet/rpc/walletnames.cpp` |

## What to watch out for

### The proof-of-work check was disabled for six years

`ContextualCheckBlockHeader` had its difficulty check commented out, both on
this branch's ancestor and in the production 0.20 code. It is restored.

This was verified four independent ways before restoring it: by parsing block
headers from a local copy, by RPC against the production node, by reindexing
the local chain, and by a full network sync. Across 213 retarget points the
chain matches `GetNextWorkRequired` exactly — zero deviations. For comparison,
Bitcoin's retarget formula deviates at 193 of those 213 points, which confirms
that Doichain really follows the Namecoin rule (`nBlocksBack = 2016`, active
from height 1 because `nAuxpowStartHeight = 1`).

**Do not comment this check out again.** If a block is rejected with
`bad-diffbits`, that is information, not an obstacle.

### powLimit leaves little headroom in the retarget arithmetic

`CalculateNextWorkRequired` computes `bnNew *= nActualTimespan` before
dividing. `arith_uint256` wraps silently on overflow.

    Doichain powLimit : 2^239
    Namecoin powLimit : 2^223
    safe threshold    : 2^233

Doichain's limit is 74x above the threshold. Measured over all 213 historical
retargets, the product never overflowed; the closest approach was 11.2% of
2^256 at height 4032, a margin of 3.2 bits. At today's difficulty the margin is
about 29 bits, so the scenario needs difficulty to collapse to near 1.

If it ever did overflow, the target would wrap to a small value, difficulty
would jump to an impossible level and the chain would stall — mainnet has no
minimum-difficulty escape (`fPowAllowMinDifficultyBlocks = false`). Mining and
validation run the same code, so nodes would stay consistent with each other.

`pow_tests` fails on this, and the failure is left in place deliberately: the
test is doing its job. This is a parameter property inherited from the original
fork, not something the port introduced.

### The chain cannot recover from a hashrate drop on its own

Measured on 2026-08-12 at height 430882:

| | last 2016 blocks | 2016 blocks ~70 days earlier |
|---|---|---|
| mean interval | 122.8 min | 12.4 min |
| median | 27.9 min | 8.3 min |
| 90th percentile | 369 min | 29 min |
| longer than 6 h | 10.2% | 0.00% |

The retarget window is counted in blocks, so when blocks slow down the window
stretches with them. The current period has produced 1474 blocks in 167 days;
the next retarget is 542 blocks and roughly 62 days away, and the adjustment is
clamped to a factor of 4 while about 16x is needed. Recovery would take three
retargets, on the order of four months, assuming hashrate does not fall
further.

This is the situation Bitcoin Cash's per-block difficulty algorithm was
designed for. Moving to ASERT (`aserti3-2d`) is under discussion; it would be a
hard fork and would also retire the overflow concern above, since
`CalculateNextWorkRequired` would no longer be used for new blocks.

### Rebranding can change cryptographic constants

The mechanical `Namecoin` -> `Doichain` rename touched two values that are not
user-visible text:

* `MESSAGE_MAGIC` in `src/common/signmessage.cpp` — the domain separator for
  signed messages. It now reads `"Doichain Signed Message:\n"`, which is what
  the production 0.20 nodes already use, so signatures stay compatible.
* The HKDF info string in `getNameSalt` — it now reads
  `"Doichain Registration Salt"`. Nothing depended on the old value, because
  that function entered the code base upstream after Doichain forked.

Both were caught only by unit tests. When rebranding again, check every string
that feeds a hash, a signature or a key derivation.

Bech32 addresses cannot be rewritten by search and replace either: the checksum
covers the human-readable part. 48 addresses in the test framework and test
vectors had to be decoded and re-encoded.

### Known defects that are carried over unchanged

`CheckNameTransaction` contains a guard meant to stop an existing DOI from
being overwritten above height 170000. It reads `coinIn.nHeight`, which is only
assigned when a name input exists, and the guard sits in the branch where there
is none — so it never fires and overwriting is always allowed. The behaviour is
kept as it is, with a `FIXME`, because changing it would change consensus.

## Building

    cmake -B build -DBUILD_GUI=OFF -DBUILD_TESTS=ON -DENABLE_WALLET=ON \
          -DENABLE_IPC=OFF -DPKG_CONFIG_EXECUTABLE=/usr/bin/pkg-config
    cmake --build build -j$(nproc)

Boost is no longer required — Bitcoin Core dropped it. GCC 11.4 on Ubuntu 22.04
compiles this fine, despite the documented requirement of GCC 12.1+.
`-DENABLE_IPC=OFF` avoids needing Cap'n Proto; decide separately whether
production builds want multiprocess support and ZMQ.

The binaries are `doichaind`, `doichain-cli`, `doichain-tx`, `doichain-util`,
`doichain-wallet` and `test_doichain`. The configuration file is
`doichain.conf` and the default data directory is `~/.doichain`, matching the
production deployment.
