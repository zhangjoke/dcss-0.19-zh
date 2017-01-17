/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#include "AppHdr.h"
#include "database.h"
#include "english.h"
#include "japanese.h"
#include "stringutil.h"

const string conjugate_verb_j(const string &verb, bool /* ignore */)
{
    return verb_j(verb);
}

const string decline_pronoun_j(gender_type gender, pronoun_type variant)
{
    ASSERT_RANGE(gender, 0, NUM_GENDERS);
    ASSERT_RANGE(variant, 0, NUM_PRONOUN_CASES);

    const char *variant_tag = "buggy";
    switch (variant)
    {
    case PRONOUN_SUBJECTIVE: variant_tag = "subj"; break;
    case PRONOUN_POSSESSIVE: variant_tag = "poss"; break;
    case PRONOUN_REFLEXIVE:  variant_tag = "refl"; break;
    case PRONOUN_OBJECTIVE:  variant_tag = "obj"; break;
    default: break;
    }

    switch (gender)
    {
    case GENDER_NEUTER:
        return tagged_jtrans(make_stringf("[gender neuter %s]", variant_tag),
                             decline_pronoun(gender, variant));
    case GENDER_MALE:
        return tagged_jtrans(make_stringf("[gender male %s]", variant_tag),
                             decline_pronoun(gender, variant));
    case GENDER_FEMALE:
        return tagged_jtrans(make_stringf("[gender female %s]", variant_tag),
                             decline_pronoun(gender, variant));
    case GENDER_YOU:
        return tagged_jtrans(make_stringf("[gender you %s]", variant_tag),
                             decline_pronoun(gender, variant));
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
