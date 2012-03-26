#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <queue>

using namespace std;

const int ADDD_CYCLES = 3;//fully pipelined
const int DIVD_CYCLES = 10;//5 cycles before new instruction
const int MULD_CYCLES = 4;//fully pipelined
const int LD_CYCLES = 3;//fully pipelined
const int SD_CYCLES = 1;//fully pipelined


//Tags
//0 - 7 ADDD
//8 - 15 MULD
//16 - 18 LD

struct RegisterFile
{
	float Data[8];
	int Busy[8];
	int Tag[8];
};

struct RegisterStore
{
	int SinkTag;
	float Sink;
	int SrcTag;
	float Src;
	int Cycles;
};

RegisterStore  AdderStore[8];
RegisterStore  MultDivStore[8];
RegisterStore  LDStore[8];
RegisterFile RegFile;

int count = 0;
int cycle = 0;
int fullStoreStall = 0;
int cdbStall = 0;
queue<int> completionQueue;

inline int findFirstOpen(RegisterStore[]);
inline bool instRunning();
inline void completeInst(RegisterStore, int, int);
inline void addToUnit(RegisterStore store[], int destNum, int src1Num, int src2Num, int CYCLES, int tagOffset);
inline void executeCycle();

int main(int argc, char* argv[])
{
	ifstream infile;
	infile.open("trace.txt");

	string line = "";

	//set starting values
	for(int i = 0; i < 8; i++)
	{
		AdderStore[i].SinkTag = -1;
		AdderStore[i].Sink = -1;
		AdderStore[i].SrcTag = -1;
		AdderStore[i].Src = -1;
		AdderStore[i].Cycles = 0;
		MultDivStore[i].SinkTag = -1;
		MultDivStore[i].Sink = -1;
		MultDivStore[i].SrcTag = -1;
		MultDivStore[i].Src = -1;
		MultDivStore[i].Cycles = 0;
		LDStore[i].SinkTag = -1;
		LDStore[i].Sink = -1;
		LDStore[i].SrcTag = -1;
		LDStore[i].Src = -1;
		LDStore[i].Cycles = 0;
		RegFile.Data[i] = -1;
		RegFile.Busy[i] = 0;
		RegFile.Tag[i] = -1;
	}

	//loop as long as there are lines left in the trace file
	while(getline(infile, line))// && count++ < 1000)
	{
		int idx1 = line.find(" ");
		int idx2 = line.find(" ", idx1 + 1);
		int idx3 = line.find(" ", idx2 + 1);

		if(idx3 <= 0) idx3 = line.size() - 1;
		
		string inst = line.substr(0, idx1);
		string dest = line.substr(idx1 + 1, idx2 - idx1 - 1);
		string src1 = line.substr(idx2 + 1, idx3 - idx2 - 1);
		string src2 = line.substr(idx3 + 1, line.size() - idx3 - 1);

		//printf("%s/%s/%s/%s\n", inst.c_str(), dest.c_str(), src1.c_str(), src2.c_str());

		int destNum = atoi(dest.substr(1).c_str());

		//Not all instructions have a second source
		int src1Num = -1;
		int src2Num = -1;
		if(src2.size() > 0)
		{
			src2Num = atoi(src2.substr(1).c_str());
			src1Num = atoi(src1.substr(1).c_str());
		}

		if(inst.compare("ADDD") == 0)
		{
			addToUnit(AdderStore, destNum, src1Num, src2Num, ADDD_CYCLES, 0);
			/*int addIdx = findFirstOpen(AdderStore);
			AdderStore[addIdx].Src = src1Num;
			AdderStore[addIdx].Sink = src2Num;
			AdderStore[addIdx].Cycles = ADDD_CYCLES;
			RegFile.Busy[destNum] = 1;
			RegFile.Tag[destNum] = addIdx;*/
		}
		else if(inst.compare("MULD") == 0)
		{
			addToUnit(MultDivStore, destNum, src1Num, src2Num, MULD_CYCLES, 8);
			/*int muldIdx = findFirstOpen(MultDivStore);
			MultDivStore[muldIdx].Src = src1Num;
			MultDivStore[muldIdx].Sink = src2Num;
			MultDivStore[addIdx].Cycles = MULD_CYCLES;
			RegFile.Busy[destNum] = 1;
			RegFile.Tag[destNum] = muldIdx + 8;//Offset for the muld tags*/
		}
		else if(inst.compare("LD") == 0)
		{
			addToUnit(LDStore, destNum, src1Num, src2Num, LD_CYCLES, 16);
		}
		else if(inst.compare("SD") == 0);
		else
			printf("Unknown instruction: %s\n", inst.c_str());

		executeCycle();
	}

	while(instRunning())
		executeCycle();

	printf("Cycles: %d\nStalls %d\nCDB Stalls %d\n", cycle, fullStoreStall, cdbStall);
}

bool instRunning()
{
	/*for(int i = 0; i < 8; i++)
	{
		if(AdderStore[i].SrcTag >= 0 || AdderStore[i].SinkTag >= 0) return true;
	}
	for(int i = 0; i < 8; i++)
	{
		if(MultDivStore[i].SrcTag >= 0 || MultDivStore[i].SinkTag >= 0) return true;
	}*/
	for(int i = 0; i < 8; i++)
		if(RegFile.Busy[i]) return true;
	return false;
}

void addToUnit(RegisterStore store[], int destNum, int src1Num, int src2Num, int CYCLES, int tagOffset)
{
	int idx = findFirstOpen(store);
	while(idx < 0)//Stall until an open unit is found
	{
		fullStoreStall++;
		executeCycle();
		idx = findFirstOpen(store);
	}

	if(RegFile.Busy[src1Num] && src2Num >= 0)//See if the register is busy
		store[idx].SrcTag = RegFile.Tag[src1Num];
	else
		store[idx].Src = src1Num;
	if(RegFile.Busy[src2Num] && src2Num >= 0)//See if the register is busy and it's not a LD instruction
		store[idx].SinkTag = RegFile.Tag[src2Num];
	else
		store[idx].Sink = src2Num;

	store[idx].Cycles = CYCLES;
	RegFile.Busy[destNum] = 1;
	RegFile.Tag[destNum] = idx + tagOffset;

}

void completeInst(RegisterStore store[], int idx, int offset)
{

	//find register that is the destination
	for(int i = 0; i < 8; i++)
	{
		if(RegFile.Tag[i] == idx + offset)
		{
			RegFile.Busy[i] = 0;
			RegFile.Tag[i] = -1;
		}
	}
	store[idx].SinkTag = -1;
	store[idx].Sink = -1;
	store[idx].SrcTag = -1;
	store[idx].Src = -1;
	store[idx].Cycles = 0;

	//Let other units know this data is available
	for(int i = 0; i < 8; i++)
	{
		if(AdderStore[i].SrcTag == idx + offset)
			AdderStore[i].SrcTag = -1;

		if(AdderStore[i].SinkTag == idx + offset)
			AdderStore[i].SinkTag = -1;
	}
	for(int i = 0; i < 8; i++)
	{
		if(MultDivStore[i].SrcTag == idx + offset)
			MultDivStore[i].SrcTag = -1;

		if(MultDivStore[i].SinkTag == idx + offset)
			MultDivStore[i].SinkTag = -1;
	}
}

void executeCycle()
{
	bool progress = false;
	if(++cycle % 5000 == 0) printf("Cycle #%d\n", cycle);
	for(int i = 0; i < 8; i++)
	{
		if(AdderStore[i].Cycles > 0 && !(AdderStore[i].SrcTag >= 0 || AdderStore[i].SinkTag >= 0))
		{
			progress = true;
			AdderStore[i].Cycles--;
			//This instruction is done
			if(AdderStore[i].Cycles == 0)
				completionQueue.push(i);
				//completeInst(AdderStore, i, 0);
		}
	}
	for(int i = 0; i < 8; i++)
	{
		if(MultDivStore[i].Cycles > 0 && !(MultDivStore[i].SrcTag >= 0|| MultDivStore[i].SinkTag >= 0))
		{
			progress = true;
			MultDivStore[i].Cycles--;
			//This instruction is done
			if(MultDivStore[i].Cycles == 0)
				completionQueue.push(i + 8);
				//completeInst(MultDivStore, i, 8);
		}
	}
	for(int i = 0; i < 8; i++)
	{
		if(LDStore[i].Cycles > 0 && !(LDStore[i].SrcTag >= 0|| LDStore[i].SinkTag >= 0))
		{
			progress = true;
			LDStore[i].Cycles--;
			//This instruction is done
			if(LDStore[i].Cycles == 0)
				completionQueue.push(i + 16);
				//completeInst(LDStore, i, 16);
		}
	}

	if(!completionQueue.empty())
	{
		int idx = completionQueue.front();
		completionQueue.pop();
		if( idx < 8)
			completeInst(AdderStore, idx, 0);
		else if(idx < 16)
			completeInst(MultDivStore, idx - 8, 8);
		else
			completeInst(LDStore, idx - 16, 16);

		//Add to CDB stalls by seeing if any other instructions were complete in the same cycle
		cdbStall += completionQueue.size();
	}
	/*if(!progress && cycle > 2)
	{
		printf("No Progress %d\n", cycle);
		exit(-1);
	}*/
}

int findFirstOpen(RegisterStore store[])
{
	for(int i = 0; i < 8; i++)
	{
		if(store[i].SinkTag == -1 && store[i].Sink == -1 && store[i].SrcTag == -1 && store[i].Src == -1 && store[i].Cycles == 0) return i;
	}
	return -1;
}
