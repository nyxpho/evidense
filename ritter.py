#!/usr/bin/python

import sys
import os
import re
import subprocess
import platform
import time
import codecs

from signal import *

from optparse import OptionParser

BASE_DIR = 'twitter_nlp.jar'

if os.environ.has_key('TWITTER_NLP'):
    BASE_DIR = os.environ['TWITTER_NLP']

sys.path.append('%s/python' % (BASE_DIR))
sys.path.append('%s/python/ner' % (BASE_DIR))
sys.path.append('%s/hbc/python' % (BASE_DIR))

import Features
import twokenize
from LdaFeatures import LdaFeatures
from Dictionaries import Dictionaries
from Vocab import Vocab

sys.path.append('%s/python/cap' % (BASE_DIR))
sys.path.append('%s/python' % (BASE_DIR))
import cap_classifier
import pos_tagger_stdin
import chunk_tagger_stdin
import event_tagger_stdin

def GetNer(ner_model):
    return subprocess.Popen('java -Xmx256m -cp %s/mallet-2.0.6/lib/mallet-deps.jar:%s/mallet-2.0.6/class cc.mallet.fst.SimpleTaggerStdin --weights sparse --model-file %s/models/ner/%s' % (BASE_DIR, BASE_DIR, BASE_DIR, ner_model),
                           shell=True,
                           close_fds=True,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE)

def GetLLda():
    return subprocess.Popen('%s/hbc/models/LabeledLDA_infer_stdin.out %s/hbc/data/combined.docs.hbc %s/hbc/data/combined.z.hbc 100 100' % (BASE_DIR, BASE_DIR, BASE_DIR),
                           shell=True,
                           close_fds=True,
                           stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE)

#if platform.architecture() != ('64bit', 'ELF'):
#    sys.exit("Requires 64 bit Linux")

class RitterTagger:
    def __init__(self):
        self.eventTagger = None
        self.posTagger = None
        self.chunkTagger = None
        self.llda = GetLLda()
        self.ner = GetNer('ner_nopos_nochunk.model')
        self.fe = Features.FeatureExtractor('%s/data/dictionaries' % (BASE_DIR))
        self.capClassifier = cap_classifier.CapClassifier()

        self.vocab = Vocab('%s/hbc/data/vocab' % (BASE_DIR))

        self.dictMap = {}
        i = 1
        for line in open('%s/hbc/data/dictionaries' % (BASE_DIR)):
            dictionary = line.rstrip('\n')
            self.dictMap[i] = dictionary
            i += 1

        dict2index = {}
        for i in self.dictMap.keys():
            dict2index[self.dictMap[i]] = i

        if self.llda:
            self.dictionaries = Dictionaries('%s/data/LabeledLDA_dictionaries3' % (BASE_DIR), dict2index)
        self.entityMap = {}
        i = 0
        if self.llda:
            for line in open('%s/hbc/data/entities' % (BASE_DIR)):
                entity = line.rstrip('\n')
                self.entityMap[entity] = i
                i += 1

        self.dict2label = {}
        for line in open('%s/hbc/data/dict-label3' % (BASE_DIR)):
            (dictionary, label) = line.rstrip('\n').split(' ')
            self.dict2label[dictionary] = label

   



    def process_tweet(self, line):
    
        words = twokenize.tokenize(line)
        seq_features = []
        tags = []

        goodCap = self.capClassifier.Classify(words) > 0.9

        if self.posTagger:
            pos = self.posTagger.TagSentence(words)
            #pos = [p.split(':')[0] for p in pos]  # remove weights
            pos = [re.sub(r':[^:]*$', '', p) for p in pos]  # remove weights
        else:
            pos = None

        # Chunking the tweet
        if self.posTagger and self.chunkTagger:
            word_pos = zip(words, [p.split(':')[0] for p in pos])
            chunk = self.chunkTagger.TagSentence(word_pos)
            chunk = [c.split(':')[0] for c in chunk]  # remove weights
        else:
            chunk = None

        #Event tags
        if self.posTagger and self.eventTagger:
            events = self.eventTagger.TagSentence(words, [p.split(':')[0] for p in pos])
            events = [e.split(':')[0] for e in events]
        else:
            events = None

        quotes = Features.GetQuotes(words)
        for i in range(len(words)):
            features = self.fe.Extract(words, pos, chunk, i, goodCap) + ['DOMAIN=Twitter']
            if quotes[i]:
                features.append("QUOTED")
            seq_features.append(" ".join(features))
        self.ner.stdin.write(("\t".join(seq_features) + "\n").encode('utf8'))

        for i in range(len(words)):
            tags.append(self.ner.stdout.readline().rstrip('\n').strip(' '))

        features = LdaFeatures(words, tags)

        #Extract and classify entities
        for i in range(len(features.entities)):
            type = None
            wids = [str(self.vocab.GetID(x.lower())) for x in features.features[i] if self.vocab.HasWord(x.lower())]
            if self.llda and len(wids) > 0:
                entityid = "-1"
                if self.entityMap.has_key(features.entityStrings[i].lower()):
                    entityid = str(self.entityMap[features.entityStrings[i].lower()])
                labels = self.dictionaries.GetDictVector(features.entityStrings[i])

                if sum(labels) == 0:
                    labels = [1 for x in labels]
                self.llda.stdin.write("\t".join([entityid, " ".join(wids), " ".join([str(x) for x in labels])]) + "\n")
                sample = self.llda.stdout.readline().rstrip('\n')
                labels = [self.dict2label[self.dictMap[int(x)]] for x in sample[4:len(sample)-8].split(' ')]

                count = {}
                for label in labels:
                    count[label] = count.get(label, 0.0) + 1.0
                maxL = None
                maxP = 0.0
                for label in count.keys():
                    p = count[label] / float(len(count))
                    if p > maxP or maxL == None:
                        maxL = label
                        maxP = p

                if maxL != 'None':
                    tags[features.entities[i][0]] = "B-%s" % (maxL)
                    for j in range(features.entities[i][0]+1,features.entities[i][1]):
                        tags[j] = "I-%s" % (maxL)
                else:
                    tags[features.entities[i][0]] = "O"
                    for j in range(features.entities[i][0]+1,features.entities[i][1]):
                        tags[j] = "O"
            else:
                tags[features.entities[i][0]] = "B-ENTITY"
                for j in range(features.entities[i][0]+1,features.entities[i][1]):
                    tags[j] = "I-ENTITY"

        output = " ".join("%s/%s" % (words[x], tags[x]) for x in range(len(words)))
        if pos:
            output = " ".join("%s/%s" % (output[x], pos[x]) for x in range(len(output)))
        if chunk:
            output = " ".join("%s/%s" % (output[x], chunk[x]) for x in range(len(output)))
        if events:
            output = " ".join("%s/%s" % (output[x], events[x]) for x in range(len(output)))

        return output

