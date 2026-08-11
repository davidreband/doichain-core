// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const CScript& genesisInputScript, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = genesisInputScript;
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "USA überraschen Europa mit einem Gesetz zur Online-Durchsuchung";
    const CScript genesisInputScript = CScript() << 0x1f00ffff << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    const CScript genesisOutputScript = CScript() << ParseHex("042f413ffbe86df73d195832efd61003fe4e7f6c061e75afa06365928fa649dd4a408c4029ca98c9a0bfda2b99e53c587c95d566375036c7f85be51b5a4e150118") << OP_CHECKSIG;
    return CreateGenesisBlock(genesisInputScript, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Build genesis block for testnet.  In Doichain, it has a changed timestamp
 * and output script (it uses Bitcoin's).
 */
static CBlock CreateTestnetGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Mark Zuckerberg und Apple-Chef Tim Cook gehen aufeinander los";
    const CScript genesisInputScript = CScript() << 0x1d00ffff << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    const CScript genesisOutputScript = CScript() << ParseHex("042f413ffbe86df73d195832efd61003fe4e7f6c061e75afa06365928fa649dd4a408c4029ca98c9a0bfda2b99e53c587c95d566375036c7f85be51b5a4e150118") << OP_CHECKSIG;
    return CreateGenesisBlock(genesisInputScript, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 0;
        /* Note that these are not the actual activation heights, but blocks
           after them.  They are too deep in the chain to be ever reorged,
           and thus this is also fine.  */
        consensus.BIP34Height = 100000000;
        consensus.BIP34Hash =  	uint256S("0x1414f096a62bee2501807a9a2a97c20557df1fdfb1284f9eaff88d60defe0750");
        consensus.BIP65Height = 130000;
        consensus.BIP66Height = 130000;
        /* Doichain activates CSV/Segwit with BIP16.  */
        consensus.CSVHeight = 216500;
        consensus.SegwitHeight = 216500;
        consensus.MinBIP9WarningHeight = 218500; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT; // December 31, 2008

        // The best chain should have at least this much work.
        // The value is the chain work of the Doichain mainnet chain at height
        // 500'000, with best block hash:
        // 42c5147b7204a00ad0e56f167ed52e214da623a5459261db32d6da1687d601d6
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000ba50a60f8b56c7fe0");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00006b0bc91e80054369e07b330c243bf32f4b97f66da976756265b81a9b5828"); //250000 500'000

        consensus.nAuxpowChainId = 0x0002;
        consensus.nAuxpowStartHeight = 1;
        consensus.fStrictChainId = false;
        consensus.nLegacyBlocksBefore = 1;

        consensus.rules.reset(new Consensus::MainNetConsensus());

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf8;
        pchMessageStart[1] = 0xb2;
        pchMessageStart[2] = 0xb2;
        pchMessageStart[3] = 0xff;
        nDefaultPort = 8338;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 6;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1522756358, 77495, 0x1f00ffff, 1, 50 * COIN);


        //bnProofOfWorkLimit = ~arith_uint256(0) >> 32;	//        consensus.hashGenesisBlock = uint256S("0x");

//
       /*consensus.hashGenesisBlock = uint256S("0x01");	//                 // calculate Genesis Block
       if (true && genesis.GetHash() != consensus.hashGenesisBlock)	//                 // Reset genesis
                {	//                 consensus.hashGenesisBlock = uint256S("0x");
                    printf("recalculating params for mainnet.\n");	//                 std::cout << std::string("Begin calculating Mainnet Genesis Block:\n");
                    printf("old mainnet genesis nonce: %u\n", genesis.nNonce);	//                 if (true && (genesis.GetHash() != consensus.hashGenesisBlock)) {
                    printf("old mainnet genesis hash:  %u\n", consensus.hashGenesisBlock);	//                     LogPrintf("Calculating Mainnet Genesis Block:\n");
                    // deliberately empty for loop finds nonce value.	//                     arith_uint256 hashTarget = arith_uint256().SetCompact(genesis.nBits);
                    for(genesis.nNonce == 0; UintToArith256(genesis.GetHash()) >  bnProofOfWorkLimit; genesis.nNonce++){ }	//                     uint256 hash;
                    printf("new mainnet genesis merkle root: %s\n", genesis.hashMerkleRoot.ToString().c_str());	//                     genesis.nNonce = 0;
                    printf("new mainnet genesis nonce: %u\n", genesis.nNonce);	//                     // This will figure out a valid hash and Nonce if you're
                    printf("new mainnet genesis hash: %s\n", genesis.GetHash().ToString().c_str());	//                     // creating a different genesis block:
                }*/	//                     // uint256 hashTarget = CBigNum().SetCompact(genesis.nBits).getuint256();

//                     // hashTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow).getuint256();
     /*   consensus.hashGenesisBlock = uint256S("0x");	//                     // while (genesis.GetHash() > hashTarget)
//                     while (UintToArith256(genesis.GetHash()) > hashTarget)
                 // calculate Genesis Block	//                     {
                 // Reset genesis	//                         ++genesis.nNonce;
                 consensus.hashGenesisBlock = uint256S("0x");	//                         if (genesis.nNonce == 0)
                 std::cout << std::string("Begin calculating Mainnet Genesis Block:\n");	//                         {
                 if (true && (genesis.GetHash() != consensus.hashGenesisBlock)) {	//                             LogPrintf("NONCE WRAPPED, incrementing time");
                     LogPrintf("Calculating Mainnet Genesis Block:\n");	//                             std::cout << std::string("NONCE WRAPPED, incrementing time:\n");
                     arith_uint256 hashTarget = arith_uint256().SetCompact(genesis.nBits);	//                             ++genesis.nTime;
                     uint256 hash;	//                         }
                     genesis.nNonce = 0;	//                         if (genesis.nNonce % 10000 == 0)
                     // This will figure out a valid hash and Nonce if you're	//                         {
                     // creating a different genesis block:	//                             LogPrintf("Mainnet: nonce %08u: hash = %s \n", genesis.nNonce, genesis.GetHash().ToString().c_str());
                     // uint256 hashTarget = CBigNum().SetCompact(genesis.nBits).getuint256();	//                             // std::cout << strNetworkID << " nonce: " << genesis.nNonce << " time: " << genesis.nTime << " hash: " << genesis.GetHash().ToString().c_str() << "\n";
                     // hashTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow).getuint256();	//                         }
                     // while (genesis.GetHash() > hashTarget)	//                     }
                     while (UintToArith256(genesis.GetHash()) > hashTarget)	//                     std::cout << "Mainnet ---\n";
                     {	//                     std::cout << "  nonce: " << genesis.nNonce <<  "\n";
                         ++genesis.nNonce;	//                     std::cout << "   time: " << genesis.nTime << "\n";
                         if (genesis.nNonce == 0)	//                     std::cout << "   hash: " << genesis.GetHash().ToString().c_str() << "\n";
                         {	//                     std::cout << "   merklehash: "  << genesis.hashMerkleRoot.ToString().c_str() << "\n";
                             LogPrintf("NONCE WRAPPED, incrementing time");	//                     // Mainnet --- nonce: 296277 time: 1390095618 hash: 000000bdd771b14e5a031806292305e563956ce2584278de414d9965f6ab54b0
                             std::cout << std::string("NONCE WRAPPED, incrementing time:\n");	//                 }
                             ++genesis.nTime;	//                 std::cout << std::string("Finished calculating Mainnet Genesis Block:\n");
                         }	//
                         */
        consensus.hashGenesisBlock = genesis.GetHash();

        //printf("Block: %s\n", genesis.GetHash().GetHex().c_str());
        //printf("hashMerkleRoot: %s\n", genesis.hashMerkleRoot.GetHex().c_str());
        assert(consensus.hashGenesisBlock == uint256S("000006fdd8b4d786fd9bdde5bae9486c464e3aa4336c5f8415dfdd3fc1679134"));
        assert(genesis.hashMerkleRoot == uint256S("234651063df5f8b01ecc2fc3a134fa1cb9dc9da9cce0149049483ba1b1469dfb "));	        assert(genesis.hashMerkleRoot == uint256S("234651063df5f8b01ecc2fc3a134fa1cb9dc9da9cce0149049483ba1b1469dfb"));


        /*	     
Mainnet ---	Mainnet ---
  nonce: 81850	  nonce: 77495
   time: 1522426764	   time: 1522756358
   hash: 00006b0bc91e80054369e07b330c243bf32f4b97f66da976756265b81a9b5828	   hash: 000006fdd8b4d786fd9bdde5bae9486c464e3aa4336c5f8415dfdd3fc1679134
   merklehash: 2e32519b6d57327bfc9c7244068dc5405e6302c0a597324404427a3389ed5b9f	   merklehash: 234651063df5f8b01ecc2fc3a134fa1cb9dc9da9cce0149049483ba1b1469dfb
Finished calculating Mainnet Genesis Block:	Finished calculating Mainnet Genesis Block:
Block: 00006b0bc91e80054369e07b330c243bf32f4b97f66da976756265b81a9b5828	Block: 000006fdd8b4d786fd9bdde5bae9486c464e3aa4336c5f8415dfdd3fc1679134
hashMerkleRoot: 2e32519b6d57327bfc9c7244068dc5405e6302c0a597324404427a3389ed5b9f	hashMerkleRoot: 234651063df5f8b01ecc2fc3a134fa1cb9dc9da9cce0149049483ba1b1469dfb
*/	

        //vSeeds.emplace_back("seeder.doichain.org");
        vSeeds.emplace_back("dnsseed.doichain.org");
        vSeeds.emplace_back("seed.doi.works");
      
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,52);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,13);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,180);
        /* FIXME: Update these below.  */
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "dc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {  0, uint256S("000006fdd8b4d786fd9bdde5bae9486c464e3aa4336c5f8415dfdd3fc1679134")}
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 42c5147b7204a00ad0e56f167ed52e214da623a5459261db32d6da1687d601d6
            1522756358,          // * UNIX timestamp of last known number of transactions
            0, // * UNIX timestamp of last known number of transactions	            0, // * total number of transactions between genesis and that timestamp
            1    // * total number of transactions between genesis and that timestamp	            //   (the tx=... number in the SetBestChain debug.log lines)
                        //   (the tx=... number in the SetBestChain debug.log lines)	            0 // * estimated number of transactions per second after checkpoint
        };
}

    int DefaultCheckNameDB () const
    {
        return -1;
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 1000;
        /* As before, these are not the actual activation heights but some
           blocks after them.  */
        consensus.BIP34Height = 130000;
        consensus.BIP34Hash = uint256S("0xe0a05455d89a54bb7c1b5bb785d6b1b7c5bda42ed4ce8dc19d68652ba8835954");
        consensus.BIP65Height = 1000;
        consensus.BIP66Height = 1000;
        /* Doichain activates CSV/Segwit with BIP16.  */
        consensus.CSVHeight = 1000;
        consensus.SegwitHeight = 1000;
        consensus.MinBIP9WarningHeight = 1000; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.nMinDifficultySince = 4500; // 15 Mar 2014
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        // The value is the chain work of the Doichain testnet chain at height
        // 233,000, with best block hash:
        // bc66fc22b8a2988bdc519c4c6aa431bb57201e5102ad8b8272fcde2937b4d2f7
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000001c71");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000cd7572b3ecc78b7cddf49eda95e718d4df77c236ca2e375125e111e7e9c4"); // 233,100

        consensus.nAuxpowStartHeight = 1;
        consensus.nAuxpowChainId = 0x0003;
        consensus.fStrictChainId = false;
        consensus.nLegacyBlocksBefore = 1;

        consensus.rules.reset(new Consensus::TestNetConsensus());

        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xba;
        pchMessageStart[2] = 0xb2;
        pchMessageStart[3] = 0xfb;

        nDefaultPort = 18338;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateTestnetGenesisBlock(1522756358, 6658, 0x1f08ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0000cd7572b3ecc78b7cddf49eda95e718d4df77c236ca2e375125e111e7e9c4"));
        assert(genesis.hashMerkleRoot == uint256S("8de06f9a125793c3b6bfe7e3bc473ba2bb505b234af5d7e999bda03ed3f4ac34"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        //vSeeds.emplace_back("dnsseed.test.doichain.webbtc.com");
        //vSeeds.emplace_back("ncts.roanapur.info");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        /* FIXME: Update these below.  */
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "td";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                    {  0, uint256S("0000cd7572b3ecc78b7cddf49eda95e718d4df77c236ca2e375125e111e7e9c4")}
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 bc66fc22b8a2988bdc519c4c6aa431bb57201e5102ad8b8272fcde2937b4d2f7
            /* nTime    */ 1522756358,
            /* nTxCount */ 173446,
            /* dTxRate  */ 0.0027,
        };

        assert(mapHistoricBugs.empty());
    }

    int DefaultCheckNameDB () const
    {
        return -1;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nMinDifficultySince = 0;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 0x0001;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 0;

        consensus.rules.reset(new Consensus::RegTestConsensus());

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18445;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateTestnetGenesisBlock(1296688602, 0, 0x207fffff, 1, 50 * COIN);

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0231881e96d6690eb00bb69cd8e221df3564e2cd95829d47d131ed5110a34e9d"));
        assert(genesis.hashMerkleRoot == uint256S("8de06f9a125793c3b6bfe7e3bc473ba2bb505b234af5d7e999bda03ed3f4ac34"));
        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("5287b3809b71433729402429b7d909a853cfac5ed40f09117b242c275e6b2d63")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "ncrt";

        assert(mapHistoricBugs.empty());
    }

    int DefaultCheckNameDB () const
    {
        return 0;
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-bip16height")) {
        int64_t height = args.GetArg("-bip16height", consensus.BIP16Height);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for BIP16 is out of valid range. Use -1 to disable BIP16.", height));
        } else if (height == -1) {
            LogPrintf("BIP16 disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.BIP16Height = static_cast<int>(height);
    }
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
