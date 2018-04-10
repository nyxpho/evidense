#include "graph.h"


void quasi(graph &g,double alpha);

void quasiPerInterval(char* file, double alpha, uint32_t threshold, int32_t frame);

void timeWindow(char* file, double alpha, uint32_t threshold, int32_t frame);

void timeWindowNode(char *fileHistory, char* fileText, double alpha, uint32_t threshold, int32_t frame, int32_t beta);

weight_t expectedAvgWeight(graph& avgG, double proportion, std::set<std::string> nodes,  labelmap lm);


