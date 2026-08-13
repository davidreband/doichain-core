# Open consensus work

Three questions are open, and two of them are hard forks. They should activate
at one height, in one coordinated upgrade — asking miners and node operators to
coordinate twice for changes that are ready at the same time is wasted effort.

Nothing here is scheduled yet. This file records what was measured and what has
to be decided, so the analysis does not have to be redone.

## 1. Remove name expiration -- implemented, not activated

### Where it stands today

`Consensus::MainNetConsensus::NameExpirationDepth` in `src/consensus/params.h`:

    if (nHeight < 24000) return 12000;
    if (nHeight < 48000) return nHeight - 12000;
    return 36000;

36000 blocks is about 250 days at the intended ten-minute spacing. The values
are Namecoin's; Doichain never changed them, and they are identical to the 0.20
code running in production.

**DOI names expire like any other name.** `isAnyUpdate()` returns true for
`OP_NAME_DOI`, so a DOI is written into the name database exactly like a
Namecoin name, and `ExpireNames` treats it the same: at the expiration height
it spends the name output, and the 0.01 DOI locked in it goes with it. For
records meant to carry an identity this is probably not the intended
behaviour — which is what this task is about.

There is an asymmetry worth knowing before touching anything. `NAME_UPDATE`
refuses to update an expired name:

    if (oldName.isExpired (nHeight))
        return state.Invalid (..., "tx-nameupdate-expired", ...);

`NAME_DOI` has no such check. The 0.20 code had one, commented out; it was
removed as dead code during the port. So updating an expired DOI is allowed in
principle — the name just does not survive long enough for it to matter,
because `ExpireNames` has already spent its output.

**The rule is implemented** on the `doichain/consensus` branch and described in
[name-expiration.md](name-expiration.md).  It is switched off on every network
until an activation height is chosen.

Sampling the live chain answered the scope question by itself: `bp/`, `e/`,
`pe/` and the unprefixed names were all created with `name_doi`, and the
classic `name_new` plus `name_firstupdate` path is used by exactly one name.
"DOI names only" and "all names" therefore describe the same set here.

### What has to be decided

**Scope.** Drop expiration for DOI names only, or for every name including the
`d/` namespace? Dropping it only for DOI keeps the `d/` namespace behaving the
way Namecoin does, and makes the rule a property of the operation — easier to
reason about and to test. Dropping it for everything is simpler in the code but
diverges from Namecoin across the whole name system.

**Names that have already expired.** Leave them expired, or bring them back?
Reviving them means the expired outputs have to come back into the UTXO set,
which is not something an activation height alone can express.

**Activation height.** Blocks below it keep the current rule, otherwise the
historical chain stops validating — and it does validate today, all 430881
blocks of it.

### What to touch

`NameExpirationDepth` decides the rule; `ExpireNames` and `UnexpireNames` in
`src/names/main.cpp` act on it, together with the `DB_NAME_EXPIRY` index in
`src/txdb.cpp`. On the surface, `name_show` reports `expired`, `name_scan` and
the `-allowexpired` option filter on it, and the wallet's `name_list` shows it.

## 2. Per-block difficulty adjustment (Bitcoin Cash style)

### Why

Measured on 2026-08-12 at height 430882:

| | last 2016 blocks | 2016 blocks ~70 days earlier |
|---|---|---|
| mean interval | 122.8 min | 12.4 min |
| median | 27.9 min | 8.3 min |
| 90th percentile | 369 min | 29 min |
| longer than 6 h | 10.2% | 0.00% |

The chain held its ten-minute target almost exactly until roughly ten weeks
ago. It cannot recover on its own: the retarget window is counted in blocks, so
when blocks slow down the window stretches with them. The current period has
produced 1474 blocks in 167 days, the next retarget is 542 blocks and about 62
days away, and one adjustment is clamped to a factor of 4 while about 16x is
needed. Three retargets, on the order of four months — if hashrate does not
fall further.

Doichain is merge-mined, so its hashrate belongs to someone else and leaves
without notice. That is exactly the case a fast-reacting algorithm is for.

### Which algorithm

Bitcoin Cash went through two. `cw-144`, a sliding window over 144 blocks,
worked but oscillated, and miners learned to switch back and forth across the
oscillation. **ASERT (`aserti3-2d`)**, active since November 2020, replaced it:

    target = anchor_target · 2^((t − t_anchor − ideal·(h − h_anchor)) / half_life)

It is stateless — it depends only on an anchor block and the current height and
time — so it accumulates no error and does not oscillate. Prefer it. Bitcoin
Cash publishes test vectors for `aserti3-2d`, which takes most of the risk out
of the implementation.

### What has to be decided

**Half-life.** Bitcoin Cash uses two days. At ten-minute blocks that is 288
blocks, a reasonable starting point.

**How activation is expressed.** By height or by time. With blocks currently
taking over two hours, "in 2000 blocks" is half a year, so height may be the
wrong unit here.

**Anchor block.** Conventionally the block one below the activation height,
with its `nBits` and timestamp.

### Side effects

The `powLimit` question below disappears for new blocks:
`CalculateNextWorkRequired` would no longer be reached above the activation
height.

Nothing outside the node needs to change. p2pool takes `bits` from
`createauxblock` and passes it through; ElectrumX and the dApp do not compute
difficulty.

## 3. Decide what to do about powLimit

`CalculateNextWorkRequired` multiplies before it divides, and `arith_uint256`
wraps silently:

    Doichain powLimit : 2^239
    Namecoin powLimit : 2^223
    safe threshold    : 2^233

Doichain's limit is 74x above the threshold. Measured across all 213 historical
retargets the product never overflowed; the closest was 11.2% of 2^256 at
height 4032, a margin of 3.2 bits. At today's difficulty the margin is about 29
bits, so reaching it needs difficulty to fall back to nearly 1.

If it did overflow, the target would wrap to a small value, difficulty would
jump out of reach and the chain would stall — mainnet has no minimum-difficulty
escape. Mining and validation share the code, so nodes would agree with each
other while doing it.

`pow_tests` fails on this deliberately. Three ways out: accept the risk and
record it, widen the intermediate arithmetic, or let task 2 make the question
moot for new blocks. The third is free if ASERT lands.

## Note on sequencing

Tasks 1 and 2 are both hard forks and should share one activation height.
Task 3 needs no separate decision if task 2 goes ahead, beyond keeping the old
rule intact for historical blocks.

Whatever is chosen, the acceptance test stays the same as for the port itself:
a node built from the change must revalidate the whole chain and end on the
same block hash as the production nodes. See [testing.md](testing.md).
