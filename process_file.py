import sys
import os
import bz2
import json
import codecs
import ritter
import io
import re

stopwords = []
with io.open("stopwords", encoding = "utf-8") as f:
    stopwords = f.read().splitlines()

def filter(line):
    new_line = []
    exp = re.compile("""[\w_'/-]+""")
    for e in exp.findall(line.lower()):
         if u"http" not in e and len(e) >= 5 and u"/" in e and e[0: e.index('/')] not in stopwords:
             new_line.append(e)
    n = 0
    tagged_line = ""
    while n < len(new_line):
            entity = "";
            if u"/b-"  in new_line[n]:
                    typeent = new_line[n][new_line[n].index("/")+1:]
                    entity = new_line[n][0: new_line[n].index("/")]
                    while n+1 < len(new_line) and (u"/i-" in new_line[n+1]):
                        n=n+1
                        entity = entity + new_line[n][0: new_line[n].index("/")]
                    tagged_line+=(typeent+"_"+entity+" ")
            else:
                    tagged_line+=(new_line[n][0: new_line[n].index("/")]+" ")
            n=n+1
            #print tagged_line
    return tagged_line.strip()


def process_tweets(file_in, file_out):
    rb = bz2.BZ2File(file_in)
    line = rb.readline()
    wb = io.open(file_out, "w",  encoding="utf-8")
    ritterTagger = ritter.RitterTagger()
    while line != "" :
        tweet_json = json.loads(line)
        text = tweet_json["text"]
        time = tweet_json["timestamp_ms"]
	time = time[0:len(time)-3]
        if "retweeted_status.text" in tweet_json:
            text = tweet_json["retweeted_status.text"]
        processed_line = ritterTagger.process_tweet(text)
        filtered_line = filter(processed_line)
        wb.write(time + "\t" + filtered_line + "\n")
        line = rb.readline().strip()
    rb.close()
    wb.close()



def process_news(file_name, file_out):
    rb = codecs.open(file_name, "r", "utf-8", errors='ignore')
    line = rb.readline().strip()
    wb = open(file_out, "w")
    while line != "" :
        processed_line = process_tweet(line.split("\t")[1])
        filtered_line = filter(processed_line)
        time = line.split("\t")[0]
        wb.write(time + "\t" + filtered_line + "\n")
        line = rb.readline().strip()
    rb.close()
    wb.close()

if __name__ == '__main__':
    process_tweets(sys.argv[1], sys.argv[2])
    
