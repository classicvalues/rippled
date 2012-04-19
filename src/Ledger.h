#ifndef __LEDGER__
#define __LEDGER__

#include <map>
#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "../json/value.h"

#include "Transaction.h"
#include "types.h"
#include "BitcoinUtil.h"
#include "Hanko.h"
#include "AccountState.h"
#include "SHAMap.h"
#include "SerializedLedger.h"


enum LedgerStateParms
{
	// input flags
	lepCREATE,		  			// Create if not present

	// output flags
	lepOKAY,					// success
	lepMISSING,					// No node in that slot
	lepWRONGTYPE,				// Node of different type there
	lepCREATED,					// Node was created
	lepERROR,					// error
};

class Ledger : public boost::enable_shared_from_this<Ledger>
{ // The basic Ledger structure, can be opened, closed, or synching
public:
	typedef boost::shared_ptr<Ledger> pointer;


	enum TransResult
	{
		TR_ERROR	=-1,
		TR_SUCCESS	=0,
		TR_NOTFOUND	=1,
		TR_ALREADY	=2,
		TR_BADTRANS =3,	// the transaction itself is corrupt
		TR_BADACCT	=4,	// one of the accounts is invalid
		TR_INSUFF	=5,	// the sending(apply)/receiving(remove) account is broke
		TR_PASTASEQ	=6,	// account is past this transaction
		TR_PREASEQ	=7,	// account is missing transactions before this
		TR_BADLSEQ	=8,	// ledger too early
		TR_TOOSMALL =9, // amount is less than Tx fee
	};


private:
	uint256 mHash, mParentHash, mTransHash, mAccountHash;
	uint64 mFeeHeld, mTimeStamp;
	uint32 mLedgerSeq;
	bool mClosed, mValidHash, mAccepted, mImmutable;
	
	SHAMap::pointer mTransactionMap, mAccountStateMap;

	mutable boost::recursive_mutex mLock;
	
	Ledger(const Ledger&);				// no implementation
	Ledger& operator=(const Ledger&);	// no implementation

protected:
	Ledger(Ledger& previous, uint64 timestamp);	// ledger after this one
	void updateHash();

	bool addAccountState(AccountState::pointer);
	bool updateAccountState(AccountState::pointer);
	bool addTransaction(Transaction::pointer);
	bool delTransaction(const uint256& id);
	
	static Ledger::pointer getSQL(const std::string& sqlStatement);

	SerializedLedgerEntry::pointer getASNode(LedgerStateParms& parms, const uint256& nodeID,
	 LedgerEntryType let);

public:
	Ledger(const NewcoinAddress& masterID, uint64 startAmount); // used for the starting bootstrap ledger
	Ledger(const uint256 &parentHash, const uint256 &transHash, const uint256 &accountHash,
	 uint64 feeHeld, uint64 timeStamp, uint32 ledgerSeq); // used for received ledgers
	Ledger(const std::vector<unsigned char>& rawLedger);
	Ledger(const std::string& rawLedger);

	void setClosed() { mClosed=true; }
	void setAccepted() { mAccepted=true; }
	bool isClosed() { return mClosed; }
	bool isAccepted() { return mAccepted; }

	// ledger signature operations
	void addRaw(Serializer &s);

	uint256 getHash();
	const uint256& getParentHash() const { return mParentHash; }
	const uint256& getTransHash() const { return mTransHash; }
	const uint256& getAccountHash() const { return mAccountHash; }
	uint64 getFeeHeld() const { return mFeeHeld; }
	uint64 getTimeStamp() const { return mTimeStamp; }
	uint32 getLedgerSeq() const { return mLedgerSeq; }

	// low level functions
	SHAMap::pointer peekTransactionMap() { return mTransactionMap; }
	SHAMap::pointer peekAccountStateMap() { return mAccountStateMap; }

	// ledger sync functions
	void setAcquiring(void);
	bool isAcquiring(void);
	bool isAcquiringTx(void);
	bool isAcquiringAS(void);

	// Transaction Functions
	bool hasTransaction(const uint256& TransID) const;
	Transaction::pointer getTransaction(const uint256& transID) const;

	// OLD high level functions
	uint64 getBalance(const NewcoinAddress& acctID) const;
	AccountState::pointer getAccountState(const NewcoinAddress& acctID);
	TransResult applyTransaction(Transaction::pointer trans);
	TransResult removeTransaction(Transaction::pointer trans);
	TransResult hasTransaction(Transaction::pointer trans);
	Ledger::pointer switchPreviousLedger(Ledger::pointer oldPrevious, Ledger::pointer newPrevious,  int limit);

	// high-level functions
	LedgerStateParms writeBack(LedgerStateParms parms, SerializedLedgerEntry::pointer);
	SerializedLedgerEntry::pointer getAccountRoot(LedgerStateParms& parms, const uint160& accountID);
	SerializedLedgerEntry::pointer getNickname(LedgerStateParms& parms, const std::string& nickname);
	SerializedLedgerEntry::pointer getNickname(LedgerStateParms& parms, const uint256& nickHash);
//	SerializedLedgerEntry::pointer getRippleState(LedgerStateParms parms, const uint160& offeror,
//		const uint160& borrower, const Currency& currency);

	// database functions
	static void saveAcceptedLedger(Ledger::pointer);
	static Ledger::pointer loadByIndex(uint32 ledgerIndex);
	static Ledger::pointer loadByHash(const uint256& ledgerHash);

	// index calculation functions
	static uint256 getAccountRootIndex(const uint160& account);
	static uint256 getRippleIndex(const uint160& account, const uint160& extendTo, const uint160& currency);

	Ledger::pointer closeLedger(uint64 timestamp);
	bool isCompatible(boost::shared_ptr<Ledger> other);
	bool signLedger(std::vector<unsigned char> &signature, const LocalHanko &hanko);

	void addJson(Json::Value&);

	static bool unitTest();
};

inline LedgerStateParms operator|(const LedgerStateParms& l1, const LedgerStateParms& l2)
{
	return static_cast<LedgerStateParms>(static_cast<int>(l1) | static_cast<int>(l2));
}

inline LedgerStateParms operator&(const LedgerStateParms& l1, const LedgerStateParms& l2)
{
	return static_cast<LedgerStateParms>(static_cast<int>(l1) & static_cast<int>(l2));
}

#endif
