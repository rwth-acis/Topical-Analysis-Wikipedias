Files are available at [RWTH-Sciebo/Topical-Wikipedia-Analysis](https://rwth-aachen.sciebo.de/s/VgtWe8NJ7BV3ON5)
# Data
This directory contains subdirectories with the data we collected for the three different versions of Wikipedia. These are the vietnamese (**vi**), hebrew (**he**), and serbo-croatian (**sh**) version (language codes are given in the brackets).
All of these subdirectories have the same structure.
## Network
Directory contains CSV and JSON files which may uploaded to a ArangoDB via `arangoimp`. Important in this case is to follow the name conventions so that the records of the edge collections are valid and in order for [wikipics](https://github.com/rwth-acis/Topical-Analysis-Wikipedias/tree/master/arangodb) to work. An overview is provided by the following table:
|File|Collection Name|Description|
|-|-|-|
|**Document Collections**|
|authors|`<lng>`Authors|Wikipedia authors identified by user ID as *key* and *username*|
|pages|`<lng>`Pages|Wikipedia pages identified by ID as *key*, *title*, and *namespace* by ID|
|**Edge Collections**|
|history|`<lng>`History|Edges of the *history network* with *revision ID*, *page title + id* of source node, *username + id* of target node, and *timestamp*|
|linkmap|`<lng>`PageLinks|Edges of the *category network* with *id* of connected nodes and *type* denoting the namesapce of source node|
|authorLinks|`<lng>`AuthorLinks|Edges of the *author network* with *id* of connected nodes and *pages* listing articles both authors have edited|
## Path Lengths
This subdirectory contains files with shortest path lengths connecting Wikipedia categories over the the category network which measure the topical relatedness of articles, edited within communities as described in the paper.
The file name denotes the network on which the community detection was conducted, and the number of iterations the algorithm (SLPA) performed.
The network encodings are as follows:
|Code|Description|
|-|-|
|H|Directed History Network (Articles point to Authors who edited the article)|
|HU|Undirected History Network (Same as *H*, only with undirected edges)|
|AN|Undirected Author Network (Authors are adjacent, if the mutually edited an article)|
|OH|Overlapping History Network (Same as *HU*, only with overlapping communities)|

The file named `arbitrary` contains path lengths corresponding to arbitrary article pairs.
