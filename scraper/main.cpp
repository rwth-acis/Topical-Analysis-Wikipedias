#include <stdio.h>
//#include <iostream>
#include "src/xmlscraper.h"
#include "src/sqlscraper.h"
#include "src/graph.h"
#include <getopt.h>

// default logfile path
#define XML_LOGFILE "./xmlLogfile"
#define SQL_LOGFILE "./sqlLogfile"
#define PARSING_LOGFILE "./parsingLogfile"
#define TXT_EXT ".txt"
#define CSV_EXT ".csv"
// number of attributes
#define NUM_ATTRIBUTES_LINKS 7
#define NUM_ATTRIBUTES_TAXONOMY 2

using namespace std;

/*
 * ERROR CODES::
 * -1: No valid path given
 */
static struct option options[] = {
    {"help", no_argument, NULL, 'h'},
    {"logfile", no_argument, NULL, 'l'},
    {"scrape", no_argument, NULL, 's'},
    {"linkmap", no_argument, NULL, 'i'},
    {"history", required_argument, NULL, 'y'},
    {"csv2json", required_argument, NULL, 'c'},
    {"author_network", no_argument, NULL, 'a'},
    {"authors_history", no_argument, NULL, 'u'},
    {"authors_file", no_argument, NULL, 'f'},
    {"author_article", no_argument, NULL, 't'},
    {"pages", no_argument, NULL, 'p'},
    {"modularity", no_argument, NULL, 'm'},
    {"graph", no_argument, NULL, 'g'},
    {"count", no_argument, NULL, 'n'},
    {0, 0, 0, 0}
};

int main(int argc, char* argv[])
{
    // create scraper object with logfile
    XMLScraper* scraper = NULL;
    SQLScraper* otherScraper = NULL;

    // find out what to do
    char opt;
    string input;
    language lng = EN;
    while ((opt = getopt_long(argc, argv, "hlsiy;c;auftpmgn", options, NULL)) != -1)
    switch(opt)
    {
        // change path to logfile
        case 'h':
            cout << "Hello, I'm your favourite parsing program and can do the following things:\n"
                    "-h/--help prints this hopefully helpful message.\n"
                    "-l/--logfile changes the path to the logfile. Only works if it is passed as first option.\n"
                    "-s/--scrape parses the pages.\n"
                    "-i/--linkmap parses the category links.\n"
                    "-y/--history parses the history.\n"
                    "-a/--author_network parses the links connecting authors by page edits.\n"
                    "-u/--authors_history parses the authors based on the history files.\n"
                    "-f/--authors_file parses the authors from a given file.\n"
                    "-c/--csv2json converts a given csv file to json format\n";
            break;
        case 'l':
            input = "";
            if (argc < 3)
            {
                cout << "Haha! I see that you want me to change the path of the logfile without even doing "
                        "anything. That's very funny :D If you need me, use one of the flags that actually "
                        "do something" << endl;
                return 0;
            }
            while (input.size() < 1)
            {
                cout << "Hello, I am a cool parsing program and am exited that you decided to change the old "
                        "stinky default path of the logfile! Please let me know where I'm suppose to put my "
                        "warnings" << endl;
                cin >> input;
            }
           scraper = new XMLScraper(PARSING_LOGFILE, input.c_str());
            break;
        // parse articles
        case 's':
            input = "";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            cout << "Please tell me language\n";
            cin >> input;
            switch(input[0])
            {
                case 'h':
                    lng = HE;
                    break;
                case 'H':
                    lng = HE;
                    break;
                case 'v':
                    lng = VI;
                    break;
                case 'V':
                    lng = VI;
                    break;
                case 's':
                    lng = SH;
                    break;
                case 'S':
                    lng = SH;
                    break;
            }
            scraper->scrapePages(lng);
            break;
        case 'i':
            input = "";
            if (!otherScraper)
                otherScraper = new SQLScraper(PARSING_LOGFILE, SQL_LOGFILE);
            cout << "Please tell me language\n";
            cin >> input;
            switch(input[0])
            {
                case 'h':
                    lng = HE;
                    break;
                case 'H':
                    lng = HE;
                    break;
                case 'v':
                    lng = VI;
                    break;
                case 'V':
                    lng = VI;
                    break;
                case 's':
                    lng = SH;
                    break;
                case 'S':
                    lng = SH;
                    break;
            }
            otherScraper->createLinkmap(lng);
            break;
        case 'y':
        {
            short fileNr = 0;
            input = "";
            cout << "Please tell me language\n";
            cin >> input;
            switch(input[0])
            {
                case 'h':
                    lng = HE;
                    break;
                case 'H':
                    lng = HE;
                    break;
                case 'v':
                    lng = VI;
                    break;
                case 'V':
                    lng = VI;
                    break;
                case 's':
                    lng = SH;
                    break;
                case 'S':
                    lng = SH;
                    break;
            }
            input = "";
            if (lng == HE)
            {
                if (!optarg)
                    cout << "Hello, I am your cool parsing program and enjoy parsing history files!\n"
                            "However, this is a bit much information to get through all at once, so you'll "
                            "have to tell me which file I should get to parsing" << endl;
                else
                   fileNr = stoi(optarg);
                while (!fileNr)
                {
                    cin >> input;
                    try {
                        fileNr = stoi(input);}
                    catch (const invalid_argument) {
                        cout << "Hm... I seem to be unable to convert your input into a number.\n"
                                "I'm sure you just mistyped, try again" << endl;}
                }
            }
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            scraper->historyToCSV(fileNr, lng);
            break;
        }
        case 'c':
            if (!optarg)
                cout << "Hey, what's up with that?! If you want me to convert something, maybe tell me what\n";
            else
                ParsingUtil::csv2json(optarg);
            break;
        case 'a':
            input = "";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            cout << "Please tell me language\n";
            cin >> input;
            switch(input[0])
            {
                case 'h':
                    lng = HE;
                    break;
                case 'H':
                    lng = HE;
                    break;
                case 'v':
                    lng = VI;
                    break;
                case 'V':
                    lng = VI;
                    break;
                case 's':
                    lng = SH;
                    break;
                case 'S':
                    lng = SH;
                    break;
            }
            scraper->getAuthorLinks(lng);
            break;
        case 'u':
            input = "";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            cout << "Please tell me language\n";
            cin >> input;
            switch(input[0])
            {
                case 'h':
                    lng = HE;
                    break;
                case 'H':
                    lng = HE;
                    break;
                case 'v':
                    lng = VI;
                    break;
                case 'V':
                    lng = VI;
                    break;
                case 's':
                    lng = SH;
                    break;
                case 'S':
                    lng = SH;
                    break;
            }
            scraper->getAuthors(lng);
            break;
        case 'f':
        {
            input = "";
            string inputTwo = "";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            while (input == "")
            {
                cout << "Heyho! Would you kindly tell me the source file path\n";
                cin >> input;
            }
            while (inputTwo == "")
            {
                cout << "Yeah, that's what I'm talking about!\nAnd now please tell me the path of the output file\n";
                cin >> inputTwo;
            }
            scraper->getAuthors(input.c_str(), inputTwo.c_str());
        }
            break;
        case 't':
        {
            input = "";
            string inputTwo = "";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            while (input == "")
            {
                cout << "Heyho! Would you kindly tell me the source file path\n";
                cin >> input;
            }
            while (inputTwo == "")
            {
                cout << "Yeah, that's what I'm talking about!\nAnd now please tell me the path of the output file\n";
                cin >> inputTwo;
            }
            scraper->getAuthorArticle(input.c_str(), inputTwo.c_str());
        }
            break;
        case 'p':
        {
            input = "";
            string inputTwo = "";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            while (input == "")
            {
                cout << "Heyho! Would you kindly tell me the source file path\n";
                cin >> input;
            }
            while (inputTwo == "")
            {
                cout << "Yeah, that's what I'm talking about!\nAnd now please tell me the path of the output file\n";
                cin >> inputTwo;
            }
            scraper->getPages(input.c_str(), inputTwo.c_str());
        }
            break;
        case 'm':
        {
            input = "";
            cout << "Please insert the name of the collection in according to which the graph should be constructed!\n";
            cin >> input;
            Graph* graph = new Graph(input);
            graph->getModularity();
        }
            break;
        case 'g':
        {
            input = "";
            cout << "Good Day! I'm happy to construct your graph, please just provide the base path to the file containing the edge information!\n";
            cin >> input;
            Graph* graph = new Graph(input.c_str());
            delete graph;
            break;
        }
        case 'n':
        {
            input = "";
            cout << "Good Day! I'm happy to count so tell me a file path, bro!!\n";
            cin >> input;
            ParsingUtil* parser = new ParsingUtil();
            parser->countPages(input.c_str());
            break;
        }
        default:
            cout << "Hello, I'm your favourite parsing program and can do the following things:\n"
                    "-h/--help prints this hopefully helpful message.\n"
                    "-l/--logfile changes the path to the logfile. Only works if it is passed as first option.\n"
                    "-s/--scrape parses the pages.\n"
                    "-i/--linkmap parses the category links.\n"
                    "-y/--history parses the history.\n"
                    "-a/--author_network parses the links connecting authors by page edits.\n"
                    "-u/--authors_history parses the authors based on the history files.\n"
                    "-f/--authors_file parses the authors from a given file.\n"
                    "-c/--csv2json converts a given csv file to json format\n";
            break;
    }
    return 0;
}
