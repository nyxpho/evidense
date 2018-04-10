//============================================================================
// Name        : quasicliques.cpp
// Version     :
// Copyright   : This code can be used for non-commercial purpose.
// Description : Quasi cliques
//============================================================================

#include "graph.h"
#include "quasicliques.h"
#include <math.h>
#include <ctime>
#include <tuple>
#include <iostream>
#include <fstream>
#include <queue>
#include <inttypes.h>
#include <string.h>
#include <boost/algorithm/string.hpp>

class Subgraph
{
public:
	weight_t w = 0.0;
	uint32_t u,v;
	double quasi;
	std::set<std::string> subgraph={};
	std::list<std::tuple<std::string, std::string, weight_t>> edges;
	Subgraph(double q, uint32_t u, uint32_t v, double w, std::set<std::string> &subgraph)
	{
		this->quasi=q;
		this->w = w;
		this->u = u;
		this->v = v;
		this->subgraph.insert(subgraph.begin(), subgraph.end());
	}
};


struct Comp
{
	bool operator()(const Subgraph &s1, const Subgraph &s2) const
	{
		return s1.w > s2.w;
	}
};


struct tempEdge{
	uint32_t u,v;
	int32_t time;
};

struct tempNode{
	std::string u;
	int32_t time;
};

struct maxEdge{
	uint16_t noWindows;
	int32_t startW, window;
	weight_t w, lastCount;
	std::list<std::pair<uint32_t, uint32_t>> appear;
};


struct maxNode{
	int16_t noWindows;
	int32_t startW, window, startWideW, endWideW, indexW;
	weight_t w, lastCount;
	int32_t noTweets, noTweetsW;
	std::list<std::pair<uint32_t, uint32_t>> appear;
};



struct finalEdge{
	int32_t startW, startWideW, endWideW;
	weight_t w,maxw;
	uint32_t u,v;
};

struct finalNode{
	int32_t startW,  startWideW, endWideW;
	weight_t w,maxw;
	std::string u;
	std::set<std::string> subgraph={};
};

struct avgNode{
	int32_t lastTime;
	weight_t w;
	weight_t lastW;
};

struct avgEdge{
	int32_t lastTime;
	weight_t w;
	weight_t lastW;
};
bool sortWeightE (finalEdge e1, finalEdge e2)
{ return e1.w > e2.w; }

bool sortTimeE (finalEdge e1, finalEdge e2)
{ return e1.startW < e2.startW; }

bool sortWeightN (finalNode e1, finalNode e2)
{ return e1.w > e2.w; }

bool sortTimeN (finalNode e1, finalNode e2)
{ return e1.startW < e2.startW; }


Subgraph maxQuasiForEdgeSmart(graph &g, uint32_t uu, uint32_t vv, double alpha)
{

	weightmap wm = boost::get(boost::edge_weight, g);
	namemap labels = boost::get(boost::vertex_name, g);
	std::multimap<weight_t, uint32_t> neighbors = {};
	std::map<uint32_t, weight_t> poz = {};
	eiterator ei, ei_end;
	weightedgemap queue;
	indexmap index = boost::get(boost::vertex_index, g);
	oiterator oi, oi_end;

	//add the initial weight for the neighbours of uu and vv
	for (boost::tie(oi, oi_end) = boost::out_edges(vv, g); oi != oi_end; ++oi) {
		uint32_t j =  index[boost::target(*oi, g)];
		if(j==uu)
			continue;
		if(poz.find(j) == poz.end())
		{
			neighbors.insert(std::pair<weight_t, uint32_t>(wm[*oi], j));
			poz[j] = wm[*oi];
		}
		else
		{
			neighbors.insert(std::pair<weight_t, uint32_t>(wm[*oi] + poz[j], j));
			poz[j] = wm[*oi] + poz[j];
		}
	}

	for (boost::tie(oi, oi_end) = boost::out_edges(uu, g); oi != oi_end; ++oi) {
		uint32_t j =  index[boost::target(*oi, g)];
		if(j==vv)
			continue;
		if(poz.find(j) == poz.end())
		{
			neighbors.insert(std::pair<weight_t, uint32_t>(wm[*oi], j));
			poz[j] = wm[*oi];
		}
		else
		{
			neighbors.insert(std::pair<weight_t, uint32_t>(wm[*oi] + poz[j], j));

			poz[j] = wm[*oi] + poz[j];
		}
	}



	// I iterate from the node with the biggest weight until the node with the smallest
	std::multimap<weight_t, uint32_t>::reverse_iterator it(neighbors.rbegin());

	//std::cout<<neighbors.size()<<"\n";
	std::set<uint32_t> subgraphNodes = {uu,vv};
	std::set<std::string> subgraph={labels[uu],labels[vv]};

	edge e = boost::edge(uu,vv,g).first;
	if(neighbors.size() == 0)
		return Subgraph(1.0,uu,vv,wm[e]/2,subgraph);


	weight_t sw = wm[e];
	weight_t thres = wm[e];

	while(it->first + sw >= (subgraph.size()+1)*subgraph.size() * 0.5 * thres * alpha)
	{
		while(subgraphNodes.find(it->second)!=subgraphNodes.end() && neighbors.size() >0)
		{
			if(neighbors.size()==1)
			{
				neighbors.clear();
				break;
			}
			std::multimap<weight_t, uint32_t>::iterator it2((++it).base());
			neighbors.erase(it2);
			it = neighbors.rbegin();
		}

		if(neighbors.size() == 0)
			break;

		uint32_t bestj = it->second;
		weight_t bestw = it->first;
		weight_t newth = thres;
		weight_t bestquasi = 0;
		std::set<uint32_t> seen={};
		while(it!=neighbors.rend())
		{
                        if(subgraph.size() == 9)
                            break;

			while((subgraphNodes.find(it->second)!=subgraphNodes.end() || seen.find(it->second)!=seen.end() ) && it!=neighbors.rend() )
			{
				if(neighbors.size()==1)
				{
					neighbors.clear();
					break;
				}
				if(seen.find(it->second)!=seen.end())
				{
					it++;
					continue;
				}
				std::multimap<weight_t, uint32_t>::iterator it2((++it).base());
				neighbors.erase(it2);
				it = neighbors.rbegin();
			}

			if(it == neighbors.rend())
				break;


			if((it->first + sw) < ((subgraph.size()+1)*subgraph.size() * 0.5 * thres * alpha))
			{
				break;
			}

			seen.emplace(it->second);

			weight_t max = 0;
			for(uint32_t nn: subgraphNodes)
			{
				if(boost::edge(it->second,nn,g).second)
				{
					edge ee = boost::edge(it->second,nn,g).first;
					if(wm[ee]>max)
						max = wm[ee];
				}
			}

			if(it->second != bestj)
			{
				weight_t qq = 0.0;
				if(max>thres)
					qq = (it->first + sw) / ((subgraph.size()+1)*subgraph.size() * 0.5 * max) ;
				else
					qq = (it->first + sw) / ((subgraph.size()+1)*subgraph.size() * 0.5 * thres) ;
				if(qq>bestquasi)
				{
					bestquasi = qq;
					bestj=it->second;
					bestw = it->first;
					newth = thres;
					if(max>thres)
						newth = max;
				}

			}
			else
			{
				if(max>thres)
					bestquasi = (it->first + sw) / ((subgraph.size()+1)*subgraph.size() * 0.5 * max) ;
				else
					bestquasi = (it->first + sw) / ((subgraph.size()+1)*subgraph.size() * 0.5 * thres) ;
				newth = thres;
				if(max>thres)
					newth = max;
			}
			it++;

		}


		if(bestquasi >= alpha)
                {
			subgraph.insert(labels[bestj]);
			subgraphNodes.insert(bestj);
			sw += bestw;;
			thres = newth;
			for (boost::tie(oi, oi_end) = boost::out_edges(bestj, g); oi != oi_end; ++oi) {
				uint32_t j =  index[boost::target(*oi, g)];
				if(subgraphNodes.find(j)!=subgraphNodes.end())
					continue;
				if(poz.find(j) == poz.end())
				{
					neighbors.insert(std::pair<weight_t, uint32_t>(wm[*oi], j));
					poz[j] = wm[*oi];
				}
				else
				{
					neighbors.insert(std::pair<weight_t, uint32_t>(wm[*oi] + poz[j], j));
					poz[j] = wm[*oi] + poz[j];
				}
			}

		}
		else
			break;




		if(neighbors.size() == 0)
			break;
		it = neighbors.rbegin();

	}



	Subgraph best = Subgraph(sw*1.0/((subgraph.size()-1)*subgraph.size() * 0.5 * thres),uu,vv,sw/subgraph.size(),subgraph);

	for (uint32_t first:subgraphNodes)
		for (uint32_t second:subgraphNodes)
			if(first<second)
			{
				if(!boost::edge(first,second,g).second)
					continue;
				edge e = boost::edge(first,second,g).first;
				best.edges.push_back(std::make_tuple(labels[first], labels[second], wm[e]));
			}


	uint count = 0;
	std::set<uint32_t> seen;
	it = neighbors.rbegin();
	while(true)
	{
		if(it == neighbors.rend())
			break;
		while(it!= neighbors.rend() && (subgraphNodes.find(it->second) != subgraphNodes.end() || seen.find(it->second) != seen.end()) )
			it++;
		if(it == neighbors.rend())
			break;

		count ++;
		seen.emplace(it->second);
		for (uint32_t second:subgraphNodes)
		{
			if(!boost::edge(it->second,second,g).second)
				continue;
			edge e = boost::edge(it->second,second,g).first;
			best.edges.push_back(std::make_tuple(labels[it->second], labels[second], wm[e]));
		}
		if(count == 3)
			break;
	}
	return best;
}


void retrieveEvents(char* file, double alpha, int32_t frame, std::vector<finalNode> sortedNodesSmall)
{

	std::ifstream fin;
	fin.open(file, std::ios::in);

	std::streampos pos = fin.tellg();
	std::string a, b,c;

	int32_t currTime;
	labelmap reverse;
	graph *gr = NULL;
	auto it = sortedNodesSmall.begin();

	std::map<std::string,uint32_t> visited;

	while (it!=sortedNodesSmall.end()) {
		pos=fin.tellg();
		std::getline(fin,c,'\t');
		std::getline(fin,a,'\t');
		std::getline(fin,b);
		if (!fin.good())
			break;

		currTime = atoi(c.c_str());

		if(currTime == it->startW)
		{
			if(gr!=NULL)
				delete gr;
			fin.clear();
			fin.seekg(pos);
			std::pair<labelmap, graph*> p = readTimeTweet(fin, frame);

			fin.clear();
			fin.seekg(pos);
			gr = p.second;
			reverse = p.first;
		}
		while(currTime == it->startW)
		{

			if(currTime != it->startW)
				break;

			if(it==sortedNodesSmall.end())
				break;


			oiterator oi, oi_end;
			weight_t maxw = 0; uint32_t jmax = 0;
			indexmap index = boost::get(boost::vertex_index, *gr);
			if(reverse.find(it->u)==reverse.end())
				std::cout<<"\n nodul"<<it->u<<" nu exista, time: "<<it->startW<<"\n";
			for (boost::tie(oi, oi_end) = boost::out_edges(reverse[it->u], *gr); oi != oi_end; ++oi) {
				if(maxw <  boost::get(boost::edge_weight_t(), *gr, *oi))
				{
					maxw = boost::get(boost::edge_weight_t(), *gr, *oi);
					jmax = index[boost::target(*oi, *gr)];
				}
			}
			if(maxw ==0)
			{std::cout<<"\nniciunvecin?"<<" "<<it->w<<", time: "<<it->startW<<", name"<<it->u<<"\n";return;}

			char buff[20];
			struct tm * timeinfo;
			time_t tt = (time_t) currTime;
			timeinfo = localtime (&tt);
			strftime(buff, sizeof(buff), "%b %d %H:%M", timeinfo);
			Subgraph s = maxQuasiForEdgeSmart(*gr, reverse[it->u], jmax, alpha);

			tt = (time_t) it->startW;
			timeinfo = localtime (&tt);
			strftime(buff, sizeof(buff), "%b %d %H:%M", timeinfo);


			it->subgraph.insert(s.subgraph.begin(), s.subgraph.end());
			it++;
		}

	}

	std::sort(sortedNodesSmall.begin(), sortedNodesSmall.end(), sortWeightN);
	it = sortedNodesSmall.begin();
        int thresh = sortedNodesSmall.size()/2;

        int found = 0;
        double avgtime = 0;
	while (it!=sortedNodesSmall.end())
	{
                if (found == thresh)
                    break;
		bool visit = true;
		for(auto it2 = it->subgraph.begin();it2!=it->subgraph.end();it2++)
                   if((*it2).find("b-geo-loc") != std::string::npos ||    (*it2).find("b-facility") != std::string::npos )
			if(visited.find(*it2) != visited.end() && ( visited[*it2] < it->startW+frame &&  visited[*it2] > it->startW-frame ))
				visit = false;
		if(visit)
		{
                        found ++;
			visited[it->u] = it->startW;
			char start[20],maxim[20],end[20];
			struct tm * timeinfo;
			time_t tt = (time_t) it->startW;
			timeinfo = localtime (&tt);
			strftime(maxim, sizeof(maxim), "%b %d %H:%M", timeinfo);
			tt = (time_t) it->startWideW;
			timeinfo = localtime (&tt);
			strftime(start, sizeof(start), "%b %d %H:%M", timeinfo);
			tt = (time_t) it->endWideW;
			timeinfo = localtime (&tt);
			strftime(end, sizeof(end), "%b %d %H:%M", timeinfo);
                        avgtime += (it->endWideW - it->startWideW)/3600.0;
		      std::cout<<"TIME: "<<start<<" , "<<end<<", LOCATION: "<<it->u<<", DEVIATION: "<<it->maxw<<", EVENT: ";
                        for(auto it2 = it->subgraph.begin();it2!=it->subgraph.end();it2++)
			{
				std::cout<<*it2<<" ";
				visited[*it2] = it->startW;
			}
			std::cout<<"\n";
		}
		it++;
	}
        std::cout<<"TOTAL TIME: "<<avgtime;
	delete gr;
	fin.close();
}

double min_value = 0.1;
double meantotal =0.1;
std::map<std::string, std::pair<double, double>> sample(char* file, int32_t frame, uint16_t thresh)
									{
	std::ifstream fin;
	std::ofstream fout;


	fin.open(file, std::ios::in);
	std::string a, b,c;

	int32_t t1;
	fin>>c;
	t1 = atoi(c.c_str());

	fin.seekg(0);

	std::map<std::string, maxNode> nodes;

	int32_t currTime = 0;
	int32_t noTweets = 0;
	while (fin.good()) {
		std::getline(fin,c,'\t');
		std::getline(fin,a,'\t');
		std::getline(fin,b);
		noTweets++;
		currTime = atoi(c.c_str());
		std::vector<std::string> strs;
		boost::split(strs, b, boost::is_any_of(" "));
		for(auto itb = strs.begin();itb!=strs.end();itb++)
		{
			if( (*itb).find("b-geo-loc") == std::string::npos &&    (*itb).find("b-facility") == std::string::npos )
				continue;
			std::string u = *itb;
			if (nodes.find(u) == nodes.end())
				nodes[u].lastCount = 0;
			nodes[u].lastCount++;
                        //noTweets++;

		}

	}

	fin.close();
	std::map<std::string, std::pair<double, double>> nodesAvg;
         std::vector<double> vec;
	for(auto it = nodes.begin(); it!=nodes.end(); it++)
	{
		double mean = nodes[it->first].lastCount*1.0/noTweets;
                meantotal+=mean;
		if(mean < min_value)
			min_value = mean;
                vec.push_back(mean);
		nodesAvg[it->first] = std::make_pair(mean, 1-mean);

	}
        std::sort(vec.begin(), vec.end());
        meantotal=vec[vec.size()/2];
        //for(auto it = nodes.begin(); it!=nodes.end(); it++)
        //{
                
         //       nodesAvg[it->first] = std::make_pair(vec[vec.size()/2], 1-vec[vec.size()/2]);

       // }
        //the median is use to initialize the probability for unseen locations
        //min_value = vec[vec.size()/2];
        //min_value=meantotal;
        std::cout<<min_value<<" that was the mean! \n";
	return nodesAvg;

}
void timeWindowNode(char *fileHistory, char* fileText, double alpha, uint32_t threshold, int32_t frame, int32_t beta)
{
	std::ifstream fin;

	fin.open(fileText, std::ios::in);

	std::string a, b,c;

	uint32_t t1;
	fin>>a>>b>>c;
	t1 = atoi(c.c_str());

	fin.seekg(0);
	std::queue<tempNode> myqueue;
	std::queue<int32_t> tweetsTime;
	std::map<std::string, std::list<maxNode>> nodes;
	std::map<std::string, weight_t> nodesCurr;
	clock_t time1 = clock();
	std::map<std::string, std::pair<double,double>> nodesAvg = sample(fileHistory, frame, 5);
	clock_t time2 = clock();
	double duration = double( time2 - time1 )/CLOCKS_PER_SEC;;
	std::cout<<"sampled the graph, have statistics for "<< nodesAvg.size()<<" words, in "<<duration<<" seconds...\n";

	int32_t start = 0, end = 0, currTime = 0, indexTime = 0;
        double min_np = 10.0;        
	currTime = atoi(c.c_str());
	if(start ==0)
		start = currTime;
	end = currTime;
	int32_t noTweets = 0;
	time1 = clock();
	while (fin.good()) {
		std::getline(fin,c,'\t');
		std::getline(fin,a,'\t');
		std::getline(fin,b);
		indexTime ++;
		currTime = atoi(c.c_str());
		end = currTime;
		tweetsTime.push(currTime);
		while(tweetsTime.size()>0 && tweetsTime.front() <= currTime - frame)
			tweetsTime.pop();
		noTweets = tweetsTime.size();
		std::vector<std::string> strs;
		boost::split(strs, b, boost::is_any_of(" "));


		for(auto itb = strs.begin();itb!=strs.end();itb++)
		{
			if( (*itb).find("b-geo-loc") == std::string::npos &&	(*itb).find("b-facility") == std::string::npos)
				continue;
                     

			while(myqueue.size() > 0 && myqueue.front().time <= currTime - frame)
			{
				tempNode n = myqueue.front();
				myqueue.pop();
				nodesCurr[n.u]--;
				if(myqueue.size() > 0)
					t1 = myqueue.front().time;
				else
					t1 = currTime;
			}

			std::string u = *itb;

			tempNode n;
			n.u = u; n.time = currTime;
			myqueue.push(n);
			if(nodesCurr.find(u) == nodesCurr.end())
				nodesCurr[u] = 1;
			else
				nodesCurr[u]++;

                        if(nodesAvg.find(u) == nodesAvg.end())
				nodesAvg[u] = std::make_pair(min_value, 1-min_value);
                        double expVal = nodesAvg[u].first*noTweets >5 ? nodesAvg[u].first*noTweets:5;
			double stDev = sqrt(nodesAvg[u].first*nodesAvg[u].second*noTweets) > 1 ? sqrt(nodesAvg[u].first*nodesAvg[u].second*noTweets) : 1;
			
                        int32_t thresh = expVal + beta*stDev;
                        if(nodes.find(u) == nodes.end())
			{
				maxNode mx;
				mx.w = 1;
				mx.noWindows = 1;
				mx.window = t1;
				mx.startW = t1;
				mx.startWideW = 0;
				mx.lastCount = 0;
				mx.noTweets =  noTweets;
				mx.indexW = 0;
				mx.noTweetsW = 0;
				nodes[u].push_front(mx);
			}
			else
				if (nodes[u].front().w > 1 && nodesCurr[u] < thresh)
				{

					nodes[u].front().endWideW = currTime;
					maxNode mx;
					mx.w = 1;
					mx.noWindows = 1;
					mx.window = t1;
					mx.startW = t1;
					mx.startWideW = currTime;
					mx.lastCount = 0;
					mx.indexW = 0;
					mx.noTweets =  noTweets;
					mx.noTweetsW = 0;
					nodes[u].push_front(mx);

				}
				else
					if(nodes[u].front().w  < nodesCurr[u] &&  nodesCurr[u] >= thresh)
					{
						if (nodes[u].front().w == 1 && nodes[u].size() >1)
						{
							if(nodes[u].front().startWideW >= t1)
								nodes[u].pop_front();
							else
								nodes[u].front().startWideW = 0;
						}
						if(nodes[u].front().w  < nodesCurr[u])
						{nodes[u].front().w = nodesCurr[u];
						nodes[u].front().startW = t1;}
						nodes[u].front().endWideW = currTime;
						if(nodes[u].front().noTweetsW == 0) //why did I comment this?? ah, did it gave better results??
							nodes[u].front().noTweetsW  = nodesCurr[u];
						if(nodes[u].front().indexW == 0)
							nodes[u].front().indexW = indexTime - noTweets;
						if(nodes[u].front().startWideW == 0)
							nodes[u].front().startWideW = currTime;
						nodes[u].front().noTweets = noTweets;
					}

			if(nodes[u].front().noTweetsW !=0)
				nodes[u].front().noTweetsW++;

		}
	}

	fin.close();
	time2 = clock();
	duration = double( time2 - time1 )/CLOCKS_PER_SEC;;
	std::cout<<"read the graph and computed frequencies for "<<nodes.size()<<" words, in "<<duration<<" seconds ... \n";
	std::vector<finalNode> sortedNodes;
        std::cout<<"computed the min np: "<<min_np<<"\n";
	time1 = clock();
	for(auto it = nodes.begin(); it!=nodes.end(); it++)
	{

		for(auto itn = it->second.begin(); itn != it->second.end(); itn++)
		{
			finalNode n;

			n.startW = itn->startW;
			n.endWideW = itn->endWideW;
			n.startWideW = itn->startWideW;
			n.u = it->first;

			double expVal = nodesAvg[n.u].first * itn->noTweets > 5 ? nodesAvg[n.u].first * itn->noTweets : 5;
			double stDev = sqrt(nodesAvg[n.u].first*nodesAvg[n.u].second * itn->noTweets) > 1 ? sqrt(nodesAvg[n.u].first*nodesAvg[n.u].second * itn->noTweets) : 1;
			 n.w = (itn->w - expVal)*1.0/stDev;
                         n.maxw = itn->w;
			if(n.w >= beta && n.startW != 0)
				sortedNodes.insert(sortedNodes.end(), n);
		}
	}

	std::sort(sortedNodes.begin(), sortedNodes.end(), sortWeightN);

	time2 = clock();
	duration = double( time2 - time1 )/CLOCKS_PER_SEC;
	std::cout<<"sorted nodes by weight, have "<<sortedNodes.size()<<" events in total, taking the top "<<threshold<<" events, in "<<duration<<"...\n";
	std::vector<finalNode> sortedNodesSmall;
	if(sortedNodes.size()>2*threshold)
		sortedNodesSmall.insert(sortedNodesSmall.begin(), sortedNodes.begin(), sortedNodes.begin() + 2*threshold);
	else
		sortedNodesSmall.insert(sortedNodesSmall.begin(), sortedNodes.begin(), sortedNodes.end());

	std::sort(sortedNodesSmall.begin(), sortedNodesSmall.end(), sortTimeN);

	time1 = clock();
	std::cout<<"sorted nodes by time...\n";
	retrieveEvents(fileText, alpha, frame, sortedNodesSmall);
	time2 = clock();
	duration = double( time2 - time1 )/CLOCKS_PER_SEC;;
	std::cout<<"\nretrieving events took "<<duration<<" seconds \n";

}

