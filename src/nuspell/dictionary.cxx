/* Copyright 2016-2020 Dimitrij Mijoski
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
#include "utils.hxx"

#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <unicode/uchar.h>

using namespace std;

namespace nuspell {
inline namespace v5 {

template <class L>
class At_Scope_Exit {
	L& lambda;

      public:
	At_Scope_Exit(L& action) : lambda(action) {}
	~At_Scope_Exit() { lambda(); }
};

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#define ASE_INTERNAL1(lname, aname, ...)                                       \
	auto lname = [&]() { __VA_ARGS__; };                                   \
	At_Scope_Exit<decltype(lname)> aname(lname)

#define ASE_INTERNAL2(ctr, ...)                                                \
	ASE_INTERNAL1(MACRO_CONCAT(Auto_func_, ctr),                           \
	              MACRO_CONCAT(Auto_instance_, ctr), __VA_ARGS__)

#define AT_SCOPE_EXIT(...) ASE_INTERNAL2(__COUNTER__, __VA_ARGS__)

auto Dict_Base::spell_priv(string& s) const -> bool
{
	// do input conversion (iconv)
	input_substr_replacer.replace(s);

	// triming whitespace should be part of tokenization, not here

	if (s.empty())
		return true;
	bool abbreviation = s.back() == '.';
	if (abbreviation) {
		// trim trailing periods
		auto i = s.find_last_not_of('.');
		// if i == npos, i + 1 == 0, so no need for extra if.
		s.erase(i + 1);
		if (s.empty()) {
			// TODO add back removed periods
			return true;
		}
	}

	// accept number
	if (is_number(s))
		return true;

	erase_chars(s, ignored_chars);

	// handle break patterns
	auto copy = s;
	auto ret = spell_break(s);
	assert(s == copy);
	if (!ret && abbreviation) {
		s += '.';
		ret = spell_break(s);
	}
	return ret;
}

auto Dict_Base::spell_break(std::string& s, size_t depth) const -> bool
{
	// check spelling accoring to case
	auto res = spell_casing(s);
	if (res) {
		// handle forbidden words
		if (res->contains(forbiddenword_flag)) {
			return false;
		}
		if (forbid_warn && res->contains(warn_flag)) {
			return false;
		}
		return true;
	}
	if (depth == 9)
		return false;

	// handle break pattern at start of a word
	for (auto& pat : break_table.start_word_breaks()) {
		if (begins_with(s, pat)) {
			auto substr = s.substr(pat.size());
			auto res = spell_break(substr, depth + 1);
			if (res)
				return res;
		}
	}

	// handle break pattern at end of a word
	for (auto& pat : break_table.end_word_breaks()) {
		if (ends_with(s, pat)) {
			auto substr = s.substr(0, s.size() - pat.size());
			auto res = spell_break(substr, depth + 1);
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
			auto res1 = spell_break(part1, depth + 1);
			if (!res1)
				continue;
			auto res2 = spell_break(part2, depth + 1);
			if (res2)
				return res2;
		}
	}

	return false;
}

auto Dict_Base::spell_casing(std::string& s) const -> const Flag_Set*
{
	auto casing_type = classify_casing(s);
	const Flag_Set* res = nullptr;

	switch (casing_type) {
	case Casing::SMALL:
	case Casing::CAMEL:
	case Casing::PASCAL:
		res = check_word(s);
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

auto Dict_Base::spell_casing_upper(std::string& s) const -> const Flag_Set*
{
	auto& loc = icu_locale;

	auto res = check_word(s, ALLOW_BAD_FORCEUCASE);
	if (res)
		return res;

	// handle prefixes separated by apostrophe for Catalan, French and
	// Italian, e.g. SANT'ELIA -> Sant'+Elia
	auto apos = s.find('\'');
	if (apos != s.npos && apos != s.size() - 1) {
		// apostophe is at beginning of word or dividing the word
		auto part1 = s.substr(0, apos + 1);
		auto part2 = s.substr(apos + 1);
		to_lower(part1, loc, part1);
		to_title(part2, loc, part2);
		auto t = part1 + part2;
		res = check_word(t, ALLOW_BAD_FORCEUCASE);
		if (res)
			return res;
		to_title(part1, loc, part1);
		t = part1 + part2;
		res = check_word(t, ALLOW_BAD_FORCEUCASE);
		if (res)
			return res;
	}
	auto s2 = string();

	// handle sharp s for German
	if (checksharps && s.find("SS") != s.npos) {
		to_lower(s, loc, s2);
		res = spell_sharps(s2);
		if (res)
			return res;

		to_title(s, loc, s2);
		res = spell_sharps(s2);
		if (res)
			return res;
	}
	to_title(s, loc, s2);
	res = check_word(s2, ALLOW_BAD_FORCEUCASE);
	if (res && !res->contains(keepcase_flag))
		return res;

	to_lower(s, loc, s2);
	res = check_word(s2, ALLOW_BAD_FORCEUCASE);
	if (res && !res->contains(keepcase_flag))
		return res;
	return nullptr;
}

auto Dict_Base::spell_casing_title(std::string& s) const -> const Flag_Set*
{
	auto& loc = icu_locale;

	// check title case
	auto res = check_word(s, ALLOW_BAD_FORCEUCASE, SKIP_HIDDEN_HOMONYM);
	if (res)
		return res;

	auto s2 = string();
	to_lower(s, loc, s2);
	res = check_word(s2, ALLOW_BAD_FORCEUCASE);

	// with CHECKSHARPS, ß is allowed too in KEEPCASE words with title case
	if (res && res->contains(keepcase_flag) &&
	    !(checksharps && (s2.find("ß") != s.npos))) {
		res = nullptr;
	}
	return res;
}

/**
 * @internal
 * @brief Checks german word with double SS
 *
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
auto Dict_Base::spell_sharps(std::string& base, size_t pos, size_t n,
                             size_t rep) const -> const Flag_Set*
{
	const size_t MAX_SHARPS = 5;
	pos = base.find("ss", pos);
	if (pos != base.npos && n < MAX_SHARPS) {
		base.replace(pos, 2, "ß");
		auto res = spell_sharps(base, pos + 1, n + 1, rep + 1);
		base.replace(pos, 2, "ss");
		if (res)
			return res;
		res = spell_sharps(base, pos + 2, n + 1, rep);
		if (res)
			return res;
	}
	else if (rep > 0) {
		return check_word(base, ALLOW_BAD_FORCEUCASE);
	}
	return nullptr;
}

auto Dict_Base::check_word(std::string& s, Forceucase allow_bad_forceucase,
                           Hidden_Homonym skip_hidden_homonym) const
    -> const Flag_Set*
{

	auto ret1 = check_simple_word(s, skip_hidden_homonym);
	if (ret1)
		return ret1;
	auto ret2 = check_compound(s, allow_bad_forceucase);
	if (ret2)
		return &ret2->second;

	return nullptr;
}

auto Dict_Base::check_simple_word(std::string& s,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> const Flag_Set*
{
	for (auto& we : Subrange(words.equal_range(s))) {
		auto& word_flags = we.second;
		if (word_flags.contains(need_affix_flag))
			continue;
		if (word_flags.contains(compound_onlyin_flag))
			continue;
		if (skip_hidden_homonym &&
		    word_flags.contains(HIDDEN_HOMONYM_FLAG))
			continue;
		return &word_flags;
	}
	{
		auto ret3 = strip_suffix_only(s, skip_hidden_homonym);
		if (ret3)
			return &ret3->second;
	}
	{
		auto ret2 = strip_prefix_only(s, skip_hidden_homonym);
		if (ret2)
			return &ret2->second;
	}
	{
		auto ret4 = strip_prefix_then_suffix_commutative(
		    s, skip_hidden_homonym);
		if (ret4)
			return &ret4->second;
	}
	if (!complex_prefixes) {
		auto ret6 = strip_suffix_then_suffix(s, skip_hidden_homonym);
		if (ret6)
			return &ret6->second;

		auto ret7 =
		    strip_prefix_then_2_suffixes(s, skip_hidden_homonym);
		if (ret7)
			return &ret7->second;

		auto ret8 = strip_suffix_prefix_suffix(s, skip_hidden_homonym);
		if (ret8)
			return &ret8->second;

		// this is slow and unused so comment
		// auto ret9 = strip_2_suffixes_then_prefix(s,
		// skip_hidden_homonym); if (ret9)
		//	return &ret9->second;
	}
	else {
		auto ret6 = strip_prefix_then_prefix(s, skip_hidden_homonym);
		if (ret6)
			return &ret6->second;
		auto ret7 =
		    strip_suffix_then_2_prefixes(s, skip_hidden_homonym);
		if (ret7)
			return &ret7->second;

		auto ret8 = strip_prefix_suffix_prefix(s, skip_hidden_homonym);
		if (ret8)
			return &ret8->second;

		// this is slow and unused so comment
		// auto ret9 = strip_2_prefixes_then_suffix(s,
		// skip_hidden_homonym); if (ret9)
		//	return &ret9->second;
	}
	return nullptr;
}

template <class AffixT>
class To_Root_Unroot_RAII {
      private:
	string& word;
	const AffixT& affix;

      public:
	To_Root_Unroot_RAII(string& w, const AffixT& a) : word(w), affix(a)
	{
		affix.to_root(word);
	}
	~To_Root_Unroot_RAII() { affix.to_derived(word); }
};

template <Affixing_Mode m>
auto Dict_Base::affix_NOT_valid(const Prefix& e) const
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
template <Affixing_Mode m>
auto Dict_Base::affix_NOT_valid(const Suffix& e) const
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
auto Dict_Base::outer_affix_NOT_valid(const AffixT& e) const
{
	if (affix_NOT_valid<m>(e))
		return true;
	if (e.cont_flags.contains(need_affix_flag))
		return true;
	return false;
}
template <class AffixT>
auto Dict_Base::is_circumfix(const AffixT& a) const
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

template <Affixing_Mode m>
auto Dict_Base::is_valid_inside_compound(const Flag_Set& flags) const
{
	if (m == AT_COMPOUND_BEGIN && !flags.contains(compound_flag) &&
	    !flags.contains(compound_begin_flag))
		return false;
	if (m == AT_COMPOUND_MIDDLE && !flags.contains(compound_flag) &&
	    !flags.contains(compound_middle_flag))
		return false;
	if (m == AT_COMPOUND_END && !flags.contains(compound_flag) &&
	    !flags.contains(compound_last_flag))
		return false;
	return true;
}

/**
 * @internal
 * @brief strip_prefix_only
 * @param s derived word with affixes
 * @return if found, root word + prefix
 */
template <Affixing_Mode m>
auto Dict_Base::strip_prefix_only(std::string& word,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Prefix>
{
	auto& dic = words;

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& e = *it;
		if (outer_affix_NOT_valid<m>(e))
			continue;
		if (is_circumfix(e))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, e);
		if (!e.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, e))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(e.cont_flags))
				continue;
			return {word_entry, e};
		}
	}
	return {};
}

/**
 * @internal
 * @brief strip_suffix_only
 * @param s derived word with affixes
 * @return if found, root word + suffix
 */
template <Affixing_Mode m>
auto Dict_Base::strip_suffix_only(std::string& word,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Suffix>
{
	auto& dic = words;
	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& e = *it;
		if (outer_affix_NOT_valid<m>(e))
			continue;
		if (e.appending.size() != 0 && m == AT_COMPOUND_END &&
		    e.cont_flags.contains(compound_onlyin_flag))
			continue;
		if (is_circumfix(e))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, e);
		if (!e.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, e))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(e.cont_flags))
				continue;
			return {word_entry, e};
		}
	}
	return {};
}

/**
 * @internal
 * @brief strip_prefix_then_suffix
 *
 * This accepts a derived word that was formed first by adding
 * suffix then prefix to the root. The stripping is in reverse.
 *
 * @param s derived word with affixes
 * @return if found, root word + suffix + prefix
 */
template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_suffix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Suffix, Prefix>
{
	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		auto ret =
		    strip_pfx_then_sfx_2<m>(pe, word, skip_hidden_homonym);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_then_sfx_2(const Prefix& pe, std::string& word,
                                     Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Suffix, Prefix>
{
	auto& dic = words;

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(se))
			continue;
		if (is_circumfix(pe) != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
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
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(se.cont_flags) &&
			    !is_valid_inside_compound<m>(pe.cont_flags))
				continue;
			return {word_entry, se, pe};
		}
	}

	return {};
}

/**
 * @internal
 * @brief strip_suffix_then_prefix
 *
 * This accepts a derived word that was formed first by adding
 * prefix then suffix to the root. The stripping is in reverse.
 *
 * @param s derived word with prefix and suffix
 * @return if found, root word + prefix + suffix
 */
template <Affixing_Mode m>
auto Dict_Base::strip_suffix_then_prefix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Prefix, Suffix>
{
	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		auto ret =
		    strip_sfx_then_pfx_2<m>(se, word, skip_hidden_homonym);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_sfx_then_pfx_2(const Suffix& se, std::string& word,
                                     Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Prefix, Suffix>
{
	auto& dic = words;

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(pe))
			continue;
		if (is_circumfix(pe) != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
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
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(se.cont_flags) &&
			    !is_valid_inside_compound<m>(pe.cont_flags))
				continue;
			return {word_entry, pe, se};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_suffix_commutative(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Suffix, Prefix>
{
	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(pe))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		auto ret =
		    strip_pfx_then_sfx_comm_2<m>(pe, word, skip_hidden_homonym);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_then_sfx_comm_2(
    const Prefix& pe, std::string& word,
    Hidden_Homonym skip_hidden_homonym) const -> Affixing_Result<Suffix, Prefix>
{
	auto& dic = words;
	auto has_needaffix_pe = pe.cont_flags.contains(need_affix_flag);
	auto is_circumfix_pe = is_circumfix(pe);

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(se))
			continue;
		auto has_needaffix_se = se.cont_flags.contains(need_affix_flag);
		if (has_needaffix_pe && has_needaffix_se)
			continue;
		if (is_circumfix_pe != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;

			auto valid_cross_pe_outer =
			    !has_needaffix_pe &&
			    cross_valid_inner_outer(word_flags, se) &&
			    (cross_valid_inner_outer(se, pe) ||
			     cross_valid_inner_outer(word_flags, pe));

			auto valid_cross_se_outer =
			    !has_needaffix_se &&
			    cross_valid_inner_outer(word_flags, pe) &&
			    (cross_valid_inner_outer(pe, se) ||
			     cross_valid_inner_outer(word_flags, se));

			if (!valid_cross_pe_outer && !valid_cross_se_outer)
				continue;

			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(se.cont_flags) &&
			    !is_valid_inside_compound<m>(pe.cont_flags))
				continue;
			return {word_entry, se, pe};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_suffix_then_suffix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Suffix, Suffix>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!suffixes.has_continuation_flags())
		return {};

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se1 = *it;

		// The following check is purely for performance, it does not
		// change correctness.
		if (!suffixes.has_continuation_flag(se1.flag))
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		auto ret = strip_sfx_then_sfx_2<FULL_WORD>(se1, word,
		                                           skip_hidden_homonym);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_sfx_then_sfx_2(const Suffix& se1, std::string& word,
                                     Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Suffix, Suffix>
{

	auto& dic = words;

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		if (is_circumfix(se2))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check here if needed
			return {word_entry, se2, se1};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_prefix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Prefix, Prefix>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!prefixes.has_continuation_flags())
		return {};

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe1 = *it;
		// The following check is purely for performance, it does not
		// change correctness.
		if (!prefixes.has_continuation_flag(pe1.flag))
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		auto ret = strip_pfx_then_pfx_2<FULL_WORD>(pe1, word,
		                                           skip_hidden_homonym);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_then_pfx_2(const Prefix& pe1, std::string& word,
                                     Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<Prefix, Prefix>
{
	auto& dic = words;

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		if (is_circumfix(pe2))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check here if needed
			return {word_entry, pe2, pe1};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_2_suffixes(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!suffixes.has_continuation_flags())
		return {};

	for (auto i1 = prefixes.iterate_prefixes_of(word); i1; ++i1) {
		auto& pe1 = *i1;
		if (pe1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = suffixes.iterate_suffixes_of(word); i2; ++i2) {
			auto& se1 = *i2;

			// The following check is purely for performance, it
			// does not change correctness.
			if (!suffixes.has_continuation_flag(se1.flag))
				continue;

			if (se1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se1))
				continue;
			if (is_circumfix(pe1) != is_circumfix(se1))
				continue;
			To_Root_Unroot_RAII<Suffix> yyy(word, se1);
			if (!se1.check_condition(word))
				continue;
			auto ret = strip_pfx_2_sfx_3<FULL_WORD>(
			    pe1, se1, word, skip_hidden_homonym);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_2_sfx_3(const Prefix& pe1, const Suffix& se1,
                                  std::string& word,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		if (is_circumfix(se2))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se1, pe1) &&
			    !cross_valid_inner_outer(word_flags, pe1))
				continue;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check here if needed
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_suffix_prefix_suffix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!suffixes.has_continuation_flags() &&
	    !prefixes.has_continuation_flags())
		return {};

	for (auto i1 = suffixes.iterate_suffixes_of(word); i1; ++i1) {
		auto& se1 = *i1;

		// The following check is purely for performance, it
		// does not change correctness.
		if (!suffixes.has_continuation_flag(se1.flag) &&
		    !prefixes.has_continuation_flag(se1.flag))
			continue;

		if (se1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = prefixes.iterate_prefixes_of(word); i2; ++i2) {
			auto& pe1 = *i2;
			if (pe1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe1))
				continue;
			To_Root_Unroot_RAII<Prefix> yyy(word, pe1);
			if (!pe1.check_condition(word))
				continue;
			auto ret = strip_s_p_s_3<FULL_WORD>(
			    se1, pe1, word, skip_hidden_homonym);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_s_p_s_3(const Suffix& se1, const Prefix& pe1,
                              std::string& word,
                              Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se2 = *it;
		if (se2.cross_product == false)
			continue;
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
		To_Root_Unroot_RAII<Suffix> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se2, pe1) &&
			    !cross_valid_inner_outer(word_flags, pe1))
				continue;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check here if needed
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_suffixes_then_prefix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!suffixes.has_continuation_flags() &&
	    !prefixes.has_continuation_flags())
		return {};

	for (auto i1 = suffixes.iterate_suffixes_of(word); i1; ++i1) {
		auto& se1 = *i1;

		// The following check is purely for performance, it
		// does not change correctness.
		if (!suffixes.has_continuation_flag(se1.flag) &&
		    !prefixes.has_continuation_flag(se1.flag))
			continue;

		if (outer_affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = suffixes.iterate_suffixes_of(word); i2; ++i2) {
			auto& se2 = *i2;
			if (se2.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se2))
				continue;
			To_Root_Unroot_RAII<Suffix> yyy(word, se2);
			if (!se2.check_condition(word))
				continue;
			auto ret = strip_2_sfx_pfx_3<FULL_WORD>(
			    se1, se2, word, skip_hidden_homonym);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_sfx_pfx_3(const Suffix& se1, const Suffix& se2,
                                  std::string& word,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe1 = *it;
		if (pe1.cross_product == false)
			continue;
		if (!cross_valid_inner_outer(se2, se1) &&
		    !cross_valid_inner_outer(pe1, se1))
			continue;
		if (affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(se2) != is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe1, se2) &&
			    !cross_valid_inner_outer(word_flags, se2))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe1))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			// needflag check here if needed
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_suffix_then_2_prefixes(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!prefixes.has_continuation_flags())
		return {};

	for (auto i1 = suffixes.iterate_suffixes_of(word); i1; ++i1) {
		auto& se1 = *i1;
		if (se1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = prefixes.iterate_prefixes_of(word); i2; ++i2) {
			auto& pe1 = *i2;

			// The following check is purely for performance, it
			// does not change correctness.
			if (!prefixes.has_continuation_flag(pe1.flag))
				continue;

			if (pe1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe1))
				continue;
			if (is_circumfix(se1) != is_circumfix(pe1))
				continue;
			To_Root_Unroot_RAII<Prefix> yyy(word, pe1);
			if (!pe1.check_condition(word))
				continue;
			auto ret = strip_sfx_2_pfx_3<FULL_WORD>(
			    se1, pe1, word, skip_hidden_homonym);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_sfx_2_pfx_3(const Suffix& se1, const Prefix& pe1,
                                  std::string& word,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		if (is_circumfix(pe2))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe1, se1) &&
			    !cross_valid_inner_outer(word_flags, se1))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_suffix_prefix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!prefixes.has_continuation_flags() &&
	    !suffixes.has_continuation_flags())
		return {};

	for (auto i1 = prefixes.iterate_prefixes_of(word); i1; ++i1) {
		auto& pe1 = *i1;

		// The following check is purely for performance, it
		// does not change correctness.
		if (!prefixes.has_continuation_flag(pe1.flag) &&
		    !suffixes.has_continuation_flag(pe1.flag))
			continue;

		if (pe1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = suffixes.iterate_suffixes_of(word); i2; ++i2) {
			auto& se1 = *i2;
			if (se1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se1))
				continue;
			To_Root_Unroot_RAII<Suffix> yyy(word, se1);
			if (!se1.check_condition(word))
				continue;
			auto ret = strip_p_s_p_3<FULL_WORD>(
			    pe1, se1, word, skip_hidden_homonym);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_p_s_p_3(const Prefix& pe1, const Suffix& se1,
                              std::string& word,
                              Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = prefixes.iterate_prefixes_of(word); it; ++it) {
		auto& pe2 = *it;
		if (pe2.cross_product == false)
			continue;
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
		To_Root_Unroot_RAII<Prefix> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(pe2, se1) &&
			    !cross_valid_inner_outer(word_flags, se1))
				continue;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_prefixes_then_suffix(
    std::string& word, Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!prefixes.has_continuation_flags() &&
	    !suffixes.has_continuation_flags())
		return {};

	for (auto i1 = prefixes.iterate_prefixes_of(word); i1; ++i1) {
		auto& pe1 = *i1;

		// The following check is purely for performance, it
		// does not change correctness.
		if (!prefixes.has_continuation_flag(pe1.flag) &&
		    !suffixes.has_continuation_flag(pe1.flag))
			continue;

		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = prefixes.iterate_prefixes_of(word); i2; ++i2) {
			auto& pe2 = *i2;
			if (pe2.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe2))
				continue;
			To_Root_Unroot_RAII<Prefix> yyy(word, pe2);
			if (!pe2.check_condition(word))
				continue;
			auto ret = strip_2_pfx_sfx_3<FULL_WORD>(
			    pe1, pe2, word, skip_hidden_homonym);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_pfx_sfx_3(const Prefix& pe1, const Prefix& pe2,
                                  std::string& word,
                                  Hidden_Homonym skip_hidden_homonym) const
    -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = suffixes.iterate_suffixes_of(word); it; ++it) {
		auto& se1 = *it;
		if (se1.cross_product == false)
			continue;
		if (!cross_valid_inner_outer(pe2, pe1) &&
		    !cross_valid_inner_outer(se1, pe1))
			continue;
		if (affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(pe2) != is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<Suffix> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto& word_entry : Subrange(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(se1, pe2) &&
			    !cross_valid_inner_outer(word_flags, pe2))
				continue;
			if (!cross_valid_inner_outer(word_flags, se1))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			if (skip_hidden_homonym &&
			    word_flags.contains(HIDDEN_HOMONYM_FLAG))
				continue;
			return {word_entry};
		}
	}

	return {};
}

auto match_compound_pattern(const Compound_Pattern& p, string_view word,
                            size_t i, Compounding_Result first,
                            Compounding_Result second)
{
	if (i < p.begin_end_chars.idx())
		return false;
	if (word.compare(i - p.begin_end_chars.idx(),
	                 p.begin_end_chars.str().size(),
	                 p.begin_end_chars.str()) != 0)
		return false;
	if (p.first_word_flag != 0 &&
	    !first->second.contains(p.first_word_flag))
		return false;
	if (p.second_word_flag != 0 &&
	    !second->second.contains(p.second_word_flag))
		return false;
	if (p.match_first_only_unaffixed_or_zero_affixed &&
	    first.affixed_and_modified)
		return false;
	return true;
}

auto is_compound_forbidden_by_patterns(const vector<Compound_Pattern>& patterns,
                                       string_view word, size_t i,
                                       Compounding_Result first,
                                       Compounding_Result second)
{
	return any_of(begin(patterns), end(patterns), [&](auto& p) {
		return match_compound_pattern(p, word, i, first, second);
	});
}

auto Dict_Base::check_compound(std::string& word,
                               Forceucase allow_bad_forceucase) const
    -> Compounding_Result
{
	auto part = string();

	if (compound_flag || compound_begin_flag || compound_middle_flag ||
	    compound_last_flag) {
		auto ret =
		    check_compound(word, 0, 0, part, allow_bad_forceucase);
		if (ret)
			return ret;
	}
	if (!compound_rules.empty()) {
		auto words_data = vector<const Flag_Set*>();
		return check_compound_with_rules(word, words_data, 0, part,
		                                 allow_bad_forceucase);
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::check_compound(std::string& word, size_t start_pos,
                               size_t num_part, std::string& part,
                               Forceucase allow_bad_forceucase) const
    -> Compounding_Result
{
	size_t min_num_cp = 3;
	if (compound_min_length != 0)
		min_num_cp = compound_min_length;

	auto i = start_pos;
	for (size_t num_cp = 0; num_cp != min_num_cp; ++num_cp) {
		if (i == size(word))
			return {};
		valid_u8_advance_index(word, i);
	}
	auto last_i = size(word);
	for (size_t num_cp = 0; num_cp != min_num_cp; ++num_cp) {
		if (last_i < i)
			return {};
		valid_u8_reverse_index(word, last_i);
	}
	for (; i <= last_i; valid_u8_advance_index(word, i)) {
		auto part1_entry = check_compound_classic<m>(
		    word, start_pos, i, num_part, part, allow_bad_forceucase);

		if (part1_entry)
			return part1_entry;

		part1_entry = check_compound_with_pattern_replacements<m>(
		    word, start_pos, i, num_part, part, allow_bad_forceucase);

		if (part1_entry)
			return part1_entry;
	}
	return {};
}

auto are_three_code_points_equal(string_view word, size_t i) -> bool
{
	auto cp = valid_u8_next_cp(word, i);
	auto prev_cp = valid_u8_prev_cp(word, i);
	if (prev_cp.cp == cp.cp) {
		if (cp.end_i != size(word)) {
			auto next_cp = valid_u8_next_cp(word, cp.end_i);
			if (cp.cp == next_cp.cp)
				return true;
		}
		if (prev_cp.begin_i != 0) {
			auto prev2_cp = valid_u8_prev_cp(word, prev_cp.begin_i);
			if (prev2_cp.cp == cp.cp)
				return true;
		}
	}
	return false;
}

template <Affixing_Mode m>
auto Dict_Base::check_compound_classic(std::string& word, size_t start_pos,
                                       size_t i, size_t num_part,
                                       std::string& part,
                                       Forceucase allow_bad_forceucase) const
    -> Compounding_Result
{
	auto old_num_part = num_part;
	part.assign(word, start_pos, i - start_pos);
	auto part1_entry = check_word_in_compound<m>(part);
	if (!part1_entry)
		return {};
	if (part1_entry->second.contains(forbiddenword_flag))
		return {};
	if (compound_check_triple) {
		if (are_three_code_points_equal(word, i))
			return {};
	}
	if (compound_check_case &&
	    has_uppercase_at_compound_word_boundary(word, i))
		return {};
	num_part += part1_entry.num_words_modifier;
	num_part += compound_root_flag &&
	            part1_entry->second.contains(compound_root_flag);

	part.assign(word, i, word.npos);
	auto part2_entry = check_word_in_compound<AT_COMPOUND_END>(part);
	if (!part2_entry)
		goto try_recursive;
	if (part2_entry->second.contains(forbiddenword_flag))
		goto try_recursive;
	if (is_compound_forbidden_by_patterns(compound_patterns, word, i,
	                                      part1_entry, part2_entry))
		goto try_recursive;
	if (compound_check_duplicate && part1_entry == part2_entry)
		goto try_recursive;
	if (compound_check_rep) {
		part.assign(word, start_pos);
		if (is_rep_similar(part))
			goto try_recursive;
	}
	if (compound_force_uppercase && !allow_bad_forceucase &&
	    part2_entry->second.contains(compound_force_uppercase))
		goto try_recursive;

	old_num_part = num_part;
	num_part += part2_entry.num_words_modifier;
	num_part += compound_root_flag &&
	            part2_entry->second.contains(compound_root_flag);
	if (compound_max_word_count != 0 &&
	    num_part + 1 >= compound_max_word_count) {
		if (compound_syllable_vowels.empty()) // is not Hungarian
			return {}; // end search here, num_part can only go up

		// else, language is Hungarian
		auto num_syllable = count_syllables(word);
		num_syllable += part2_entry.num_syllable_modifier;
		if (num_syllable > compound_syllable_max) {
			num_part = old_num_part;
			goto try_recursive;
		}
	}
	return part1_entry;

try_recursive:
	part2_entry = check_compound<AT_COMPOUND_MIDDLE>(
	    word, i, num_part + 1, part, allow_bad_forceucase);
	if (!part2_entry)
		goto try_simplified_triple;
	if (is_compound_forbidden_by_patterns(compound_patterns, word, i,
	                                      part1_entry, part2_entry))
		goto try_simplified_triple;
	// if (compound_check_duplicate && part1_entry == part2_entry)
	//	goto try_simplified_triple;
	if (compound_check_rep) {
		part.assign(word, start_pos);
		if (is_rep_similar(part))
			goto try_simplified_triple;
		auto& p2word = part2_entry->first;
		if (word.compare(i, p2word.size(), p2word) == 0) {
			// part.assign(word, start_pos,
			//            i - start_pos + p2word.size());
			// The erase() is equivaled as the assign above.
			part.erase(i - start_pos + p2word.size());
			if (is_rep_similar(part))
				goto try_simplified_triple;
		}
	}
	return part1_entry;

try_simplified_triple:
	if (!compound_simplified_triple)
		return {};
	auto prev_cp = valid_u8_prev_cp(word, i);
	if (prev_cp.begin_i == 0)
		return {};
	auto prev2_cp = valid_u8_prev_cp(word, prev_cp.begin_i);
	if (prev_cp.cp != prev2_cp.cp)
		return {};
	auto const enc_cp = U8_Encoded_CP(prev_cp.cp);
	word.insert(i, enc_cp);
	AT_SCOPE_EXIT(word.erase(i, size(enc_cp)));
	part.assign(word, i, word.npos);
	part2_entry = check_word_in_compound<AT_COMPOUND_END>(part);
	if (!part2_entry)
		goto try_simplified_triple_recursive;
	if (part2_entry->second.contains(forbiddenword_flag))
		goto try_simplified_triple_recursive;
	if (is_compound_forbidden_by_patterns(compound_patterns, word, i,
	                                      part1_entry, part2_entry))
		goto try_simplified_triple_recursive;
	if (compound_check_duplicate && part1_entry == part2_entry)
		goto try_simplified_triple_recursive;
	if (compound_check_rep) {
		part.assign(word, start_pos);

		// The added char above should not be checked for rep
		// similarity, instead check the original word.
		part.erase(i - start_pos, size(enc_cp));

		if (is_rep_similar(part))
			goto try_simplified_triple_recursive;
	}
	if (compound_force_uppercase && !allow_bad_forceucase &&
	    part2_entry->second.contains(compound_force_uppercase))
		goto try_simplified_triple_recursive;

	if (compound_max_word_count != 0 &&
	    num_part + 1 >= compound_max_word_count)
		return {};
	return part1_entry;

try_simplified_triple_recursive:
	part2_entry = check_compound<AT_COMPOUND_MIDDLE>(
	    word, i, num_part + 1, part, allow_bad_forceucase);
	if (!part2_entry)
		return {};
	if (is_compound_forbidden_by_patterns(compound_patterns, word, i,
	                                      part1_entry, part2_entry))
		return {};
	// if (compound_check_duplicate && part1_entry == part2_entry)
	//	return {};
	if (compound_check_rep) {
		part.assign(word, start_pos);
		part.erase(i - start_pos, size(enc_cp)); // for the added CP
		if (is_rep_similar(part))
			return {};
		auto& p2word = part2_entry->first;
		if (word.compare(i, p2word.size(), p2word) == 0) {
			part.assign(word, start_pos,
			            i - start_pos + p2word.size());
			part.erase(i - start_pos,
			           size(enc_cp)); // for the added CP
			if (is_rep_similar(part))
				return {};
		}
	}
	return part1_entry;
}

template <Affixing_Mode m>
auto Dict_Base::check_compound_with_pattern_replacements(
    std::string& word, size_t start_pos, size_t i, size_t num_part,
    std::string& part, Forceucase allow_bad_forceucase) const
    -> Compounding_Result
{
	for (auto& p : compound_patterns) {
		if (p.replacement.empty())
			continue;
		if (word.compare(i, p.replacement.size(), p.replacement) != 0)
			continue;

		// at this point p.replacement is substring in word
		word.replace(i, p.replacement.size(), p.begin_end_chars.str());
		i += p.begin_end_chars.idx();
		AT_SCOPE_EXIT({
			i -= p.begin_end_chars.idx();
			word.replace(i, p.begin_end_chars.str().size(),
			             p.replacement);
		});

		part.assign(word, start_pos, i - start_pos);
		auto part1_entry = check_word_in_compound<m>(part);
		if (!part1_entry)
			continue;
		if (part1_entry->second.contains(forbiddenword_flag))
			continue;
		if (p.first_word_flag != 0 &&
		    !part1_entry->second.contains(p.first_word_flag))
			continue;
		if (compound_check_triple) {
			if (are_three_code_points_equal(word, i))
				continue;
		}

		part.assign(word, i, word.npos);
		auto part2_entry =
		    check_word_in_compound<AT_COMPOUND_END>(part);
		if (!part2_entry)
			goto try_recursive;
		if (part2_entry->second.contains(forbiddenword_flag))
			goto try_recursive;
		if (p.second_word_flag != 0 &&
		    !part2_entry->second.contains(p.second_word_flag))
			goto try_recursive;
		if (compound_check_duplicate && part1_entry == part2_entry)
			goto try_recursive;
		if (compound_check_rep) {
			part.assign(word, start_pos);
			part.replace(i - start_pos - p.begin_end_chars.idx(),
			             p.begin_end_chars.str().size(),
			             p.replacement);
			if (is_rep_similar(part))
				goto try_recursive;
		}
		if (compound_force_uppercase && !allow_bad_forceucase &&
		    part2_entry->second.contains(compound_force_uppercase))
			goto try_recursive;

		if (compound_max_word_count != 0 &&
		    num_part + 1 >= compound_max_word_count)
			return {};
		return part1_entry;

	try_recursive:
		part2_entry = check_compound<AT_COMPOUND_MIDDLE>(
		    word, i, num_part + 1, part, allow_bad_forceucase);
		if (!part2_entry)
			goto try_simplified_triple;
		if (p.second_word_flag != 0 &&
		    !part2_entry->second.contains(p.second_word_flag))
			goto try_simplified_triple;
		// if (compound_check_duplicate && part1_entry == part2_entry)
		//	goto try_simplified_triple;
		if (compound_check_rep) {
			part.assign(word, start_pos);
			part.replace(i - start_pos - p.begin_end_chars.idx(),
			             p.begin_end_chars.str().size(),
			             p.replacement);
			if (is_rep_similar(part))
				goto try_simplified_triple;
			auto& p2word = part2_entry->first;
			if (word.compare(i, p2word.size(), p2word) == 0) {
				part.assign(word, start_pos,
				            i - start_pos + p2word.size());
				if (is_rep_similar(part))
					goto try_simplified_triple;
			}
		}
		return part1_entry;

	try_simplified_triple:
		// TODO: check code points, not units
		if (!compound_simplified_triple)
			continue;
		auto prev_cp = valid_u8_prev_cp(word, i);
		if (prev_cp.begin_i == 0)
			continue;
		auto prev2_cp = valid_u8_prev_cp(word, prev_cp.begin_i);
		if (prev_cp.cp != prev2_cp.cp)
			continue;
		auto const enc_cp = U8_Encoded_CP(prev_cp.cp);
		word.insert(i, enc_cp);
		AT_SCOPE_EXIT(word.erase(i, size(enc_cp)));
		part.assign(word, i, word.npos);
		part2_entry = check_word_in_compound<AT_COMPOUND_END>(part);
		if (!part2_entry)
			goto try_simplified_triple_recursive;
		if (part2_entry->second.contains(forbiddenword_flag))
			goto try_simplified_triple_recursive;
		if (p.second_word_flag != 0 &&
		    !part2_entry->second.contains(p.second_word_flag))
			goto try_simplified_triple_recursive;
		if (compound_check_duplicate && part1_entry == part2_entry)
			goto try_simplified_triple_recursive;
		if (compound_check_rep) {
			part.assign(word, start_pos);
			part.erase(i - start_pos,
			           size(enc_cp)); // for the added char
			part.replace(i - start_pos - p.begin_end_chars.idx(),
			             p.begin_end_chars.str().size(),
			             p.replacement);
			if (is_rep_similar(part))
				goto try_simplified_triple_recursive;
		}
		if (compound_force_uppercase && !allow_bad_forceucase &&
		    part2_entry->second.contains(compound_force_uppercase))
			goto try_simplified_triple_recursive;

		if (compound_max_word_count != 0 &&
		    num_part + 1 >= compound_max_word_count)
			return {};
		return part1_entry;

	try_simplified_triple_recursive:
		part2_entry = check_compound<AT_COMPOUND_MIDDLE>(
		    word, i, num_part + 1, part, allow_bad_forceucase);
		if (!part2_entry)
			continue;
		if (p.second_word_flag != 0 &&
		    !part2_entry->second.contains(p.second_word_flag))
			continue;
		// if (compound_check_duplicate && part1_entry == part2_entry)
		//	continue;
		if (compound_check_rep) {
			part.assign(word, start_pos);
			part.erase(i - start_pos,
			           size(enc_cp)); // for the added char
			part.replace(i - start_pos - p.begin_end_chars.idx(),
			             p.begin_end_chars.str().size(),
			             p.replacement);
			if (is_rep_similar(part))
				continue;
			auto& p2word = part2_entry->first;
			if (word.compare(i, p2word.size(), p2word) == 0) {
				part.assign(word, start_pos,
				            i - start_pos + p2word.size());
				part.erase(i - start_pos,
				           size(enc_cp)); // for the added char
				if (is_rep_similar(part))
					continue;
			}
		}
		return part1_entry;
	}
	return {};
}

template <class AffixT>
auto is_modiying_affix(const AffixT& a)
{
	return !a.stripping.empty() || !a.appending.empty();
}

template <Affixing_Mode m>
auto Dict_Base::check_word_in_compound(std::string& word) const
    -> Compounding_Result
{
	auto cpd_flag = char16_t();
	if (m == AT_COMPOUND_BEGIN)
		cpd_flag = compound_begin_flag;
	else if (m == AT_COMPOUND_MIDDLE)
		cpd_flag = compound_middle_flag;
	else if (m == AT_COMPOUND_END)
		cpd_flag = compound_last_flag;

	auto range = words.equal_range(word);
	for (auto& we : Subrange(range)) {
		auto& word_flags = we.second;
		if (word_flags.contains(need_affix_flag))
			continue;
		if (!word_flags.contains(compound_flag) &&
		    !word_flags.contains(cpd_flag))
			continue;
		if (word_flags.contains(HIDDEN_HOMONYM_FLAG))
			continue;
		auto num_syllable_mod = calc_syllable_modifier<m>(we);
		return {&we, 0, num_syllable_mod};
	}
	auto x2 = strip_suffix_only<m>(word, SKIP_HIDDEN_HOMONYM);
	if (x2) {
		auto num_syllable_mod = calc_syllable_modifier<m>(*x2, *x2.a);
		return {x2, 0, num_syllable_mod, is_modiying_affix(*x2.a)};
	}

	auto x1 = strip_prefix_only<m>(word, SKIP_HIDDEN_HOMONYM);
	if (x1) {
		auto num_words_mod = calc_num_words_modifier(*x1.a);
		return {x1, num_words_mod, 0, is_modiying_affix(*x1.a)};
	}

	auto x3 =
	    strip_prefix_then_suffix_commutative<m>(word, SKIP_HIDDEN_HOMONYM);
	if (x3) {
		auto num_words_mod = calc_num_words_modifier(*x3.b);
		auto num_syllable_mod = calc_syllable_modifier<m>(*x3, *x3.a);
		return {x3, num_words_mod, num_syllable_mod,
		        is_modiying_affix(*x3.a) || is_modiying_affix(*x3.b)};
	}
	return {};
}

auto Dict_Base::calc_num_words_modifier(const Prefix& pfx) const
    -> unsigned char
{
	if (compound_syllable_vowels.empty())
		return 0;
	auto c = count_syllables(pfx.appending);
	return c > 1;
}

template <Affixing_Mode m>
auto Dict_Base::calc_syllable_modifier(Word_List::const_reference we) const
    -> signed char
{
	auto subtract_syllable =
	    m == AT_COMPOUND_END && !compound_syllable_vowels.empty() &&
	    we.second.contains('I') && !we.second.contains('J');
	return 0 - subtract_syllable;
}

template <Affixing_Mode m>
auto Dict_Base::calc_syllable_modifier(Word_List::const_reference we,
                                       const Suffix& sfx) const -> signed char
{
	if (m != AT_COMPOUND_END)
		return 0;
	if (compound_syllable_vowels.empty())
		return 0;
	auto& appnd = sfx.appending;
	signed char num_syllable_mod = 0 - count_syllables(appnd);
	auto sfx_extra = !appnd.empty() && appnd.back() == 'i';
	if (sfx_extra && appnd.size() > 1) {
		auto c = appnd[appnd.size() - 2];
		sfx_extra = c != 'y' && c != 't';
	}
	num_syllable_mod -= sfx_extra;

	if (compound_syllable_num) {
		switch (sfx.flag) {
		case 'c':
			num_syllable_mod += 2;
			break;

		case 'J':
			num_syllable_mod += 1;
			break;

		case 'I':
			num_syllable_mod += we.second.contains('J');
			break;
		}
	}
	return num_syllable_mod;
}

auto Dict_Base::count_syllables(std::string_view word) const -> size_t
{
	return count_appereances_of(word, compound_syllable_vowels);
}

auto Dict_Base::check_compound_with_rules(
    std::string& word, std::vector<const Flag_Set*>& words_data,
    size_t start_pos, std::string& part, Forceucase allow_bad_forceucase) const
    -> Compounding_Result
{
	size_t min_num_cp = 3;
	if (compound_min_length != 0)
		min_num_cp = compound_min_length;

	auto i = start_pos;
	for (size_t num_cp = 0; num_cp != min_num_cp; ++num_cp) {
		if (i == size(word))
			return {};
		valid_u8_advance_index(word, i);
	}
	auto last_i = size(word);
	for (size_t num_cp = 0; num_cp != min_num_cp; ++num_cp) {
		if (last_i < i)
			return {};
		valid_u8_reverse_index(word, last_i);
	}
	for (; i <= last_i; valid_u8_advance_index(word, i)) {
		part.assign(word, start_pos, i - start_pos);
		auto part1_entry = Word_List::const_pointer();
		auto range = words.equal_range(part);
		for (auto& we : Subrange(range)) {
			auto& word_flags = we.second;
			if (word_flags.contains(need_affix_flag))
				continue;
			if (!compound_rules.has_any_of_flags(word_flags))
				continue;
			part1_entry = &we;
			break;
		}
		if (!part1_entry)
			continue;
		words_data.push_back(&part1_entry->second);
		AT_SCOPE_EXIT(words_data.pop_back());

		part.assign(word, i, word.npos);
		auto part2_entry = Word_List::const_pointer();
		range = words.equal_range(part);
		for (auto& we : Subrange(range)) {
			auto& word_flags = we.second;
			if (word_flags.contains(need_affix_flag))
				continue;
			if (!compound_rules.has_any_of_flags(word_flags))
				continue;
			part2_entry = &we;
			break;
		}
		if (!part2_entry)
			goto try_recursive;

		{
			words_data.push_back(&part2_entry->second);
			AT_SCOPE_EXIT(words_data.pop_back());

			auto m = compound_rules.match_any_rule(words_data);
			if (!m)
				goto try_recursive;
			if (compound_force_uppercase && !allow_bad_forceucase &&
			    part2_entry->second.contains(
			        compound_force_uppercase))
				goto try_recursive;

			return {part1_entry};
		}
	try_recursive:
		part2_entry = check_compound_with_rules(
		    word, words_data, i, part, allow_bad_forceucase);
		if (part2_entry)
			return {part2_entry};
	}
	return {};
}

auto static insert_sug_first(const string& word, List_Strings& out)
{
	out.insert(begin(out), word);
}

auto& operator|=(Dict_Base::High_Quality_Sugs& lhs,
                 Dict_Base::High_Quality_Sugs rhs)
{
	lhs = Dict_Base::High_Quality_Sugs(lhs || rhs);
	return lhs;
}

auto Dict_Base::suggest_priv(string_view input_word, List_Strings& out) const
    -> void
{
	if (empty(input_word))
		return;
	auto word = string(input_word);
	input_substr_replacer.replace(word);
	auto abbreviation = word.back() == '.';
	if (abbreviation) {
		// trim trailing periods
		auto i = word.find_last_not_of('.');
		// if i == npos, i + 1 == 0, so no need for extra if.
		word.erase(i + 1);
		if (word.empty())
			return;
	}
	auto buffer = string();
	auto casing = classify_casing(word);
	auto hq_sugs = High_Quality_Sugs();
	switch (casing) {
	case Casing::SMALL:
		if (compound_force_uppercase &&
		    check_compound(word, ALLOW_BAD_FORCEUCASE)) {
			to_title(word, icu_locale, buffer);
			out.push_back(buffer);
			return;
		}
		hq_sugs |= suggest_low(word, out);
		break;
	case Casing::INIT_CAPITAL:
		hq_sugs |= suggest_low(word, out);
		to_lower(word, icu_locale, buffer);
		hq_sugs |= suggest_low(buffer, out);
		break;
	case Casing::CAMEL:
	case Casing::PASCAL: {
		hq_sugs |= suggest_low(word, out);
		auto dot_idx = word.find('.');
		if (dot_idx != word.npos) {
			auto after_dot = string_view(word).substr(dot_idx + 1);
			auto casing_after_dot = classify_casing(after_dot);
			if (casing_after_dot == Casing::INIT_CAPITAL) {
				word.insert(dot_idx + 1, 1, ' ');
				insert_sug_first(word, out);
				word.erase(dot_idx + 1, 1);
			}
		}
		if (casing == Casing::PASCAL) {
			buffer = word;
			to_lower_char_at(buffer, 0, icu_locale);
			if (spell_priv(buffer))
				insert_sug_first(buffer, out);
			hq_sugs |= suggest_low(buffer, out);
		}
		to_lower(word, icu_locale, buffer);
		if (spell_priv(buffer))
			insert_sug_first(buffer, out);
		hq_sugs |= suggest_low(buffer, out);
		if (casing == Casing::PASCAL) {
			to_title(word, icu_locale, buffer);
			if (spell_priv(buffer))
				insert_sug_first(buffer, out);
			hq_sugs |= suggest_low(buffer, out);
		}
		for (auto it = begin(out); it != end(out); ++it) {
			auto& sug = *it;
			auto space_idx = sug.find(' ');
			if (space_idx == sug.npos)
				continue;
			auto i = space_idx + 1;
			auto len = sug.size() - i;
			if (len > word.size())
				continue;
			if (sug.compare(i, len, word, word.size() - len) == 0)
				continue;
			to_title_char_at(sug, i, icu_locale);
			rotate(begin(out), it, it + 1);
		}
		break;
	}
	case Casing::ALL_CAPITAL:
		to_lower(word, icu_locale, buffer);
		if (keepcase_flag != 0 && spell_priv(buffer))
			insert_sug_first(buffer, out);
		hq_sugs |= suggest_low(buffer, out);
		to_title(word, icu_locale, buffer);
		hq_sugs |= suggest_low(buffer, out);
		for (auto& sug : out)
			to_upper(sug, icu_locale, sug);
		break;
	}

	if (!hq_sugs && max_ngram_suggestions != 0) {
		if (casing == Casing::SMALL)
			buffer = word;
		else
			to_lower(word, icu_locale, buffer);
		auto old_size = out.size();
		ngram_suggest(buffer, out);
		if (casing == Casing::ALL_CAPITAL) {
			for (auto i = old_size; i != out.size(); ++i)
				to_upper(out[i], icu_locale, out[i]);
		}
	}

	auto has_dash = word.find('-') != word.npos;
	auto has_dash_sug =
	    has_dash && any_of(begin(out), end(out), [](const string& s) {
		    return s.find('-') != s.npos;
	    });
	if (has_dash && !has_dash_sug) {
		auto sugs_tmp = List_Strings();
		auto i = size_t();
		for (;;) {
			auto j = word.find('-', i);
			buffer.assign(word, i, j - i);
			if (!spell_priv(buffer)) {
				suggest_priv(buffer, sugs_tmp);
				for (auto& t : sugs_tmp) {
					buffer = word;
					buffer.replace(i, j - i, t);
					auto flg = check_word(buffer);
					if (!flg ||
					    !flg->contains(forbiddenword_flag))
						out.push_back(buffer);
				}
			}
			if (j == word.npos)
				break;
			i = j + 1;
		}
	}

	if (casing == Casing::INIT_CAPITAL || casing == Casing::PASCAL) {
		for (auto& sug : out)
			to_title_char_at(sug, 0, icu_locale);
	}

	// Suggest with dots can go here but nobody uses it so no point in
	// implementing it.

	if ((casing == Casing::INIT_CAPITAL || casing == Casing::ALL_CAPITAL) &&
	    (keepcase_flag != 0 || forbiddenword_flag != 0)) {
		auto is_ok = [&](string& s) {
			if (s.find(' ') != s.npos)
				return true;
			if (spell_priv(s))
				return true;
			to_lower(s, icu_locale, s);
			if (spell_priv(s))
				return true;
			to_title(s, icu_locale, s);
			return spell_priv(s);
		};
		auto it = begin(out);
		auto last = end(out);
		// Bellow is remove_if(it, last, is_not_ok);
		// We don't use remove_if because is_ok modifies
		// the argument.
		for (; it != last; ++it)
			if (!is_ok(*it))
				break;
		if (it != last) {
			for (auto it2 = it + 1; it2 != last; ++it2)
				if (is_ok(*it2))
					*it++ = move(*it2);
			out.erase(it, last);
		}
	}
	{
		auto it = begin(out);
		auto last = end(out);
		for (; it != last; ++it)
			last = remove(it + 1, last, *it);
		out.erase(last, end(out));
	}
	for (auto& sug : out)
		output_substr_replacer.replace(sug);
}

auto Dict_Base::suggest_low(std::string& word, List_Strings& out) const
    -> High_Quality_Sugs
{
	auto ret = ALL_LOW_QUALITY_SUGS;
	auto old_size = out.size();
	uppercase_suggest(word, out);
	rep_suggest(word, out);
	map_suggest(word, out);
	ret = High_Quality_Sugs(old_size != out.size());
	adjacent_swap_suggest(word, out);
	distant_swap_suggest(word, out);
	keyboard_suggest(word, out);
	extra_char_suggest(word, out);
	forgotten_char_suggest(word, out);
	move_char_suggest(word, out);
	bad_char_suggest(word, out);
	doubled_two_chars_suggest(word, out);
	two_words_suggest(word, out);
	return ret;
}

auto Dict_Base::add_sug_if_correct(std::string& word, List_Strings& out) const
    -> bool
{
	auto res = check_word(word, FORBID_BAD_FORCEUCASE, SKIP_HIDDEN_HOMONYM);
	if (!res)
		return false;
	if (res->contains(forbiddenword_flag))
		return false;
	if (forbid_warn && res->contains(warn_flag))
		return false;
	out.push_back(word);
	return true;
}

auto Dict_Base::uppercase_suggest(const std::string& word,
                                  List_Strings& out) const -> void
{
	auto upp = to_upper(word, icu_locale);
	add_sug_if_correct(upp, out);
}

auto Dict_Base::rep_suggest(std::string& word, List_Strings& out) const

    -> void
{
	auto& reps = replacements;
	for (auto& r : reps.whole_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		if (word == from) {
			word = to;
			try_rep_suggestion(word, out);
			word = from;
		}
	}
	for (auto& r : reps.start_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		if (begins_with(word, from)) {
			word.replace(0, from.size(), to);
			try_rep_suggestion(word, out);
			word.replace(0, to.size(), from);
		}
	}
	for (auto& r : reps.end_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		if (ends_with(word, from)) {
			auto pos = word.size() - from.size();
			word.replace(pos, word.npos, to);
			try_rep_suggestion(word, out);
			word.replace(pos, word.npos, from);
		}
	}
	for (auto& r : reps.any_place_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		for (auto i = word.find(from); i != word.npos;
		     i = word.find(from, i + 1)) {
			word.replace(i, from.size(), to);
			try_rep_suggestion(word, out);
			word.replace(i, to.size(), from);
		}
	}
}

auto Dict_Base::try_rep_suggestion(std::string& word, List_Strings& out) const
    -> void
{
	if (add_sug_if_correct(word, out))
		return;

	auto i = size_t(0);
	auto j = word.find(' ');
	if (j == word.npos)
		return;
	auto part = string();
	for (; j != word.npos; i = j + 1, j = word.find(' ', i)) {
		part.assign(word, i, j - i);
		if (!check_word(part, FORBID_BAD_FORCEUCASE,
		                SKIP_HIDDEN_HOMONYM))
			return;
	}
	out.push_back(word);
}

auto Dict_Base::is_rep_similar(std::string& word) const -> bool
{
	auto& reps = replacements;
	for (auto& r : reps.whole_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		if (word == from) {
			word = to;
			auto ret = check_simple_word(word, SKIP_HIDDEN_HOMONYM);
			word = from;
			if (ret)
				return true;
		}
	}
	for (auto& r : reps.start_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		if (begins_with(word, from)) {
			word.replace(0, from.size(), to);
			auto ret = check_simple_word(word, SKIP_HIDDEN_HOMONYM);
			word.replace(0, to.size(), from);
			if (ret)
				return true;
		}
	}
	for (auto& r : reps.end_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		if (ends_with(word, from)) {
			auto pos = word.size() - from.size();
			word.replace(pos, word.npos, to);
			auto ret = check_simple_word(word, SKIP_HIDDEN_HOMONYM);
			word.replace(pos, word.npos, from);
			if (ret)
				return true;
		}
	}
	for (auto& r : reps.any_place_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		for (auto i = word.find(from); i != word.npos;
		     i = word.find(from, i + 1)) {
			word.replace(i, from.size(), to);
			auto ret = check_simple_word(word, SKIP_HIDDEN_HOMONYM);
			word.replace(i, to.size(), from);
			if (ret)
				return true;
		}
	}
	return false;
}

auto Dict_Base::map_suggest(std::string& word, List_Strings& out,
                            size_t i) const -> void
{
	for (size_t next_i = i; i != size(word); i = next_i) {
		valid_u8_advance_index(word, next_i);
		auto word_cp = U8_Encoded_CP(word, {i, next_i});
		for (auto& e : similarities) {
			auto j = e.chars.find(word_cp);
			if (j == word.npos)
				goto try_find_strings;
			for (size_t k = 0, next_k = 0; k != size(e.chars);
			     k = next_k) {
				valid_u8_advance_index(e.chars, next_k);
				if (k == j)
					continue;
				auto rep_cp =
				    string_view(&e.chars[k], next_k - k);
				word.replace(i, size(word_cp), rep_cp);
				add_sug_if_correct(word, out);
				map_suggest(word, out, i + size(rep_cp));
				word.replace(i, size(rep_cp), word_cp);
			}
			for (auto& r : e.strings) {
				word.replace(i, size(word_cp), r);
				add_sug_if_correct(word, out);
				map_suggest(word, out, i + size(r));
				word.replace(i, size(r), word_cp);
			}
		try_find_strings:
			for (auto& f : e.strings) {
				if (word.compare(i, size(f), f) != 0)
					continue;
				for (size_t k = 0, next_k = 0;
				     k != size(e.chars); k = next_k) {
					valid_u8_advance_index(e.chars, next_k);
					auto rep_cp = string_view(&e.chars[k],
					                          next_k - k);
					word.replace(i, size(f), rep_cp);
					add_sug_if_correct(word, out);
					map_suggest(word, out,
					            i + size(rep_cp));
					word.replace(i, size(rep_cp), f);
				}
				for (auto& r : e.strings) {
					if (f == r)
						continue;
					word.replace(i, size(f), r);
					add_sug_if_correct(word, out);
					map_suggest(word, out, i + size(r));
					word.replace(i, size(r), f);
				}
			}
		}
	}
}

auto Dict_Base::adjacent_swap_suggest(std::string& word,
                                      List_Strings& out) const -> void
{
	if (word.empty())
		return;

	auto i1 = size_t(0);
	auto i2 = valid_u8_next_index(word, i1);
	for (size_t i3 = i2; i3 != size(word); i1 = i2, i2 = i3) {
		valid_u8_advance_index(word, i3);
		i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
		add_sug_if_correct(word, out);
		i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
	}
	i1 = 0;
	i2 = valid_u8_next_index(word, i1);
	if (i2 == size(word))
		return;
	auto i3 = valid_u8_next_index(word, i2);
	if (i3 == size(word))
		return;
	auto i4 = valid_u8_next_index(word, i3);
	if (i4 == size(word))
		return;
	auto i5 = valid_u8_next_index(word, i4);
	if (i5 == size(word)) {
		// word has 4 CPs
		i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
		i4 = u8_swap_adjacent_cp(word, i3, i4, i5);
		add_sug_if_correct(word, out);
		i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
		i4 = u8_swap_adjacent_cp(word, i3, i4, i5);
		return;
	}
	auto i6 = valid_u8_next_index(word, i5);
	if (i6 == size(word)) {
		// word has 5 CPs
		i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
		i5 = u8_swap_adjacent_cp(word, i4, i5, i6);
		add_sug_if_correct(word, out);
		i2 = u8_swap_adjacent_cp(word, i1, i2, i3); // revert first two
		i3 = u8_swap_adjacent_cp(word, i2, i3, i4);
		add_sug_if_correct(word, out);
		i3 = u8_swap_adjacent_cp(word, i2, i3, i4);
		i5 = u8_swap_adjacent_cp(word, i4, i5, i6);
	}
}

auto Dict_Base::distant_swap_suggest(std::string& word, List_Strings& out) const
    -> void
{
	if (empty(word))
		return;
	auto i1 = size_t(0);
	auto i2 = valid_u8_next_index(word, i1);
	for (auto i3 = i2; i3 != size(word); i1 = i2, i2 = i3) {
		valid_u8_advance_index(word, i3);
		for (size_t j = i3, j2 = i3; j != size(word); j = j2) {
			valid_u8_advance_index(word, j2);
			auto [new_i2, new_j] =
			    u8_swap_cp(word, {i1, i2}, {j, j2});
			add_sug_if_correct(word, out);
			u8_swap_cp(word, {i1, new_i2}, {new_j, j2});
		}
	}
}

auto Dict_Base::keyboard_suggest(std::string& word, List_Strings& out) const
    -> void
{
	auto& kb = keyboard_closeness;
	for (size_t j = 0, next_j = 0; j != size(word); j = next_j) {
		char32_t c;
		valid_u8_advance_cp(word, next_j, c);
		auto enc_cp = U8_Encoded_CP(word, {j, next_j});
		auto upp_c = char32_t(u_toupper(c));
		if (upp_c != c) {
			auto enc_upp_c = U8_Encoded_CP(upp_c);
			word.replace(j, size(enc_cp), enc_upp_c);
			add_sug_if_correct(word, out);
			word.replace(j, size(enc_upp_c), enc_cp);
		}
		for (auto i = kb.find(enc_cp); i != kb.npos;
		     i = kb.find(enc_cp, i + size(enc_cp))) {
			if (i != 0 && kb[i - 1] != '|') {
				auto prev_i = valid_u8_prev_index(kb, i);
				auto kb_c = U8_Encoded_CP(kb, {prev_i, i});
				word.replace(j, size(enc_cp), kb_c);
				add_sug_if_correct(word, out);
				word.replace(j, size(kb_c), enc_cp);
			}
			auto next_i = i + size(enc_cp);
			if (next_i != size(kb) && kb[next_i] != '|') {
				auto next2_i = valid_u8_next_index(kb, next_i);
				auto kb_c =
				    U8_Encoded_CP(kb, {next_i, next2_i});
				word.replace(j, size(enc_cp), kb_c);
				add_sug_if_correct(word, out);
				word.replace(j, size(kb_c), enc_cp);
			}
		}
	}
}

auto Dict_Base::extra_char_suggest(std::string& word, List_Strings& out) const
    -> void
{
	for (size_t i = 0, next_i = 0; i != size(word); i = next_i) {
		valid_u8_advance_index(word, next_i);
		auto cp = U8_Encoded_CP(word, {i, next_i});
		word.erase(i, size(cp));
		add_sug_if_correct(word, out);
		word.insert(i, cp);
	}
}

auto Dict_Base::forgotten_char_suggest(std::string& word,
                                       List_Strings& out) const -> void
{
	for (size_t t = 0, next_t = 0; t != size(try_chars); t = next_t) {
		valid_u8_advance_index(try_chars, next_t);
		auto cp = string_view(&try_chars[t], next_t - t);
		for (size_t i = 0;; valid_u8_advance_index(word, i)) {
			word.insert(i, cp);
			add_sug_if_correct(word, out);
			word.erase(i, size(cp));
			if (i == size(word))
				break;
		}
	}
}

auto Dict_Base::move_char_suggest(std::string& word, List_Strings& out) const
    -> void
{
	if (empty(word))
		return;

	auto i1 = size_t(0);
	auto i2 = valid_u8_next_index(word, i1);
	for (auto i3 = i2; i3 != size(word); i1 = i2, i2 = i3) {
		valid_u8_advance_index(word, i3);
		auto new_i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
		for (auto j1 = new_i2, j2 = i3, j3 = i3; j3 != size(word);
		     j1 = j2, j2 = j3) {
			valid_u8_advance_index(word, j3);
			j2 = u8_swap_adjacent_cp(word, j1, j2, j3);
			add_sug_if_correct(word, out);
		}
		rotate(begin(word) + i1, end(word) - (i2 - i1), end(word));
	}

	auto i3 = size(word);
	i2 = valid_u8_prev_index(word, i3);
	for (i1 = i2; i1 != 0; i3 = i2, i2 = i1) {
		valid_u8_reverse_index(word, i1);
		auto new_i2 = u8_swap_adjacent_cp(word, i1, i2, i3);
		for (auto j3 = new_i2, j2 = i1, j1 = i1; j1 != 0;
		     j3 = j2, j2 = j1) {
			valid_u8_reverse_index(word, j1);
			j2 = u8_swap_adjacent_cp(word, j1, j2, j3);
			add_sug_if_correct(word, out);
		}
		rotate(begin(word), begin(word) + (i3 - i2), begin(word) + i3);
	}
}

auto Dict_Base::bad_char_suggest(std::string& word, List_Strings& out) const
    -> void
{
	for (size_t t = 0, next_t = 0; t != size(try_chars); t = next_t) {
		char32_t t_cp;
		valid_u8_advance_cp(try_chars, next_t, t_cp);
		auto t_enc_cp = string_view(&try_chars[t], next_t - t);
		for (size_t i = 0, next_i = 0; i != size(word); i = next_i) {
			char32_t w_cp;
			valid_u8_advance_cp(word, next_i, w_cp);
			auto w_enc_cp = U8_Encoded_CP(word, {i, next_i});
			if (t_cp == w_cp)
				continue;
			word.replace(i, size(w_enc_cp), t_enc_cp);
			add_sug_if_correct(word, out);
			word.replace(i, size(t_enc_cp), w_enc_cp);
		}
	}
}

auto Dict_Base::doubled_two_chars_suggest(std::string& word,
                                          List_Strings& out) const -> void
{
	char32_t cp[5];
	size_t i[5];
	size_t j = 0;
	size_t num_cp = 0;
	for (; j != size(word) && num_cp != 4; ++num_cp) {
		i[num_cp] = j;
		valid_u8_advance_cp(word, j, cp[num_cp]);
	}
	if (num_cp != 4) // Not really needed. Makes static analysis happy.
		return;
	while (j != size(word)) {
		i[4] = j;
		valid_u8_advance_cp(word, j, cp[4]);
		if (cp[0] == cp[2] && cp[1] == cp[3] && cp[0] == cp[4]) {
			word.erase(i[3], j - i[3]);
			add_sug_if_correct(word, out);
			word.insert(i[3], word, i[1], i[3] - i[1]);
		}
		copy(begin(i) + 1, end(i), begin(i));
		copy(begin(cp) + 1, end(cp), begin(cp));
	}
}

auto Dict_Base::two_words_suggest(const std::string& word,
                                  List_Strings& out) const -> void
{
	if (empty(word))
		return;

	auto w1_num_cp = size_t(0);
	auto word1 = string();
	auto word2 = string();
	for (size_t i = 0, next_i = 0;; i = next_i, ++w1_num_cp) {
		valid_u8_advance_index(word, next_i);
		if (next_i == size(word))
			break;
		word1.append(word, i, next_i - i);
		// TODO: maybe switch to check_word()
		auto w1 = check_simple_word(word1, SKIP_HIDDEN_HOMONYM);
		if (!w1)
			continue;
		word2.assign(word, next_i);
		auto w2 = check_simple_word(word2, SKIP_HIDDEN_HOMONYM);
		if (!w2)
			continue;
		word1 += ' ';
		word1 += word2;
		if (find(begin(out), end(out), word1) == end(out))
			out.push_back(word1);
		auto w2_more_than_1_cp =
		    valid_u8_next_index(word2, 0) != size(word2);
		if (w1_num_cp > 1 && w2_more_than_1_cp && !empty(try_chars) &&
		    (try_chars.find('a') != try_chars.npos ||
		     try_chars.find('-') != try_chars.npos)) {
			word1[next_i] = '-';
			if (find(begin(out), end(out), word1) == end(out))
				out.push_back(word1);
		}
		word1.erase(next_i);
	}
}

namespace {
auto ngram_similarity_low_level(size_t n, u32string_view a, u32string_view b)
    -> ptrdiff_t
{
	auto score = ptrdiff_t(0);
	n = min(n, a.size());
	for (size_t k = 1; k != n + 1; ++k) {
		auto k_score = ptrdiff_t(0);
		for (size_t i = 0; i != a.size() - k + 1; ++i) {
			auto kgram = a.substr(i, k);
			auto find = b.find(kgram);
			if (find != b.npos)
				++k_score;
		}
		score += k_score;
		if (k_score < 2)
			break;
	}
	return score;
}
auto ngram_similarity_weighted_low_level(size_t n, u32string_view a,
                                         u32string_view b) -> ptrdiff_t
{
	auto score = ptrdiff_t(0);
	n = min(n, a.size());
	for (size_t k = 1; k != n + 1; ++k) {
		auto k_score = ptrdiff_t(0);
		for (size_t i = 0; i != a.size() - k + 1; ++i) {
			auto kgram = a.substr(i, k);
			auto find = b.find(kgram);
			if (find != b.npos) {
				++k_score;
			}
			else {
				--k_score;
				if (i == 0 || i == a.size() - k)
					--k_score;
			}
		}
		score += k_score;
	}
	return score;
}

auto ngram_similarity_longer_worse(size_t n, u32string_view a, u32string_view b)
    -> ptrdiff_t
{
	if (b.empty())
		return 0;
	auto score = ngram_similarity_low_level(n, a, b);
	auto d = ptrdiff_t(b.size() - a.size()) - 2;
	if (d > 0)
		score -= d;
	return score;
}
auto ngram_similarity_any_mismatch(size_t n, u32string_view a, u32string_view b)
    -> ptrdiff_t
{
	if (b.empty())
		return 0;
	auto score = ngram_similarity_low_level(n, a, b);
	auto d = abs(ptrdiff_t(b.size() - a.size())) - 2;
	if (d > 0)
		score -= d;
	return score;
}
auto ngram_similarity_any_mismatch_weighted(size_t n, u32string_view a,
                                            u32string_view b) -> ptrdiff_t
{
	if (b.empty())
		return 0;
	auto score = ngram_similarity_weighted_low_level(n, a, b);
	auto d = abs(ptrdiff_t(b.size() - a.size())) - 2;
	if (d > 0)
		score -= d;
	return score;
}

auto left_common_substring_length(u32string_view a, u32string_view b)
    -> ptrdiff_t
{
	if (a.empty() || b.empty())
		return 0;
	if (a[0] != b[0] && UChar32(a[0]) != u_tolower(b[0]))
		return 0;
	auto it = std::mismatch(begin(a) + 1, end(a), begin(b) + 1, end(b));
	return it.first - begin(a);
}
auto longest_common_subsequence_length(u32string_view a, u32string_view b,
                                       vector<size_t>& state_buffer)
    -> ptrdiff_t
{
	state_buffer.assign(b.size(), 0);
	auto row1_prev = size_t(0);
	for (size_t i = 0; i != a.size(); ++i) {
		row1_prev = size_t(0);
		auto row2_prev = size_t(0);
		for (size_t j = 0; j != b.size(); ++j) {
			auto row1_current = state_buffer[j];
			auto& row2_current = state_buffer[j];
			if (a[i] == b[j])
				row2_current = row1_prev + 1;
			else
				row2_current = max(row1_current, row2_prev);
			row1_prev = row1_current;
			row2_prev = row2_current;
		}
		row1_prev = row2_prev;
	}
	return ptrdiff_t(row1_prev);
}
struct Count_Eq_Chars_At_Same_Pos_Result {
	ptrdiff_t num;
	bool is_swap;
};
auto count_eq_chars_at_same_pos(u32string_view a, u32string_view b)
    -> Count_Eq_Chars_At_Same_Pos_Result
{
	auto n = min(a.size(), b.size());
	auto count = size_t();
	for (size_t i = 0; i != n; ++i) {
		if (a[i] == b[i])
			++count;
	}
	auto is_swap = false;
	if (a.size() == b.size() && n - count == 2) {
		auto miss1 = mismatch(begin(a), end(a), begin(b));
		auto miss2 =
		    mismatch(miss1.first + 1, end(a), miss1.second + 1);
		is_swap = *miss1.first == *miss2.second &&
		          *miss1.second == *miss2.first;
	}
	return {ptrdiff_t(count), is_swap};
}
struct Word_Entry_And_Score {
	Word_List::const_pointer word_entry = {};
	ptrdiff_t score = {};
	[[maybe_unused]] auto operator<(const Word_Entry_And_Score& rhs) const
	{
		return score > rhs.score; // Greater than
	}
};
struct Word_And_Score {
	u32string word = {};
	ptrdiff_t score = {};
	[[maybe_unused]] auto operator<(const Word_And_Score& rhs) const
	{
		return score > rhs.score; // Greater than
	}
};
} // namespace

auto Dict_Base::ngram_suggest(const std::string& word_u8,
                              List_Strings& out) const -> void
{
	auto const wrong_word = valid_utf8_to_32(word_u8);
	auto wide_buf = u32string();
	auto roots = vector<Word_Entry_And_Score>();
	auto dict_word = u32string();
	for (size_t bucket = 0; bucket != words.bucket_count(); ++bucket) {
		for (auto& word_entry : words.bucket_data(bucket)) {
			auto& [dict_word_u8, flags] = word_entry;
			if (flags.contains(forbiddenword_flag) ||
			    flags.contains(HIDDEN_HOMONYM_FLAG) ||
			    flags.contains(nosuggest_flag) ||
			    flags.contains(compound_onlyin_flag))
				continue;
			valid_utf8_to_32(dict_word_u8, dict_word);
			auto score =
			    left_common_substring_length(wrong_word, dict_word);
			auto& lower_dict_word = wide_buf;
			to_lower(dict_word, icu_locale, lower_dict_word);
			score += ngram_similarity_longer_worse(3, wrong_word,
			                                       lower_dict_word);
			if (roots.size() != 100) {
				roots.push_back({&word_entry, score});
				push_heap(begin(roots), end(roots));
			}
			else if (score > roots.front().score) {
				pop_heap(begin(roots), end(roots));
				roots.back() = {&word_entry, score};
				push_heap(begin(roots), end(roots));
			}
		}
	}

	auto threshold = ptrdiff_t();
	for (auto k : {1u, 2u, 3u}) {
		auto& mangled_wrong_word = wide_buf;
		mangled_wrong_word = wrong_word;
		for (size_t i = k; i < mangled_wrong_word.size(); i += 4)
			mangled_wrong_word[i] = '*';
		threshold += ngram_similarity_any_mismatch(
		    wrong_word.size(), wrong_word, mangled_wrong_word);
	}
	threshold /= 3;

	auto expanded_list = List_Strings();
	auto expanded_cross_afx = vector<bool>();
	auto expanded_word = u32string();
	auto guess_words = vector<Word_And_Score>();
	for (auto& root : roots) {
		expand_root_word_for_ngram(*root.word_entry, word_u8,
		                           expanded_list, expanded_cross_afx);
		for (auto& expanded_word_u8 : expanded_list) {
			valid_utf8_to_32(expanded_word_u8, expanded_word);
			auto score = left_common_substring_length(
			    wrong_word, expanded_word);
			auto& lower_expanded_word = wide_buf;
			to_lower(expanded_word, icu_locale,
			         lower_expanded_word);
			score += ngram_similarity_any_mismatch(
			    wrong_word.size(), wrong_word, lower_expanded_word);
			if (score < threshold)
				continue;

			if (guess_words.size() != 200) {
				guess_words.push_back(
				    {move(expanded_word), score});
				push_heap(begin(guess_words), end(guess_words));
			}
			else if (score > guess_words.front().score) {
				pop_heap(begin(guess_words), end(guess_words));
				guess_words.back() = {move(expanded_word),
				                      score};
				push_heap(begin(guess_words), end(guess_words));
			}
		}
	}
	sort_heap(begin(guess_words), end(guess_words)); // is this needed?

	auto lcs_state = vector<size_t>();
	for (auto& [guess_word, score] : guess_words) {
		auto& lower_guess_word = wide_buf;
		to_lower(guess_word, icu_locale, lower_guess_word);
		auto lcs = longest_common_subsequence_length(
		    wrong_word, lower_guess_word, lcs_state);

		if (wrong_word.size() == lower_guess_word.size() &&
		    wrong_word.size() == size_t(lcs)) {
			score += 2000;
			break;
		}

		auto ngram2 = ngram_similarity_any_mismatch_weighted(
		    2, wrong_word, lower_guess_word);
		ngram2 += ngram_similarity_any_mismatch_weighted(
		    2, lower_guess_word, wrong_word);
		auto ngram4 = ngram_similarity_any_mismatch(4, wrong_word,
		                                            lower_guess_word);
		auto left_common =
		    left_common_substring_length(wrong_word, lower_guess_word);
		auto num_eq_chars_same_pos =
		    count_eq_chars_at_same_pos(wrong_word, lower_guess_word);

		score = 2 * lcs;
		score -=
		    abs(ptrdiff_t(wrong_word.size() - lower_guess_word.size()));
		score += left_common + ngram2 + ngram4;
		if (num_eq_chars_same_pos.num != 0)
			score += 1;
		if (num_eq_chars_same_pos.is_swap)
			score += 10;
		if (5 * ngram2 <
		    ptrdiff_t(wrong_word.size() + lower_guess_word.size()) *
		        (10 - max_diff_factor))
			score -= 1000;
	}

	sort(begin(guess_words), end(guess_words));

	auto more_selective =
	    !guess_words.empty() && guess_words.front().score > 1000;
	auto old_num_sugs = out.size();
	auto max_sug =
	    min(MAX_SUGGESTIONS, old_num_sugs + max_ngram_suggestions);
	for (auto& [guess_word, score] : guess_words) {
		if (out.size() == max_sug)
			break;
		if (more_selective && score <= 1000)
			break;
		if (score < -100 &&
		    (old_num_sugs != out.size() || only_max_diff))
			break;
		auto guess_word_u8 = utf32_to_utf8(guess_word);
		if (any_of(begin(out), end(out),
		           [&g = guess_word_u8](auto& sug) {
			           return g.find(sug) != g.npos;
		           })) {
			if (score < -100)
				break;
			else
				continue;
		}
		out.push_back(move(guess_word_u8));
	}
}

auto Dict_Base::expand_root_word_for_ngram(
    Word_List::const_reference root_entry, std::string_view wrong,
    List_Strings& expanded_list, std::vector<bool>& cross_affix) const -> void
{
	expanded_list.clear();
	cross_affix.clear();
	auto& [root, flags] = root_entry;
	if (!flags.contains(need_affix_flag)) {
		expanded_list.push_back(root);
		cross_affix.push_back(false);
	}
	if (flags.empty())
		return;
	for (auto& suffix : suffixes) {
		if (!cross_valid_inner_outer(flags, suffix))
			continue;
		if (outer_affix_NOT_valid<FULL_WORD>(suffix))
			continue;
		if (is_circumfix(suffix))
			continue;
		// TODO Suffixes marked with needaffix or circumfix should not
		// be just skipped as we can later add prefix. This is not
		// handled in hunspell, too.
		if (!ends_with(root, suffix.stripping))
			continue;
		if (!suffix.check_condition(root))
			continue;

		if (!suffix.appending.empty() &&
		    !ends_with(wrong, suffix.appending))
			continue;

		auto expanded = suffix.to_derived_copy(root);
		expanded_list.push_back(move(expanded));
		cross_affix.push_back(suffix.cross_product);
	}

	for (size_t i = 0, n = expanded_list.size(); i != n; ++i) {
		if (!cross_affix[i])
			continue;

		for (auto& prefix : prefixes) {
			auto& root_sfx = expanded_list[i];
			if (!cross_valid_inner_outer(flags, prefix))
				continue;
			if (outer_affix_NOT_valid<FULL_WORD>(prefix))
				continue;
			if (is_circumfix(prefix))
				continue;
			if (!begins_with(root_sfx, prefix.stripping))
				continue;
			if (!prefix.check_condition(root_sfx))
				continue;

			if (!prefix.appending.empty() &&
			    !begins_with(wrong, prefix.appending))
				continue;

			auto expanded = prefix.to_derived_copy(root_sfx);
			expanded_list.push_back(move(expanded));
		}
	}

	for (auto& prefix : prefixes) {
		if (!cross_valid_inner_outer(flags, prefix))
			continue;
		if (outer_affix_NOT_valid<FULL_WORD>(prefix))
			continue;
		if (is_circumfix(prefix))
			continue;
		if (!begins_with(root, prefix.stripping))
			continue;
		if (!prefix.check_condition(root))
			continue;

		if (!prefix.appending.empty() &&
		    !begins_with(wrong, prefix.appending))
			continue;

		auto expanded = prefix.to_derived_copy(root);
		expanded_list.push_back(move(expanded));
	}
}

Dictionary::Dictionary(std::istream& aff, std::istream& dic)
{
	if (!parse_aff_dic(aff, dic))
		throw Dictionary_Loading_Error("error parsing");
}

Dictionary::Dictionary() = default;

/**
 * @brief Create a dictionary from opened files as iostreams
 *
 * Prefer using load_from_path(). Use this if you have a specific use case,
 * like when .aff and .dic are in-memory buffers istringstream.
 *
 * @param aff The iostream of the .aff file
 * @param dic The iostream of the .dic file
 * @return Dictionary object
 * @throws Dictionary_Loading_Error on error
 */
auto Dictionary::load_from_aff_dic(std::istream& aff, std::istream& dic)
    -> Dictionary
{
	return Dictionary(aff, dic);
}

/**
 * @brief Create a dictionary from files
 * @param file_path_without_extension path *without* extensions (without .dic or
 * .aff)
 * @return Dictionary object
 * @throws Dictionary_Loading_Error on error
 */
auto Dictionary::load_from_path(const std::string& file_path_without_extension)
    -> Dictionary
{
	auto path = file_path_without_extension;
	path += ".aff";
	std::ifstream aff_file(path);
	if (aff_file.fail()) {
		auto err = "Aff file " + path + " not found";
		throw Dictionary_Loading_Error(err);
	}
	path.replace(path.size() - 3, 3, "dic");
	std::ifstream dic_file(path);
	if (dic_file.fail()) {
		auto err = "Dic file " + path + " not found";
		throw Dictionary_Loading_Error(err);
	}
	return load_from_aff_dic(aff_file, dic_file);
}

/**
 * @brief Checks if a given word is correct
 * @param word any word
 * @return true if correct, false otherwise
 */
auto Dictionary::spell(std::string_view word) const -> bool
{
	auto ok_enc = validate_utf8(word);
	if (unlikely(word.size() > 360))
		return false;
	if (unlikely(!ok_enc))
		return false;
	auto word_buf = string(word);
	return spell_priv(word_buf);
}

/**
 * @brief Suggests correct words for a given incorrect word
 * @param[in] word incorrect word
 * @param[out] out this object will be populated with the suggestions
 */
auto Dictionary::suggest(std::string_view word,
                         std::vector<std::string>& out) const -> void
{
	out.clear();
	auto ok_enc = validate_utf8(word);
	if (unlikely(word.size() > 360))
		return;
	if (unlikely(!ok_enc))
		return;
	suggest_priv(word, out);
}
} // namespace v5
} // namespace nuspell
