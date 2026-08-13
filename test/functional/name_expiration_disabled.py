#!/usr/bin/env python3
# Copyright (c) 2026 The Doichain developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test the Doichain rule that removes name expiration from an activation
# height onwards.
#
# The rule is tied to the height at which a name was last updated, not to the
# current height.  A name last updated below the activation height keeps
# expiring on the old schedule, so the historical chain still validates and
# names that expired before activation stay expired.  A name updated at or
# above it never expires.

from test_framework.names import NameTestFramework
from test_framework.util import *

# Chosen so that a name can be registered, expire, and a second one be
# registered past the activation height, all within a short regtest chain.
ACTIVATION = 260
EXPIRY = 30


class NameExpirationDisabledTest (NameTestFramework):

  def set_test_params (self):
    self.setup_clean_chain = True
    args = ["-namehistory", "-txindex", "-allowexpired",
            f"-nameexpirationdisabledheight={ACTIVATION}"]
    self.setup_name_test ([args] * 1)

  def mineTo (self, height):
    """Mine until the chain has reached the given height."""
    node = self.nodes[0]
    missing = height - node.getblockcount ()
    assert missing >= 0, "already past height %d" % height
    if missing:
      self.generatetoaddress (node, missing, node.getnewaddress ())

  def run_test (self):
    node = self.nodes[0]
    self.log.info ("expiration depth on regtest is %d blocks" % EXPIRY)

    self.generate (node, 200)
    assert_equal (node.getblockcount (), 200)
    assert node.getblockcount () < ACTIVATION

    # A name registered below the activation height still expires.
    self.log.info ("a name updated before activation still expires")
    node.name_doi ("e/before", "old rule")
    self.generatetoaddress (node, 1, node.getnewaddress ())
    heightBefore = node.name_show ("e/before")["height"]
    assert heightBefore < ACTIVATION
    assert_equal (node.name_show ("e/before")["expired"], False)

    self.mineTo (heightBefore + EXPIRY)
    assert_equal (node.name_show ("e/before")["expired"], True)

    # Its output is gone from the UTXO set, which is what expiration does.
    data = node.name_show ("e/before")
    assert_equal (node.gettxout (data["txid"], data["vout"]), None)

    # Past the activation height, a name does not expire any more.
    self.log.info ("a name updated after activation never expires")
    self.mineTo (ACTIVATION)
    node.name_doi ("e/after", "new rule")
    self.generatetoaddress (node, 1, node.getnewaddress ())
    heightAfter = node.name_show ("e/after")["height"]
    assert heightAfter >= ACTIVATION

    self.mineTo (heightAfter + 3 * EXPIRY)
    data = node.name_show ("e/after")
    assert_equal (data["expired"], False)
    assert_equal (data["value"], "new rule")

    # Its output is still there, so the name remains spendable.
    assert node.gettxout (data["txid"], data["vout"]) is not None

    # And it can still be updated far beyond the old expiration depth.
    node.name_doi ("e/after", "updated late")
    self.generatetoaddress (node, 1, node.getnewaddress ())
    assert_equal (node.name_show ("e/after")["value"], "updated late")

    # The name that expired before activation stays expired: the rule does
    # not reach backwards.  Reviving it would leave the name database
    # inconsistent with the UTXO set, since its output is already spent.
    self.log.info ("activation does not revive names that expired earlier")
    assert_equal (node.name_show ("e/before")["expired"], True)

    # A fresh registration of that name is possible, as it was before.
    node.name_doi ("e/before", "re-registered")
    self.generatetoaddress (node, 1, node.getnewaddress ())
    reg = node.name_show ("e/before")
    assert_equal (reg["expired"], False)
    assert_equal (reg["value"], "re-registered")

    # Having been re-registered above the activation height, it is now
    # permanent too.
    self.mineTo (node.getblockcount () + 3 * EXPIRY)
    assert_equal (node.name_show ("e/before")["expired"], False)

    # The name database stayed consistent throughout.
    self.log.info ("checking the name database")
    node.stop_node ()
    node.start (extra_args=self.extra_args[0] + ["-checknamedb=1"])
    node.wait_for_rpc_connection ()
    assert_equal (node.name_show ("e/after")["expired"], False)


if __name__ == '__main__':
  NameExpirationDisabledTest (__file__).main ()
