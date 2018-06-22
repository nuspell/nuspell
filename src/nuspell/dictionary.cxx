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
 * @file dictionary.cxx
 * Dictionary spelling.
 */

#include "dictionary.hxx"
#include "string_utils.hxx"

#include <iostream>
#include <stdexcept>

#include <boost/locale.hpp>

namespace nuspell {

using namespace std;
using boost::make_iterator_range;
using boost::locale::to_lower;
using boost::locale::to_title;

/** Check spelling for a word.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
template <class CharT>
auto Dictionary::spell_priv(std::basic_string<CharT> s) -> Spell_Result
{
	auto& d = get_structures<CharT>();

	// allow words under maximum length
	size_t MAXWORDLENGTH = 180;
	if (s.size() >= MAXWORDLENGTH)
		return BAD_WORD;

	// do input conversion (iconv)
	d.input_substr_replacer.replace(s);

	// triming whitespace should be part of tokenization, not here
	// boost::trim(s, locale_aff);
	if (s.empty())
		return GOOD_WORD;
	bool abbreviation = s.back() == '.';
	if (abbreviation) {
		// trim trailing periods
		auto i = s.find_last_not_of('.');
		// if i == npos, i + 1 == 0, so no need for extra if.
		s.erase(i + 1);
		if (s.empty())
			return GOOD_WORD;
	}

	// accept number
	if (is_number(s))
		return GOOD_WORD;

	// handle break patterns
	auto copy = s;
	auto ret = spell_break<CharT>(s);
	assert(s == copy);
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
auto Dictionary::spell_break(std::basic_string<CharT>& s, size_t depth)
    -> Spell_Result
{
	// check spelling accoring to case
	auto res = spell_casing<CharT>(s);
	if (res) {
		// handle forbidden words
		if (res->contains(forbiddenword_flag)) {
			return BAD_WORD;
		}
		if (forbid_warn && res->contains(warn_flag)) {
			return BAD_WORD;
		}
		return GOOD_WORD;
	}
	if (depth == 9)
		return BAD_WORD;

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
			auto res1 = spell_break<CharT>(part1, depth + 1);
			if (!res1)
				continue;
			auto res2 = spell_break<CharT>(part2, depth + 1);
			if (res2)
				return res2;
		}
	}

	return BAD_WORD;
}

/**
 * Checks spelling according to casing of the provided word.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
template <class CharT>
auto Dictionary::spell_casing(std::basic_string<CharT>& s) -> const Flag_Set*
{
	auto casing_type = classify_casing(s, locale_aff);
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
	}
	return res;
}

/**
 * Checks spelling for a word which is in all upper case.
 *
 * @param s string to check spelling for.
 * @return The flags of the corresponding dictionary word.
 */
template <class CharT>
auto Dictionary::spell_casing_upper(std::basic_string<CharT>& s)
    -> const Flag_Set*
{
	auto& loc = locale_aff;

	auto res = checkword(s);
	if (res)
		return res;

	// handle prefixes separated by apostrophe for Catalan, French and
	// Italian, e.g. SANT'ELIA -> Sant'+Elia
	auto apos = s.find('\'');
	if (apos != s.npos && apos != s.size() - 1) {
		// apostophe is at beginning of word or dividing the word
		auto part1 = s.substr(0, apos + 1);
		auto part2 = s.substr(apos + 1);
		part1 = to_lower(part1, loc);
		part2 = to_title(part2, loc);
		auto t = part1 + part2;
		res = checkword(t);
		if (res)
			return res;
		part1 = to_title(part1, loc);
		t = part1 + part2;
		res = checkword(t);
		if (res)
			return res;
	}

	// handle sharp s for German
	if (checksharps && s.find(LITERAL(CharT, "SS")) != s.npos) {
		auto t = to_lower(s, loc);
		res = spell_sharps(t);
		if (!res)
			t = to_title(s, loc);
		res = spell_sharps(t);
		if (res)
			return res;
	}
	auto t = to_title(s, loc);
	res = checkword(t);
	if (res && !res->contains(keepcase_flag))
		return res;

	t = to_lower(s, loc);
	res = checkword(t);
	if (res && !res->contains(keepcase_flag))
		return res;
	return nullptr;
}

/**
 * Checks spelling for a word which is in title casing.
 *
 * @param s string to check spelling for.
 * @return The flags of the corresponding dictionary word.
 */
template <class CharT>
auto Dictionary::spell_casing_title(std::basic_string<CharT>& s)
    -> const Flag_Set*
{
	auto& loc = locale_aff;

	// check title case
	auto res = checkword<CharT>(s);

	// forbid bad capitalization
	if (res && res->contains(forbiddenword_flag))
		return nullptr;
	if (res)
		return res;

	// attempt checking lower case spelling
	auto t = to_lower(s, loc);
	res = checkword<CharT>(t);

	// with CHECKSHARPS, ß is allowed too in KEEPCASE words with title case
	if (res && res->contains(keepcase_flag) &&
	    !(checksharps && (t.find(static_cast<CharT>(223)) != t.npos))) {
		res = nullptr;
	}
	return res;
}

/**
 * Checks recursively spelling starting on a word in title or lower case which
 * originate from a word in upper case containing the letters 'SS'. The
 * technique used is use recursion for checking all variations with repetitions
 * of minimal one replacement of 'ss' with sharp s 'ß'. Maximum recursion depth
 * is limited with a hardcoded value.
 *
 * @param base string to check spelling for where zero or more occurences of
 * 'ss' have been replaced by sharp s 'ß'.
 * @param pos position in the string to start next find and replacement.
 * @param n counter for the recursion depth.
 * @param rep counter for the number of replacements done.
 * @return The flags of the corresponding dictionary word.
 */
template <class CharT>
auto Dictionary::spell_sharps(std::basic_string<CharT>& base, size_t pos,
                              size_t n, size_t rep) -> const Flag_Set*
{
	const size_t MAX_SHARPS = 5;
	pos = base.find(LITERAL(CharT, "ss"), pos);
	if (pos != std::string::npos && n < MAX_SHARPS) {
		base[pos] = static_cast<CharT>(223); // ß
		base.erase(pos + 1, 1);
		auto res = spell_sharps(base, pos + 1, n + 1, rep + 1);
		base[pos] = 's';
		base.insert(pos + 1, 1, 's');
		if (res)
			return res;
		res = spell_sharps(base, pos + 2, n + 1, rep);
		if (res)
			return res;
	}
	else if (rep > 0) {
		return checkword(base);
	}
	return nullptr;
}

/**
 * Checks spelling for various unaffixed versions of the provided word.
 * Unaffixing is done by combinations of zero or more unsuffixing and
 * unprefixing operations.
 *
 * @param s string to check spelling for.
 * @return The flags of the corresponding dictionary word.
 */
template <class CharT>
auto Dictionary::checkword(std::basic_string<CharT>& s) const -> const Flag_Set*
{

	for (auto& we : make_iterator_range(words.equal_range(s))) {
		auto& word_flags = we.second;
		if (word_flags.contains(need_affix_flag))
			continue;
		if (word_flags.contains(compound_onlyin_flag))
			continue;
		return &word_flags;
	}
	{
		auto ret2 = strip_prefix_only(s);
		if (ret2)
			return &get<0>(*ret2).second;
	}
	{
		auto ret3 = strip_suffix_only(s);
		if (ret3)
			return &get<0>(*ret3).second;
	}
	{
		auto ret4 = strip_prefix_then_suffix(s);
		if (ret4)
			return &get<0>(*ret4).second;
	}
	{
		auto ret5 = strip_suffix_then_prefix(s);
		if (ret5)
			return &get<0>(*ret5).second;
	}
	if (complex_prefixes == false) {
		auto ret6 = strip_suffix_then_suffix(s);
		if (ret6)
			return &get<0>(*ret6).second;

		auto ret7 = strip_prefix_then_2_suffixes(s);
		if (ret7)
			return &get<0>(*ret7).second;

		auto ret8 = strip_suffix_prefix_suffix(s);
		if (ret8)
			return &get<0>(*ret8).second;

		auto ret9 = strip_2_suffixes_then_prefix(s);
		if (ret9)
			return &get<0>(*ret9).second;
	}
	else {
		auto ret6 = strip_prefix_then_prefix(s);
		if (ret6)
			return &get<0>(*ret6).second;
		auto ret7 = strip_suffix_then_2_prefixes(s);
		if (ret7)
			return &get<0>(*ret7).second;

		auto ret8 = strip_prefix_suffix_prefix(s);
		if (ret8)
			return &get<0>(*ret8).second;

		auto ret9 = strip_2_prefixes_then_suffix(s);
		if (ret9)
			return &get<0>(*ret9).second;
	}
	auto ret10 = check_compound(s);
	if (ret10)
		return &get<0>(*ret10).second;

	return nullptr;
}

template <class CharT, template <class> class AffixT>
class To_Root_Unroot_RAII {
      private:
	basic_string<CharT>& word;
	const AffixT<CharT>& affix;

      public:
	To_Root_Unroot_RAII(basic_string<CharT>& w, const AffixT<CharT>& a)
	    : word(w), affix(a)
	{
		affix.to_root(word);
	}
	~To_Root_Unroot_RAII() { affix.to_derived(word); }
};

template <Affixing_Mode m, class CharT>
auto Dictionary::affix_NOT_valid(const Prefix<CharT>& e) const
{
	if (m == FULL_WORD && e.cont_flags.contains(compound_onlyin_flag))
		return true;
	if (m == AT_COMPOUND_END &&
	    !e.cont_flags.contains(compound_permit_flag))
		return true;
	if (m != FULL_WORD && e.cont_flags.contains(compound_forbid_flag))
		return true;
	return false;
}
template <Affixing_Mode m, class CharT>
auto Dictionary::affix_NOT_valid(const Suffix<CharT>& e) const
{
	if (m == FULL_WORD && e.cont_flags.contains(compound_onlyin_flag))
		return true;
	if (m == AT_COMPOUND_BEGIN &&
	    !e.cont_flags.contains(compound_permit_flag))
		return true;
	if (m != FULL_WORD && e.cont_flags.contains(compound_forbid_flag))
		return true;
	return false;
}
template <Affixing_Mode m, class AffixT>
auto Dictionary::outer_affix_NOT_valid(const AffixT& e) const
{
	if (affix_NOT_valid<m>(e))
		return true;
	if (e.cont_flags.contains(need_affix_flag))
		return true;
	return false;
}
template <class AffixT>
auto Dictionary::is_circumfix(const AffixT& a) const
{
	return a.cont_flags.contains(circumfix_flag);
}

template <class AffixInner, class AffixOuter>
auto cross_valid_inner_outer(const AffixInner& inner, const AffixOuter& outer)
{
	return inner.cont_flags.contains(outer.flag);
}

template <class Affix>
auto cross_valid_inner_outer(const Flag_Set& word_flags, const Affix& afx)
{
	return word_flags.contains(afx.flag);
}

template <class CharT>
auto prefix(const basic_string<CharT>& word, size_t len)
{
	return my_string_view<CharT>(word).substr(0, len);
}
template <class CharT>
auto prefix(basic_string<CharT>&& word, size_t len) = delete;

template <class CharT>
auto suffix(const basic_string<CharT>& word, size_t len)
{
	return my_string_view<CharT>(word).substr(word.size() - len);
}
template <class CharT>
auto suffix(basic_string<CharT>&& word, size_t len) = delete;

/**
 * @brief Iterator of prefix entres that match a word.
 *
 * Iterates all prefix entries where the .appending member is prefix of a given
 * word.
 */
template <class CharT>
class Prefix_Iter {
	const Prefix_Table<CharT>& tbl;
	const basic_string<CharT>& word;
	size_t len;
	my_string_view<CharT> pfx;
	using iter = typename Prefix_Table<CharT>::iterator;
	iter a;
	iter b;
	bool valid;

      public:
	Prefix_Iter(const Prefix_Table<CharT>& tbl,
	            const basic_string<CharT>& word)
	    : tbl(tbl), word(word)
	{
		for (len = 0; len <= word.size(); ++len) {
			pfx = prefix(word, len);
			tie(a, b) = tbl.equal_range(pfx);
			for (; a != b; ++a) {
				valid = true;
				return;
			}
		}
		valid = false;
	}
	auto& operator++()
	{
		goto resume_pfx_of_word;

		for (len = 0; len <= word.size(); ++len) {
			pfx = prefix(word, len);
			tie(a, b) = tbl.equal_range(pfx);
			for (; a != b; ++a) {
				return *this;
			resume_pfx_of_word:;
			}
		}
		valid = false;
		return *this;
	}
	operator bool() { return valid; }
	auto& operator*() { return *a; }
};

/**
 * @brief Iterator of suffix entres that match a word.
 *
 * Iterates all suffix entries where the .appending member is suffix of a given
 * word.
 */
template <class CharT>
class Suffix_Iter {
	const Suffix_Table<CharT>& tbl;
	const basic_string<CharT>& word;
	size_t len;
	my_string_view<CharT> pfx;
	using iter = typename Suffix_Table<CharT>::iterator;
	iter a;
	iter b;
	bool valid;

      public:
	Suffix_Iter(const Suffix_Table<CharT>& tbl,
	            const basic_string<CharT>& word)
	    : tbl(tbl), word(word)
	{
		for (len = 0; len <= word.size(); ++len) {
			pfx = suffix(word, len);
			tie(a, b) = tbl.equal_range(pfx);
			for (; a != b; ++a) {
				valid = true;
				return;
			}
		}
		valid = false;
	}
	auto& operator++()
	{
		goto resume_sfx_of_word;

		for (len = 0; len <= word.size(); ++len) {
			pfx = suffix(word, len);
			tie(a, b) = tbl.equal_range(pfx);
			for (; a != b; ++a) {
				return *this;
			resume_sfx_of_word:;
			}
		}
		valid = false;
		return *this;
	}
	operator bool() { return valid; }
	auto& operator*() { return *a; }
	auto aff_len() { return len; }
};

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_only(std::basic_string<CharT>& word) const
    -> boost::optional<
        std::tuple<Dic_Data::const_reference, const Prefix<CharT>&>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& e = *it;
		if (outer_affix_NOT_valid<m>(e))
			continue;
		if (is_circumfix(e))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, e);
		if (!e.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, e))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check
			if (m == AT_COMPOUND_BEGIN &&
			    !word_flags.contains(compound_flag) &&
			    !e.cont_flags.contains(compound_flag) &&
			    !word_flags.contains(compound_begin_flag) &&
			    !e.cont_flags.contains(compound_begin_flag))
				continue;
			if (m == AT_COMPOUND_MIDDLE &&
			    !word_flags.contains(compound_flag) &&
			    !e.cont_flags.contains(compound_flag) &&
			    !word_flags.contains(compound_middle_flag) &&
			    !e.cont_flags.contains(compound_middle_flag))
				continue;
			if (m == AT_COMPOUND_END &&
			    !word_flags.contains(compound_flag) &&
			    !e.cont_flags.contains(compound_flag) &&
			    !word_flags.contains(compound_last_flag) &&
			    !e.cont_flags.contains(compound_last_flag))
				continue;
			return {{word_entry, e}};
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_only(std::basic_string<CharT>& word) const
    -> boost::optional<
        std::tuple<Dic_Data::const_reference, const Suffix<CharT>&>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& e = *it;
		if (outer_affix_NOT_valid<m>(e))
			continue;
		if ((it.aff_len() == 0 && m == AT_COMPOUND_END) &&
		    e.cont_flags.contains(compound_onlyin_flag))
			continue;
		if (is_circumfix(e))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, e);
		if (!e.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, e))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check
			if (m == AT_COMPOUND_BEGIN &&
			    !word_flags.contains(compound_flag) &&
			    !e.cont_flags.contains(compound_flag) &&
			    !word_flags.contains(compound_begin_flag) &&
			    !e.cont_flags.contains(compound_begin_flag))
				continue;
			if (m == AT_COMPOUND_MIDDLE &&
			    !word_flags.contains(compound_flag) &&
			    !e.cont_flags.contains(compound_flag) &&
			    !word_flags.contains(compound_middle_flag) &&
			    !e.cont_flags.contains(compound_middle_flag))
				continue;
			if (m == AT_COMPOUND_END &&
			    !word_flags.contains(compound_flag) &&
			    !e.cont_flags.contains(compound_flag) &&
			    !word_flags.contains(compound_last_flag) &&
			    !e.cont_flags.contains(compound_last_flag))
				continue;
			return {{word_entry, e}};
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_then_suffix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Suffix<CharT>&, const Prefix<CharT>&>>
{
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		auto ret = strip_pfx_then_sfx_2<m>(pe, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_pfx_then_sfx_2(const Prefix<CharT>& pe,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Suffix<CharT>&, const Prefix<CharT>&>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(se))
			continue;
		if (is_circumfix(pe) != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se, pe) &&
			    !cross_valid_inner_outer(word_flags, pe))
				continue;
			if (!cross_valid_inner_outer(word_flags, se))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {{word_entry, se, pe}};
		}
	}

	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_then_prefix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Prefix<CharT>&, const Suffix<CharT>&>>
{
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		auto ret = strip_sfx_then_pfx_2<m>(se, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_sfx_then_pfx_2(const Suffix<CharT>& se,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Prefix<CharT>&, const Suffix<CharT>&>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(pe))
			continue;
		if (is_circumfix(pe) != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe, se) &&
			    !cross_valid_inner_outer(word_flags, se))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {{word_entry, pe, se}};
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_then_suffix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Suffix<CharT>&, const Suffix<CharT>&>>
{
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se1 = *it;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		auto ret = strip_sfx_then_sfx_2<FULL_WORD>(se1, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_sfx_then_sfx_2(const Suffix<CharT>& se1,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Suffix<CharT>&, const Suffix<CharT>&>>
{

	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		if (is_circumfix(se2))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			// needflag check here if needed
			return {{word_entry, se2, se1}};
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_then_prefix(std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Prefix<CharT>&, const Prefix<CharT>&>>
{
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe1 = *it;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		auto ret = strip_pfx_then_pfx_2<FULL_WORD>(pe1, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_pfx_then_pfx_2(const Prefix<CharT>& pe1,
                                      std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference,
                                  const Prefix<CharT>&, const Prefix<CharT>&>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		if (is_circumfix(pe2))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			// needflag check here if needed
			return {{word_entry, pe2, pe1}};
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_then_2_suffixes(std::basic_string<CharT>& word)
    const -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& prefixes = get_structures<CharT>().prefixes;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto i1 = Prefix_Iter<CharT>(prefixes, word); i1; ++i1) {
		auto& pe1 = *i1;
		if (pe1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = Suffix_Iter<CharT>(suffixes, word); i2; ++i2) {
			auto& se1 = *i2;
			if (se1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se1))
				continue;
			if (is_circumfix(pe1) != is_circumfix(se1))
				continue;
			To_Root_Unroot_RAII<CharT, Suffix> yyy(word, se1);
			if (!se1.check_condition(word))
				continue;
			auto ret = strip_pfx_2_sfx_3<FULL_WORD>(pe1, se1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_pfx_2_sfx_3(const Prefix<CharT>& pe1,
                                   const Suffix<CharT>& se1,
                                   std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		if (is_circumfix(se2))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se1, pe1) &&
			    !cross_valid_inner_outer(word_flags, pe1))
				continue;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			// needflag check here if needed
			return {{word_entry}};
		}
	}

	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_prefix_suffix(std::basic_string<CharT>& word)
    const -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& prefixes = get_structures<CharT>().prefixes;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto i1 = Suffix_Iter<CharT>(suffixes, word); i1; ++i1) {
		auto& se1 = *i1;
		if (se1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = Prefix_Iter<CharT>(prefixes, word); i2; ++i2) {
			auto& pe1 = *i2;
			if (pe1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe1))
				continue;
			To_Root_Unroot_RAII<CharT, Prefix> yyy(word, pe1);
			if (!pe1.check_condition(word))
				continue;
			auto ret = strip_s_p_s_3<FULL_WORD>(se1, pe1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_s_p_s_3(const Suffix<CharT>& se1,
                               const Prefix<CharT>& pe1,
                               std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1) &&
		    !cross_valid_inner_outer(pe1, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		auto circ1ok = (is_circumfix(pe1) == is_circumfix(se1)) &&
		               !is_circumfix(se2);
		auto circ2ok = (is_circumfix(pe1) == is_circumfix(se2)) &&
		               !is_circumfix(se1);
		if (!circ1ok && !circ2ok)
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se2, pe1) &&
			    !cross_valid_inner_outer(word_flags, pe1))
				continue;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			// needflag check here if needed
			return {{word_entry}};
		}
	}

	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_2_suffixes_then_prefix(std::basic_string<CharT>& word)
    const -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto i1 = Suffix_Iter<CharT>(suffixes, word); i1; ++i1) {
		auto& se1 = *i1;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = Suffix_Iter<CharT>(suffixes, word); i2; ++i2) {
			auto& se2 = *i2;
			if (se2.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se2))
				continue;
			To_Root_Unroot_RAII<CharT, Suffix> yyy(word, se2);
			if (!se2.check_condition(word))
				continue;
			auto ret = strip_2_sfx_pfx_3<FULL_WORD>(se1, se2, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_2_sfx_pfx_3(const Suffix<CharT>& se1,
                                   const Suffix<CharT>& se2,
                                   std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe1 = *it;
		if (!cross_valid_inner_outer(se2, se1) &&
		    !cross_valid_inner_outer(pe1, se1))
			continue;
		if (affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(se2) != is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe1, se2) &&
			    !cross_valid_inner_outer(word_flags, se2))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe1))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			// needflag check here if needed
			return {{word_entry}};
		}
	}

	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_suffix_then_2_prefixes(std::basic_string<CharT>& word)
    const -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& prefixes = get_structures<CharT>().prefixes;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto i1 = Suffix_Iter<CharT>(suffixes, word); i1; ++i1) {
		auto& se1 = *i1;
		if (se1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = Prefix_Iter<CharT>(prefixes, word); i2; ++i2) {
			auto& pe1 = *i2;
			if (pe1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe1))
				continue;
			if (is_circumfix(se1) != is_circumfix(pe1))
				continue;
			To_Root_Unroot_RAII<CharT, Prefix> yyy(word, pe1);
			if (!pe1.check_condition(word))
				continue;
			auto ret = strip_sfx_2_pfx_3<FULL_WORD>(se1, pe1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_sfx_2_pfx_3(const Suffix<CharT>& se1,
                                   const Prefix<CharT>& pe1,
                                   std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		if (is_circumfix(pe2))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe1, se1) &&
			    !cross_valid_inner_outer(word_flags, se1))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			return {{word_entry}};
		}
	}

	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_prefix_suffix_prefix(std::basic_string<CharT>& word)
    const -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& prefixes = get_structures<CharT>().prefixes;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto i1 = Prefix_Iter<CharT>(prefixes, word); i1; ++i1) {
		auto& pe1 = *i1;
		if (pe1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = Suffix_Iter<CharT>(suffixes, word); i2; ++i2) {
			auto& se1 = *i2;
			if (se1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se1))
				continue;
			To_Root_Unroot_RAII<CharT, Suffix> yyy(word, se1);
			if (!se1.check_condition(word))
				continue;
			auto ret = strip_p_s_p_3<FULL_WORD>(pe1, se1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_p_s_p_3(const Prefix<CharT>& pe1,
                               const Suffix<CharT>& se1,
                               std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& dic = words;
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto it = Prefix_Iter<CharT>(prefixes, word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1) &&
		    !cross_valid_inner_outer(se1, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		auto circ1ok = (is_circumfix(se1) == is_circumfix(pe1)) &&
		               !is_circumfix(pe2);
		auto circ2ok = (is_circumfix(se1) == is_circumfix(pe2)) &&
		               !is_circumfix(pe1);
		if (!circ1ok && !circ2ok)
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe2, se1) &&
			    !cross_valid_inner_outer(word_flags, se1))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			return {{word_entry}};
		}
	}

	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_2_prefixes_then_suffix(std::basic_string<CharT>& word)
    const -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& prefixes = get_structures<CharT>().prefixes;

	for (auto i1 = Prefix_Iter<CharT>(prefixes, word); i1; ++i1) {
		auto& pe1 = *i1;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<CharT, Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = Prefix_Iter<CharT>(prefixes, word); i2; ++i2) {
			auto& pe2 = *i2;
			if (pe2.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe2))
				continue;
			To_Root_Unroot_RAII<CharT, Prefix> yyy(word, pe2);
			if (!pe2.check_condition(word))
				continue;
			auto ret = strip_2_pfx_sfx_3<FULL_WORD>(pe1, pe2, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m, class CharT>
auto Dictionary::strip_2_pfx_sfx_3(const Prefix<CharT>& pe1,
                                   const Prefix<CharT>& pe2,
                                   std::basic_string<CharT>& word) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
	auto& dic = words;
	auto& suffixes = get_structures<CharT>().suffixes;

	for (auto it = Suffix_Iter<CharT>(suffixes, word); it; ++it) {
		auto& se1 = *it;
		if (!cross_valid_inner_outer(pe2, pe1) &&
		    !cross_valid_inner_outer(se1, pe1))
			continue;
		if (affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(pe2) != is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<CharT, Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se1, pe2) &&
			    !cross_valid_inner_outer(word_flags, pe2))
				continue;
			if (!cross_valid_inner_outer(word_flags, se1))
				continue;
			// badflag check
			// if (m == FULL_WORD &&
			//    word_flags.contains(compound_onlyin_flag))
			//	continue;
			return {{word_entry}};
		}
	}

	return {};
}

template <class CharT>
auto Dictionary::check_compound(std::basic_string<CharT>& word,
                                size_t num) const
    -> boost::optional<std::tuple<Dic_Data::const_reference>>
{
        size_t min_length = 3;
        if (word.size() < min_length * 2)
                return {};
        auto part_str = basic_string<CharT>();
        size_t max_length = word.size() - min_length;
        for (auto i = min_length; i <= max_length; ++i) {
	        part_str.assign(word, 0, i);
		auto part1_entry =
		    check_nonaffixed_word_in_compound(part_str, num);
		if (!part1_entry) {
			auto x = strip_prefix_only<AT_COMPOUND_BEGIN>(part_str);
			if (x)
				part1_entry = &get<0>(*x);
		}
		if (!part1_entry) {
			auto x = strip_suffix_only<AT_COMPOUND_BEGIN>(part_str);
			if (x)
				part1_entry = &get<0>(*x);
		}
		if (!part1_entry) {
			return {};
		}

		part_str.assign(word, i, word.npos);
		auto part2_entry =
		    check_nonaffixed_word_in_compound(part_str, num);
		if (!part2_entry) {
			auto x = strip_prefix_only<AT_COMPOUND_END>(part_str);
			if (x)
				part2_entry = &get<0>(*x);
		}
		if (!part2_entry) {
			auto x = strip_suffix_only<AT_COMPOUND_END>(part_str);
			if (x)
				part2_entry = &get<0>(*x);
		}
		if (!part2_entry) {
			return {};
		}
		return {{*part1_entry}};
        }
        return {};
}

template <class CharT>
auto Dictionary::check_nonaffixed_word_in_compound(
    std::basic_string<CharT>& word, size_t num) const -> Dic_Data::const_pointer
{
	auto range = words.equal_range(word);
	for (auto& we : make_iterator_range(range)) {
		auto& word_flags = we.second;
		if (word_flags.contains(need_affix_flag))
			continue;
		if (word_flags.contains(compound_flag))
			return &we;
		if (num == 0 && word_flags.contains(compound_begin_flag))
			return &we;
		if (num != 0 && word_flags.contains(compound_middle_flag))
			return &we;
	}
	return nullptr;
}

} // namespace nuspell
