#Documented Doichain Changes from Namecoin 0.20

## the following changes where done when we forked from name_coin. it should serve as a documentation.

- mempool.h 
	- definition of map<valtype, std::set<uint256>> mapNameDoi
	- registersDoi
	- clear
	
- mempool.cpp (is called when a transaction hits the mempool) 
	- check 
	- addUnchecked (105)
	- remove (155) - removes a mempoolentry from mapNameDois
	- checkTx - don't except d/ names in name_doi transactions
	
- rpcname.cpp  
	- name_doi 

- tx_verify.cpp
- txdb.cpp
	- ValidateNameDB 
- names/main.cpp
	- CheckNameTransaction  


WORKFLOW
- new name tx
	- consensus/tx_verify.cpp - CheckTxInputs -> CheckNameTransaction
	- 
	
	
TODO:
- mempool.cpp 
	- removeConflicts - please check if we can resolve any possible db conflicts here for doi
	
	