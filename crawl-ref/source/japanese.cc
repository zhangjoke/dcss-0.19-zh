/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#include "AppHdr.h"
#include "database.h"
#include "japanese.h"
#include "stringutil.h"

const string conjugate_verb_j(const string &verb, bool /* ignore */)
{
    return verb_j(verb);
}

static const char * const _pronoun_declension_j[][NUM_PRONOUN_CASES] =
{
    // subj  poss    refl        obj
    { "it",  "its",  "itself",   "it"  }, // neuter
    { "he",  "his",  "himself",  "him" }, // masculine
    { "she", "her",  "herself",  "her" }, // feminine
    { "you", "your", "yourself", "you" }, // 2nd person
};

const string decline_pronoun_j(gender_type gender, pronoun_type variant)
{
    COMPILE_CHECK(ARRAYSZ(_pronoun_declension_j) == NUM_GENDERS);
    ASSERT_RANGE(gender, 0, NUM_GENDERS);
    ASSERT_RANGE(variant, 0, NUM_PRONOUN_CASES);

    switch (gender)
    {
    case GENDER_NEUTER:
        return tagged_jtrans("[gender neuter]", _pronoun_declension_j[gender][variant]);
    case GENDER_MALE:
        return tagged_jtrans("[gender male]", _pronoun_declension_j[gender][variant]);
    case GENDER_FEMALE:
        return tagged_jtrans("[gender female]", _pronoun_declension_j[gender][variant]);
    case GENDER_YOU:
        return tagged_jtrans("[gender you]", _pronoun_declension_j[gender][variant]);
    default:
        return make_stringf("buggy pronoun: %d, %d",
                            gender, variant);
    }
}

const char * general_counter_suffix(const int size)
{
    if (size <= 9)
        return "つ";
    else
        return "個";
}
