/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <vector>
#include <stdio.h>
#include <iostream>

using std::vector;

const char* opcodes[] = {"NOP", "ADD", "SUB", "ADDI", "SUBI", "LOAD", "STORE", "HALT"}; // debug
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
	int* getContext() { return registers.data(); }
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
	
	/**
	 * @brief execute the next command of <thread>, and update reg/memory
	 * 		  exec -> stall update -> next command update
	 * @return true if stay on same thread 
	 * @return false if need to stall
	 */
	bool executeInst();
	
	/**
	 * @brief calculate time of contextswitch (in cycles). the cycles when the cpu in idle are also calculted in this function
	 * 
	 * @param latency if there is a latency to the context switch
	 */
	void contextSwitch(bool latency = false);

	/**
	 * @brief checks if all thread were finished
	 * 
	 * @return true there are running threads
	 * @return false all threads were finished
	 */
	bool notDone();
	double getCPI();
	void getContext(tcontext* c, int thread_id);

private:
	int cycle;
	int thread;
	int inst_count;

	int load_lat;
	int store_lat;
	int switch_lat;
	
	std::vector<ContextData> threads;
};

Core::Core(): cycle(0), thread(0), inst_count(0), load_lat(SIM_GetLoadLat()), store_lat(SIM_GetStoreLat()),
				switch_lat(SIM_GetSwitchCycles()), threads(SIM_GetThreadsNum()) { }

bool Core::executeInst()
{
	Instruction inst;

	SIM_MemInstRead(threads[thread].getInst(), &inst, thread);
	
	//what do we do with nope
	int dst_val = threads[thread].getRegister(inst.dst_index);
	int src1 = threads[thread].getRegister(inst.src1_index);
	int src2 = inst.isSrc2Imm ? inst.src2_index_imm : threads[thread].getRegister(inst.src2_index_imm);
	int stall = 1;

	//std::cout<<cycle<<":\t"<<"["<<thread<<"]\t";

	switch (inst.opcode)
	{
	case CMD_ADDI:
	case CMD_ADD:
		threads[thread].setRegister(inst.dst_index, src1+src2);
		//std::cout<<"REG"<<inst.dst_index<<"= "<<src1<<"+"<<src2<<"="<<threads[thread].getRegister(inst.dst_index)<<std::endl;
		break;
	case CMD_HALT:
		//std::cout<<"HALT"<<std::endl;
		threads[thread].halt();
		cycle++;
		inst_count++;
		return false;
		break;
	case CMD_LOAD:
		int value;
		SIM_MemDataRead(src1+src2, &value);
		threads[thread].setRegister(inst.dst_index, value);
		stall += load_lat;
		//std::cout<<"REG"<<inst.dst_index<<"= MEM["<<src1<<"+"<<src2<<"]="<<value<<std::endl;
		//std::cout<<" wait until "<<cycle+stall<<std::endl;
		//std::cout << load_lat << std::endl;
		break;
	case CMD_STORE:
		SIM_MemDataWrite(dst_val+src2, src1);
		stall += store_lat;
		//std::cout<<"MEM["<<dst_val<<"+"<<src2<<"]="<<src1<<std::endl;
		//std::cout<<" wait until "<<cycle+stall<<std::endl;

		break;
	case CMD_SUBI:
	case CMD_SUB:
		threads[thread].setRegister(inst.dst_index, src1-src2);
		//std::cout<<"REG"<<inst.dst_index<<"= "<<src1<<"-"<<src2<<"="<<threads[thread].getRegister(inst.dst_index)<<std::endl;
		break;
	
	default:
		break;
	}

	threads[thread].updateStall(cycle+stall);
	threads[thread].excInst();
	cycle++;
	inst_count++;
	return (stall == 1);
}


void Core::contextSwitch(bool latency)
{
	int next_thread;
	if (latency)
	{
		while(1)
		{
			if (!threads[thread].isFinished() && threads[thread].isExecutable(cycle))
				return;
			for(unsigned int i = 1; i < threads.size(); i++)
			{
				next_thread = (thread + i) % threads.size();
				if (!threads[next_thread].isFinished() && threads[next_thread].isExecutable(cycle))
				{
					thread = next_thread;
					cycle += switch_lat;
					return;
				}
			}
			cycle++;
		}
	}
	while(1)
	{
		for(unsigned int i = 1; i < threads.size(); i++)
		{
			next_thread = (thread + i) % threads.size();
			if (!threads[next_thread].isFinished() && threads[next_thread].isExecutable(cycle))
			{
				thread = next_thread;
				return;
			}
		}
		if (!threads[thread].isFinished() && threads[thread].isExecutable(cycle))
			return;
		cycle++;
	}
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
	// std::cout <<"\ncycles/inst= "<<cycle<<"/"<<inst_count<<std::endl;
	return (double)cycle/inst_count;
}

void Core::getContext(tcontext* c, int thread_id)
{
	int* regs = threads[thread_id].getContext();
	for (int i = 0; i < REGS_COUNT; i++)
	{
		c[thread_id].reg[i] = regs[i];
	}
	return;
}


/**********************************************************************************************************************/
/*                                                                                                                    */
/**********************************************************************************************************************/

Core* core;

void CORE_BlockedMT() 
{
	core = new Core();
	if(core->notDone())
	{
		while(core->executeInst()) {}
	}
	while(core->notDone())
	{	
		core->contextSwitch(true);
		while(core->executeInst()){}
	}
}

void CORE_FinegrainedMT() 
{
	core = new Core();
	if(core->notDone())
		core->executeInst();
	while(core->notDone())
	{
		core->contextSwitch();
		core->executeInst();
	}
}

double CORE_BlockedMT_CPI()
{
	double cpi = core->getCPI();
	delete core;
	return cpi;
}

double CORE_FinegrainedMT_CPI()
{
	return CORE_BlockedMT_CPI();
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid)
{
	core->getContext(context,threadid);
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid)
{
	core->getContext(context,threadid);
}
