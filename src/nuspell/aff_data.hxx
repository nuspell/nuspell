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

#ifndef HUNSPELL_AFF_MANAGER_HXX
#define HUNSPELL_AFF_MANAGER_HXX

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "structures.hxx"

namespace nuspell {

auto get_locale_name(std::string lang, std::string enc,
                     const std::string& filename = "") -> std::string;

class Encoding {
	std::string name;

      public:
	enum Type { SINGLEBYTE = false, UTF8 = true };

	Encoding() = default;
	Encoding(const std::string& e);
	auto empty() const -> bool { return name.empty(); }
	operator const std::string&() const { return name; }
	auto value() const -> const std::string& { return name; }
	auto is_utf8() const -> bool { return name == "UTF-8"; }
	operator Type() const { return is_utf8() ? UTF8 : SINGLEBYTE; }
};

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
	String_Set<CharT> ignored_chars;
};

struct Affix {
	using string = std::string;
	template <class T>
	using vector = std::vector<T>;

	char16_t flag;
	bool cross_product;
	string stripping;
	string affix;
	Flag_Set new_flags;
	string condition;
	vector<string> morphological_fields;
};

struct Compound_Check_Pattern {
	using string = std::string;

	string end_chars;
	char16_t end_flag;
	string begin_chars;
	char16_t begin_flag;
	string replacement;
};

struct Aff_Data {
	using string = std::string;
	using u16string = std::u16string;
	using istream = std::istream;
	template <class T>
	using vector = std::vector<T>;
	template <class T, class U>
	using pair = std::pair<T, U>;

	Aff_Structures<char> structures;
	Aff_Structures<wchar_t> wide_structures;

	template <class CharT>
	auto get_structures() -> Aff_Structures<CharT>&;

	Encoding encoding;
	Flag_Type flag_type;
	bool complex_prefixes;
	string language_code;
	string ignore_chars;
	vector<Flag_Set> flag_aliases;
	vector<vector<string>> morphological_aliases;

	std::locale locale_aff;

	vector<Affix> prefixes;
	vector<Affix> suffixes;

	// suggestion options
	string keyboard_layout;
	string try_chars;
	char16_t nosuggest_flag;
	short max_compound_suggestions;
	short max_ngram_suggestions;
	short max_diff_factor;
	bool only_max_diff;
	bool no_split_suggestions;
	bool suggest_with_dots;
	vector<pair<string, string>> replacements;
	vector<string> map_related_chars; // vector<vector<string>>?
	vector<pair<string, string>> phonetic_replacements;
	char16_t warn_flag;
	bool forbid_warn;

	// compounding options
	vector<string> break_patterns;
	vector<string> compound_rules;
	short compound_minimum;
	char16_t compound_flag;
	char16_t compound_begin_flag;
	char16_t compound_last_flag;
	char16_t compound_middle_flag;
	char16_t compound_onlyin_flag;
	char16_t compound_permit_flag;
	char16_t compound_forbid_flag;
	bool compound_more_suffixes;
	char16_t compound_root_flag;
	short compound_word_max;
	bool compound_check_up;
	bool compound_check_rep;
	bool compound_check_case;
	bool compound_check_triple;
	bool compound_simplified_triple;

	vector<Compound_Check_Pattern> compound_check_patterns;
	char16_t compound_force_uppercase;
	short compound_syllable_max;
	string compound_syllable_vowels;
	Flag_Set compound_syllable_num;

	// others
	char16_t circumfix_flag;
	char16_t forbiddenword_flag;
	bool fullstrip;
	char16_t keepcase_flag;
	vector<pair<string, string>> input_conversion;
	vector<pair<string, string>> output_conversion;
	char16_t need_affix_flag;
	char16_t substandard_flag;
	string wordchars; // deprecated?
	bool checksharps;

	// methods
	auto parse(istream& in) -> bool;
	void log(const string& affpath);

	auto decode_flags(istream& in, size_t line_num = 0) const -> u16string;
	auto decode_single_flag(istream& in, size_t line_num = 0) const
	    -> char16_t;
};

template <>
auto inline Aff_Data::get_structures<char>() -> Aff_Structures<char>&
{
	return structures;
}
template <>
auto inline Aff_Data::get_structures<wchar_t>() -> Aff_Structures<wchar_t>&
{
	return wide_structures;
}
void parse_morhological_fields(std::istream& in,
                               std::vector<std::string>& vecOut);
}

#endif
