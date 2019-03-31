#include "graph.h"
#include "util/loggingutil.h"

#define FROM "from:\""
#define TO "to:\""
#define HISTORY_FILE "History"
#define PAGES_PATTERN "Pages/"
#define AUTHOR_PATTERN "Authors/"
#define KEY_PATTERN "_key\":\""
#define COMMUNITY_PATTERN "community\":"
#define H3 "slpaH3\":"
#define H10 "slpaH10\":"
#define HU3 "slpaHU3\":"
#define HU10 "slpaHU10\":"
#define HU100 "slpaHU100\":"
#define AN3 "slpaAN3\":"
#define AN10 "slpaAN10\":"
#define AN100 "slpaAN100\":"


using namespace std;
// construct graph based on an edge file
Graph::Graph(const char* ipFilePath)
{
    vertices = new unordered_set<string>();
    edges = new unordered_set<string>();
    degrees = new unordered_map<string,size_t>();
    size_t pageCount = 0;
    unsigned char fileCount = 1;
    FILE* ipFile = fopen(ipFilePath, "r");
    string fPath = ipFilePath;
    if (!ipFile)
    {
        fPath = ipFilePath + to_string(fileCount) + ".csv";
        ipFile = LoggingUtil::openFile(fPath, false);
        if (!ipFile)
            return;
        else
            do
            {
             fclose(ipFile);
             fileCount++;
             fPath = ipFilePath + to_string(fileCount) + ".csv";
            } while ((ipFile = fopen(fPath.c_str(),"r")));
    }
    for (unsigned char i = 1; i < fileCount; i++)
    {
        fPath = ipFilePath + to_string(i) + ".csv";
        ipFile = LoggingUtil::openFile(fPath,false);
        cout << "Reading file " << to_string(i) << '/' << (fileCount - 1) << endl;
        while (parser.findPattern(ipFile, PAGES_PATTERN))
        {
            // get Page
            string fromVertex = PAGES_PATTERN;
            fromVertex += parser.writeToString(ipFile, '\"');
            // get Author
            parser.findPattern(ipFile, AUTHOR_PATTERN);
            string toVertex = AUTHOR_PATTERN;
            toVertex += parser.writeToString(ipFile, '\"');
            // add vertices
            if (vertices->find(fromVertex) == vertices->end())
            {
                vertices->insert(fromVertex);
                pageCount++;
            }
            if (vertices->find(toVertex) == vertices->end())
                vertices->insert(toVertex);
            // add edge
            if (edges->find(fromVertex+toVertex) != edges->end())
                continue;
            edges->insert(fromVertex+toVertex);
            auto it = degrees->find(fromVertex);
            if (it != degrees->end())
                it->second++;
            degree++;
        }
    }
    cout << "Constructed graph with " << pageCount << " Articles, " << vertices->size() - pageCount << " Authors, and " << degree << " edges\n";
}

// construct graph based on given files
Graph::Graph(string collection)
{
    vertices = new unordered_set<string>();
    edges = new unordered_set<string>();
    degrees = new unordered_map<string,size_t>();
    communities = new unordered_map<string,size_t>();
    unordered_set<size_t>* allCommys = new unordered_set<size_t>();

    cout << "Constructing graph vertices!\n";
    parser = ParsingUtil();
    string communityPattern;
    this->collection = collection;
    if (collection == "H3")
    {
        communityPattern = H3;
        directed = true;
        history = true;
    }
    else if (collection == "H10")
    {
        communityPattern = H10;
        directed = true;
        history = true;
    }
    else if (collection == "HU3")
    {
        communityPattern = HU3;
        directed = false;
        history = true;
    }
    else if (collection == "HU10")
    {
        communityPattern = HU10;
        directed = false;
        history = true;
    }
    else if (collection == "HU100")
    {
        communityPattern = HU100;
        directed = false;
        history = true;
    }
    else if (collection == "AN3")
    {
        communityPattern = AN3;
        directed = false;
        history = false;
    }
    else if (collection == "AN10")
    {
        communityPattern = AN10;
        directed = false;
        history = false;
    }
    else if (collection == "AN100")
    {
        communityPattern = AN100;
        directed = false;
        history = false;
    }
    else
    {
        cerr << "No matching collection found!\n";
        return;
    }
    // laod authors
    string ipFilePath;
    cout << "Please insert path to author file\n";
    cin >> ipFilePath;
    FILE* ipFile = LoggingUtil::openFile(ipFilePath.c_str(), false);
    if (!ipFile)
            return;
    while (parser.findPattern(ipFile, AUTHOR_PATTERN))
    {
        string key = parser.writeToString(ipFile, '\"');
        string id = AUTHOR_PATTERN + key;
        vertices->insert(id);
        degrees->insert({id,0});
        parser.findPattern(ipFile, communityPattern);
        size_t community = stoi(parser.writeToString(ipFile, ','));
        communities->insert({id,community});
        allCommys->insert(community);
    }
    // load edges for author graph
    if (!history)
    {
        ipFilePath = "";
        cout << "Please insert base path to authorLinks file\n";
        cin >> ipFilePath;
        // get amount of files
        short fileCount = 1;
        string fPath = ipFilePath + to_string(fileCount) + ".csv";
        ipFile = LoggingUtil::openFile(fPath, false);
        if (!ipFile)
                return;
        do
        {
            fclose(ipFile);
            fileCount++;
            fPath = ipFilePath + to_string(fileCount) + ".csv";
        } while ((ipFile = fopen(fPath.c_str(),"r")));
        // parse files
        for (short i = 1; i < fileCount; i++)
        {
            fPath = ipFilePath + to_string(i) + ".csv";
            ipFile = LoggingUtil::openFile(fPath,false);
            cout << "Reading file " << i << '/' << (fileCount - 1) << endl;
            while (parser.findPattern(ipFile, AUTHOR_PATTERN))
            {
                // get author vertices
                string fromVertex = AUTHOR_PATTERN;
                fromVertex += parser.writeToString(ipFile, '\"');
                parser.findPattern(ipFile, AUTHOR_PATTERN);
                string toVertex = AUTHOR_PATTERN;
                toVertex += parser.writeToString(ipFile, '\"');
                // add edges
                edges->insert(fromVertex+toVertex);
                edges->insert(toVertex+fromVertex);
                auto it = degrees->find(fromVertex);
                if (it == degrees->end())
                    LoggingUtil::warning("No match for vertex "  + fromVertex);
                else
                    it->second++;
                it = degrees->find(toVertex);
                if (it == degrees->end())
                    LoggingUtil::warning("No match for vertex "  + toVertex);
                else
                    it->second++;
                degree++;
            }
        }
        // done for author network
        return;
    }
    // load page vertices
    ipFilePath = "";
    cout << "Please insert path to pages file\n";
    cin >> ipFilePath;
    ipFile = LoggingUtil::openFile(ipFilePath, false);
    while (parser.findPattern(ipFile, PAGES_PATTERN))
    {
        // get page id
        string key = parser.writeToString(ipFile, '\"');
        string id = PAGES_PATTERN + key;
        // get community
        parser.findPattern(ipFile, communityPattern);
        size_t community = stoi(parser.writeToString(ipFile, {',','}'}));
        // skip community if no author is part of it
        if (allCommys->find(community) == allCommys->end())
            continue;
        // add vertex
        if (vertices->find(id) == vertices->end())
        {
            vertices->insert(id);
            communities->insert({id,community});
            degrees->insert({id,0});
        }
    }
    // load edges
    ipFilePath = "";
    cout << "Please insert base path to history files" << endl;
    cin >> ipFilePath;
    // get amount of files
    short fileCount = 1;
    string fPath = ipFilePath + to_string(fileCount) + ".csv";
    ipFile = LoggingUtil::openFile(fPath, false);
    if (!ipFile)
        return;
    do
    {
        fclose(ipFile);
        fileCount++;
        fPath = ipFilePath + to_string(fileCount) + ".csv";
    } while ((ipFile = fopen(fPath.c_str(),"r")));
    // parse files
    for (short i = 1; i < fileCount; i++)
    {
        fPath = ipFilePath + to_string(i) + ".csv";
        ipFile = LoggingUtil::openFile(fPath,false);
        cout << "Reading file " << i << '/' << (fileCount - 1) << endl;
        while (parser.findPattern(ipFile, PAGES_PATTERN))
        {
            // get Page
            string fromVertex = PAGES_PATTERN;
            fromVertex += parser.writeToString(ipFile, '\"');
            // get Author
            parser.findPattern(ipFile, AUTHOR_PATTERN);
            string toVertex = AUTHOR_PATTERN;
            toVertex += parser.writeToString(ipFile, '\"');
            // add edge
            if (edges->find(fromVertex+toVertex) != edges->end())
                continue;
            edges->insert(fromVertex+toVertex);
            auto it = degrees->find(fromVertex);
            if (it != degrees->end())
                it->second++;
            if (!directed)
            {
                edges->insert(toVertex+fromVertex);
                it = degrees->find(toVertex);
                if (it != degrees->end())
                    it->second++;
            }
            degree++;
        }
    }
    // done
}

double Graph::getModularity()
{
    if (modularity != 42)
        return modularity;
    double sum = 0;
    size_t count = 0, vertexCount = vertices->size();
    short progress = 1;
    cout << "Computing modularity for network of " << vertexCount << " vertices and degree " << degree << endl;
    // iterate over all vertices
    for (auto it = vertices->begin(); it != vertices->end(); it++)
    {
        // if vertex is assigned no community skip
        auto itCommunity = communities->find(*it);
        if (itCommunity == communities->end())
            continue;
        for (auto jt = it; jt != vertices->end(); jt++)
        {
            // no loops
            if (it == jt)
                continue;
            // only consider pairs of articles and authors for history
            if (history && (*it)[0] == (*jt)[0])
                continue;
            // if vertex is assigned no community skip
            auto jtCommunity = communities->find(*jt);
            if (jtCommunity == communities->end())
                continue;
            // vertices of different communities get skipped
            if (itCommunity->second != jtCommunity->second)
                    continue;
            // get entry of adjacency matrix
            bool a = (edges->find((*it) + (*jt)) != edges->end());
            // get minuend
            double frac = degrees->find(*it)->second * degrees->find(*jt)->second / (2 * degree);
            sum += (a - frac);
        }
        count++;
        // progress indicator
        if ((((double) count*20)/vertexCount) > progress || (vertexCount - count < 10 && progress < 20)) // a magic 10
        {
            cout << "Progress: " << progress*5 << "%\n";
            progress++;
        }
    }
    cout << "Modularity: " << (sum / degree) << endl;
    return (sum / degree);
}
