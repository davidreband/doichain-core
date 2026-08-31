#!/usr/bin/env python3
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for circular dependencies

import os
import re
import subprocess
import sys

EXPECTED_CIRCULAR_DEPENDENCIES = (
    "chainparamsbase -> common/args -> chainparamsbase",
    "node/blockstorage -> validation -> node/blockstorage",
    "node/utxo_snapshot -> validation -> node/utxo_snapshot",
    "qt/addresstablemodel -> qt/walletmodel -> qt/addresstablemodel",
    "qt/recentrequeststablemodel -> qt/walletmodel -> qt/recentrequeststablemodel",
    "qt/sendcoinsdialog -> qt/walletmodel -> qt/sendcoinsdialog",
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel",
    "wallet/wallet -> wallet/walletdb -> wallet/wallet",
    "kernel/coinstats -> validation -> kernel/coinstats",
    "versionbits -> versionbits_impl -> versionbits",

    # Temporary, removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    "index/base -> node/context -> net_processing -> index/blockfilterindex -> index/base",

    # Doichain/Namecoin name subsystem, AuxPoW, and the CBlockIndex::GetBlockHeader(BlockManager)
    # change (which adds the chain -> node/blockstorage edge) introduce these cycles.
    "auxpow -> primitives/block -> auxpow",
    "chain -> node/blockstorage -> chain",
    "init -> rpc/names -> init",
    "names/main -> undo -> names/main",
    "names/main -> validation -> names/main",
    "names/mempool -> txmempool -> names/mempool",
    "names/mempool -> validation -> names/mempool",
    "qt/nametablemodel -> qt/walletmodel -> qt/nametablemodel",
    "rpc/blockchain -> rpc/names -> rpc/blockchain",
    "rpc/blockchain -> rpc/rawtransaction -> rpc/blockchain",
    "rpc/names -> rpc/util -> rpc/names",
    "script/names -> script/script -> script/names",
    "txdb -> validation -> txdb",
    "chain -> node/blockstorage -> pow -> chain",
    "chain -> node/blockstorage -> validation -> chain",
    "consensus/tx_verify -> names/main -> txmempool -> consensus/tx_verify",
    "consensus/tx_verify -> names/main -> validation -> consensus/tx_verify",
    "kernel/chainstatemanager_opts -> txdb -> validation -> kernel/chainstatemanager_opts",
    "chain -> node/blockstorage -> validation -> deploymentstatus -> chain",
    "chain -> node/blockstorage -> validation -> kernel/chain -> chain",
    "chain -> node/blockstorage -> validation -> txmempool -> chain",
    "chain -> node/blockstorage -> validation -> validationinterface -> chain",
    "chain -> node/blockstorage -> validation -> versionbits -> chain",
    "chain -> node/blockstorage -> validation -> versionbits -> versionbits_impl -> chain",
)

CODE_DIR = "src"


def main():
    circular_dependencies = []
    exit_code = 0

    os.chdir(CODE_DIR)
    files = subprocess.check_output(
        ['git', 'ls-files', '--', '*.h', '*.cpp'],
        text=True,
    ).splitlines()

    command = [sys.executable, "../contrib/devtools/circular-dependencies.py", *files]
    dependencies_output = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        text=True,
    )

    for dependency_str in dependencies_output.stdout.rstrip().split("\n"):
        circular_dependencies.append(
            re.sub("^Circular dependency: ", "", dependency_str)
        )

    # Check for an unexpected dependencies
    for dependency in circular_dependencies:
        if dependency not in EXPECTED_CIRCULAR_DEPENDENCIES:
            exit_code = 1
            print(
                f'A new circular dependency in the form of "{dependency}" appears to have been introduced.\n',
                file=sys.stderr,
            )

    # Check for missing expected dependencies
    for expected_dependency in EXPECTED_CIRCULAR_DEPENDENCIES:
        if expected_dependency not in circular_dependencies:
            exit_code = 1
            print(
                f'Good job! The circular dependency "{expected_dependency}" is no longer present.',
            )
            print(
                f"Please remove it from EXPECTED_CIRCULAR_DEPENDENCIES in {__file__}",
            )
            print(
                "to make sure this circular dependency is not accidentally reintroduced.\n",
            )

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
