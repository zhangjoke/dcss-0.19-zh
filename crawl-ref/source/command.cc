/**
 * @file
 * @brief Misc commands.
**/

#include "AppHdr.h"

#include "command.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>

#include "chardump.h"
#include "database.h"
#include "describe.h"
#include "env.h"
#include "files.h"
#include "invent.h"
#include "itemprop.h"
#include "items.h"
#include "libutil.h"
#include "lookup_help.h"
#include "macro.h"
#include "message.h"
#include "output.h"
#include "prompt.h"
#include "showsymb.h"
#include "state.h"
#include "stringutil.h"
#include "syscalls.h"
#include "unicode.h"
#include "version.h"
#include "viewchar.h"
#include "view.h"

static const char *features[] =
{
#ifdef CLUA_BINDINGS
    "Lua user scripts",
#endif

#ifdef USE_TILE_LOCAL
    "Tile support",
#endif

#ifdef USE_TILE_WEB
    "Web Tile support",
#endif

#ifdef WIZARD
    "Wizard mode",
#endif

#ifdef DEBUG
    "Debug mode",
#endif

#if defined(REGEX_POSIX)
    "POSIX regexps",
#elif defined(REGEX_PCRE)
    "PCRE regexps",
#else
    "Glob patterns",
#endif

#if defined(SOUND_PLAY_COMMAND) || defined(WINMM_PLAY_SOUNDS)
    "Sound support",
#endif

#ifdef DGL_MILESTONES
    "Milestones",
#endif
};

static string _get_version_information()
{
    return string("このゲームは<w>" CRAWL "日本語版</w>のバージョン<w>") + Version::Long + "</w>です\n";
}

static string _get_version_features()
{
    string result = jtrans_notrim("<w>Features</w>\n"
                                  "--------\n");

    for (const char *feature : features)
    {
        result += " * ";
        result += jtrans(feature);
        result += "\n";
    }

    return result;
}

static void _add_file_to_scroller(FILE* fp, formatted_scroller& m,
                                  int first_hotkey  = 0,
                                  bool auto_hotkeys = false);

static string _get_version_changes()
{
    // Attempts to print "Highlights" of the latest version.
    FILE* fp = fopen_u(datafile_path("changelog.txt", false).c_str(), "r");
    if (!fp)
        return "";

    string result = "";
    string help;
    char buf[200];
    bool start = false;
    while (fgets(buf, sizeof buf, fp))
    {
        // Remove trailing spaces.
        for (int i = strlen(buf) - 1; i >= 0; i++)
        {
            if (isspace(buf[i]))
                buf[i] = 0;
            else
                break;
        }
        help = buf;

        // Look for version headings
        if (starts_with(help, "Stone Soup "))
        {
            // Stop if this is for an older major version; otherwise, highlight
            if (help.find(string("Stone Soup ")+Version::Major) == string::npos)
                break;
            else
                goto highlight;
        }

        if (help.find("Highlights") != string::npos ||
            help.find(jtrans("Highlights")) != string::npos)
        {
        highlight:
            // Highlight the Highlights, so to speak.
            result += "<w>" + help + "</w>\n";
            // And start printing from now on.
            start = true;
        }
        else if (!start)
            continue;
        else
        {
            result += buf;
            result += "\n";
        }
    }
    fclose(fp);

    // Did we ever get to print the Highlights?
    if (start)
    {
        result.erase(1+result.find_last_not_of('\n'));
        result += "\n\n";
        result += jtrans("For earlier changes, see changelog.txt "
                         "in the docs/ directory.");
    }
    else
    {
        result += jtrans("For a list of changes, see changelog.txt in the docs/ "
                         "directory.");
    }

    result += "\n\n";

    return result;
}

//#define DEBUG_FILES
static void _print_version()
{
    formatted_scroller cmd_version;

    // Set flags.
    int flags = MF_NOSELECT | MF_ALWAYS_SHOW_MORE | MF_NOWRAP | MF_EASY_EXIT;
    cmd_version.set_flags(flags, false);
    cmd_version.set_tag("version");
    cmd_version.set_more();

    cmd_version.add_text(_get_version_information(), true);
    cmd_version.add_text(_get_version_features(), true);
    cmd_version.add_text(_get_version_changes(), true);

    cmd_version.show();
}

void list_armour()
{
    ostringstream estr;
    for (int j = EQ_MIN_ARMOUR; j <= EQ_MAX_ARMOUR; j++)
    {
        const equipment_type i = static_cast<equipment_type>(j);
        const int armour_id = you.equip[i];
        int       colour    = MSGCOL_BLACK;

        estr.str("");
        estr.clear();

        estr << chop_string(tagged_jtrans("[eq slot]", 
                ((i == EQ_CLOAK)       ? "Cloak  " :
                 (i == EQ_HELMET)      ? "Helmet " :
                 (i == EQ_GLOVES)      ? "Gloves " :
                 (i == EQ_SHIELD)      ? "Shield " :
                 (i == EQ_BODY_ARMOUR) ? "Armour " :
                 (i == EQ_BOOTS) ?
                 ((you.species == SP_CENTAUR
                   || you.species == SP_NAGA) ? "Barding"
                                              : "Boots  ")
                                 : "unknown")), 9)
             + " : ";

        if (you_can_wear(i) == MB_FALSE)
            estr << jtrans_notrim("    (unavailable)");
        else if (you_can_wear(i, true) == MB_FALSE)
            estr << jtrans_notrim("    (currently unavailable)");
        else if (armour_id != -1)
        {
            estr << you.inv[armour_id].name(DESC_INVENTORY);
            colour = menu_colour(estr.str(), item_prefix(you.inv[armour_id]),
                                 "equip");
        }
        else if (you_can_wear(i) == MB_MAYBE)
            estr << jtrans_notrim("    (restricted)");
        else
            estr << jtrans_notrim("    none");

        if (colour == MSGCOL_BLACK)
            colour = menu_colour(estr.str(), "", "equip");

        mprf(MSGCH_EQUIPMENT, colour, "%s", estr.str().c_str());
    }
}

void list_jewellery()
{
    string jstr;
    int cols = get_number_of_cols() - 1;
    bool split = you.species == SP_OCTOPODE && cols > 84;

    for (int j = EQ_LEFT_RING; j < NUM_EQUIP; j++)
    {
        const equipment_type i = static_cast<equipment_type>(j);
        if (!you_can_wear(i))
            continue;

        const int jewellery_id = you.equip[i];
        int       colour       = MSGCOL_BLACK;

        const char *slot =
                 (i == EQ_LEFT_RING)   ? "Left ring" :
                 (i == EQ_RIGHT_RING)  ? "Right ring" :
                 (i == EQ_AMULET)      ? "Amulet" :
                 (i == EQ_RING_ONE)    ? "1st ring" :
                 (i == EQ_RING_TWO)    ? "2nd ring" :
                 (i == EQ_RING_THREE)  ? "3rd ring" :
                 (i == EQ_RING_FOUR)   ? "4th ring" :
                 (i == EQ_RING_FIVE)   ? "5th ring" :
                 (i == EQ_RING_SIX)    ? "6th ring" :
                 (i == EQ_RING_SEVEN)  ? "7th ring" :
                 (i == EQ_RING_EIGHT)  ? "8th ring" :
                 (i == EQ_RING_AMULET) ? "Amulet ring"
                                       : "unknown";

        string item;
        if (you_can_wear(i, true) == MB_FALSE)
            item = jtrans_notrim("    (currently unavailable)");
        else if (jewellery_id != -1)
        {
            item = you.inv[jewellery_id].name(DESC_INVENTORY);
            string prefix = item_prefix(you.inv[jewellery_id]);
            colour = menu_colour(item, prefix, "equip");
        }
        else
            item = jtrans_notrim("    none");

        if (colour == MSGCOL_BLACK)
            colour = menu_colour(item, "", "equip");

        int max_header_width = strwidth(tagged_jtrans("[eq slot]", "Amulet ring")) + 1;
        item = chop_string(make_stringf("%-*s: %s",
                                        max_header_width,
                                        chop_stringc(tagged_jtransc("[eq slot]", slot), 
                                                     max_header_width), item.c_str()),
                           split && i > EQ_AMULET ? (cols - 1) / 2 : cols);
        item = colour_string(item, colour);

        if (i == EQ_RING_SEVEN && you.species == SP_OCTOPODE &&
                player_mutation_level(MUT_MISSING_HAND))
        {
            mprf(MSGCH_EQUIPMENT, "%s", item.c_str());
        }
        else if (split && i > EQ_AMULET && (i - EQ_AMULET) % 2)
            jstr = item + " ";
        else
            mprf(MSGCH_EQUIPMENT, "%s%s", jstr.c_str(), item.c_str());
    }
}

#define targeting_help_1 (jtransln("targeting help 1"))
/*
    "<h>Examine surroundings ('<w>x</w><h>' in main):\n"
    "<w>Esc</w> : cancel (also <w>Space</w>, <w>x</w>)\n"
    "<w>Dir.</w>: move cursor in that direction\n"
    "<w>.</w> : move to cursor (also <w>Enter</w>, <w>Del</w>)\n"
    "<w>g</w> : pick up item at cursor\n"
    "<w>v</w> : describe monster under cursor\n"
    "<w>+</w> : cycle monsters forward (also <w>=</w>)\n"
    "<w>-</w> : cycle monsters backward\n"
    "<w>*</w> : cycle objects forward (also <w>'</w>)\n"
    "<w>/</w> : cycle objects backward (also <w>;</w>)\n"
    "<w>^</w> : cycle through traps\n"
    "<w>_</w> : cycle through altars\n"
    "<w><<</w>/<w>></w> : cycle through up/down stairs\n"
    "<w>Tab</w> : cycle through shops and portals\n"
    "<w>r</w> : move cursor to you\n"
    "<w>e</w> : create/remove travel exclusion\n"
    "<w>Ctrl-P</w> : repeat prompt\n"
;
*/
#ifdef WIZARD
#define targeting_help_wiz (jtrans_notrim("targeting help wiz"))
/*
    "<h>Wizard targeting commands:</h>\n"
    "<w>Ctrl-C</w> : cycle through beam paths\n"
    "<w>D</w>: get debugging information about the monster\n"
    "<w>o</w>: give item to monster\n"
    "<w>F</w>: cycle monster friendly/good neutral/neutral/hostile\n"
    "<w>G</w>: make monster gain experience\n"
    "<w>Ctrl-H</w>: heal the monster fully\n"
    "<w>P</w>: apply divine blessing to monster\n"
    "<w>m</w>: move monster or player\n"
    "<w>M</w>: cause spell miscast for monster or player\n"
    "<w>s</w>: force monster to shout or speak\n"
    "<w>S</w>: make monster a summoned monster\n"
    "<w>w</w>: calculate shortest path to any point on the map\n"
    "<w>\"</w>: get debugging information about a portal\n"
    "<w>~</w>: polymorph monster to specific type\n"
    "<w>,</w>: bring down the monster to 1 hp\n"
    "<w>(</w>: place a mimic\n"
    "<w>Ctrl-B</w>: banish monster\n"
    "<w>Ctrl-K</w>: kill monster\n"
;
*/
#endif

#define targeting_help_2 (jtrans("targeting help 2"))
/*
    "<h>Targeting (zap wands, cast spells, etc.):\n"
    "Most keys from examine surroundings work.\n"
    "Some keys fire at the target. By default,\n"
    "range is respected and beams don't stop.\n"
    "<w>Enter</w> : fire (<w>Space</w>, <w>Del</w>)\n"
    "<w>.</w> : fire, stop at target\n"
    "<w>@</w> : fire, stop at target, ignore range\n"
    "<w>!</w> : fire, don't stop, ignore range\n"
    "<w>p</w> : fire at Previous target (also <w>f</w>)\n"
    "<w>:</w> : show/hide beam path\n"
    "<w>Shift-Dir.</w> : fire straight-line beam\n"
    "\n"
    "<h>Firing or throwing a missile:\n"
    "<w>(</w> : cycle to next suitable missile.\n"
    "<w>)</w> : cycle to previous suitable missile.\n"
    "<w>i</w> : choose from Inventory.\n"
;
*/
// Add the contents of the file fp to the scroller menu m.
// If first_hotkey is nonzero, that will be the hotkey for the
// start of the contents of the file.
// If auto_hotkeys is true, the function will try to identify
// sections and add appropriate hotkeys.
static void _add_file_to_scroller(FILE* fp, formatted_scroller& m,
                                  int first_hotkey, bool auto_hotkeys)
{
    bool next_is_hotkey = false;
    bool is_first = true;
    char buf[200];

    // Bracket with MEL_TITLES, so that you won't scroll into it or above it.
    m.add_entry(new MenuEntry(string(), MEL_TITLE));
    for (int i = 0; i < get_number_of_lines(); ++i)
        m.add_entry(new MenuEntry(string()));
    m.add_entry(new MenuEntry(string(), MEL_TITLE));

    while (fgets(buf, sizeof buf, fp))
    {
        MenuEntry* me = new MenuEntry(buf);
        if (next_is_hotkey && (isaupper(buf[0]) || isadigit(buf[0]))
            || is_first && first_hotkey)
        {
            int hotkey = (is_first ? first_hotkey : buf[0]);
            if (!is_first && buf[0] == 'X'
                && strlen(buf) >= 3 && isadigit(buf[2]))
            {
                // X.# is hotkeyed to the #
                hotkey = buf[2];
            }
            me->add_hotkey(hotkey);
            if (isaupper(hotkey))
                me->add_hotkey(toalower(hotkey));
            me->level  = MEL_SUBTITLE;
            me->colour = WHITE;
        }
        m.add_entry(me);
        // FIXME: There must be a better way to identify sections!
        next_is_hotkey =
            (auto_hotkeys
                && (strstr(buf, "------------------------------------------"
                                "------------------------------") == buf));
        is_first = false;
    }
}

struct help_file
{
    const char* name;
    int hotkey;
    bool auto_hotkey;
};

static help_file help_files[] =
{
    { "crawl_manual.txt",  '*', true },
    { "aptitudes.txt",     '%', false },
    { "quickstart.txt",    '^', false },
    { "macros_guide.txt",  '~', false },
    { "options_guide.txt", '&', false },
#ifdef USE_TILE_LOCAL
    { "tiles_help.txt",    'T', false },
#endif
    { nullptr, 0, false }
};

// Reads all questions from database/FAQ.txt, outputs them in the form of
// a selectable menu and prints the corresponding answer for a chosen question.
static bool _handle_FAQ()
{
    clrscr();
    viewwindow();

    vector<string> question_keys = getAllFAQKeys();
    if (question_keys.empty())
    {
        mpr(jtrans("No questions found in FAQ! Please submit a bug report!"));
        return false;
    }
    Menu FAQmenu(MF_SINGLESELECT | MF_ANYPRINTABLE | MF_ALLOW_FORMATTING);
    MenuEntry *title = new MenuEntry(jtrans("Frequently Asked Questions"));
    title->colour = YELLOW;
    FAQmenu.set_title(title);
    const int width = get_number_of_cols();

    for (unsigned int i = 0, size = question_keys.size(); i < size; i++)
    {
        const char letter = index_to_letter(i);

        string question = getFAQ_Question(question_keys[i]);
        // Wraparound if the question is longer than fits into a line.
        linebreak_string(question, width - 4);
        vector<formatted_string> fss;
        formatted_string::parse_string_to_multiple(question, fss);

        MenuEntry *me;
        for (unsigned int j = 0; j < fss.size(); j++)
        {
            if (j == 0)
            {
                me = new MenuEntry(question, MEL_ITEM, 1, letter);
                me->data = &question_keys[i];
            }
            else
            {
                question = "    " + fss[j].tostring();
                me = new MenuEntry(question, MEL_ITEM, 1);
            }
            FAQmenu.add_entry(me);
        }
    }

    while (true)
    {
        vector<MenuEntry*> sel = FAQmenu.show();
        redraw_screen();
        if (sel.empty())
            return false;
        else
        {
            ASSERT(sel.size() == 1);
            ASSERT(sel[0]->hotkeys.size() == 1);

            string key = *((string*) sel[0]->data);
            string answer = getFAQ_Answer(key);
            if (answer.empty())
            {
                answer = jtrans("No answer found in the FAQ! Please submit a "
                                "bug report!");
            }
            answer = "Q: " + getFAQ_Question(key) + "\n" + answer;
            linebreak_string(answer, width - 1, true);
            {
#ifdef USE_TILE_WEB
                tiles_crt_control show_as_menu(CRT_MENU, "faq_entry");
#endif
                print_description(answer);
                getchm();
            }
        }
    }

    return true;
}

static int _keyhelp_keyfilter(int ch)
{
    switch (ch)
    {
    case ':':
        // If the game has begun, show notes.
        if (crawl_state.need_save)
        {
            display_notes();
            return -1;
        }
        break;

    case '#':
        // If the game has begun, show dump.
        if (crawl_state.need_save)
        {
            display_char_dump();
            return -1;
        }
        break;

    case '/':
        keyhelp_query_descriptions();
        return -1;

    case 'q':
    case 'Q':
    {
        bool again;
        do
        {
            // resets 'again'
            again = _handle_FAQ();
            if (again)
                clear_messages();
        }
        while (again);

        return -1;
    }

    case 'v':
    case 'V':
        _print_version();
        return -1;
    }
    return ch;
}

///////////////////////////////////////////////////////////////////////////
// Manual menu highlighter.

class help_highlighter : public MenuHighlighter
{
public:
    help_highlighter(string = "");
    int entry_colour(const MenuEntry *entry) const override;
private:
    text_pattern pattern;
    string get_species_key() const;
};

help_highlighter::help_highlighter(string highlight_string) :
    pattern(highlight_string == "" ? get_species_key() : highlight_string)
{
}

int help_highlighter::entry_colour(const MenuEntry *entry) const
{
    return !pattern.empty() && pattern.matches(entry->text) ? WHITE : -1;
}

// To highlight species in aptitudes list. ('?%')
string help_highlighter::get_species_key() const
{
    string result = species_name(you.species);
    // The table doesn't repeat the word "Draconian".
    if (you.species != SP_BASE_DRACONIAN && species_is_draconian(you.species))
        strip_tag(result, "Draconian");

    result += "  ";
    return result;
}
////////////////////////////////////////////////////////////////////////////

static int _show_keyhelp_menu(const vector<formatted_string> &lines,
                              bool with_manual, bool easy_exit = false,
                              int hotkey = 0,
                              string highlight_string = "")
{
    formatted_scroller cmd_help;

    // Set flags, and use easy exit if necessary.
    int flags = MF_NOSELECT | MF_ALWAYS_SHOW_MORE | MF_NOWRAP;
    if (easy_exit)
        flags |= MF_EASY_EXIT;
    cmd_help.set_flags(flags, false);
    cmd_help.set_tag("help");
    cmd_help.set_more();

    if (with_manual)
    {
        cmd_help.set_highlighter(new help_highlighter(highlight_string));
        cmd_help.f_keyfilter = _keyhelp_keyfilter;
        column_composer cols(2, 40);

        cols.add_formatted(
            0,
#ifdef USE_TILE_LOCAL
            jtransln("crawl help index use tile local")
#else
            jtransln("crawl help index")
#endif
/*
            "<h>Dungeon Crawl Help\n"
            "\n"
            "Press one of the following keys to\n"
            "obtain more information on a certain\n"
            "aspect of Dungeon Crawl.\n"

            "<w>?</w>: List of commands\n"
            "<w>^</w>: Quickstart Guide\n"
            "<w>:</w>: Browse character notes\n"
            "<w>#</w>: Browse character dump\n"
            "<w>~</w>: Macros help\n"
            "<w>&</w>: Options help\n"
            "<w>%</w>: Table of aptitudes\n"
            "<w>/</w>: Lookup description\n"
            "<w>Q</w>: FAQ\n"
#ifdef USE_TILE_LOCAL
            "<w>T</w>: Tiles key help\n"
#endif
            "<w>V</w>: Version information\n"
            "<w>Home</w>: This screen\n"*/);

        cols.add_formatted(
            1, jtransln("manual contents")
/*
            "<h>Manual Contents\n\n"
            "<w>*</w>       Table of contents\n"
            "<w>A</w>.      Overview\n"
            "<w>B</w>.      Starting Screen\n"
            "<w>C</w>.      Attributes and Stats\n"
            "<w>D</w>.      Exploring the Dungeon\n"
            "<w>E</w>.      Experience and Skills\n"
            "<w>F</w>.      Monsters\n"
            "<w>G</w>.      Items\n"
            "<w>H</w>.      Spellcasting\n"
            "<w>I</w>.      Targeting\n"
            "<w>J</w>.      Religion\n"
            "<w>K</w>.      Mutations\n"
            "<w>L</w>.      Licence, Contact, History\n"
            "<w>M</w>.      Macros, Options, Performance\n"
            "<w>N</w>.      Philosophy\n"
            "<w>1</w>.      List of Character Species\n"
            "<w>2</w>.      List of Character Backgrounds\n"
            "<w>3</w>.      List of Skills\n"
            "<w>4</w>.      List of Keys and Commands\n"
            "<w>5</w>.      Inscriptions\n"*/);

        vector<formatted_string> blines = cols.formatted_lines();
        unsigned i;
        for (i = 0; i < blines.size(); ++i)
            cmd_help.add_item_formatted_string(blines[i]);

        while (static_cast<int>(++i) < get_number_of_lines())
            cmd_help.add_item_string("");

        // unscrollable
        cmd_help.add_entry(new MenuEntry(string(), MEL_TITLE));
    }

    for (unsigned i = 0; i < lines.size(); ++i)
        cmd_help.add_item_formatted_string(lines[i], (i == 0 ? '?' : 0));

    if (with_manual)
    {
        for (int i = 0; help_files[i].name != nullptr; ++i)
        {
            // Attempt to open this file, skip it if unsuccessful.
            string fname = canonicalise_file_separator(help_files[i].name);
            FILE* fp = fopen_u(datafile_path(fname, false).c_str(), "r");

            if (!fp)
                continue;

            // Put in a separator...
            cmd_help.add_item_string("");
            cmd_help.add_item_string(string(get_number_of_cols()-1,'='));
            cmd_help.add_item_string("");

            // ...and the file itself.
            _add_file_to_scroller(fp, cmd_help, help_files[i].hotkey,
                                  help_files[i].auto_hotkey);

            // Done with this file.
            fclose(fp);
        }
    }

    if (hotkey)
        cmd_help.jump_to_hotkey(hotkey);

    cmd_help.show();

    return cmd_help.getkey();
}

void show_specific_help(const string &key)
{
    const string help = getHelpString(key);
    vector<formatted_string> formatted_lines;
    for (const string &line : split_string("\n", help, false, true))
        formatted_lines.push_back(formatted_string::parse_string(line));
    _show_keyhelp_menu(formatted_lines, false, Options.easy_exit_menu);
}

void show_levelmap_help()
{
    show_specific_help("level-map");
}

void show_targeting_help()
{
    column_composer cols(2, 40);
    // Page size is number of lines - one line for --more-- prompt.
    cols.set_pagesize(get_number_of_lines() - 1);

#ifdef WIZARD
    if (you.wizard)
        cols.add_formatted(0, targeting_help_1 +
#ifndef USE_TILE_LOCAL
                              ZWSP "\n" +
#else
                              "\n" +
#endif
                              targeting_help_wiz, true);
    else
        cols.add_formatted(0, targeting_help_1, true);
#else
    cols.add_formatted(0, targeting_help_1, true);
#endif

    cols.add_formatted(1, targeting_help_2, true);
    _show_keyhelp_menu(cols.formatted_lines(), false, Options.easy_exit_menu);
}
void show_interlevel_travel_branch_help()
{
    show_specific_help("interlevel-travel.branch.prompt");
}

void show_interlevel_travel_depth_help()
{
    show_specific_help("interlevel-travel.depth.prompt");
}

void show_stash_search_help()
{
    show_specific_help("stash-search.prompt");
}

void show_butchering_help()
{
    show_specific_help("butchering");
}

void show_skill_menu_help()
{
    show_specific_help("skill-menu");
}

static void _add_command(column_composer &cols, const int column,
                         const command_type cmd,
                         const string &desc,
                         const unsigned int space_to_colon = 7)
{
    string command_name = command_to_string(cmd);
    if (strcmp(command_name.c_str(), "<") == 0)
        command_name += "<";

    const int cmd_len = strwidth(command_name);
    string line = "<w>" + command_name + "</w>";
    for (unsigned int i = cmd_len; i < space_to_colon; ++i)
        line += " ";
    line += ": " + untag_tiles_console(desc) + "\n";

    cols.add_formatted(
            column,
            line.c_str(),
            false);
}

static void _add_insert_commands(column_composer &cols, const int column,
                                 const unsigned int space_to_colon,
                                 const string &desc, const int first, ...)
{
    const command_type cmd = (command_type) first;

    va_list args;
    va_start(args, first);
    int nargs = 10;

    vector<command_type> cmd_vector;
    while (nargs-- > 0)
    {
        int value = va_arg(args, int);
        if (!value)
            break;

        cmd_vector.push_back((command_type) value);
    }
    va_end(args);

    string line = desc;
    insert_commands(line, cmd_vector);
    line += "\n";
    _add_command(cols, column, cmd, line, space_to_colon);
}

static void _add_insert_commands(column_composer &cols, const int column,
                                 const string desc, const int first, ...)
{
    vector<command_type> cmd_vector;
    cmd_vector.push_back((command_type) first);

    va_list args;
    va_start(args, first);
    int nargs = 10;

    while (nargs-- > 0)
    {
        int value = va_arg(args, int);
        if (!value)
            break;

        cmd_vector.push_back((command_type) value);
    }
    va_end(args);

    string line = desc;
    insert_commands(line, cmd_vector);
    line += "\n";
    cols.add_formatted(
            column,
            line.c_str(),
            false);
}

static void _add_formatted_keyhelp(column_composer &cols)
{
    cols.add_formatted(
            0, jtransln("command movement help")
/*
            "<h>Movement:\n"
            "To move in a direction or to attack, \n"
            "use the numpad (try Numlock off and \n"
            "on) or vi keys:\n"*/);

    _add_insert_commands(cols, 0, "                 <w>7 8 9      % % %",
                         CMD_MOVE_UP_LEFT, CMD_MOVE_UP, CMD_MOVE_UP_RIGHT, 0);
    _add_insert_commands(cols, 0, "                  \\|/        \\|/", 0);
    _add_insert_commands(cols, 0, "                 <w>4</w>-<w>5</w>-<w>6</w>      <w>%</w>-<w>%</w>-<w>%</w>",
                         CMD_MOVE_LEFT, CMD_WAIT, CMD_MOVE_RIGHT, 0);
    _add_insert_commands(cols, 0, "                  /|\\        /|\\", 0);
    _add_insert_commands(cols, 0, "                 <w>1 2 3      % % %",
                         CMD_MOVE_DOWN_LEFT, CMD_MOVE_DOWN, CMD_MOVE_DOWN_RIGHT, 0);

    cols.add_formatted(
            0,
            jtrans_notrim("<h>Rest:\n"));

    _add_command(cols, 0, CMD_WAIT, jtrans("wait a turn (also <w>s</w>, <w>Del</w>)"), 2);
    _add_command(cols, 0, CMD_REST, jtrans("rest and long wait; stops when"), 2);
    cols.add_formatted(
            0, jtransln("command help rest"),
/*
            "    Health or Magic become full or\n"
            "    something is detected. If Health\n"
            "    and Magic are already full, stops\n"
            "    when 100 turns over (<w>numpad-5</w>)\n"),
*/
            false);

    cols.add_formatted(
            0,
            jtrans_notrim("<h>Extended Movement:\n"));

    _add_command(cols, 0, CMD_EXPLORE, jtrans("auto-explore"));
    _add_command(cols, 0, CMD_INTERLEVEL_TRAVEL, jtrans("interlevel travel"));
    _add_command(cols, 0, CMD_SEARCH_STASHES, jtrans("Find items"));
    _add_command(cols, 0, CMD_FIX_WAYPOINT, jtrans("set Waypoint"));

    cols.add_formatted(
            0,
            jtrans_notrim("<w>/ Dir.</w>, <w>Shift-Dir.</w>: long walk\n") +
            jtrans_notrim("<w>* Dir.</w>, <w>Ctrl-Dir.</w> : attack without move \n"),
            false);

    cols.add_formatted(
            0, jtransln("command help autofight")
/*
            "<h>Autofight:\n"
            "<w>Tab</w>       : attack nearest monster,\n"
            "            moving if necessary\n"
            "<w>Shift-Tab</w> : attack nearest monster\n"
            "            without moving\n"*/);

    cols.add_formatted(
            0,
            jtrans_notrim("<h>Item types (and common commands)\n"));

    _add_insert_commands(cols, 0, jtrans("<cyan>)</cyan> : hand weapons (<w>%</w>ield)"),
                         CMD_WIELD_WEAPON, 0);
    _add_insert_commands(cols, 0, jtrans("<brown>(</brown> : missiles (<w>%</w>uiver, "
                                         "<w>%</w>ire, <w>%</w>/<w>%</w> cycle)"),
                         CMD_QUIVER_ITEM, CMD_FIRE, CMD_CYCLE_QUIVER_FORWARD,
                         CMD_CYCLE_QUIVER_BACKWARD, 0);
    _add_insert_commands(cols, 0, jtrans("<cyan>[</cyan> : armour (<w>%</w>ear and <w>%</w>ake off)"),
                         CMD_WEAR_ARMOUR, CMD_REMOVE_ARMOUR, 0);
    _add_insert_commands(cols, 0, jtrans("<brown>percent</brown> : corpses and food "
                                         "(<w>%</w>hop up and <w>%</w>at)"),
                         CMD_BUTCHER, CMD_EAT, 0);
    _add_insert_commands(cols, 0, jtrans("<w>?</w> : scrolls (<w>%</w>ead)"),
                         CMD_READ, 0);
    _add_insert_commands(cols, 0, jtrans("<magenta>!</magenta> : potions (<w>%</w>uaff)"),
                         CMD_QUAFF, 0);
    _add_insert_commands(cols, 0, jtrans("<blue>=</blue> : rings (<w>%</w>ut on and <w>%</w>emove)"),
                         CMD_WEAR_JEWELLERY, CMD_REMOVE_JEWELLERY, 0);
    _add_insert_commands(cols, 0, jtrans("<red>\"</red> : amulets (<w>%</w>ut on and <w>%</w>emove)"),
                         CMD_WEAR_JEWELLERY, CMD_REMOVE_JEWELLERY, 0);
    _add_insert_commands(cols, 0, jtrans("<lightgrey>/</lightgrey> : wands (e<w>%</w>oke)"),
                         CMD_EVOKE, 0);

    string item_types = "<lightcyan>";
    item_types += stringize_glyph(get_item_symbol(SHOW_ITEM_BOOK));
    item_types +=
        jtrans("</lightcyan> : books (<w>%</w>ead, <w>%</w>emorise, <w>%</w>ap, <w>%</w>ap)");
    _add_insert_commands(cols, 0, item_types,
                         CMD_READ, CMD_MEMORISE_SPELL, CMD_CAST_SPELL,
                         CMD_FORCE_CAST_SPELL, 0);
    _add_insert_commands(cols, 0, jtrans("<brown>\\</brown> : staves and rods (<w>%</w>ield and e<w>%</w>oke)"),
                         CMD_WIELD_WEAPON, CMD_EVOKE_WIELDED, 0);
    _add_insert_commands(cols, 0, jtrans("<lightgreen>}</lightgreen> : miscellaneous items (e<w>%</w>oke)"),
                         CMD_EVOKE, 0);
    _add_insert_commands(cols, 0, jtrans("<yellow>$</yellow> : gold (<w>%</w> counts gold)"),
                         CMD_LIST_GOLD, 0);

    cols.add_formatted(
            0,
            jtrans_notrim("<lightmagenta>0</lightmagenta> : the Orb of Zot\n") +
            jtrans_notrim("    Carry it to the surface and win!\n"),
            false);

    cols.add_formatted(
            0,
            jtrans_notrim("<h>Other Gameplay Actions:\n"));

    _add_insert_commands(cols, 0, 2, jtrans("use special Ability (<w>%!</w> for help)"),
                         CMD_USE_ABILITY, CMD_USE_ABILITY, 0);
    _add_command(cols, 0, CMD_CAST_SPELL, jtrans("cast spell, abort without targets"), 2);
    _add_command(cols, 0, CMD_FORCE_CAST_SPELL, jtrans("cast spell, no matter what"), 2);
    _add_command(cols, 0, CMD_DISPLAY_SPELLS, jtrans("list all spells"), 2);

    _add_insert_commands(cols, 0, 2, jtrans("tell allies (<w>%t</w> to shout)"),
                         CMD_SHOUT, CMD_SHOUT, 0);
    _add_command(cols, 0, CMD_PREV_CMD_AGAIN, jtrans("re-do previous command"), 2);
    _add_command(cols, 0, CMD_REPEAT_CMD, jtrans("repeat next command # of times"), 2);

    cols.add_formatted(
            0,
            jtrans_notrim("<h>Non-Gameplay Commands / Info\n"));

    _add_command(cols, 0, CMD_REPLAY_MESSAGES, jtrans("show Previous messages"));
    _add_command(cols, 0, CMD_REDRAW_SCREEN, jtrans("Redraw screen"));
    _add_command(cols, 0, CMD_CLEAR_MAP, jtrans("Clear main and level maps"));
    _add_command(cols, 0, CMD_ANNOTATE_LEVEL, jtrans("annotate the dungeon level"), 2);
    _add_command(cols, 0, CMD_CHARACTER_DUMP, jtrans("dump character to file"), 2);
    _add_insert_commands(cols, 0, 2, jtrans("add note (use <w>%:</w> to read notes)"),
                         CMD_MAKE_NOTE, CMD_DISPLAY_COMMANDS);
    _add_command(cols, 0, CMD_MACRO_ADD, jtrans("add macro (also <w>Ctrl-D</w>)"));
    _add_command(cols, 0, CMD_ADJUST_INVENTORY, jtrans("reassign inventory/spell letters"), 2);
#ifdef USE_TILE_LOCAL
    _add_command(cols, 0, CMD_EDIT_PLAYER_TILE, jtrans("edit player doll"));
#else
#ifdef USE_TILE_WEB
    if (tiles.is_controlled_from_web())
    {
        cols.add_formatted(0, jtrans("<w>F12</w> : read messages (online play only)"),
                           false);
    }
    else
#endif
    _add_command(cols, 0, CMD_READ_MESSAGES, jtrans("read messages (online play only)"));
#endif

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Game Saving and Quitting:\n"));

    _add_command(cols, 1, CMD_SAVE_GAME, jtrans("Save game and exit"));
    _add_command(cols, 1, CMD_SAVE_GAME_NOW, jtrans("Save and exit without query"));
    _add_command(cols, 1, CMD_QUIT, jtrans("Abandon the current character"));
    cols.add_formatted(1, jtrans_notrim("         and quit the game\n"),
                       false);

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Player Character Information:\n"));

    _add_command(cols, 1, CMD_DISPLAY_CHARACTER_STATUS, jtrans("display character status"), 2);
    _add_command(cols, 1, CMD_DISPLAY_SKILLS, jtrans("show skill screen"), 2);
    _add_command(cols, 1, CMD_RESISTS_SCREEN, jtrans("character overview"), 2);
    _add_command(cols, 1, CMD_DISPLAY_RELIGION, jtrans("show religion screen"), 2);
    _add_command(cols, 1, CMD_DISPLAY_MUTATIONS, jtrans("show Abilities/mutations"), 2);
    _add_command(cols, 1, CMD_DISPLAY_KNOWN_OBJECTS, jtrans("show item knowledge"), 2);
    _add_command(cols, 1, CMD_DISPLAY_RUNES, jtrans("show runes collected"), 2);
    _add_command(cols, 1, CMD_LIST_ARMOUR, jtrans("display worn armour"), 2);
    _add_command(cols, 1, CMD_LIST_JEWELLERY, jtrans("display worn jewellery"), 2);
    _add_command(cols, 1, CMD_LIST_GOLD, jtrans("display gold in possession"), 2);
    _add_command(cols, 1, CMD_EXPERIENCE_CHECK, jtrans("display experience info"), 2);

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Dungeon Interaction and Information:\n"));

    _add_insert_commands(cols, 1, jtrans("<w>%</w>/<w>%</w> : Open/Close door"),
                         CMD_OPEN_DOOR, CMD_CLOSE_DOOR, 0);
    _add_insert_commands(cols, 1, jtrans("<w>%</w>/<w>%</w> : use staircase"),
                         CMD_GO_UPSTAIRS, CMD_GO_DOWNSTAIRS, 0);

    _add_command(cols, 1, CMD_INSPECT_FLOOR, jtrans("examine occupied tile and"));
    cols.add_formatted(1, jtrans_notrim("         pickup part of a single stack\n"),
                       false);

    _add_command(cols, 1, CMD_LOOK_AROUND, jtrans("eXamine surroundings/targets"));
    _add_insert_commands(cols, 1, 7, jtrans("eXamine level map (<w>%?</w> for help)"),
                         CMD_DISPLAY_MAP, CMD_DISPLAY_MAP, 0);
    _add_command(cols, 1, CMD_FULL_VIEW, jtrans("list monsters, items, features"));
    cols.add_formatted(1, jtrans_notrim("         in view\n"),
                       false);
    _add_command(cols, 1, CMD_SHOW_TERRAIN, jtrans("toggle view layers"));
    _add_command(cols, 1, CMD_DISPLAY_OVERMAP, jtrans("show dungeon Overview"));
    _add_command(cols, 1, CMD_TOGGLE_AUTOPICKUP, jtrans("toggle auto-pickup"));
    _add_command(cols, 1, CMD_TOGGLE_TRAVEL_SPEED, jtrans("set your travel speed to your"));
    cols.add_formatted(1, jtrans_notrim("         slowest ally\n"),
                           false);

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Item Interaction (inventory):\n"));

    _add_command(cols, 1, CMD_DISPLAY_INVENTORY, jtrans("show Inventory list"), 2);
    _add_command(cols, 1, CMD_INSCRIBE_ITEM, jtrans("inscribe item"), 2);
    _add_command(cols, 1, CMD_FIRE, jtrans("Fire next appropriate item"), 2);
    _add_command(cols, 1, CMD_THROW_ITEM_NO_QUIVER, jtrans("select an item and Fire it"), 2);
    _add_command(cols, 1, CMD_QUIVER_ITEM, jtrans("select item slot to be quivered"), 2);

    {
        string interact = (you.species == SP_VAMPIRE ? "Drain corpses"
                                                     : "Eat food");
        interact += " (tries floor first)\n";
        _add_command(cols, 1, CMD_EAT, jtrans_notrim(interact), 2);
    }

    _add_command(cols, 1, CMD_QUAFF, jtrans("Quaff a potion"), 2);
    _add_command(cols, 1, CMD_READ, jtrans("Read a scroll or book"), 2);
    _add_command(cols, 1, CMD_MEMORISE_SPELL, jtrans("Memorise a spell from a book"), 2);
    _add_command(cols, 1, CMD_WIELD_WEAPON, jtrans("Wield an item (<w>-</w> for none)"), 2);
    _add_command(cols, 1, CMD_WEAPON_SWAP, jtrans("wield item a, or switch to b"), 2);

    _add_insert_commands(cols, 1, jtrans_notrim("    (use <w>%</w> to assign slots)"),
                         CMD_ADJUST_INVENTORY, 0);

    _add_command(cols, 1, CMD_EVOKE_WIELDED, jtrans("eVoke power of wielded item"), 2);
    _add_command(cols, 1, CMD_EVOKE, jtrans("eVoke wand and miscellaneous item"), 2);

    _add_insert_commands(cols, 1, jtrans("<w>%</w>/<w>%</w> : Wear or Take off armour"),
                         CMD_WEAR_ARMOUR, CMD_REMOVE_ARMOUR, 0);
    _add_insert_commands(cols, 1, jtrans("<w>%</w>/<w>%</w> : Put on or Remove jewellery"),
                         CMD_WEAR_JEWELLERY, CMD_REMOVE_JEWELLERY, 0);

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Item Interaction (floor):\n"));

    _add_command(cols, 1, CMD_PICKUP, jtrans("pick up items (also <w>g</w>)"), 2);
    cols.add_formatted(
            1,
            jtrans_notrim("    (press twice for pick up menu)\n"),
            false);

    _add_command(cols, 1, CMD_DROP, jtrans("Drop an item"), 2);
    _add_insert_commands(cols, 1, jtrans("<w>%#</w>: Drop exact number of items"),
                         CMD_DROP, 0);
    _add_command(cols, 1, CMD_DROP_LAST, jtrans("Drop the last item(s) you picked up"), 2);
    {
        const bool vampire = you.species == SP_VAMPIRE;
        string butcher = vampire ? "Bottle blood from"
                                 : "Chop up";
        _add_command(cols, 1, CMD_BUTCHER, jtrans(butcher + " a corpse"), 2);

        string eat = vampire ? "Drain corpses on"
                             : "Eat food from";
        eat += " floor\n";
        _add_command(cols, 1, CMD_EAT, jtrans_notrim(eat), 2);
    }

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Additional help:\n"));

    string text = jtrans("command additional help");
/*
            "Many commands have context sensitive "
            "help, among them <w>%</w>, <w>%</w>, <w>%</w> (or any "
            "form of targeting), <w>%</w>, and <w>%</w>.\n"
            "You can read descriptions of your "
            "current spells (<w>%</w>), skills (<w>%?</w>) and "
            "abilities (<w>%!</w>).";
*/
    insert_commands(text, { CMD_DISPLAY_MAP, CMD_LOOK_AROUND, CMD_FIRE,
                            CMD_SEARCH_STASHES, CMD_INTERLEVEL_TRAVEL,
                            CMD_DISPLAY_SPELLS, CMD_DISPLAY_SKILLS,
                            CMD_USE_ABILITY
                          });
    linebreak_string(text, 40);

    cols.add_formatted(
            1, text,
            false);
}

static void _add_formatted_hints_help(column_composer &cols)
{
    // First column.
    cols.add_formatted(
            0,
            jtransln("command movement help"),
/*
            "<h>Movement:\n"
            "To move in a direction or to attack, \n"
            "use the numpad (try Numlock off and \n"
            "on) or vi keys:\n",
*/
            false);

    _add_insert_commands(cols, 0, "                 <w>7 8 9      % % %",
                         CMD_MOVE_UP_LEFT, CMD_MOVE_UP, CMD_MOVE_UP_RIGHT, 0);
    _add_insert_commands(cols, 0, "                  \\|/        \\|/", 0);
    _add_insert_commands(cols, 0, "                 <w>4</w>-<w>5</w>-<w>6</w>      <w>%</w>-<w>%</w>-<w>%</w>",
                         CMD_MOVE_LEFT, CMD_WAIT, CMD_MOVE_RIGHT, 0);
    _add_insert_commands(cols, 0, "                  /|\\        /|\\", 0);
    _add_insert_commands(cols, 0, "                 <w>1 2 3      % % %",
                         CMD_MOVE_DOWN_LEFT, CMD_MOVE_DOWN, CMD_MOVE_DOWN_RIGHT, 0);

    cols.add_formatted(0, " ", false);
    cols.add_formatted(0, jtrans("<w>Shift-Dir.</w> runs into one direction"),
                       false);
    _add_insert_commands(cols, 0, jtrans("<w>%</w> or <w>%</w> : ascend/descend the stairs"),
                         CMD_GO_UPSTAIRS, CMD_GO_DOWNSTAIRS, 0);
    _add_command(cols, 0, CMD_EXPLORE, jtrans("autoexplore"), 2);

    cols.add_formatted(
            0,
            jtrans_notrim("<h>Rest:\n"));

    _add_command(cols, 0, CMD_WAIT, jtrans("wait a turn (also <w>s</w>, <w>Del</w>)"), 2);
    _add_command(cols, 0, CMD_REST, jtrans("rest and long wait; stops when"), 2);
    cols.add_formatted(
            0,
            jtransln("command help rest"),
/*
            "    Health or Magic become full or\n"
            "    something is detected. If Health\n"
            "    and Magic are already full, stops\n"
            "    when 100 turns over (<w>numpad-5</w>)\n",
*/
            false);

    cols.add_formatted(
            0, jtrans_notrim("\ncommand help attacking monster"),
/*
            "\n<h>Attacking monsters\n"
            "Walking into a monster will attack it\n"
            "with the wielded weapon or barehanded.",
*/
            false);

    cols.add_formatted(
            0,
            jtrans_notrim("\n<h>Ranged combat and magic\n"),
            false);

    _add_insert_commands(cols, 0, jtrans("<w>%</w> to throw/fire missiles"),
                         CMD_FIRE, 0);
    _add_insert_commands(cols, 0, jtrans("<w>%</w>/<w>%</w> to cast spells "
                                         "(<w>%?/%</w> lists spells)"),
                         CMD_CAST_SPELL, CMD_FORCE_CAST_SPELL, CMD_CAST_SPELL,
                         CMD_DISPLAY_SPELLS, 0);
    _add_command(cols, 0, CMD_MEMORISE_SPELL, jtrans("Memorise a new spell"), 2);
    _add_command(cols, 0, CMD_READ, jtrans("read a book to see spell descriptions"), 2);

    // Second column.
    cols.add_formatted(
            1, jtrans_notrim("<h>Item types (and common commands)\n"),
            false);

    _add_insert_commands(cols, 1, jtrans(
                         "<console><cyan>)</cyan> : </console>"
                         "hand weapons (<w>%</w>ield)"),
                         CMD_WIELD_WEAPON, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><brown>(</brown> : </console>"
                         "missiles (<w>%</w>uiver, <w>%</w>ire, <w>%</w>/<w>%</w> cycle)"),
                         CMD_QUIVER_ITEM, CMD_FIRE, CMD_CYCLE_QUIVER_FORWARD,
                         CMD_CYCLE_QUIVER_BACKWARD, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><cyan>[</cyan> : </console>"
                         "armour (<w>%</w>ear and <w>%</w>ake off)"),
                         CMD_WEAR_ARMOUR, CMD_REMOVE_ARMOUR, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><brown>percent</brown> : </console>"
                         "corpses and food (<w>%</w>hop up and <w>%</w>at)"),
                         CMD_BUTCHER, CMD_EAT, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><w>?</w> : </console>"
                         "scrolls (<w>%</w>ead)"),
                         CMD_READ, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><magenta>!</magenta> : </console>"
                         "potions (<w>%</w>uaff)"),
                         CMD_QUAFF, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><blue>=</blue> : </console>"
                         "rings (<w>%</w>ut on and <w>%</w>emove)"),
                         CMD_WEAR_JEWELLERY, CMD_REMOVE_JEWELLERY, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><red>\"</red> : </console>"
                         "amulets (<w>%</w>ut on and <w>%</w>emove)"),
                         CMD_WEAR_JEWELLERY, CMD_REMOVE_JEWELLERY, 0);
    _add_insert_commands(cols, 1, jtrans(
                         "<console><lightgrey>/</lightgrey> : </console>"
                         "wands (e<w>%</w>oke)"),
                         CMD_EVOKE, 0);

    string item_types =
                  "<console><lightcyan>";
    item_types += stringize_glyph(get_item_symbol(SHOW_ITEM_BOOK));
    item_types += jtrans(
        "</lightcyan> : </console>"
        "books (<w>%</w>ead, <w>%</w>emorise, <w>%</w>ap, <w>%</w>ap)");
    _add_insert_commands(cols, 1, item_types,
                         CMD_READ, CMD_MEMORISE_SPELL, CMD_CAST_SPELL,
                         CMD_FORCE_CAST_SPELL, 0);

    item_types =
                  "<console><brown>";
    item_types += stringize_glyph(get_item_symbol(SHOW_ITEM_STAFF));
    item_types += jtrans(
        "</brown> : </console>"
        "staves and rods (<w>%</w>ield and e<w>%</w>oke)");
    _add_insert_commands(cols, 1, item_types,
                         CMD_WIELD_WEAPON, CMD_EVOKE_WIELDED, 0);

    cols.add_formatted(1, " ", false);
    _add_command(cols, 1, CMD_DISPLAY_INVENTORY, jtrans("list inventory (select item to view it)"), 2);
    _add_command(cols, 1, CMD_PICKUP, jtrans("pick up item from ground (also <w>g</w>)"), 2);
    _add_command(cols, 1, CMD_DROP, jtrans("drop item"), 2);
    _add_command(cols, 1, CMD_DROP_LAST, jtrans("drop the last item(s) you picked up"), 2);

    cols.add_formatted(
            1,
            jtrans_notrim("<h>Additional important commands\n"));

    _add_command(cols, 1, CMD_SAVE_GAME_NOW, jtrans("Save the game and exit"), 2);
    _add_command(cols, 1, CMD_REPLAY_MESSAGES, jtrans("show previous messages"), 2);
    _add_command(cols, 1, CMD_USE_ABILITY, jtrans("use an ability"), 2);
    _add_command(cols, 1, CMD_RESISTS_SCREEN, jtrans("show character overview"), 2);
    _add_command(cols, 1, CMD_DISPLAY_RELIGION, jtrans("show religion overview"), 2);
    _add_command(cols, 1, CMD_DISPLAY_MAP, jtrans("show map of the whole level"), 2);
    _add_command(cols, 1, CMD_DISPLAY_OVERMAP, jtrans("show dungeon overview"), 2);

    cols.add_formatted(
            1, jtrans_notrim("\ncommand help targeting\n"),
/*
            "\n<h>Targeting\n"
            "<w>Enter</w> or <w>.</w> or <w>Del</w> : confirm target\n"
            "<w>+</w> and <w>-</w> : cycle between targets\n"
            "<w>f</w> or <w>p</w> : shoot at previous target\n"
            "         if still alive and in sight\n",
*/
            false);
}

void list_commands(int hotkey, bool do_redraw_screen, string highlight_string)
{
    // 2 columns, split at column 40.
    column_composer cols(2, 41);

    // Page size is number of lines - one line for --more-- prompt.
    cols.set_pagesize(get_number_of_lines() - 1);

    if (crawl_state.game_is_hints_tutorial())
        _add_formatted_hints_help(cols);
    else
        _add_formatted_keyhelp(cols);

    _show_keyhelp_menu(cols.formatted_lines(), true, Options.easy_exit_menu,
                       hotkey, highlight_string);

    if (do_redraw_screen)
    {
        clrscr();
        redraw_screen();
    }
}

#ifdef WIZARD
int list_wizard_commands(bool do_redraw_screen)
{
    // 2 columns
    column_composer cols(2, 44);
    // Page size is number of lines - one line for --more-- prompt.
    cols.set_pagesize(get_number_of_lines());

    cols.add_formatted(0, jtransln("wizard command help 1"),
/*
                       "<yellow>Player stats</yellow>\n"
                       "<w>A</w>      set all skills to level\n"
                       "<w>Ctrl-D</w> change enchantments/durations\n"
                       "<w>g</w>      exercise a skill\n"
                       "<w>l</w>      change experience level\n"
                       "<w>Ctrl-P</w> list props\n"
                       "<w>r</w>      change character's species\n"
                       "<w>s</w>      set skill to level\n"
                       "<w>x</w>      gain an experience level\n"
                       "<w>$</w>      set gold to a specified value\n"
                       "<w>]</w>      get a mutation\n"
                       "<w>_</w>      gain religion\n"
                       "<w>^</w>      set piety to a value\n"
                       "<w>@</w>      set Str Int Dex\n"
                       "<w>#</w>      load character from a dump file\n"
                       "<w>&</w>      list all divine followers\n"
                       "<w>=</w>      show info about skill points\n"
                       "\n"
                       "<yellow>Create level features</yellow>\n"
                       "<w>L</w>      place a vault by name\n"
                       "<w>T</w>      make a trap\n"
                       "<w>,</w>/<w>.</w>    create up/down staircase\n"
                       "<w>(</w>      turn cell into feature\n"
                       "<w>\\</w>      make a shop\n"
                       "<w>Ctrl-K</w> mark all vaults as unused\n"
                       "\n"
                       "<yellow>Other level related commands</yellow>\n"
                       "<w>Ctrl-A</w> generate new Abyss area\n"
                       "<w>b</w>      controlled blink\n"
                       "<w>B</w>      controlled teleport\n"
                       "<w>Ctrl-B</w> banish yourself to the Abyss\n"
                       "<w>k</w>      shift section of a labyrinth\n"
                       "<w>R</w>      change monster spawn rate\n"
                       "<w>Ctrl-S</w> change Abyss speed\n"
                       "<w>u</w>/<w>d</w>    shift up/down one level\n"
                       "<w>~</w>      go to a specific level\n"
                       "<w>:</w>      find branches and overflow\n"
                       "       temples in the dungeon\n"
                       "<w>;</w>      list known levels and counters\n"
                       "<w>{</w>      magic mapping\n"
                       "<w>}</w>      detect all traps on level\n"
                       "<w>Ctrl-W</w> change Shoals' tide speed\n"
                       "<w>Ctrl-E</w> dump level builder information\n"
                       "<w>Ctrl-R</w> regenerate current level\n"
                       "<w>P</w>      create a level based on a vault\n",
*/
                       true);

    cols.add_formatted(1, jtransln("wizard command help 2"),
/*
                       "<yellow>Other player related effects</yellow>\n"
                       "<w>c</w>      card effect\n"
#ifdef DEBUG_BONES
                       "<w>Ctrl-G</w> save/load ghost (bones file)\n"
#endif
                       "<w>h</w>/<w>H</w>    heal yourself (super-Heal)\n"
                       "<w>e</w>      set hunger state\n"
                       "<w>X</w>      make Xom do something now\n"
                       "<w>z</w>      cast spell by number/name\n"
                       "<w>!</w>      memorise spell\n"
                       "<w>W</w>      god wrath\n"
                       "<w>w</w>      god mollification\n"
                       "<w>p</w>      polymorph into a form\n"
                       "<w>V</w>      toggle xray vision\n"
                       "<w>E</w>      (un)freeze time\n"
                       "\n"
                       "<yellow>Monster related commands</yellow>\n"
                       "<w>m</w>/<w>M</w>    create specified monster\n"
                       "<w>D</w>      detect all monsters\n"
                       "<w>g</w>/<w>G</w>    dismiss all monsters\n"
                       "<w>\"</w>      list monsters\n"
                       "\n"
                       "<yellow>Item related commands</yellow>\n"
                       "<w>a</w>      acquirement\n"
                       "<w>C</w>      (un)curse item\n"
                       "<w>i</w>/<w>I</w>    identify/unidentify inventory\n"
                       "<w>y</w>/<w>Y</w>    id/unid item types+level items\n"
                       "<w>o</w>/<w>%</w>    create an object\n"
                       "<w>t</w>      tweak object properties\n"
                       "<w>v</w>      show gold value of an item\n"
                       "<w>-</w>      get a god gift\n"
                       "<w>|</w>      create all unrand artefacts\n"
                       "<w>+</w>      make randart from item\n"
                       "<w>'</w>      list items\n"
                       "<w>J</w>      Jiyva off-level sacrifice\n"
                       "\n"
                       "<yellow>Debugging commands</yellow>\n"
                       "<w>f</w>      quick fight simulation\n"
                       "<w>F</w>      single scale fsim\n"
                       "<w>Ctrl-F</w> double scale fsim\n"
                       "<w>Ctrl-I</w> item generation stats\n"
                       "<w>O</w>      measure exploration time\n"
                       "<w>Ctrl-T</w> dungeon (D)Lua interpreter\n"
                       "<w>Ctrl-U</w> client (C)Lua interpreter\n"
                       "<w>Ctrl-X</w> Xom effect stats\n"
#ifdef DEBUG_DIAGNOSTICS
                       "<w>Ctrl-Q</w> make some debug messages quiet\n"
#endif
                       "<w>Ctrl-C</w> force a crash\n"
                       "\n"
                       "<yellow>Other wizard commands</yellow>\n"
                       "(not prefixed with <w>&</w>!)\n"
                       "<w>x?</w>     list targeted commands\n"
                       "<w>X?</w>     list map-mode commands\n",
*/
                       true);

    int key = _show_keyhelp_menu(cols.formatted_lines(), false,
                                 Options.easy_exit_menu);
    if (do_redraw_screen)
        redraw_screen();
    return key;
}
#endif
