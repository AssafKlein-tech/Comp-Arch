/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include <bitset>
#include <vector>
#include <stdexcept>

#include "bp_api.h"

using std::bitset;


static unsigned log2(unsigned n)
{
	int count = 0;
	for (; n != 0; count++, n>>=1) { }
	return count;
}
/*********************************************************************************************/
/*									Classes													 */
/*********************************************************************************************/

enum using_share_enum {no_share,lsb_share,mid_share};

class BTB
{
public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, int Shared);
	~BTB();
	
	/**
	 * @brief in Exec - update BTB after actual instruction
	 * @param pc 
	 * @param isTaken
	 * @param target_pc 
	 */
	void update(uint32_t pc, bool isTaken, uint32_t target_pc);

	/**
	 * @brief Get the Tables Index - dont check whether <pc> is currently in btb
	 * 			so need to call Function predict first
	 * @param pc 
	 * @return uint32_t 
	 */
	uint32_t getTableIndex(uint32_t pc);

	/**
	 * @brief check if pc is branch, if no/new - <target_pc> = pc+4, <histo> = 0
	 * 				else, <target_pc> = next pc, <histo> = curr histo
	 * @param pc 
	 * @param target_pc 
	 * @param histo 
	 * @return true if is known branch, false if no/new branch
	 */
	uint32_t predictTarget(uint32_t pc);

	/**
	 * @brief check if pc is known branch using tag
	 * @param pc 
	 * @return true if known, false if unknown
	 */
	bool isKnownBranch(uint32_t pc);
	
	/**
	 * @brief find histo fo pc - if unknown branch - return 0 (for safety check isKnownBranch() first)
	 * for global history - return curr histo, for loacl history - return histo[btb_i]
	 * @param pc 
	 * @return uint32_t history
	 */
	uint32_t* findCurrHisto(uint32_t pc);
	
	/**
	 * @brief get btb index out of pc, using bit manipulation
	 * @param pc 
	 * @return uint32_t btb_index
	 */
	uint32_t getBTBIndex(uint32_t pc);

private:
	bool isGlobalHist;
	unsigned historySize;
	unsigned tagSize;
	unsigned btbSize;
	using_share_enum shared;

	uint32_t histo_mask;	// to keep histo size correct to histoSize
	
	uint32_t* histo;
	uint32_t* tags;
	uint32_t* targets;
};
/*****************************************************************************************************************/
BTB::BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, int Shared) :
		isGlobalHist(isGlobalHist), historySize(historySize), tagSize(tagSize), 
			btbSize(btbSize), shared(using_share_enum(Shared)), histo_mask(UINT32_MAX >> (32-historySize)),
				histo(nullptr), tags(nullptr), targets(nullptr)
{
	if (historySize < 1 || historySize > 8 || tagSize > 30-log2(btbSize))
	{
		throw std::runtime_error("Bad Args");
	}
	
	if (isGlobalHist)
	{
		histo = new uint32_t(0);
		
	}
	else
	{
		histo = new uint32_t[btbSize]();
	}

	tags = new uint32_t[btbSize]();
	targets = new uint32_t[btbSize]();
}
/*****************************************************************************************************************/
BTB::~BTB()
{
	if (!isGlobalHist)
	{
		delete[] histo;
	}
	else
	{
		delete histo;
	}

	delete[] tags;
	delete[] targets;
}
/*****************************************************************************************************************/
void BTB::update(uint32_t pc, bool isTaken, uint32_t target_pc)
{
	uint32_t tag_mask = UINT32_MAX >> (32-tagSize);
	uint32_t tag = tag_mask & (pc>>(log2(btbSize)+2));
	
	uint32_t btb_i = getBTBIndex(pc);
	uint32_t* curr_histo = findCurrHisto(pc);

	targets[btb_i] = target_pc;

	if (!isKnownBranch(pc) && !isGlobalHist)
	{
		*curr_histo = 0;
		tags[btb_i] = tag;
	}
	else
	{
		*curr_histo <<=1;
		*curr_histo &= histo_mask;				// to stay in range [0..2^histoSize]
		*curr_histo |= isTaken;
	}
}
/*****************************************************************************************************************/
uint32_t BTB::getTableIndex(uint32_t pc)
{
	uint32_t retval = 0;
	uint32_t* curr_histo = findCurrHisto(pc);

	switch (shared)
	{
	case no_share:
		retval = *curr_histo;
		break;
	case lsb_share:
		retval = (*curr_histo ^ (pc>>2)) & histo_mask;
		break;
	case mid_share:
		retval = (*curr_histo ^ (pc>>16)) & histo_mask;
		break;
	default:
		throw(std::runtime_error("Bad Shared Arg"));
		break;
	}

	return retval;
}
/*****************************************************************************************************************/
uint32_t BTB::predictTarget(uint32_t pc)
{	
	uint32_t predict_target = pc+4;
	if (!isKnownBranch(pc))
	{
		predict_target = targets[getBTBIndex(pc)];
	}

	return predict_target;
}
/*****************************************************************************************************************/
bool BTB::isKnownBranch(uint32_t pc)
{
	uint32_t tag_mask = UINT32_MAX >> (32-tagSize);
	uint32_t tag = tag_mask & (pc>>(log2(btbSize)+2));
	
	return (tags[getBTBIndex(pc)] == tag);
}
/*****************************************************************************************************************/
uint32_t* BTB::findCurrHisto(uint32_t pc)
{
	uint32_t* curr_histo = 0;
	if (isKnownBranch(pc))
	{
		if (isGlobalHist)
		{
			curr_histo = histo;
		}
		else
		{
			curr_histo = histo + getBTBIndex(pc);
		}
	}

	return curr_histo;
}
/*****************************************************************************************************************/
uint32_t BTB::getBTBIndex(uint32_t pc)
{
	uint32_t btb_mask = UINT32_MAX >> (32-btbSize);
	uint32_t btb_i = btb_mask & (pc>>2);

	return btb_i;
}
/*********************************************************************************************/
/*********************************************************************************************/

typedef enum {SNT = 0,WNT,WT,ST} fsm_state;

class FSM
{
private:
	fsm_state state;
public:
	FSM(fsm_state def_state): state(def_state){};
	fsm_state operator*() const { return state;}
	FSM&  operator++() {state = (state==ST) ? ST: fsm_state(state + 1);}
	FSM& operator--(){state = (state==SNT) ? SNT: fsm_state(state - 1);}
	~FSM() = default;
};

/**
 * @brief 
 * 
 */
class Table
{
private:
	std::vector<FSM> fsm_array;
public:
	Table(int fsmSize,int fsmState): fsm_array(fsmSize,fsm_state(fsmState)){}
	fsm_state operator[](int index) const {return *fsm_array[index];}
	void update(int index, bool taken);
	~Table() = default;
};

void Table::update(int index, bool taken)
{
	if (taken)
		++fsm_array[index];
	else
		--fsm_array[index];
}



/**
 * @brief 
 * 
 */
class Table
{
private:
	std::vector<FSM> fsm_array;
public:
	Table(int fsmSize,int fsmState): fsm_array(fsmSize,fsm_state(fsmState)){}
	fsm_state operator[](int index) const {return *fsm_array[index];}
	void update(int index, bool taken);
	~Table() = default;
};

void Table::update(int index, bool taken)
{
	if (taken)
		++fsm_array[index];
	else
		--fsm_array[index];
}

class BP
{
public:
	BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	~BP();

	bool predict(uint32_t pc, uint32_t *dst);

	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

	void BP_GetStats(SIM_stats *curStats);
private:

	BTB btb;
	Table tables;
	SIM_stats stats;
};

BP::BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared): 
				btb(btbSize,historySize,tagSize,isGlobalHist,Shared), tables(btbSize,fsmState) { }	// table size?

/*********************************************************************************************/
/*********************************************************************************************/

BP* bp;
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

	bp = new BP(btbSize, historySize,tagSize,fsmState,isGlobalHist,isGlobalTable,Shared);

	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

