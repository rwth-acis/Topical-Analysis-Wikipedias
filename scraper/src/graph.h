#ifndef GRAPH_H
#define GRAPH_H

#include "util/parsingutil.h"
#include <stdio.h>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

using namespace std;
class Graph
{
	string collection;
	bool history, directed;
	size_t degree;
	unordered_set<string>* vertices;
        unordered_set<string>* edges;
	unordered_map<string,size_t>* degrees;
	unordered_map<string,size_t>* communities;
	ParsingUtil parser;
	double modularity = 42;
public:
        Graph(const char* fPath);
        Graph(string collection);
        double getModularity();
private:

};
#endif // GRAPH_H
