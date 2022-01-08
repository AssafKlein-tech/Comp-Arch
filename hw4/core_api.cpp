/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>

using std::vector;


class ContextData
{
private:
	vector<int> registers;
	int inst;
	int stall_untill;
	bool finished;
public:

	ContextData(): registers(REGS_COUNT,0), inst(0),stall_untill(-1), finished(false){} 
	~ContextData() = default;
	void setRegister(int reg, int value) {registers[reg] = value;}
	int getRegister(int reg) {return registers[reg];}
	void excInst() {inst++;}
	int getInst() {return inst;}
	void updateStall(int stall) {stall_untill = stall;}
	bool isExecutable(int cyc) {return cyc >= stall_untill;} //false if it stalled
	bool isFinished() { return finished;}
	void halt() {finished = true;}
};


class Core
{
public:
	Core();
	
	// execute the next command of <thread>
	// if the instruction
	bool executeInst();
	bool contextSwitch();
	bool notDone();
	double getCPI();
	
private:
	int cycle;
	int thread;
	int inst_count;

	int load_lat;
	int store_lat;
	int switch_lat;
	int num_of_threads;
	
	std::vector<ContextData> threads;
};

Core::Core(): cycle(0), thread(0), inst_count(0), load_lat(SIM_GetLoadLat()), store_lat(SIM_GetStoreLat()),
				switch_lat(SIM_GetSwitchCycles()), threads(SIM_GetThreadsNum()) { }

bool Core::executeInst()
{
	Instruction inst;

	SIM_MemInstRead(threads[thread].getInst(), &inst, thread);
	
}
bool Core::contextSwitch()
{

}
bool Core::notDone()
{
	for (ContextData ctxt : threads)
		if (!ctxt.isFinished())
			return true;
	
	return false;
}

double Core::getCPI()
{
	return (double)cycle/inst_count;
}


/**********************************************************************************************************************/
/*                                                                                                                    */
/**********************************************************************************************************************/

Core core;

void CORE_BlockedMT() 
{
	core = new Core();
	while(core.notDone())
	{	
		while(core.executeInst())
		core.contextSwitch();
	}
}

void CORE_FinegrainedMT() 
{
	core = new Core();
	while(core.notDone())
	{
		core.executeInst();
		core.contextSwitch();
	}
}

double CORE_BlockedMT_CPI()
{
	double cpi = core.getCPI();
	delete core;
	return cpi;
}

double CORE_FinegrainedMT_CPI()
{
	double cpi = core.getCPI();
	delete core;
	return cpi;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) 
{
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) 
{
}
