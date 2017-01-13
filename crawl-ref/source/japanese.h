/**
 * @file
 * @brief Functions and data structures dealing with the syntax,
 *        morphology, and orthography of the Japanese language.
**/

#ifndef JAPANESE_H
#define JAPANESE_H

const string conjugate_verb_j(const string &verb, bool plural);
const string decline_pronoun_j(gender_type gender, pronoun_type variant);
const char * general_counter_suffix(const int size);

#endif
