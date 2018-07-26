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
#define SQL_USERS_PATH "../../media/english/rawFiles/enwiki-latest-user_groups.sql"
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
    {"scrape", no_argument, NULL, 's'},
    {"linkmap", no_argument, NULL, 'i'},
    {"botset", no_argument, NULL, 'b'},
    {"history", required_argument, NULL, 'h'},
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
    while ((opt = getopt_long(argc, argv, "lsibh;", options, NULL)) != -1)
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
        case 's':
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            scraper->scrapePages();
            break;
        case 'i':
            if (!otherScraper)
                otherScraper = new SQLScraper(PARSING_LOGFILE, SQL_LOGFILE);
            otherScraper->createLinkmap(SQL_CATEGORYLINKS_PATH);
            break;
        case 'b':
            if (!otherScraper)
                otherScraper = new SQLScraper(PARSING_LOGFILE, SQL_LOGFILE);
            otherScraper->createBotSet(SQL_USERS_PATH);
            break;
        case 'h':
            short fileNr = 0;
            if (!optarg)
            {
                input = "";
                cout << "Hello, I am your cool parsing program and enjoy parsing history files!\n"
                        "However, this is a bit much information to get through all at once, so you'll "
                        "have to tell me which file I should get to parsing" << endl;
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
            else
            {
                try {
                    fileNr = stoi(input);}
                catch (const invalid_argument) {
                    cout << "Hm... I seem to be unable to convert your input into a number.\n"
                            "I'm sure you just mistyped, try again" << endl;}
            }
            if (!scraper)
                scraper = new XMLScraper(PARSING_LOGFILE, XML_LOGFILE);
            scraper->historyToCSV(fileNr);
            break;
    }
    return 0;
}
