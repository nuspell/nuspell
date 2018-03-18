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

#include "dictionary.hxx"
#include "string_utils.hxx"

#include <iostream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/locale.hpp>

namespace nuspell {

using namespace std;

/**
 * Checks prefix.
 *
 * @param dic dictionary to use.
 * @param affix_table table with prefixes.
 * @param word to check.
 * @return
 */
template <class CharT>
auto prefix_check(const Dic_Data& dic, const Prefix_Table<CharT>& affix_table,
                  const basic_string<CharT>& word)
{
	for (size_t aff_len = 1; 0 < aff_len && aff_len <= word.size();
	     ++aff_len) {

		auto affix = word.substr(0, aff_len);
		auto entries = affix_table.equal_range(affix);
		for (auto& e : boost::make_iterator_range(entries)) {

			e.to_root(word);
			auto matched = e.check_condition(word);
			if (matched) {
				auto flags = dic.lookup(word);
				if (flags && flags->exists(e.flag)) {
					// success
					return true;
				}
			}
			e.to_derived(word);
		}
	}
	return false;
}

template <class CharT>
auto Dictionary::spell_priv(std::basic_string<CharT> s) -> Spell_Result
{
	auto& loc = aff_data.locale_aff;
	auto& d = aff_data.get_structures<CharT>();

	// allow words under maximum length
	// TODO move definition below to more generic place
	size_t MAXWORDLENGTH = 100;
	if (s.size() >= MAXWORDLENGTH)
		return BAD_WORD;

	// do input conversion (iconv)
	d.input_substr_replacer.replace(s);

	// clean word from whitespaces and periods
	boost::trim(s, loc);
	if (s.empty())
		return GOOD_WORD;
	bool abbreviation = s.back() == '.';
	boost::trim_right_if(s, [](auto c) { return c == '.'; });
	if (s.empty())
		return GOOD_WORD;

	// omit numbers containing, except those with double separators
	if (is_number(s))
		return GOOD_WORD;

	// handle break patterns
	auto ret = spell_break<CharT>(s);
	if (!ret && abbreviation) {
		s += '.';
		ret = spell_break<CharT>(s);
	}
	return ret;
}
template auto Dictionary::spell_priv(const string s) -> Spell_Result;
template auto Dictionary::spell_priv(const wstring s) -> Spell_Result;

template <class CharT>
auto Dictionary::spell_break(std::basic_string<CharT>& s) -> Spell_Result
{
	// check spelling accoring to case
	auto res = spell_casing<CharT>(s);
	if (res)
		return res;

	auto& break_table = aff_data.get_structures<CharT>().break_table;

	// handle break pattern at start of a word
	for (auto& pat : break_table.start_word_breaks()) {
		if (s.compare(0, pat.size(), pat) == 0) {
			auto substr = s.substr(pat.size());
			auto res = spell_break<CharT>(substr);
			if (res)
				return res;
		}
	}

	// handle break pattern at end of a word
	for (auto& pat : break_table.end_word_breaks()) {
		if (pat.size() > s.size())
			continue;
		if (s.compare(s.size() - pat.size(), pat.size(), pat) == 0) {
			auto substr = s.substr(0, s.size() - pat.size());
			auto res = spell_break<CharT>(substr);
			if (res)
				return res;
		}
	}

	// handle break pattern in middle of a word
	for (auto& pat : break_table.middle_word_breaks()) {
		auto i = s.find(pat);
		if (i > 0 && i < s.size() - pat.size()) {
			auto part1 = s.substr(0, i);
			auto part2 = s.substr(i + pat.size());
			auto res1 = spell_break<CharT>(part1);
			if (!res1)
				continue;
			auto res2 = spell_break<CharT>(part2);
			if (res2)
				return res2;
		}
	}

	return BAD_WORD;
}
template auto Dictionary::spell_break(string& s) -> Spell_Result;
template auto Dictionary::spell_break(wstring& s) -> Spell_Result;

template <class CharT>
auto Dictionary::spell_casing(std::basic_string<CharT>& s) -> Spell_Result
{
	auto INFO_WARNING = false;  // needs global storage
	auto SPELL_ORIGCAP = false; // needs more generic storing
	auto SPELL_INITCAP = false; // needs more generic storing
	auto c = classify_casing(s);

	// handle small case, camel case and pascal case
	if (c == Casing::SMALL || c == Casing::CAMEL || c == Casing::PASCAL) {
		if (c == Casing::CAMEL || c == Casing::PASCAL)
			SPELL_ORIGCAP = true;
		auto res = checkword<CharT>(s);
		if (!res) {
			auto t = std::basic_string<CharT>(s);
			t += '.';
			res = checkword<CharT>(t);
		}
	}
	// handle upper case and capitalized case
	else if (c == Casing::ALL_CAPITAL || c == Casing::INIT_CAPITAL) {
		SPELL_ORIGCAP = true;
		if (c == Casing::ALL_CAPITAL) {
			auto res = checkword<CharT>(s);
			if (!res) {
				auto t = std::basic_string<CharT>(s);
				t += '.';
				res = checkword<CharT>(t);
			}
			if (!res) {
				// handle apostrophe for Catalan, French and
				// Italian prefixes
				if (!res) {
					// handle German sharp s
				}
			}
		}
		if (c == Casing::INIT_CAPITAL) {
			auto t = boost::locale::to_title(s);
			// handle idot
		}
		if (c == Casing::INIT_CAPITAL) {
			SPELL_INITCAP = true;
		}
		auto res = checkword<CharT>(s);
		if (c == Casing::INIT_CAPITAL) {
			SPELL_INITCAP = false;
		}
		// forbid bad capitalization
		// (for example, ijs -> Ijs instead of IJs in Dutch)
		// use explicit forms in dic: Ijs/F (F = FORBIDDENWORD flag)
		//
		// the example above is no longer valid, could not find other
		// examples
		//		if (res &&
		//res->lookup<CharT>(aff_data.forbiddenword_flag)) {
		//			res = NULL;
		//		} else {
		//			if (res &&
		//res->lookup<CharT>(aff_data.keepcase_flag && c ==
		//Casing::ALL_CAPITAL))
		//				res = NULL;
		//		}
	}
	else {
		cerr << "Nuspell error: unsupported casing" << std::endl;
	}

	// handle forbidden words
	//	if (res) {
	//		if (res->lookup<CharT>(aff_data.forbiddenword_flag)) {
	//			INFO_WARNING = true;
	//			if (aff_data.forbid_warn) {
	//				return BAD_WORD;
	//			}
	//		}
	//		return GOOD_WORD;
	//	}

	if (dic_data.lookup(s))   // remove
		return GOOD_WORD; // remove
	return BAD_WORD;
}
template auto Dictionary::spell_casing(string& s) -> Spell_Result;
template auto Dictionary::spell_casing(wstring& s) -> Spell_Result;

template <class CharT>
auto Dictionary::checkword(std::basic_string<CharT>& s) -> const Flag_Set*
{
	return dic_data.lookup(s);
}
template auto Dictionary::checkword(string& s) -> const Flag_Set*;
template auto Dictionary::checkword(wstring& s) -> const Flag_Set*;
}
