/* Copyright 2016-2017 Dimitrij Mijoski
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

#include <istream>
#include <string>
#include <utility>
#include <vector>

#include "structures.hxx"

namespace hunspell {

auto get_locale_name(std::string lang, std::string enc,
                     const std::string& filename = "") -> std::string;

class Encoding {
	std::string name;

      public:
	Encoding() = default;
	Encoding(const std::string& e,
	         const std::locale& ascii_loc = std::locale::classic());
	auto empty() const -> bool { return name.empty(); }
	operator const std::string&() const { return name; }
	auto value() const -> const std::string& { return name; }
	auto is_utf8() const -> bool { return name == "UTF-8"; }
};

enum Flag_Type { FLAG_SINGLE_CHAR, FLAG_DOUBLE_CHAR, FLAG_NUMBER, FLAG_UTF8 };

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

	Encoding encoding; // TODO This is only used during parsing. To prevent
	                   // accidental usage of empty value, move this out of
	                   // this class and let it only live locally in the
	                   // parse method.
	Flag_Type flag_type;
	bool complex_prefixes;
	string language_code;
	string ignore_chars;
	vector<Flag_Set> flag_aliases;
	vector<vector<string>> morphological_aliases;

	std::locale locale_aff;

	// suggestion options
	string keyboard_layout;
	string try_chars;
	char16_t nosuggest_flag;
	short max_compound_suggestions = -1;
	short max_ngram_suggestions = -1;
	short max_diff_factor = -1;
	bool only_max_diff;
	bool no_split_suggestions;
	bool suggest_with_dots;
	vector<pair<string, string>> replacements;
	vector<string> map_related_chars;
	vector<pair<string, string>> phonetic_replacements;
	char16_t warn_flag;
	bool forbid_warn;

	// compouding options
	vector<string> break_patterns;
	vector<string> compound_rules;
	short compoud_minimum = -1;
	char16_t compound_flag;
	char16_t compound_begin_flag;
	char16_t compound_last_flag;
	char16_t compound_middle_flag;
	char16_t compound_onlyin_flag;
	char16_t compound_permit_flag;
	char16_t compound_forbid_flag;
	bool compound_more_suffixes;
	char16_t compound_root_flag;
	short compound_word_max = -1;
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

	vector<Affix> prefixes;
	vector<Affix> suffixes;

	// others
	char16_t circumfix_flag;
	char16_t forbiddenword_flag;
	bool fullstrip;
	char16_t keepcase_flag;
	vector<pair<string, string>> input_conversion;
	vector<pair<string, string>> output_conversion;
	char16_t need_affix_flag;
	char16_t substandard_flag;
	string wordchars;
	bool checksharps;

	// methods
	auto parse(istream& in) -> bool;
	void log(const string& affpath);

	auto decode_flags(istream& in, size_t line_num = 0) const -> u16string;
	auto decode_single_flag(istream& in, size_t line_num = 0) const
	    -> char16_t;
};
void parse_morhological_fields(std::istream& in,
                               std::vector<std::string>& vecOut);
}

#endif
