The ids of the tweets used in the evaluation of the algorithm presented in 

"EviDense: a Graph-based Method for Finding Unique High-impact Events with
Succinct Keyword-based Descriptions"


are publicly available, together with the code.


# Preprocessing the data
The preprocessing step requires the entity tagger which can be found here:
https://github.com/aritter/twitter_nlp

The tagger directory (twitter_nlp) should be in the same folder with the python files ritter.py and process_file.py. 
Usage:
export TWITTER_NLP=twitter_nlp
python process_file.py input_file.bz2 output_file.txt

The input_file.bz2 is a bz2 archive of a txt file which contains on each line a json of a tweet collected with 
the Twitter Streaming API. 
The output_file.txt contains on each line a timestamp separated by a tab from the filtered and tagged text of a tweet.


