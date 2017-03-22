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

string jconj_verb(const string& verb, jconj conj)
{
    string v = verb;

    switch (conj)
    {
        // 必要に応じて随時追加
    case JCONJ_IRRE:  break;
    case JCONJ_CONT:  break;
    case JCONJ_TERM:  break;
    case JCONJ_ATTR:  break;
    case JCONJ_HYPO:  break;
    case JCONJ_IMPR:  break;
    case JCONJ_PERF:
        v = replace_all(v, "立てる", "立てた");
        v = replace_all(v, "鳴く", "鳴いた");
        v = replace_all(v, "放つ", "放った");
        v = replace_all(v, "吠える", "吠えた");
        break;
    case JCONJ_PASS:
        v = replace_all(v, "を追放した", "が追放された"); // _takenote_kill_verb()
        v = replace_all(v, "を中立化した", "が中立化させられた");
        v = replace_all(v, "を隷属させた", "が隷属させられた");
        v = replace_all(v, "をスライムに変えた", "がスライムに変えられた");
        v = replace_all(v, "を殺した", "が殺された");
        break;
    case JCONJ_PRES:
        v = replace_all(v, "射撃した", "射撃する");
    }

    return v;
}

/*
 * english.cc/thing_do_grammar()の代替
 * 英語処理が必要な箇所もあるためthing_do_grammarは残す
 */
string thing_do_grammar_j(description_level_type dtype, bool add_stop,
                        bool force_article, string desc)
{
    if (dtype == DESC_PLAIN || !force_article)
        return desc;

    switch (dtype)
    {
    case DESC_THE:
    case DESC_A:
        return desc;
    case DESC_NONE:
        return "";
    default:
        return desc;
    }
}
