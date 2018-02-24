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

#include <boost/algorithm/string/trim.hpp>

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

	d.input_substr_replacer.replace(s);
	boost::trim(s, loc);
	if (s.empty())
		return GOOD_WORD;
	bool abbreviation = s.back() == '.';
	boost::trim_right_if(s, [](auto c) { return c == '.'; });
	if (s.empty())
		return GOOD_WORD;

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
	auto res = spell_casing<CharT>(s);
	if (res)
		return res;

	auto& break_table = aff_data.get_structures<CharT>().break_table;
	for (auto& pat : break_table.start_word_breaks()) {
		if (s.compare(0, pat.size(), pat) == 0) {
			auto substr = s.substr(pat.size());
			auto res = spell_break<CharT>(substr);
			if (res)
				return res;
		}
	}
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
	if (dic_data.lookup(s))
		return GOOD_WORD;
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
