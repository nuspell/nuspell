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
 * @file aff_data.hxx
 * Affixing data structures.
 */

#ifndef NUSPELL_AFF_DATA_HXX
#define NUSPELL_AFF_DATA_HXX

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "locale_utils.hxx"
#include "structures.hxx"

namespace nuspell {

enum Flag_Type {
	FLAG_SINGLE_CHAR /**< single-character flag, e.g. for "a" */,
	FLAG_DOUBLE_CHAR /**< double-character flag, e.g for "aa" */,
	FLAG_NUMBER /**< numerical flag, e.g. for 61 */,
	FLAG_UTF8 /**< UTF-8 flag, e.g. for "á" */
};

template <class CharT>
struct Aff_Structures {
	Substr_Replacer<CharT> input_substr_replacer;
	Substr_Replacer<CharT> output_substr_replacer;
	Break_Table<CharT> break_table;
	std::basic_string<CharT> ignored_chars;
	Prefix_Table<CharT> prefixes;
	Suffix_Table<CharT> suffixes;
	std::vector<Compound_Pattern<CharT>> compound_patterns;
	Replacement_Table<CharT> replacements;
	std::vector<Similarity_Group<CharT>> similarities;
	std::basic_string<CharT> keyboard_closeness;
	std::basic_string<CharT> try_chars;
	Phonetic_Table<CharT> phonetic_table;
};

struct Affix {
	using string = std::string;
	template <class T>
	using vector = std::vector<T>;

	char16_t flag;
	bool cross_product;
	string stripping;
	string appending;
	Flag_Set new_flags;
	string condition;
	vector<string> morphological_fields;
};

struct Compound_Check_Pattern {
	using string = std::string;

	string first_word_end;
	string second_word_begin;
	string replacement;
	char16_t first_word_flag;
	char16_t second_word_flag;
};

using Word_List_Base =
    Hash_Multiset<std::pair<std::string, Flag_Set>, std::string,
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
	// types
	using string = std::string;
	using u16string = std::u16string;
	using istream = std::istream;
	template <class T>
	using vector = std::vector<T>;
	template <class T, class U>
	using pair = std::pair<T, U>;

	// data members
	// word list
	Word_List words;

	Aff_Structures<char> structures;
	Aff_Structures<wchar_t> wide_structures;

	// general options
	std::locale internal_locale;
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

	vector<Flag_Set> flag_aliases;
	string wordchars; // deprecated?

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
	string compound_syllable_vowels;
	Flag_Set compound_syllable_num;

	// methods
	auto set_encoding_and_language(const string& enc,
	                               const string& lang = "") -> void;
	auto parse_aff(istream& in) -> bool;
	auto parse_dic(istream& in) -> bool;
	auto parse_aff_dic(std::istream& aff, std::istream& dic)
	{
		if (parse_aff(aff))
			return parse_dic(dic);
		return false;
	}
	template <class CharT>
	auto get_structures() const -> const Aff_Structures<CharT>&;
};

template <>
auto inline Aff_Data::get_structures<char>() const
    -> const Aff_Structures<char>&
{
	return structures;
}
template <>
auto inline Aff_Data::get_structures<wchar_t>() const
    -> const Aff_Structures<wchar_t>&
{
	return wide_structures;
}
} // namespace nuspell

#endif // NUSPELL_AFF_DATA_HXX
