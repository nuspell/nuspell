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
#include <memory>
#include <stdexcept>

#include <boost/algorithm/string/trim.hpp>
#include <boost/locale.hpp>

namespace nuspell {

using namespace std;
using boost::make_iterator_range;
template <class CharT>
using str_view = boost::basic_string_view<CharT>;

/** Check spelling for a word.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
template <class CharT>
auto Dictionary::spell_priv(std::basic_string<CharT> s) -> Spell_Result
{
	auto& loc = locale_aff;
	auto& d = get_structures<CharT>();

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
// only these explicit template instantiations are needed for the spell_*
// functions.
template auto Dictionary::spell_priv(const string s) -> Spell_Result;
template auto Dictionary::spell_priv(const wstring s) -> Spell_Result;

/**
 * Checks recursively the spelling according to break patterns.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
template <class CharT>
auto Dictionary::spell_break(std::basic_string<CharT> s) -> Spell_Result
{
	// check spelling accoring to case
	auto res = spell_casing<CharT>(s);
	if (res)
		return res;

	auto& break_table = get_structures<CharT>().break_table;

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

/**
 * Calls appropriate spell cehck method accoring to the word's casing.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
template <class CharT>
auto Dictionary::spell_casing(std::basic_string<CharT> s) -> Spell_Result
{
	auto casing_type = classify_casing(s);
	const Flag_Set* res = nullptr;

	switch (casing_type) {
	case Casing::SMALL:
	case Casing::CAMEL:
	case Casing::PASCAL:
		res = checkword(s);
		break;
	case Casing::ALL_CAPITAL:
		res = spell_casing_upper(s);
		break;
	case Casing::INIT_CAPITAL:
		res = spell_casing_title(s);
		break;
	default:
		throw logic_error(
		    "Fatal error, bad casing type, should never execute");
		break;
	}
	if (res) {
		// handle forbidden words
		if (res->exists(forbiddenword_flag)) {
			return BAD_WORD;
		}
		if (forbid_warn && res->exists(warn_flag)) {
			return BAD_WORD;
		}
		return GOOD_WORD;
	}
	return BAD_WORD;
}

/**
 * Checks spelling for word in upper case. Note that additionally, when at the
 * end no result has been found, that spell_casing_title is called.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
template <class CharT>
auto Dictionary::spell_casing_upper(std::basic_string<CharT> s)
    -> const Flag_Set*
{
	auto& loc = locale_aff;
	auto res = checkword(s);
	if (res)
		return res;

	// handling of prefixes separated by apostrophe for Catalan, French and
	// Italian, e.g. SANT'ELIA -> Sant'+Elia
	auto apos = s.find('\'');
	if (apos != std::string::npos) {
		if (apos == s.size() - 1) {
			// apostrophe is at end of word
			auto t = boost::locale::to_title(s, loc);
			res = checkword(t);
		}
		else {
			// apostophe is at beginning of word or diving the word
			auto part1 =
			    boost::locale::to_title(s.substr(0, apos + 1), loc);
			auto part2 =
			    boost::locale::to_title(s.substr(apos + 1), loc);
			auto t = part1 + part2;
			res = checkword(t);
		}
		if (res)
			return res;
	}

	// handle German sharp s
	if (checksharps && s.find(LITERAL(CharT, "SS")) != std::string::npos) {
		auto t = boost::locale::to_lower(s, loc);
		res = spell_sharps(t);
		if (!res)
			t = boost::locale::to_title(s, loc);
		res = spell_sharps(t);
		if (res)
			return res;
	}

	// if not returned earlier
	auto t = boost::locale::to_title(s, loc);
	res = spell_casing_title(t);
	if (res && res->exists(keepcase_flag))
		res = nullptr;
	return res;
}

template <class CharT>
auto Dictionary::spell_casing_title(std::basic_string<CharT> s)
    -> const Flag_Set*
{
	auto& loc = locale_aff;

	// check title case
	auto res = checkword<CharT>(s);

	// handle forbidden words
	if (res && res->exists(forbiddenword_flag)) {
		res = nullptr;
	}

	// return result
	if (res) {
		return res;
	}

	// attempt checking lower case spelling
	auto t = boost::locale::to_lower(s, loc);
	res = checkword<CharT>(t);

	// with CHECKSHARPS, ß is allowed too in KEEPCASE words with title case
	if (res && res->exists(keepcase_flag) &&
	    !(checksharps &&
	      (t.find(static_cast<CharT>(223)) != std::string::npos))) {
		res = nullptr;
	}

	return res;
}

template <class CharT>
auto Dictionary::spell_sharps(std::basic_string<CharT> base, size_t pos,
                              size_t n, size_t rep) -> const Flag_Set*
{
	const size_t MAX_SHARPS = 5;
	pos = base.find(LITERAL(CharT, "ss"), pos);
	if (pos != std::string::npos && (n < MAX_SHARPS)) {
		base[pos] = static_cast<CharT>(223); // ß
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
	return nullptr;
}

template <class CharT>
auto Dictionary::checkword(std::basic_string<CharT>& s) const -> const Flag_Set*
{
	{
		auto ret = words.lookup(s);
		if (ret)
			return ret;
	}
	{
		auto ret2 = strip_prefix_only(s);
		if (ret2)
			return &get<1>(*ret2);
	}
	{
		auto ret3 = strip_suffix_only(s);
		if (ret3)
			return &get<1>(*ret3);
	}
	{
		auto ret4 = strip_prefix_then_suffix(s);
		if (ret4)
			return &get<1>(*ret4);
	}
	{
		auto ret5 = strip_suffix_then_prefix(s);
		if (ret5)
			return &get<1>(*ret5);
	}
	{
		auto ret6 = strip_suffix_then_suffix(s);
		if (ret6)
			return &get<1>(*ret6);
	}
	return nullptr;
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_only(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Prefix<CharT>&>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto pfx = str_view<CharT>(word).substr(0, aff_len);
		auto entries = prefixes.equal_range(pfx);
		for (auto& e : make_iterator_range(entries)) {
			if (m == FULL_WORD &&
			    e.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_END &&
			    !e.cont_flags.exists(compound_permit_flag))
				continue;
			if (e.cont_flags.exists(need_affix_flag))
				continue;
			if (e.cont_flags.exists(circumfix_flag))
				continue;

			e.to_root(word);
			auto unrooter = [&](auto* rt) { e.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!e.check_condition(word))
				continue;
			for (auto& word_entry :
			     make_iterator_range(dic.equal_range(word))) {
				auto& word_flags = word_entry.second;
				if (!word_flags.exists(e.flag))
					continue;
				// needflag check here if needed
				return {{word, word_flags, e}};
			}
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_only(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Suffix<CharT>&>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto sfx = str_view<CharT>(word).substr(word.size() - aff_len);
		auto entries = suffixes.equal_range(sfx);
		for (auto& e : make_iterator_range(entries)) {
			if ((m == FULL_WORD ||
			     (aff_len == 0 && m == AT_COMPOUND_END)) &&
			    e.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_BEGIN &&
			    !e.cont_flags.exists(compound_permit_flag))
				continue;
			if (e.cont_flags.exists(need_affix_flag))
				continue;
			if (e.cont_flags.exists(circumfix_flag))
				continue;

			e.to_root(word);
			auto unrooter = [&](auto* rt) { e.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!e.check_condition(word))
				continue;
			for (auto& word_entry :
			     make_iterator_range(dic.equal_range(word))) {
				auto& word_flags = word_entry.second;
				if (!word_flags.exists(e.flag))
					continue;
				if (m != FULL_WORD &&
				    word_flags.exists(compound_onlyin_flag))
					continue;
				// needflag check here if needed
				return {{word, word_flags, e}};
			}
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_then_suffix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Suffix<CharT>&, const Prefix<CharT>&>>
{
	auto& prefixes = get_structures<CharT>().prefixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto pfx = str_view<CharT>(word).substr(0, aff_len);
		auto entries = prefixes.equal_range(pfx);
		for (auto& pe : make_iterator_range(entries)) {
			if (pe.cross_product == false)
				continue;
			if (m == FULL_WORD &&
			    pe.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_END &&
			    !pe.cont_flags.exists(compound_permit_flag))
				continue;
			if (pe.cont_flags.exists(need_affix_flag))
				continue;

			pe.to_root(word);
			auto unrooter = [&](auto* rt) { pe.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!pe.check_condition(word))
				continue;
			auto ret = strip_pfx_then_sfx_2<m>(pe, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_pfx_then_sfx_2(const Prefix<CharT>& pe,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Suffix<CharT>&, const Prefix<CharT>&>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto sfx = str_view<CharT>(word).substr(word.size() - aff_len);
		auto entries = suffixes.equal_range(sfx);
		for (auto& se : make_iterator_range(entries)) {
			if (se.cross_product == false)
				continue;
			if (m == FULL_WORD &&
			    se.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_BEGIN &&
			    !se.cont_flags.exists(compound_permit_flag))
				continue;
			if (pe.cont_flags.exists(circumfix_flag) !=
			    se.cont_flags.exists(circumfix_flag))
				continue;

			se.to_root(word);
			auto unrooter = [&](auto* rt) { se.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!se.check_condition(word))
				continue;
			for (auto& word_entry :
			     make_iterator_range(dic.equal_range(word))) {
				auto& word_flags = word_entry.second;
				if (!se.cont_flags.exists(pe.flag) &&
				    !word_flags.exists(pe.flag))
					continue;
				if (!word_flags.exists(se.flag))
					continue;
				if (m != FULL_WORD &&
				    word_flags.exists(compound_onlyin_flag))
					continue;
				// needflag check here if needed
				return {{word, word_flags, se, pe}};
			}
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_then_prefix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Prefix<CharT>&, const Suffix<CharT>&>>
{
	auto& suffixes = get_structures<CharT>().suffixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto sfx = str_view<CharT>(word).substr(word.size() - aff_len);
		auto entries = suffixes.equal_range(sfx);
		for (auto& se : make_iterator_range(entries)) {
			if (se.cross_product == false)
				continue;
			if (m == FULL_WORD &&
			    se.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_BEGIN &&
			    !se.cont_flags.exists(compound_permit_flag))
				continue;
			if (se.cont_flags.exists(need_affix_flag))
				continue;

			se.to_root(word);
			auto unrooter = [&](auto* rt) { se.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!se.check_condition(word))
				continue;
			auto ret = strip_sfx_then_pfx_2<m>(se, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_sfx_then_pfx_2(const Suffix<CharT>& se,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Prefix<CharT>&, const Suffix<CharT>&>>
{

	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto pfx = str_view<CharT>(word).substr(0, aff_len);
		auto entries = prefixes.equal_range(pfx);
		for (auto& pe : make_iterator_range(entries)) {
			if (pe.cross_product == false)
				continue;
			if (m == FULL_WORD &&
			    pe.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_END &&
			    !pe.cont_flags.exists(compound_permit_flag))
				continue;
			if (pe.cont_flags.exists(circumfix_flag) !=
			    se.cont_flags.exists(circumfix_flag))
				continue;

			pe.to_root(word);
			auto unrooter = [&](auto* rt) { pe.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!pe.check_condition(word))
				continue;
			for (auto& word_entry :
			     make_iterator_range(dic.equal_range(word))) {
				auto& word_flags = word_entry.second;
				if (!pe.cont_flags.exists(se.flag) &&
				    !word_flags.exists(se.flag))
					continue;
				if (!word_flags.exists(pe.flag))
					continue;
				if (m != FULL_WORD &&
				    word_flags.exists(compound_onlyin_flag))
					continue;
				// needflag check here if needed
				return {{word, word_flags, pe, se}};
			}
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_then_suffix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Suffix<CharT>&, const Suffix<CharT>&>>
{
	auto& suffixes = get_structures<CharT>().suffixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto sfx1 = str_view<CharT>(word).substr(0, aff_len);
		auto entries = suffixes.equal_range(sfx1);
		for (auto& se1 : make_iterator_range(entries)) {
			if (m == FULL_WORD &&
			    se1.cont_flags.exists(compound_onlyin_flag))
				continue;
			if (m == AT_COMPOUND_END &&
			    !se1.cont_flags.exists(compound_permit_flag))
				continue;
			if (se1.cont_flags.exists(need_affix_flag))
				continue;
			if (se1.cont_flags.exists(circumfix_flag))
				continue;

			se1.to_root(word);
			auto unrooter = [&](auto* rt) { se1.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!se1.check_condition(word))
				continue;
			auto ret = strip_sfx_then_sfx_2(se1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_sfx_then_sfx_2(const Suffix<CharT>& se1,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<std::basic_string<CharT>, const Flag_Set&,
                                  const Suffix<CharT>&, const Suffix<CharT>&>>
{

	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (size_t aff_len = 0; aff_len <= word.size(); ++aff_len) {
		auto sfx2 = str_view<CharT>(word).substr(0, aff_len);
		auto entries = suffixes.equal_range(sfx2);
		for (auto& se2 : make_iterator_range(entries)) {
			if (!se2.cont_flags.exists(se1.flag))
				continue;
			if (m == FULL_WORD &&
			    se2.cont_flags.exists(compound_onlyin_flag))
				continue;
			// if (m == AT_COMPOUND_END &&
			//    !se2.cont_flags.exists(compound_permit_flag))
			//	continue;
			if (se2.cont_flags.exists(circumfix_flag))
				continue;

			se2.to_root(word);
			auto unrooter = [&](auto* rt) { se2.to_derived(*rt); };
			auto unroot_at_end_of_iteration =
			    unique_ptr<basic_string<CharT>, decltype(unrooter)>{
			        &word, unrooter};

			if (!se2.check_condition(word))
				continue;
			for (auto& word_entry :
			     make_iterator_range(dic.equal_range(word))) {
				auto& word_flags = word_entry.second;
				if (!word_flags.exists(se2.flag))
					continue;
				// if (m != FULL_WORD &&
				//    word_flags.exists(compound_onlyin_flag))
				//	continue;
				// needflag check here if needed
				return {{word, word_flags, se2, se1}};
			}
		}
	}
	return {};
}

} // namespace nuspell
