#include <stdio.h>
#include <stdlib.h>
#include <fstream>

using namespace std;

const int ADDD_CYCLES = 3;//fully pipelined
const int DIVD_CYCLES = 10;//5 cycles before new instruction
const int MULD_CYCLES = 4;//fully pipelined
const int LD_CYCLES = 3;//fully pipelined
const int SD_CYCLES = 1;//fully pipelined

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
	float src;
};

RegisterStore  AdderStore[8];
RegisterStore  MultDivStore[8];

int main(int argc, char* argv[])
{
	ifstream infile;
	infile.open("trace.txt");

	string line = "";

	//loop as long as there are lines left in the trace file
	while(getline(infile, line))
	{
	}
}
