#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/subgraph.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/algorithm/string.hpp>

typedef double weight_t;

typedef std::vector<weight_t> weightvector;
typedef std::vector<uint32_t> uintvector;
typedef std::vector<bool> boolvector;
typedef std::multimap<weight_t, uint32_t> weightnodemap;
typedef std::multimap<weight_t, boost::detail::edge_desc_impl<boost::undirected_tag, uint64_t> > weightedgemap;
typedef std::map<std::string, uint32_t> labelmap;



typedef boost::adjacency_list<
	boost::vecS,
	boost::vecS,
	boost::undirectedS,
	boost::property<boost::vertex_name_t, std::string>,
	boost::property<boost::edge_weight_t, weight_t,
	boost::property<boost::edge_index_t, uint32_t>
	> > graph;


typedef boost::graph_traits<graph>::edge_iterator eiterator;
typedef boost::graph_traits<graph>::out_edge_iterator oiterator;
typedef boost::graph_traits<graph>::vertex_iterator viterator;
typedef boost::graph_traits<graph>::adjacency_iterator aiterator;

typedef boost::graph_traits<graph>::vertex_descriptor vertex;
typedef boost::graph_traits<graph>::edge_descriptor edge;
typedef std::vector<vertex> vertexvector;

typedef boost::property_map<graph, boost::vertex_name_t>::type namemap;
typedef boost::property_map<graph, boost::vertex_index_t>::type indexmap;
typedef boost::property_map<graph, boost::edge_weight_t>::type weightmap;


std::pair<graph*,labelmap> read(const char *name);


std::pair<labelmap,graph*> readTime( std::ifstream &fin, int32_t frame);
std::pair<labelmap,graph*> readTimeTweet( std::ifstream &fin, int32_t frame);
std::tuple<labelmap, uint32_t, uint32_t> getLabels(const char *name);

graph* getStats(char *file, int32_t frame, labelmap lm, uint32_t vcnt);


