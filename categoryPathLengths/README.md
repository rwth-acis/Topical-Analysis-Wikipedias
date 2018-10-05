This directory contains CSV files with shortest path lengths connecting Wikipedia categories over the the category network which measure the topical relatedness of articles, edited within communities as described in the thesis.
The file name denotes the network on which the community detection was conducted, and the number of iterations the algorithm (SLPA) performed.
The network encodings are as follows:
H	-> Directed History Network (Articles point to Authors who edited the article)
HU	-> Undirected History Network (Same as H, only with undirected edges)
AN	-> Undirected Author Network (Authors are adjacent, if the mutually edited an article)

The file named 'arbitrary' contains path lengths corresponding to arbitrary article pairs.
