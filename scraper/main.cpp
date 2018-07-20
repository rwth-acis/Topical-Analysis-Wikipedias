#include <stdio.h>
//#include <iostream>
#include "src/xmlscraper.h"
#include "src/sqlscraper.h"
#include <getopt.h>

// default logfile path
#define XML_LOGFILE "./xmlLogfile"
#define SQL_LOGFILE "./sqlLogfile"
#define PARSING_LOGFILE "./parsingLogfile"
// sql input files
#define SQL_CATEGORYLINKS_PATH "../../media/english/rawFiles/enwiki-20180701-categorylinks.sql"
#define SQL_TAXONOMY_PATH "../../media/english/rawFiles/wiki.category.taxonomy.tree.sql"
// parsed sql files
#define TXT_CATEGORYLINKS_PATH "../../media/english/categorylinks"
#define TXT_TAXONOMY_PATH "../../media/english/wiki.category.taxonomy.tree"
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
    {"logfile", no_argument, NULL, 'l'},
    {"articles", no_argument, NULL, 'a'},
    {"categories", no_argument, NULL, 'c'},
    {"filter", required_argument, NULL, 'f'},
    {"missing", required_argument, NULL, 'm'},
    {"range", required_argument, NULL, 'r'},
    {"sql", required_argument, NULL, 's'},
    {"linkmap", no_argument, NULL, 'i'},
    {0, 0, 0, 0}
};

int main(int argc, char* argv[])
{
    if (argc < 2)
        cout << "Hello, I am a cool parsing program and can do the following things:\n"
            "--filter/-f parses files in ../media/english/rawFiles and saves them to "
            "../media/english/xmlFiles. Please specify either articles, categories or both "
            "so I know what to look for.\n"
            "--articles/-a and --categories/-c parses files in ../media/english/xmlFiles "
            "for articles and categories and writes the result to ../media/english/jsonFiles\n"
            "--logfile/-l changes the path of the logfile (default: ./Logfile) but if you think "
            "that this is a good idea, please make this your first flag. Otherwise I won't know "
            "until it's already too late :(\n"
            "--filter/-f only filters either articles or categories out of ../media/english/rawFile "
            "and writes the result to ../media/english/xmlFiles\n"
            "--range/-r filter either articles or categories only out of certain file in "
            "../media/english/rawFiles and writes the result to ../media/english/xmlFiles\n"
            "--sql/-s writes sql files located at ../media/english/rawFiles either to readable txt "
            "files or CSV files" << endl;

    // create scraper object with logfile
    XMLScraper* scraper = NULL;
    SQLScraper* otherScraper = NULL;

    // find out what to do
    char opt;
    string input;
    string path;
    bool filterA = false;
    bool filterC = false;
    int total = 0;
    int totalA = 0;
    int totalC = 0;
    int count = 0;
    while ((opt = getopt_long(argc, argv, "laci", options, NULL)) != -1)
    switch(opt)
    {
        // change path to logfile
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
        case 'a':
            input = "";
            cout << "Hello, I am a horrible disappointment and leaking like an 80 year old D;\n"
                    "You'll have to tell me, which of the six article files you want me to parse...\n";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            while (input == "")
                cin >> input;
            scraper->scrapeArticles(stoi(input));
            break;
        // parse categories
        case 'c':
            cout << "Hello, I am a cool parsing program and I am gonna get right to parsing your categories :)\n";
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            scraper->scrapeCategories();
            break;
        // get filtered xml files
        case 'i':
            if (!otherScraper)
                otherScraper = new SQLScraper(PARSING_LOGFILE, SQL_LOGFILE);
            otherScraper->createLinkmap(SQL_CATEGORYLINKS_PATH);
            break;
    }
    return 0;
}
