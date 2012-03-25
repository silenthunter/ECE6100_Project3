#include <stdio.h>
#include <stdlib.h>
#include <fstream>

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
	char Busy[8];
	char Tag[8];
};

struct RegisterStore
{
	char SinkTag;
	float Sink;
	char SrcTag;
	float Src;
	int Cycles;
};

RegisterStore  AdderStore[8];
RegisterStore  MultDivStore[8];
RegisterFile RegFile;

int count = 0;
int cycle = 0;

inline int findFirstOpen(RegisterStore[]);
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
		AdderStore[i].SinkTag = 0;
		AdderStore[i].Sink = 0;
		AdderStore[i].SrcTag = 0;
		AdderStore[i].Src = 0;
		AdderStore[i].Cycles = 0;
		MultDivStore[i].SinkTag = 0;
		MultDivStore[i].Sink = 0;
		MultDivStore[i].SrcTag = 0;
		MultDivStore[i].Src = 0;
		MultDivStore[i].Cycles = 0;
		RegFile.Data[i] = 0;
		RegFile.Busy[i] = 0;
		RegFile.Tag[i] = 0;
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
		string src2 = line.substr(idx3 + 1, line.size() - idx3 - 2);

		//printf("%s/%s/%s/%s\n", inst.c_str(), dest.c_str(), src1.c_str(), src2.c_str());

		int destNum = atoi(dest.substr(1).c_str());
		int src1Num = atoi(src1.substr(1).c_str());

		//Not all instructions have a second source
		int src2Num = 0;
		if(src2.size() > 0)
			src2Num = atoi(src2.substr(1).c_str());

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
		else if(inst.compare("LD") == 0);
		else if(inst.compare("SD") == 0);
		else
			printf("Unknown instruction: %s\n", inst.c_str());

		executeCycle();
	}

	printf("Cycles: %d\n", cycle);
}

void addToUnit(RegisterStore store[], int destNum, int src1Num, int src2Num, int CYCLES, int tagOffset)
{
	int idx = findFirstOpen(store);
	if(RegFile.Busy[src1Num])//See if the register is busy
		store[idx].SrcTag = RegFile.Tag[src1Num];
	else
		store[idx].Src = src1Num;
	if(RegFile.Busy[src2Num])//See if the register is busy
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
			RegFile.Tag[i] = 0;
		}
	}
	store[idx].SinkTag = 0;
	store[idx].Sink = 0;
	store[idx].SrcTag = 0;
	store[idx].Src = 0;
	store[idx].Cycles = 0;

	//Let other units know this data is available
	for(int i = 0; i < 8; i++)
	{
		if(AdderStore[i].SrcTag == idx + offset)
			AdderStore[i].SrcTag = 0;

		if(AdderStore[i].SinkTag == idx + offset)
			AdderStore[i].SinkTag = 0;
	}
	for(int i = 0; i < 8; i++)
	{
		if(MultDivStore[i].SrcTag == idx + offset)
			AdderStore[i].SrcTag = 0;

		if(MultDivStore[i].SinkTag == idx + offset)
			AdderStore[i].SinkTag = 0;
	}
}

void executeCycle()
{
	cycle++;
	for(int i = 0; i < 8; i++)
	{
		if(AdderStore[i].Cycles > 0 && !(AdderStore[i].SrcTag | AdderStore[i].SinkTag))
		{
			AdderStore[i].Cycles--;
			//This instruction is done
			if(AdderStore[i].Cycles == 0)
				completeInst(AdderStore, i, 0);
		}
	}
	for(int i = 0; i < 8; i++)
	{
		if(MultDivStore[i].Cycles > 0 && !(MultDivStore[i].SrcTag | MultDivStore[i].SinkTag))
		{
			MultDivStore[i].Cycles--;
			//This instruction is done
			if(MultDivStore[i].Cycles == 0)
				completeInst(MultDivStore, i, 8);
		}
	}
}

int findFirstOpen(RegisterStore store[])
{
	for(int i = 0; i < 8; i++)
	{
		if(AdderStore[i].SinkTag | AdderStore[i].Sink == 0) return i;
	}
	return -1;
}
