//============================================================================
// Name        : graph.cpp
// Version     :
// Copyright   : This code can be used for non-commercial purpose.
// Description : Quasi cliques
//============================================================================


#include "graph.h"


std::pair<graph*, labelmap> read(const char *name)
{
	std::ifstream fin;

	fin.open(name, std::ios::in);
	if(fin.is_open())
		std::cout<<name<<" is open\n";
	uint32_t ecnt = 0;
	uint32_t vcnt = 0;
	labelmap lm;

	std::string a, b;
	double r;

	fin>>a>>b>>r;
	while (fin.good()) {
		fin>>a>>b>>r;
		if (lm.find(a) == lm.end()) lm[a] = vcnt++;
		if (lm.find(b) == lm.end()) lm[b] = vcnt++;
		ecnt++;
	}

	std::cout<<ecnt<<"\n";
	weightvector w(ecnt);
	std::fill(w.begin(), w.end(), 0.0);
	std::vector<std::pair<uint32_t, uint32_t> > edges(ecnt);



	fin.close();
	fin.open(name, std::ios::in);
	uint32_t i = 0; 
	while (fin.good()){
		fin>>a>>b>>r;
		edges[i] = std::make_pair(lm[a], lm[b]);

		i++;
	}
	graph *g = new graph(edges.begin(), edges.end(), w.begin(), vcnt);


	// Create labels

	std::map<uint32_t, std::string> reverse;
	for (labelmap::iterator it = lm.begin(); it != lm.end(); ++it)
		reverse[it->second] = it->first;

	namemap labels = boost::get(boost::vertex_name, *g);

	viterator vi = boost::vertices(*g).first;
	for (uint32_t i = 0; i < vcnt; i++) {
		labels[vi[i]] = reverse[i];
	}
	return std::make_pair(g, lm);
}

std::tuple<labelmap, uint32_t, uint32_t>
getLabels(const char *name)
{
	std::ifstream fin;

	fin.open(name, std::ios::in);
	uint32_t ecnt = 0;
	uint32_t vcnt = 0;
	uint32_t tcnt = 0;
	labelmap lm;

	std::string a, b, r, oldr ="";

	while (fin.good()) {
		fin>>a>>b>>r;
		if(r != oldr)
		{
			oldr = r;
			tcnt++;
		}
		if (lm.find(a) == lm.end()) lm[a] = vcnt++;
		if (lm.find(b) == lm.end()) lm[b] = vcnt++;
		ecnt++;
	}

	fin.close();
	return std::make_tuple(lm,vcnt,tcnt);
}

graph*
getStats(char *file, int32_t frame, labelmap lm, uint32_t vcnt)
{
	std::ifstream fin;
	fin.open(file, std::ios::in);
	uint32_t ecnt = 0;


	std::string a, b,c;

	int32_t initialTime;

	fin>>a>>b>>c;
	initialTime = atoi(c.c_str());
	int32_t time = 0;
	uint32_t i = 0;
	labelmap edgesTotal;

	while(true)
	{
		int f = fin.peek();
		if(f == EOF)
		{
			break;
		}
		fin>>a>>b>>c;
		time = atoi(c.c_str());
		if(initialTime + frame < time)
		{
			break;
		}
		if(a>b)
		{
			c=a;
			a=b;
			b=c;
		}
		if(edgesTotal.find(a+" "+b) != edgesTotal.end())
			edgesTotal[a+" "+b] = edgesTotal[a+" "+b] + 1;
		else
		{
			edgesTotal[a+" "+b] = 1;
			ecnt++;
		}
	}

	fin.close();
	std::vector<std::pair<uint32_t, uint32_t> > edges(ecnt);


	weightvector w(ecnt);
	std::fill(w.begin(), w.end(), 0);
	i=0;
	for(auto it = edgesTotal.begin(); it!=edgesTotal.end(); it++)
	{
		uint32_t space = it->first.find(" ");
		a = it->first.substr(0,space);
		b = it->first.substr(space+1);
		edges[i] = std::make_pair(lm[a], lm[b]);
		w[i] = it->second;
		i++;

	}
	
	graph *g = new graph(edges.begin(), edges.end(), w.begin(), vcnt);


	// Create labels

	std::map<uint32_t, std::string> reverse;
	for (labelmap::iterator it = lm.begin(); it != lm.end(); ++it)
		reverse[it->second] = it->first;

	namemap labels = boost::get(boost::vertex_name, *g);

	viterator vi = boost::vertices(*g).first;
	for (uint32_t i = 0; i < vcnt; i++) {
		labels[vi[i]] = reverse[i];
	}
	return g;
}

std::pair<labelmap, graph*>
readTimeTweet(std::ifstream &fin, int32_t frame)
{

        labelmap lm;
        graph *g = NULL;

        if(!fin.good())
        {
                fin.close();
                return std::make_pair(lm,g);
        }


      

        uint32_t ecnt = 0;


        std::string a, b,c, text, words, cup;

   
        int32_t initialTime;

        initialTime = 0;
        //std::cout<<"initial time = "<<initialTime;
        int32_t time = 0;
        uint32_t i = 0;
        labelmap edgesTotal;
	while(fin.good())
        {
                std::getline(fin,c,'\t');
                std::getline(fin,text,'\t');
                std::getline(fin, words);
                //std::cout<<a<<" "<<b<<" "<<c<<"\n";                                                                        }
                time = atoi(c.c_str());
                if(initialTime == 0)
                    initialTime = time;

                //std::cout<<"reading" <<a<<b<<time;
                if(initialTime + frame < time)
                {
                        //std::cout<<"going out"<<initialTime + frame<<" "<<time;
                        break;
                }
		std::vector<std::string> strs;
                boost::split(strs, words, boost::is_any_of(" "));


                for(auto ita = strs.begin();ita!=strs.end();ita++)
                {
                    for(auto itb = ita + 1 ;itb!=strs.end();itb++)
		    {
		        a = *ita;
		        b = *itb;
                        if(a>b)
                        {
                            cup=a;a=b;b=cup;
                        }
                        if(edgesTotal.find(a+" "+b) != edgesTotal.end())
                            edgesTotal[a+" "+b]++;
                        else
                        {
                            edgesTotal[a+" "+b] = 1;
                            ecnt++;
                        }
		    }
                }
        }

        std::vector<std::pair<uint32_t, uint32_t> > edges(ecnt);


        weightvector w(ecnt);
        std::fill(w.begin(), w.end(), 0);
        i=0;
        uint32_t vcnt = 0;

	 for(auto it = edgesTotal.begin(); it!=edgesTotal.end(); it++)
        {
                uint32_t space = it->first.find(" ");
                a = it->first.substr(0,space);
                b = it->first.substr(space+1);
                if (lm.find(a) == lm.end()) lm[a] = vcnt++;
                if (lm.find(b) == lm.end()) lm[b] = vcnt++;
                //std::cout<<a<<b;
                edges[i] = std::make_pair(lm[a], lm[b]);
                w[i] = it->second;
                i++;

        }
        //std::cout<<"\ngraph: "<<ecnt<<" "<<vcnt;

        g = new graph(edges.begin(), edges.end(), w.begin(), vcnt);


        // Create labels

        std::map<uint32_t, std::string> reverse;
        for (labelmap::iterator it = lm.begin(); it != lm.end(); ++it)
                reverse[it->second] = it->first;

        namemap labels = boost::get(boost::vertex_name, *g);

        viterator vi = boost::vertices(*g).first;
        for (uint32_t i = 0; i < vcnt; i++) {
                labels[vi[i]] = reverse[i];
        }
        return std::make_pair(lm,g);
}

std::pair<labelmap, graph*>
readTime(std::ifstream &fin, int32_t frame)
{

	labelmap lm;
	graph *g = NULL;
	
	if(!fin.good())
	{
		fin.close();
		return std::make_pair(lm,g);
	}
	

	//std::cout<<"read Time\n";


	uint32_t ecnt = 0;


	std::string a, b,c;

	//file.seekp(0, ios::end) // seek to the end of the file

	int32_t initialTime;

        initialTime = 0;
	//std::cout<<"initial time = "<<initialTime;
	int32_t time = 0;
	uint32_t i = 0;
	labelmap edgesTotal;

	while(fin.good())
	{
                fin>>a>>b>>c;
                std::string tm = "\n";
                //std::cout<<a<<" "<<b<<" "<<c<<"\n";
                if(a.find(tm)!=std::string::npos || b.find(tm)!=std::string::npos)
                                                         {std::cout<<"\n\n IN GRAPH FOUND MISTAKE!!!!!!!!!!!! "<<a<<" "<<b<<" "<<"\n\n";
                                                                                 }
                time = atoi(c.c_str());
                if(initialTime == 0)
                    initialTime = time;
    
		//std::cout<<"reading" <<a<<b<<time;
		if(initialTime + frame < time)
		{
			//std::cout<<"going out"<<initialTime + frame<<" "<<time;
			break;
		}
		if(a>b)
		{
                        std::string cup;
			cup=a;
			a=b;
			b=cup;
		}
		if(edgesTotal.find(a+" "+b) != edgesTotal.end())
			edgesTotal[a+" "+b]++;
		else
		{
			edgesTotal[a+" "+b] = 1;
			ecnt++;
		}
	}
        
	std::vector<std::pair<uint32_t, uint32_t> > edges(ecnt);


	weightvector w(ecnt);
	std::fill(w.begin(), w.end(), 0);
	i=0;
	uint32_t vcnt = 0;
	for(auto it = edgesTotal.begin(); it!=edgesTotal.end(); it++)
	{
		uint32_t space = it->first.find(" ");
		a = it->first.substr(0,space);
		b = it->first.substr(space+1);
		if (lm.find(a) == lm.end()) lm[a] = vcnt++;
		if (lm.find(b) == lm.end()) lm[b] = vcnt++;
		//std::cout<<a<<b;
		edges[i] = std::make_pair(lm[a], lm[b]);
		w[i] = it->second;
		i++;

	}
	//std::cout<<"\ngraph: "<<ecnt<<" "<<vcnt;

	g = new graph(edges.begin(), edges.end(), w.begin(), vcnt);


	// Create labels

	std::map<uint32_t, std::string> reverse;
	for (labelmap::iterator it = lm.begin(); it != lm.end(); ++it)
		reverse[it->second] = it->first;

	namemap labels = boost::get(boost::vertex_name, *g);

	viterator vi = boost::vertices(*g).first;
	for (uint32_t i = 0; i < vcnt; i++) {
		labels[vi[i]] = reverse[i];
	}
	return std::make_pair(lm,g);
}




