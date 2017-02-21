/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#ifndef JAPANESE_H
#define JAPANESE_H

enum jconj
{
    JCONJ_IRRE, // 未然形
    JCONJ_CONT, // 連用形
    JCONJ_TERM, // 終止形
    JCONJ_ATTR, // 連体形
    JCONJ_HYPO, // 仮定形
    JCONJ_IMPR, // 命令形
    JCONJ_PERF, // 完了形
    JCONJ_PASS, // 受動態
    JCONJ_PRES, // 現在形
};

const string conjugate_verb_j(const string &verb, bool plural);
const string decline_pronoun_j(gender_type gender, pronoun_type variant);
const char * general_counter_suffix(const int size);
string jconj_verb(const string& verb, jconj conj);

#endif
