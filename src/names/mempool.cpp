// Copyright (c) 2014-2024 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <names/mempool.h>

#include <coins.h>
#include <logging.h>
#include <names/encoding.h>
#include <script/names.h>
#include <txmempool.h>
#include <util/strencodings.h>
#include <validation.h>

/* ************************************************************************** */

unsigned
CNameMemPool::pendingChainLength (const valtype& name) const
{
  unsigned res = 0;
  if (registersName (name))
    ++res;

  const auto mit = updates.find (name);
  if (mit != updates.end ())
    res += mit->second.size ();

  return res;
}

namespace
{

/**
 * Returns the outpoint matching the name operation in a given mempool tx, if
 * there is any.  The txid must be for an entry in the mempool.
 */
COutPoint
getNameOutput (const CTxMemPool& pool, const Txid& txid)
{
  AssertLockHeld (pool.cs);

  const auto mit = pool.mapTx.find (txid);
  assert (mit != pool.mapTx.end ());
  const auto& vout = mit->GetTx ().vout;

  for (unsigned i = 0; i != vout.size (); ++i)
    {
      const CNameScript nameOp(vout[i].scriptPubKey);
      if (nameOp.isNameOp ())
        return COutPoint (txid, i);
    }

  return COutPoint ();
}

} // anonymous namespace

COutPoint
CNameMemPool::lastNameOutput (const valtype& name) const
{
  AssertLockHeld (pool.cs);

  const auto itUpd = updates.find (name);
  if (itUpd != updates.end ())
    {
      /* From all the pending updates, we have to find the last one.  This is
         the unique outpoint that is not also spent by some other transaction.
         Thus, we keep track of all the transactions spent as well, and then
         remove those from the sets of candidates.  Doing so by txid (rather
         than outpoint) is enough, as those transactions must be in a "chain"
         anyway.  */

      const std::set<Txid>& candidateTxids = itUpd->second;
      std::set<Txid> spentTxids;

      for (const auto& txid : candidateTxids)
        {
          const auto mit = pool.mapTx.find (txid);
          assert (mit != pool.mapTx.end ());
          for (const auto& in : mit->GetTx ().vin)
            spentTxids.insert (in.prevout.hash);
        }

      COutPoint res;
      for (const auto& txid : candidateTxids)
        {
          if (spentTxids.count (txid) > 0)
            continue;

          assert (res.IsNull ());
          res = getNameOutput (pool, txid);
        }

      assert (!res.IsNull ());
      return res;
    }

  const auto itReg = mapNameRegs.find (name);
  if (itReg != mapNameRegs.end ())
    return getNameOutput (pool, itReg->second);

  return COutPoint ();
}

void
CNameMemPool::addUnchecked (const CTxMemPoolEntry& entry)
{
  AssertLockHeld (pool.cs);

  const Txid& txHash = entry.GetTx ().GetHash ();
  if (entry.isNameNew ())
    {
      const valtype& newHash = entry.getNameNewHash ();
      const auto mit = mapNameNews.find (newHash);
      if (mit != mapNameNews.end ())
        assert (mit->second == txHash);
      else
        mapNameNews.insert (std::make_pair (newHash, txHash));
    }

  if (entry.isNameRegistration ())
    {
      const valtype& name = entry.getName ();
      assert (mapNameRegs.count (name) == 0);
      mapNameRegs.insert (std::make_pair (name, txHash));
    }

  if (entry.isNameUpdate () || entry.isNameDoi ())
    {
      const valtype& name = entry.getName ();
      const auto mit = updates.find (name);

      if (mit == updates.end ())
        updates.emplace (name, std::set<Txid> ({txHash}));
      else
        mit->second.insert (txHash);
    }

  if (entry.isNameDoi ())
    {
      const valtype& name = entry.getName ();
      const auto mit = mapNameDois.find (name);

      if (mit == mapNameDois.end ())
        mapNameDois.emplace (name, std::set<Txid> ({txHash}));
      else
        mit->second.insert (txHash);
    }
}

void
CNameMemPool::remove (const CTxMemPoolEntry& entry)
{
  AssertLockHeld (pool.cs);

  if (entry.isNameRegistration ())
    {
      const auto mit = mapNameRegs.find (entry.getName ());
      assert (mit != mapNameRegs.end ());
      mapNameRegs.erase (mit);
    }

  if (entry.isNameUpdate () || entry.isNameDoi ())
    {
      const auto itName = updates.find (entry.getName ());
      assert (itName != updates.end ());
      auto& txids = itName->second;
      const auto itTxid = txids.find (entry.GetTx ().GetHash ());
      assert (itTxid != txids.end ());
      txids.erase (itTxid);
      if (txids.empty ())
        updates.erase (itName);
    }

  if (entry.isNameDoi ())
    {
      const auto itName = mapNameDois.find (entry.getName ());
      assert (itName != mapNameDois.end ());
      auto& txids = itName->second;
      const auto itTxid = txids.find (entry.GetTx ().GetHash ());
      assert (itTxid != txids.end ());
      txids.erase (itTxid);
      if (txids.empty ())
        mapNameDois.erase (itName);
    }
}

void
CNameMemPool::removeConflicts (const CTransaction& tx)
{
  AssertLockHeld (pool.cs);

  if (!tx.IsNamecoin ())
    return;

  for (const auto& txout : tx.vout)
    {
      const CNameScript nameOp(txout.scriptPubKey);
      if (nameOp.isNameOp () && nameOp.getNameOp () == OP_NAME_FIRSTUPDATE)
        {
          const valtype& name = nameOp.getOpName ();
          const auto mit = mapNameRegs.find (name);
          if (mit != mapNameRegs.end ())
            {
              const auto mit2 = pool.mapTx.find (mit->second);
              assert (mit2 != pool.mapTx.end ());
              pool.removeRecursive (mit2->GetTx (),
                                    MemPoolRemovalReason::NAME_CONFLICT);
            }
        }
    }
}

void
CNameMemPool::removeUnexpireConflicts (const std::set<valtype>& unexpired)
{
  AssertLockHeld (pool.cs);

  for (const auto& name : unexpired)
    {
      LogDebug (BCLog::NAMES, "unexpired: %s, mempool: %u\n",
                EncodeNameForMessage (name), mapNameRegs.count (name));

      const auto mit = mapNameRegs.find (name);
      if (mit != mapNameRegs.end ())
        {
          const CTxMemPool::txiter mit2 = pool.mapTx.find (mit->second);
          assert (mit2 != pool.mapTx.end ());
          pool.removeRecursive (mit2->GetTx (),
                                MemPoolRemovalReason::NAME_CONFLICT);
        }
    }
}

void
CNameMemPool::removeExpireConflicts (const std::set<valtype>& expired)
{
  AssertLockHeld (pool.cs);

  for (const auto& name : expired)
    {
      LogDebug (BCLog::NAMES, "expired: %s\n", EncodeNameForMessage (name));

      const auto mit = updates.find (name);
      if (mit == updates.end ())
        continue;

      /* We need to make sure that we keep our copy of txids even when the
         transactions are removed one by one.  */
      const std::set<Txid> txidsCopy = mit->second;

      for (const auto& txid : txidsCopy)
        {
          const CTxMemPool::txiter mit2 = pool.mapTx.find (txid);
          assert (mit2 != pool.mapTx.end ());
          pool.removeRecursive (mit2->GetTx (),
                                MemPoolRemovalReason::NAME_CONFLICT);
        }

      assert (updates.count (name) == 0);
    }
}

void
CNameMemPool::check (const CCoinsViewCache& tip,
                     const int64_t spendheight) const
{
  AssertLockHeld (pool.cs);

  std::set<valtype> nameRegs;
  std::map<valtype, unsigned> nameUpdates;
  std::map<valtype, unsigned> nameDois;
  for (const auto& entry : pool.mapTx)
    {
      const Txid txHash = entry.GetTx ().GetHash ();
      if (entry.isNameNew ())
        {
          const valtype& newHash = entry.getNameNewHash ();
          const auto mit = mapNameNews.find (newHash);

          assert (mit != mapNameNews.end ());
          assert (mit->second == txHash);
        }

      if (entry.isNameRegistration ())
        {
          const valtype& name = entry.getName ();

          const auto mit = mapNameRegs.find (name);
          assert (mit != mapNameRegs.end ());
          assert (mit->second == txHash);

          assert (nameRegs.count (name) == 0);
          nameRegs.insert (name);

          /* The old name should be expired already.  */
          CNameData data;
          if (tip.GetName (name, data))
            assert (data.isExpired (spendheight));
        }

      if (entry.isNameUpdate () || entry.isNameDoi ())
        {
          const valtype& name = entry.getName ();

          const auto mit = updates.find (name);
          assert (mit != updates.end ());
          assert (mit->second.count (txHash) > 0);

          ++nameUpdates[name];

          if (entry.isNameDoi ())
            {
              const auto mitDoi = mapNameDois.find (name);
              assert (mitDoi != mapNameDois.end ());
              assert (mitDoi->second.count (txHash) > 0);

              ++nameDois[name];

              /* No check against the name database here.  A DOI operation is
                 its own registration, and registering a name whose earlier
                 incarnation has expired is allowed, so an expired entry may
                 legitimately be present.  */
            }
          else
            {
              /* A plain NAME_UPDATE requires the name to exist and be
                 unexpired, or else to be brought into existence by the
                 mempool.  That can happen through a classic name_firstupdate
                 or through a pending DOI operation -- consensus lets an
                 update chain onto a pending name output either way, so both
                 have to be accepted here.  */
              CNameData data;
              if (tip.GetName (name, data))
                assert (!data.isExpired (spendheight));
              else
                assert (registersName (name) || registersDoi (name));
            }
        }
    }

  assert (nameRegs.size () == mapNameRegs.size ());
  assert (nameUpdates.size () == updates.size ());
  assert (nameDois.size () == mapNameDois.size ());
  for (const auto& upd : nameUpdates)
    assert (updates.at (upd.first).size () == upd.second);
  for (const auto& doi : nameDois)
    assert (mapNameDois.at (doi.first).size () == doi.second);
}

bool
CNameMemPool::checkTx (const CTransaction& tx) const
{
  AssertLockHeld (pool.cs);

  if (!tx.IsNamecoin ())
    return true;

  for (const auto& txout : tx.vout)
    {
      const CNameScript nameOp(txout.scriptPubKey);
      if (!nameOp.isNameOp ())
        continue;

      switch (nameOp.getNameOp ())
        {
        case OP_NAME_NEW:
          {
            const valtype& newHash = nameOp.getOpHash ();
            std::map<valtype, Txid>::const_iterator mi;
            mi = mapNameNews.find (newHash);
            if (mi != mapNameNews.end () && mi->second != tx.GetHash ())
              return false;
            break;
          }

        case OP_NAME_FIRSTUPDATE:
          {
            const valtype& name = nameOp.getOpName ();
            if (registersName (name))
              return false;
            break;
          }

        case OP_NAME_UPDATE:
          /* Multiple updates of the same name in a chain are perfectly fine.
             The main mempool logic takes care that updates are ordered
             properly and really a chain, as this is automatic due to the
             coloured-coin nature of names.  */
          break;

        case OP_NAME_DOI:
          {
            /* DOI operations chain in exactly the same way.  But the d/
               namespace belongs to the classic name_new / name_firstupdate
               workflow and must not be used with name_doi.

               The name is compared as raw bytes.  The original Doichain code
               compared it against EncodeNameForMessage(), which wraps the name
               in single quotes, so the comparison could never match and d/
               names were accepted for name_doi regardless.  */
            static const valtype dPrefix{'d', '/'};
            const valtype& name = nameOp.getOpName ();
            if (name.size () >= dPrefix.size ()
                && std::equal (dPrefix.begin (), dPrefix.end (), name.begin ()))
              return false;
            break;
          }

        default:
          assert (false);
        }
    }

  return true;
}
