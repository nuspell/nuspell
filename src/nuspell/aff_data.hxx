/* Copyright 2016-2018 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Affixing data structures, private header.
 */

#ifndef NUSPELL_AFF_DATA_HXX
#define NUSPELL_AFF_DATA_HXX

#include "locale_utils.hxx"
#include "structures.hxx"

#include <iosfwd>

namespace nuspell {

enum class Flag_Type {
	SINGLE_CHAR /**< single-character flag, e.g. for "a" */,
	DOUBLE_CHAR /**< double-character flag, e.g for "aa" */,
	NUMBER /**< numerical flag, e.g. for 61 */,
	UTF8 /**< UTF-8 flag, e.g. for "á" */
};

using Word_List_Base =
    Hash_Multiset<std::pair<std::string, Flag_Set>, my_string_view<char>,
                  member<std::pair<std::string, Flag_Set>, std::string,
                         &std::pair<std::string, Flag_Set>::first>>;
/**
 * @brief Map between words and word_flags.
 *
 * Flags are stored as part of the container. Maybe for the future flags should
 * be stored elsewhere (flag aliases) and this should store pointers.
 *
 * Does not store morphological data as is low priority feature and is out of
 * scope.
 */
class Word_List : public Word_List_Base {
      public:
	using Word_List_Base::equal_range;
	auto equal_range(const std::wstring& word) const
	    -> std::pair<Word_List_Base::local_const_iterator,
	                 Word_List_Base::local_const_iterator>;
};

struct Aff_Data {
	// data members
	// word list
	Word_List words;

	Substr_Replacer<wchar_t> input_substr_replacer;
	Substr_Replacer<wchar_t> output_substr_replacer;
	Break_Table<wchar_t> break_table;
	std::basic_string<wchar_t> ignored_chars;
	Prefix_Table<wchar_t> prefixes;
	Suffix_Table<wchar_t> suffixes;
	std::vector<Compound_Pattern<wchar_t>> compound_patterns;
	Replacement_Table<wchar_t> replacements;
	std::vector<Similarity_Group<wchar_t>> similarities;
	std::basic_string<wchar_t> keyboard_closeness;
	std::basic_string<wchar_t> try_chars;
	Phonetic_Table<wchar_t> phonetic_table;

	// general options
	Encoding encoding;
	icu::Locale icu_locale;
	Flag_Type flag_type;
	bool complex_prefixes;
	bool fullstrip;
	bool checksharps;
	bool forbid_warn;
	char16_t circumfix_flag;
	char16_t forbiddenword_flag;
	char16_t keepcase_flag;
	char16_t need_affix_flag;
	char16_t substandard_flag;
	char16_t warn_flag;

	std::vector<Flag_Set> flag_aliases;
	std::string wordchars; // deprecated?

	// suggestion options
	char16_t nosuggest_flag;
	unsigned short max_compound_suggestions;
	unsigned short max_ngram_suggestions;
	unsigned short max_diff_factor;
	bool only_max_diff;
	bool no_split_suggestions;
	bool suggest_with_dots;

	// compounding options
	unsigned short compound_min_length;
	unsigned short compound_max_word_count;
	char16_t compound_flag;
	char16_t compound_begin_flag;
	char16_t compound_last_flag;
	char16_t compound_middle_flag;
	char16_t compound_onlyin_flag;
	char16_t compound_permit_flag;
	char16_t compound_forbid_flag;
	char16_t compound_root_flag;
	char16_t compound_force_uppercase;
	bool compound_more_suffixes;
	bool compound_check_duplicate;
	bool compound_check_rep;
	bool compound_check_case;
	bool compound_check_triple;
	bool compound_simplified_triple;

	Compound_Rule_Table compound_rules;

	unsigned short compound_syllable_max;
	std::string compound_syllable_vowels;
	Flag_Set compound_syllable_num;

	// methods
	auto parse_aff(std::istream& in) -> bool;
	auto parse_dic(std::istream& in) -> bool;
	auto parse_aff_dic(std::istream& aff, std::istream& dic)
	{
		if (parse_aff(aff))
			return parse_dic(dic);
		return false;
	}
};
} // namespace nuspell

#endif // NUSPELL_AFF_DATA_HXX
