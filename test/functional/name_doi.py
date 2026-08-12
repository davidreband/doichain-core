#!/usr/bin/env python3
# Copyright (c) 2026 The Doichain developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# RPC test for the Doichain-specific name_doi operation.

from test_framework.names import NameTestFramework
from test_framework.util import *


class NameDoiTest (NameTestFramework):

  def set_test_params (self):
    self.setup_clean_chain = True
    # -txindex matches how the production nodes run, and lets us look up
    # the inputs of a confirmed transaction.
    self.setup_name_test ([["-namehistory", "-txindex"]] * 2)

  def generateToOther (self, n):
    """Generate n blocks paying to the second node, so balances on the
    first node stay predictable."""
    addr = self.nodes[1].getnewaddress ()
    self.generatetoaddress (self.nodes[0], n, addr)

  def run_test (self):
    node = self.nodes[0]
    self.generate (node, 50)
    self.generateToOther (150)

    self.test_registration (node)
    self.test_update (node)
    self.test_registration_without_name_input (node)
    self.test_d_namespace_rejected (node)
    self.test_decoding_does_not_abort (node)
    self.test_pending_operations (node)
    self.test_reorg (node)

  def test_registration (self, node):
    """A name_doi on an unused name registers it."""
    self.log.info ("registering a DOI")

    txid = node.name_doi ("e/first", "value one")
    assert txid in node.getrawmempool ()

    pending = [p for p in node.name_pending () if p["name"] == "e/first"]
    assert_equal (len (pending), 1)
    assert_equal (pending[0]["op"], "name_doi")

    self.generateToOther (1)
    data = node.name_show ("e/first")
    assert_equal (data["name"], "e/first")
    assert_equal (data["value"], "value one")
    assert_equal (data["expired"], False)

    names = [n["name"] for n in node.name_list ()]
    assert "e/first" in names

  def test_update (self, node):
    """A name_doi spending the previous DOI output updates it."""
    self.log.info ("updating a DOI")

    node.name_doi ("e/first", "value two")
    self.generateToOther (1)
    assert_equal (node.name_show ("e/first")["value"], "value two")

  def test_registration_without_name_input (self, node):
    """A DOI may be registered without spending a name input.  This is the
    one consensus rule where Doichain diverges from Namecoin: mainnet block
    29966 carries three such registrations."""
    self.log.info ("registering a DOI without a name input")

    txid = node.name_doi ("e/no-input", "fresh")
    raw = node.getrawtransaction (txid, True)

    nameIns = 0
    for vin in raw["vin"]:
      prev = node.getrawtransaction (vin["txid"], True)
      spent = prev["vout"][vin["vout"]]["scriptPubKey"]
      if "nameOp" in spent:
        nameIns += 1
    assert_equal (nameIns, 0)

    self.generateToOther (1)
    assert_equal (node.name_show ("e/no-input")["value"], "fresh")

  def test_d_namespace_rejected (self, node):
    """The d/ namespace belongs to the classic Namecoin workflow and must
    not be usable through name_doi.  Note the wallet still records the
    transaction it built, so the check is that the node did not accept it."""
    self.log.info ("rejecting d/ names for name_doi")

    txid = node.name_doi ("d/forbidden", "nope")
    assert txid not in node.getrawmempool ()

    raw = node.gettransaction (txid)["hex"]
    assert_raises_rpc_error (-26, "txn-mempool-name-error",
                             node.sendrawtransaction, raw)

    # The classic workflow on d/ keeps working.
    new = node.name_new ("d/allowed")
    self.generateToOther (12)
    self.firstupdateName (0, "d/allowed", new, "classic value")
    self.generateToOther (1)
    assert_equal (node.name_show ("d/allowed")["value"], "classic value")

  def test_decoding_does_not_abort (self, node):
    """Decoding a name_doi transaction must report the operation instead of
    hitting the assert in NameOpToUniv's default branch, which would abort
    the node."""
    self.log.info ("decoding a name_doi transaction")

    txid = node.name_doi ("e/decode-me", "decoded")
    raw = node.getrawtransaction (txid, True)

    ops = [out["scriptPubKey"]["nameOp"] for out in raw["vout"]
           if "nameOp" in out["scriptPubKey"]]
    assert_equal (len (ops), 1)
    assert_equal (ops[0]["op"], "name_doi")
    assert_equal (ops[0]["name"], "e/decode-me")
    assert_equal (ops[0]["value"], "decoded")

    # The same transaction through decoderawtransaction.
    hexTx = node.getrawtransaction (txid)
    decoded = node.decoderawtransaction (hexTx)
    ops = [out["scriptPubKey"]["nameOp"] for out in decoded["vout"]
           if "nameOp" in out["scriptPubKey"]]
    assert_equal (ops[0]["op"], "name_doi")

    # The node is still alive.
    assert_equal (node.getblockcount (), node.getblockcount ())
    self.generateToOther (1)

  def test_pending_operations (self, node):
    """A second name_doi issued while the first is still pending is not
    accepted by the node: only the first stays pending and only its value
    gets mined, even though the RPC returns a txid for the second.

    This records the behaviour as it is, not as it ought to be.  Namecoin
    chains pending operations up to DEFAULT_NAME_CHAIN_LIMIT, which is 2 for
    Doichain, and name_doi is meant to do the same.  Why the replacement
    happens is still open -- most likely the wallet reuses the same funding
    input for both transactions.  See DOICHAIN_FORK_INVENTORY.md."""
    self.log.info ("checking pending name_doi operations")

    node.name_doi ("e/chained", "one")
    assert_equal (len (node.name_pending ("e/chained")), 1)

    second = node.name_doi ("e/chained", "two")
    assert second not in node.getrawmempool ()
    assert_equal (len (node.name_pending ("e/chained")), 1)

    self.generateToOther (1)
    assert_equal (node.name_show ("e/chained")["value"], "one")

  def test_reorg (self, node):
    """A DOI registration must survive being reorged out and back in, and
    the name database must stay consistent."""
    self.log.info ("reorganising a block with a name_doi")

    self.disconnect_nodes (0, 1)

    other = self.nodes[1]
    addrOther = other.getnewaddress ()

    node.name_doi ("e/reorged", "before")
    self.generatetoaddress (node, 1, addrOther, sync_fun=self.no_op)
    assert_equal (node.name_show ("e/reorged")["value"], "before")

    # The other node builds a longer chain without our transaction.
    self.generatetoaddress (other, 3, addrOther, sync_fun=self.no_op)

    self.connect_nodes (0, 1)
    self.sync_blocks ()

    # The name is gone from the chain but back in node 0's mempool, so
    # mining on node 0 brings it back.
    assert_raises_rpc_error (-4, "name never existed",
                             node.name_show, "e/reorged")
    self.generatetoaddress (node, 1, addrOther, sync_fun=self.no_op)
    self.sync_blocks ()
    assert_equal (node.name_show ("e/reorged")["value"], "before")


if __name__ == '__main__':
  NameDoiTest (__file__).main ()
