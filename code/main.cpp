//============================================================================
// Name        : main.cpp
// Version     :
// Copyright   : This code can be used for non-commercial purpose.
// Description : Quasi cliques
//============================================================================


#include <iostream>
#include "graph.h"
#include "quasicliques.h"
#include <ctime>

using namespace std;

int main(int argc, char **argv) {
	if(argc < 7)
	{
		std::cout<<"Missing arguments! 6 are needed: history_file text_file quasi number_edges time_frame threshold";
		return 0;
	}
	clock_t time1 = clock();
	std::cout<<"Algorithm started ...\n";
	timeWindowNode(argv[1], argv[2], atof(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
	clock_t time2 = clock();
	double duration = double( time2 - time1 )/CLOCKS_PER_SEC;;
	std::cout<<"\n Algorithm ended, took "<<duration<<" seconds. \n";
}
