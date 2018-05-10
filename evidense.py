import sys
import os
import bz2
import json
import codecs
import io
import numpy as np
import random
import math
from collections import deque
from graph_tool.all import *

def wilsonIntervalUpperBound(success, trials):
    fails = trials - success
    z = 1.96 # .95% confidence
    return (success + z*z/2)/(trials + z*z) + z/(trials + z*z)*math.sqrt(success*fails*1.0/trials + z*z/4)


def getStatistics(file, trials):
    wordCounts = dict()
    fin = io.open(file, encoding = "utf-8")
    tweets = fin.readlines()
    noTweets = len(tweets)
    selectedTweets = set()
    while len(selectedTweets) < trials:
        noLine = random.randint(0, noTweets - 1)
        selectedTweets.add(tweets[noLine])

    for tweet in selectedTweets:
        line = tweet.strip().split("\t")
        if len(line) < 3:
            continue
        for w in line[2].split():
            wordCounts[w] = wordCounts.get(w, 0) + 1
    wordsStats = dict()
    for w in wordCounts:
        p = wilsonIntervalUpperBound(wordCounts[w], trials)
        wordsStats[w] = (p,1-p)
    fin.close()

    return wordsStats

def makeGraph(fin, startWindow, endWindow, node):
    #print startWindow, endWindow
    g = Graph(directed=False)
    #print "I am making a graph!"
    edges = dict()
    nodes = set()
    n = 0
    c = 0
    for line in fin:
        l = line.split("\t")
        timestamp_s = long(l[0])
        if timestamp_s>endWindow:
            break
        elems = l[2].strip().split()
        for i in range(0, len(elems)):
            if elems[i] == node:
                c = c + 1

            for j in range(i+1, len(elems)):
                u = elems[i]
                v = elems[j]
                nodes.add(u)
                nodes.add(v)

                if u < v:
                    edge = (u,v)
                else:
                    edge = (v,u)
                edges[edge] = edges.get(edge, 0) + 1

    #print "Number nodes: " + str(len(nodes))
    #print "Number edges: " + str(len(edges))
    gnodesname = g.new_vertex_property("string")
    g.vertex_properties["vertexToWord"] = gnodesname
    gedgeweight = g.new_edge_property("long")
    g.edge_properties["edgeWeight"] = gedgeweight
    nodeObj = dict()
    totalWeight = 0
    for v in nodes:
        newNode = g.vertex_index[g.add_vertex()]
        nodeObj[v] = newNode
        gnodesname[newNode] = v
    for edge in edges:
        newEdge = g.add_edge(nodeObj[edge[0]], nodeObj[edge[1]])
        gedgeweight[newEdge] = edges[edge]
        totalWeight = totalWeight + edges[edge]
    #print c, totalWeight
    return g

def quasiClique(g, word, gamma, sizeClique):
    gnodesname = g.vertex_properties["vertexToWord"]
    gedgeweight = g.edge_properties["edgeWeight"]
    wordToVertex = dict()
    for index in g.get_vertices():
        wordToVertex[gnodesname[index]] = index
    subgraph = set()
    subgraph.add(wordToVertex[word])
    weightSubgraph = 0
    maxEdge = 0
    while len(subgraph) < sizeClique:
        candidates = list()
        for node in subgraph:
            for edge in g.get_out_edges(node):
                if edge[1] in subgraph and edge[0] in subgraph:
                    continue
                elif edge[1] not in subgraph:
                    candidates.append(edge[1])
                else:
                    candidates.append(edge[0])
        candidatesWeight = list()
        for candidate in candidates:
            weightCandidate = 0
            maxEdgeCandidate = maxEdge
            for node in subgraph:
                edgeObj = g.edge(node, candidate)
                if edgeObj is None:
                    continue
                weightCandidate = weightCandidate + gedgeweight[edgeObj]
                if gedgeweight[edgeObj] > maxEdgeCandidate:
                    maxEdgeCandidate = gedgeweight[edgeObj]
                noPossibleEdges = len(subgraph) * (len(subgraph)+1)/2
                gammaCandidate = (weightCandidate + weightSubgraph)*1.0/(maxEdgeCandidate * noPossibleEdges)
                if gammaCandidate >= gamma:
                    candidatesWeight.append((candidate, weightCandidate, maxEdgeCandidate))

        candidatesWeight.sort(key=lambda tup:tup[1], reverse = True)
        if len(candidatesWeight) == 0:
            break
        subgraph.add(candidatesWeight[0][0])
        weightSubgraph = weightSubgraph + candidatesWeight[0][1]
        maxEdge = candidatesWeight[0][2]
        #print subgraph, weightSubgraph, maxEdge

    wordSubgraph = []
    for node in subgraph:
        wordSubgraph.append(gnodesname[node])
    return wordSubgraph


def describeEvents(fileIn, interestingIntervals, gamma, sizeClique):
    # interestingIntervals has tuples (word, maximumDeviation, startWindow, endWindow)
    fin = io.open(fileIn, encoding = "utf-8")
    filePointer = fin.tell()
    interestingIntervals.sort(key=lambda tup: tup[2])
    pozInterval = 0
    events = list()
    for interval in interestingIntervals:
        fin.seek(filePointer)
        line = fin.readline()
        line = line.split("\t")
        timestamp_s = long(line[0])
        while  timestamp_s < interval[2]:
            line = fin.readline()
            line = line.split("\t")
            timestamp_s = long(line[0])
        filePointer = fin.tell()
        #print interval
        g = makeGraph(fin, interval[2], interval[3], interval[0])
        subgraph = quasiClique(g, interval[0], gamma, sizeClique)
        newInterval = (interval[0], interval[1], interval[4], interval[5])
        events.append((newInterval,subgraph))
    events.sort(key=lambda tup: tup[0][1], reverse = True)

    return events


def interestingIntervals(beta, fileIn, timeWindow, wordsStats, numberEvents, trials):
    fin = io.open(fileIn, encoding = "utf-8")
    wordsQueue = deque() # I keep the words I've seen in my timeWindow with the timestamp at which they appeared
    tweetsTime = deque() # I will use this queue to know how many tweets I have seen in my time window
    wordsFrequency = dict() # the frequencies of words in the timeWindow
    intervals = dict()
    standardProb = wilsonIntervalUpperBound(1, trials)
    for l in fin:
        line = l.split("\t")
        timestamp_s = long(line[0])
        tweetsTime.append(timestamp_s)
        firstElem = tweetsTime.popleft()
        while len(tweetsTime) > 0 and  firstElem <= timestamp_s - timeWindow:
            firstElem = tweetsTime.popleft()
        tweetsTime.appendleft(firstElem)
        noTweets = len(tweetsTime)
        for elem in line[2].strip().split():
            if  u"b-geo-loc" not in elem and u"b-facility" not in elem:
                continue
            firstElem = None
            if len(wordsQueue)>0:
                firstElem = wordsQueue.popleft()
            while firstElem is not None and len(wordsQueue) > 0 and firstElem[1] <= timestamp_s - timeWindow:
                wordsFrequency[firstElem[0]] = wordsFrequency[firstElem[0]] - 1
                firstElem = wordsQueue.popleft()

            if firstElem is not None:
                wordsQueue.appendleft(firstElem)
            wordsQueue.append((elem, timestamp_s))
            wordsFrequency[elem] = wordsFrequency.get(elem,0) + 1
            if elem in wordsStats:
                expVal = wordsStats[elem][0] * noTweets
                stDev = math.sqrt(wordsStats[elem][1]*wordsStats[elem][0] * noTweets)

            else:
                expVal = standardProb * noTweets
                stDev =  math.sqrt(standardProb*(1 - standardProb) * noTweets)
            threshold = expVal + beta * stDev
            deviation = (wordsFrequency[elem] - expVal)*1.0/stDev
            # intervals has tuples (maximumDeviation, startWindow, endWindow, lastTimeSeen)
            # I need to close intervals even if I don't see words
            if elem in intervals:
                lastSeen = intervals[elem].pop()

                (dev,st,end,last,beginMax) = lastSeen
                newInterval = None
                if lastSeen[2] == 0 and wordsFrequency[elem] < threshold:
                    end = last
                elif lastSeen[2] == 0 and wordsFrequency[elem] >= threshold:
                    last = timestamp_s
                    if lastSeen[0] < deviation:
                        dev = deviation
                        beginMax = timestamp_s - timeWindow
                elif lastSeen[2] != 0 and wordsFrequency[elem] >= threshold:
                    newInterval = (deviation, timestamp_s - timeWindow, 0, timestamp_s, timestamp_s - timeWindow)
                intervals[elem].append((dev,st,end,last, beginMax))
                if newInterval is not None:
                    intervals[elem].append(newInterval)
            elif elem not in intervals and wordsFrequency[elem] >= threshold:
                intervals[elem] = list()
                intervals[elem].append((deviation, timestamp_s - timeWindow, 0, timestamp_s, timestamp_s - timeWindow))

        # make the dictionary intervals a list so that I can sort them
    listOfIntervals = list()
        # intervals has tuples (maximumDeviation, startWindow, endWindow, lastTimeSeen, startMaxInterval)
    for elem in intervals:
        for interval in intervals[elem]:
            end = interval[2]
            if interval[2] == 0:
                end = interval[3]
            # intervals becames  (node, maximumDeviation, startMaxInterval, endWindow, lastTimeSeen, startMaxInterval)
            listOfIntervals.append((elem, interval[0], interval[4], interval[4]+timeWindow,interval[1], end))

    listOfIntervals.sort(key=lambda tup: tup[1], reverse = True)

    return listOfIntervals[:numberEvents]



if __name__ == '__main__':
    numberEvents = 10
    sizeSampleHistory = 10000
    timeWindow = 10800
    thresholdDeviation = 8
    wordsStats = getStatistics(sys.argv[2], sizeSampleHistory)
    intervals =  interestingIntervals(thresholdDeviation, sys.argv[1], timeWindow, wordsStats, numberEvents , sizeSampleHistory)
    descriptions = describeEvents(sys.argv[1], intervals, 0.4, 10)
    for d in descriptions:
        print "Location: "+ d[0][0] +", deviation:" + unicode(d[0][1])+", start time: "+ unicode(d[0][2]) + ", end time: "+ unicode(d[0][3])
        print d[1]
        print "\n"
