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
auto Dictionary::strip_prefix_only(std::basic_string<CharT>& word)
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Prefix<CharT>&>>
{
        auto& aff = aff_data;
        auto& dic = dic_data;
        auto& affixes = aff.get_structures<CharT>().affixes;

        for (size_t aff_len = 0; 0 < aff_len && aff_len <= word.size();
	     ++aff_len) {
		auto affix = word.substr(0, aff_len);
		auto entries = affixes.equal_range(affix);
		for (auto& e : boost::make_iterator_range(entries)) {
			if (/*full word &&*/ e.cont_flags.exists(
			    aff.compound_onlyin_flag))
				continue;
			// if (/*at compount end &&*/ !e.cont_flags.exists(
			//    aff.compound_permit_flag))
			//	continue;
			if (e.cont_flags.exists(aff.need_affix_flag))
				continue;
			if (e.cont_flags.exists(aff.circumfix_flag))
				continue;

			e.to_root(word);
			if (!e.check_condition(word)) {
				e.to_derived(word);
				continue;
			}
			auto word_flags = dic.lookup(word);
			if (!word_flags) {
				e.to_derived(word);
				continue;
			}
			if (!word_flags.exists(e.flag)) {
				e.to_derived(word);
				continue;
			}
			// needflag check here if needed

			auto word2 = word;
			e.to_derived(word); // revert word as it was passed
			return {word2, word_flags, e};
		}
	}
        return {};
}
template <>
auto Dictionary::strip_prefix_only(std::string& word) -> boost::optional<
    std::tuple<std::string, const Flag_Set&, const Prefix<char>&>>;
template <>
auto Dictionary::strip_prefix_only(std::wstring& word) -> boost::optional<
    std::tuple<std::wstring, const Flag_Set&, const Prefix<wchar_t>&>>;

template <class CharT>
auto Dictionary::spell_priv(std::basic_string<CharT> s) -> Spell_Result
{
	auto& loc = aff_data.locale_aff;
	auto& d = aff_data.get_structures<CharT>();

	// allow words under maximum length
	size_t MAXWORDLENGTH = 100; // TODO refactor to more global
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
	auto c = classify_casing(s);
	const Flag_Set* res = NULL;

	if (c == Casing::SMALL || c == Casing::CAMEL || c == Casing::PASCAL) {
		// handle small case, camel case and pascal case
		res = checkword<CharT>(s);
		;
	}
	else if (c == Casing::ALL_CAPITAL) {
		// handle upper case
		res = spell_casing_upper(s);
	}
	else if (c == Casing::INIT_CAPITAL) {
		// handle capitalized case
		res = spell_casing_title(s);
	}
	else {
		// handle forbidden state
		cerr << "Nuspell error: unsupported casing" << std::endl;
	}

	if (res) {
		// handle forbidden words
		if (res->exists(aff_data.forbiddenword_flag)) {
			cerr << "Nuspell warning: word " << s.c_str()
			     << " has forbiden flag" << std::endl;
			if (aff_data.forbid_warn) {
				// return BAD_WORD; //TODO enable
			}
		}
		// return GOOD_WORD; //TODO enable
	}

	if (dic_data.lookup(s))   // TODO remove
		return GOOD_WORD; // TODO remove
	return BAD_WORD;
}
template auto Dictionary::spell_casing(string& s) -> Spell_Result;
template auto Dictionary::spell_casing(wstring& s) -> Spell_Result;

template <class CharT>
auto Dictionary::spell_casing_upper(std::basic_string<CharT>& s)
    -> const Flag_Set*
{
	auto& loc = aff_data.locale_aff;
	auto res = checkword<CharT>(s);
	if (res)
		return res;

	// handling of prefixes separated by apostrophe for Catalan, French and
	// Italian, e.g. SANT'ELIA -> Sant'+Elia
	auto apos = s.find('\'');
	if (apos != std::string::npos) {
		if (apos == s.size() - 1) {
			// apostrophe is at end of word
			auto t = boost::locale::to_title(s, loc);
			res = checkword<CharT>(t);
		}
		else {
			// apostophe is at beginning of word or diving the word
			auto part1 =
			    boost::locale::to_title(s.substr(0, apos + 1), loc);
			auto part2 =
			    boost::locale::to_title(s.substr(apos + 1), loc);
			auto t = part1 + part2;
			res = checkword<CharT>(t);
		}
		if (res)
			return res;
	}

	// handle German sharp s
	if (aff_data.checksharps &&
	    s.find(LITERAL(CharT, "SS")) != std::string::npos) {
		auto t = boost::locale::to_lower(s, loc);
		res = spell_sharps(t);
		if (!res)
			t = boost::locale::to_title(s, loc);
		res = spell_sharps(t);
		if (res)
			return res;
	}

	// if not returned earlier
	res = spell_casing_title(s);
	if (res && res->exists(aff_data.keepcase_flag))
		res = NULL;
	return res;
}
template auto Dictionary::spell_casing_upper(string& s) -> const Flag_Set*;
template auto Dictionary::spell_casing_upper(wstring& s) -> const Flag_Set*;

template <class CharT>
auto Dictionary::spell_casing_title(std::basic_string<CharT>& s)
    -> const Flag_Set*
{
	auto& loc = aff_data.locale_aff;
	//	const Flag_Set* res = NULL;
	// TODO compelte this method!

	auto t = boost::locale::to_title(s, loc);
	// handle idot
	auto res = checkword<CharT>(s);

	return res;
}
template auto Dictionary::spell_casing_title(string& s) -> const Flag_Set*;
template auto Dictionary::spell_casing_title(wstring& s) -> const Flag_Set*;

template <class CharT>
auto Dictionary::spell_sharps(std::basic_string<CharT>& base, size_t n_pos,
                              size_t n, size_t rep) -> const Flag_Set*
{
	size_t MAXSHARPS = 5; // TODO refactor to more global
	auto pos = base.find(LITERAL(CharT, "ss"), n_pos);
	if (pos != std::string::npos && (n < MAXSHARPS)) {
		base[pos] = 223; // ß
		base.erase(pos + 1, 1);
		auto res = spell_sharps(base, pos + 2, n + 1, rep + 1);
		if (res)
			return res;
		base[pos] = 's';
		base.insert(pos + 1, 1, 's');
		res = spell_sharps(base, pos + 2, n + 1, rep);
		if (res)
			return res;
	}
	else if (rep > 0) {
		return checkword(base);
	}
	return NULL;
}
template auto Dictionary::spell_sharps(string& base, size_t n_pos, size_t n,
                                       size_t rep) -> const Flag_Set*;
template auto Dictionary::spell_sharps(wstring& base, size_t n_pos, size_t n,
                                       size_t rep) -> const Flag_Set*;

template <class CharT>
auto Dictionary::checkword(std::basic_string<CharT>& s) -> const Flag_Set*
{
	return dic_data.lookup(s);
}
template auto Dictionary::checkword(string& s) -> const Flag_Set*;
template auto Dictionary::checkword(wstring& s) -> const Flag_Set*;
}
