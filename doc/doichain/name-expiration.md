# Removing name expiration

Status: implemented, not activated on any live network.

Namecoin expires names so that abandoned domains return to the pool. Doichain
inherited that rule unchanged. For records that carry an identity rather than a
contested domain name it is the wrong behaviour, and the numbers on the live
chain show it: of 57 742 names, **57 699 have expired**. Only 43 survive, all
of them updated within the last 36 000 blocks.

This change lets names stop expiring from an activation height onwards. It is a
hard fork and no height has been chosen yet, so on every network the parameter
is set to "not activated" and behaviour is unchanged.

## The rule

    Names whose last update is at or above nNoNameExpirationSince never expire.

The condition is deliberately tied to **the name's own update height**, not to
the current chain height. Saying instead "from height N nothing expires" would
mark all 57 699 already-expired names as alive again — while their outputs have
long been spent by `ExpireNames`. `ValidateNameDB` cross-checks live names in
the database against the UTXO set, so the node would fail on the next
consistency check.

Tying the rule to the update height keeps the past exactly as it is:

* a name last updated below the activation height expires on the old schedule,
  so every historical block still validates,
* a name that expired before activation stays expired,
* a name updated at or above the activation height never expires,
* re-registering an expired name works as before, and because that
  re-registration happens above the activation height the new incarnation is
  permanent.

## What was changed

**`src/consensus/params.h`** — new parameter:

    unsigned nNoNameExpirationSince;

The maximum value means "not activated" and is what every network is set to in
`src/kernel/chainparams.cpp`.

**`src/names/main.cpp`, `isExpired`** — the rule itself, an early return before
the usual `nPrevHeight + NameExpirationDepth(nHeight) <= nHeight` arithmetic.

**`src/names/main.cpp`, `ExpireNames`** — the second half of the rule, and the
part that is easy to miss. `ExpireNames` collects names by their update height
over the range `[nHeight - depthOld, nHeight - depthNow]` and spends their
outputs. Left alone it would spend the outputs of names the new rule considers
permanent, and the name database would part company with the UTXO set. The
upper bound of the range is now clamped below the activation height; if the
range collapses, nothing expires.

**`src/rpc/names.cpp`, `addExpirationInfo`** — `name_show` and friends computed
`expired` from their own arithmetic rather than from the consensus rule, so a
permanent name was still reported as expired. It now returns `expired: false`
for such names and omits `expires_in`, which has no meaning for them. The field
is declared optional in the RPC result.

**`src/chainparamsbase.cpp`, `src/chainparams.cpp`, `src/kernel/chainparams.h`** —
a regtest-only `-nameexpirationdisabledheight=<height>` for the tests, carried
through `RegTestOptions` the same way `-testactivationheight` is.

## A defect this uncovered

`CNameMemPool::check` asserted `!data.isExpired(spendheight)` for a pending DOI
operation, copied from the `NAME_UPDATE` branch. But a DOI operation is both a
registration and an update, and registering a name whose earlier incarnation
has expired is allowed. Any such transaction entering the mempool aborted the
node.

This is not a consequence of the change here — the same path exists without it.
It had simply never been exercised: `name_doi` had no tests at all until
recently, and nobody had re-registered an expired name by hand. The assert is
now the bookkeeping check that is meaningful for DOI.

## Two things to know if you touch this

The argument was first called `-nonameexpirationsince` and the parser rejected
it while the help text listed it. `InterpretKey` reads a leading `no` as
negation, so it was parsed as the negation of a `-nameexpirationsince` that
does not exist. Avoid argument names starting with `no`.

The consensus part worked from the first attempt; what looked like a failure
was the RPC reporting. When something appears not to take effect, check whether
the output you are reading is computed by the rule or beside it — `gettxout` on
the name output tells you what consensus actually did.

## Decisions still open

**Scope.** The rule currently applies to every name. Nothing on this chain
distinguishes them: sampling each namespace showed that `bp/`, `e/`, `pe/` and
the unprefixed names were all created with `name_doi`, and the classic
`name_new` plus `name_firstupdate` path is used by exactly one name. So
"DOI names only" and "all names" are the same set here. If a narrower scope is
wanted it has to key on the namespace prefix instead.

**Activation height.** Blocks below it keep the old rule, which is what makes
the historical chain still validate.

**The cost.** Name outputs will never be released again, so the UTXO set grows
monotonically. For scale: the `bp/` namespace alone registered 45 702 names
across 76 000 blocks, roughly 0.6 names per block.

## Tests

`test/functional/name_expiration_disabled.py` covers the rule end to end: a
name registered below the activation height expires and its output leaves the
UTXO set; a name registered above it survives three expiration periods with its
output intact and can still be updated; activation does not revive what expired
earlier; re-registering such a name works and the new incarnation is permanent;
and the node is restarted with `-checknamedb=1` at the end to confirm the name
database stayed consistent.

The inherited `name_expiration.py` must keep passing — it exercises the old
rule, which is still what runs when the parameter is not activated.
