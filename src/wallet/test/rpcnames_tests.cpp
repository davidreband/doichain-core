// Copyright (c) 2021 yanmaani
// Licensed under CC0 (Public domain)

#include <test/util/setup_common.h>
#include <key_io.h>
#include <util/strencodings.h>
#include <wallet/rpc/walletnames.h>

#include <boost/test/unit_test.hpp>

/* The expected salts differ from Namecoin's.  getNameSalt derives them with
   HKDF using a chain-specific info string, which reads "Doichain Registration
   Salt" here.  Separating the derivation per chain is deliberate; nothing
   depends on the old values, because this function only entered the code base
   upstream after Doichain forked.  */

BOOST_FIXTURE_TEST_SUITE(rpcnames_tests, RegTestingSetup) // Keys are in regtest format

static void
TestNameSalt(const std::string& privkey_b58, const std::string& name_str, const std::string& expectedSalt_hex) {
    const CKey privkey = DecodeSecret(privkey_b58);
    const valtype name(name_str.begin(), name_str.end());
    const valtype expectedSalt = ParseHex(expectedSalt_hex);

    valtype salt(20);
    wallet::getNameSalt(privkey, name, salt);

    BOOST_CHECK(salt == expectedSalt);
}

BOOST_AUTO_TEST_CASE(name_salts)
{
    SelectParams(ChainType::REGTEST);
    TestNameSalt( // test_name_salt_addr_p2pkh
                /* private key   */ "cQDxbmQfwRV3vP1mdnVHq37nJekHLsuD3wdSQseBRA2ct4MFk5Pq",
                /* name          */ "d/wikileaks",
                /* expected salt */ "e199a1dacadaf290f689a8272df668dfa073e723");
    TestNameSalt( // test_name_salt_addr_p2wpkh_p2sh
                /* private key   */ "cU9hVzhpvfn91u2zTVn8uqF2ymS7ucYH8V5TmsTDmuyMHgRk9WsJ",
                /* name          */ "d/wikileaks",
                /* expected salt */ "6d32e18f1070e4da48c186968b528e29c004900b");
    TestNameSalt( // test_name_salt_addr_p2wpkh
                /* private key   */ "cPuQzcNEgbeYZ5at9VdGkCwkPA9r34gvEVJjuoz384rTfYpahfe7",
                /* name          */ "d/wikileaks",
                /* expected salt */ "01843a6c55977e347bdf997b58487ceccb5994f4");
}

BOOST_AUTO_TEST_SUITE_END()
