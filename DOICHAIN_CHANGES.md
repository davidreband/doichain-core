# Documented Doichain Changes from Namecoin 29.x

## Overview

This document serves as documentation for changes made when forking from Namecoin and implementing Doichain-specific functionality. The current branch focuses on implementing the `name_doi` operation and related validation systems.

## Core Changes Made

### Name Operation System (`name_doi`)

#### Implementation Files:
- **src/names/main.cpp**
  - `CheckNameTransaction` - Core transaction validation for name operations
  - Added support for `name_doi` operations alongside existing name operations

- **src/rpc/names.cpp**  
  - Added `name_doi` RPC command for DOI (Decentralized Open Identity) registration and updates
  - RPC interface for name_doi operations

#### Validation and Consensus:
- **src/consensus/tx_verify.cpp**
  - `CheckTxInputs` -> `CheckNameTransaction` validation chain
  - Ensures proper validation of name_doi transactions in consensus layer

- **src/txdb.cpp**
  - `ValidateNameDB` - Database validation for name operations
  - Maintains consistency of name operations in the database

### Mempool Changes

#### Files Modified:
- **src/txmempool.h** 
  - Definition of `map<valtype, std::set<uint256>> mapNameDoi`
  - Added `registersDoi` functionality
  - Enhanced `clear()` method for DOI-specific cleanup

- **src/txmempool.cpp** 
  - `check()` - Mempool validation with DOI support
  - `addUnchecked()` (line 105) - Adding DOI transactions to mempool
  - `remove()` (line 155) - Removes mempool entries from `mapNameDois`
  - `checkTx()` - Enhanced validation: prevents d/ names in name_doi transactions

### Recent Development Progress (doichain-nc29-migration branch)

#### Completed Implementation Steps:
1. **Basic name_doi Operation** (commit 85c0a670a3)
   - Initial implementation of name_doi functionality

2. **Missing Operation Addition** (commit d12a4742b7) 
   - Added previously missing name_doi operation support

3. **Validation Fixes** (commit b0c86d9ef9)
   - Fixed validation to properly allow name_doi operations

4. **RPC/Validation Refinement** (commit d94bc81b52)
   - Refined DOI name operation handling in both RPC and validation layers
   - Enhanced error handling and validation logic

#### Migration from Namecoin:
- Comprehensive renaming from Namecoin to Doichain across codebase
- Updated chainparams, network configurations, and branding
- Maintained compatibility with existing name operation infrastructure
- Added Doichain-specific genesis blocks and network parameters

### Workflow

#### New Name Transaction Processing:
1. **Transaction Validation Chain:**
   ```
   consensus/tx_verify.cpp - CheckTxInputs 
   -> CheckNameTransaction (names/main.cpp)
   -> Mempool validation (txmempool.cpp)
   ```

2. **Mempool Processing:**
   - Transaction hits mempool
   - `checkTx` validates DOI-specific rules
   - `addUnchecked` adds to `mapNameDoi` tracking
   - `remove` cleans up on transaction removal

### Outstanding Items

#### TODO Items:
- **mempool.cpp - removeConflicts**
  - Need to check if we can resolve any possible database conflicts for DOI operations
  - Ensure proper conflict resolution for competing name_doi transactions

#### Recent Validation Improvements:
- Enhanced DOI name operation handling with better error messages
- Improved validation logic for name_doi transactions
- Better integration with existing Namecoin name operation infrastructure

### Technical Notes

#### Name Operation Constraints:
- Names limited to 255 bytes (`MAX_NAME_LENGTH`)
- Values limited to 1023 bytes (`MAX_VALUE_LENGTH`) 
- Each name operation locks 0.01 DOI coins (`NAME_LOCKED_AMOUNT`)
- Names expire and require periodic updates

#### Database Integration:
- DOI operations properly tracked in `mapNameDoi` mempool structure
- Database validation ensures consistency of DOI name registrations
- Conflict resolution mechanisms for competing name operations

## Current Status

The implementation of `name_doi` operations is functionally complete with recent refinements to RPC and validation handling. The system now properly:

- Validates name_doi transactions in consensus layer
- Tracks DOI operations in mempool with dedicated data structures  
- Provides RPC interface for DOI registration and updates
- Maintains database consistency for DOI name operations
- Integrates seamlessly with existing Namecoin infrastructure

The outstanding TODO item regarding conflict resolution in `removeConflicts` should be addressed to ensure robust handling of competing DOI transactions.