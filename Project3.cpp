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

int count = 0;

int main(int argc, char* argv[])
{
	ifstream infile;
	infile.open("trace.txt");

	string line = "";

	//loop as long as there are lines left in the trace file
	while(getline(infile, line) && count++ < 10)
	{
		int idx1 = line.find(" ");
		int idx2 = line.find(" ", idx1 + 1);
		int idx3 = line.find(" ", idx2 + 1);

		if(idx3 <= 0) idx3 = line.size() - 1;
		
		string inst = line.substr(0, idx1);
		string dest = line.substr(idx1 + 1, idx2 - idx1 - 1);
		string src1 = line.substr(idx2 + 1, idx3 - idx2 - 1);
		string src2 = line.substr(idx3 + 1, line.size() - idx3 - 2);

		printf("%s/%s/%s/%s\n", inst.c_str(), dest.c_str(), src1.c_str(), src2.c_str());
	}
}
