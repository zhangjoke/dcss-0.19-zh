/**
 * @file
 * database.h
**/

#ifndef DATABASE_H
#define DATABASE_H

#include <list>

#ifdef DB_NDBM
extern "C" {
#   include <ndbm.h>
}
#elif defined(DB_DBH)
extern "C" {
#   define DB_DBM_HSEARCH 1
#   include <db.h>
}
#elif defined(USE_SQLITE_DBM)
#   include "sqldbm.h"
#else
#   error DBM interfaces unavailable!
#endif

#define DPTR_COERCE char *

void databaseSystemInit();
void databaseSystemShutdown();

typedef bool (*db_find_filter)(string key, string body);

string getQuoteString(const string &key);
string getLongDescription(const string &key);

vector<string> getLongDescKeysByRegex(const string &regex,
                                      db_find_filter filter = nullptr);
vector<string> getLongDescBodiesByRegex(const string &regex,
                                        db_find_filter filter = nullptr);

string getGameStartDescription(const string &key);

string getShoutString(const string &monst, const string &suffix = "");
string getSpeakString(const string &key);
string getRandNameString(const string &itemtype, const string &suffix = "");
string getHelpString(const string &topic);
string getMiscString(const string &misc, const string &suffix = "");
string getHintString(const string &key);

vector<string> getAllFAQKeys();
string getFAQ_Question(const string &key);
string getFAQ_Answer(const string &question);

string jtrans(const char* key, const bool linefeed = false);
string jtrans(const string &key, const bool linefeed = false);
#define jtransln(x) (jtrans(x, true))
#define jtransc(x) (jtrans(x).c_str())
#define jtranslnc(x) (jtrans(x, true).c_str())
bool jtrans_has_key(const string &key);
string tagged_jtrans(const string &prefix, const string &key, const string &suffix = "");
#define tagged_jtransc(p, k) (tagged_jtrans(p, k).c_str())
string jtrans_notrim(const string &key);
#define jtrans_notrimc(x) (jtrans_notrim(x).c_str())
#endif
