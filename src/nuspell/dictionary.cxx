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

#include <boost/range/iterator_range_core.hpp>

namespace hunspell {

using namespace std;

/**
 * Checks prefix.
 *
 * @param dic dictionary to use.
 * @param affix_table table with prefixes.
 * @param word to check.
 * @return The TODO.
 */
auto prefix_check(const Dic_Data& dic, const Prefix_Table& affix_table,
                  string word)
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

	if (dic_data.lookup(s))
		return GOOD_WORD;
	return BAD_WORD;
}
template auto Dictionary::spell_priv(const string s) -> Spell_Result;
template auto Dictionary::spell_priv(const wstring s) -> Spell_Result;

template <class CharT>
auto Dictionary::spell_break(std::basic_string<CharT>& s) -> Spell_Result
{
	(void)s;
	return BAD_WORD;
}
template auto Dictionary::spell_break(string& s) -> Spell_Result;
template auto Dictionary::spell_break(wstring& s) -> Spell_Result;

template <class CharT>
auto Dictionary::spell_casing(std::basic_string<CharT>& s) -> Spell_Result
{
	(void)s;
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
