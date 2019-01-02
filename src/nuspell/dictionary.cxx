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

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/locale.hpp>

namespace nuspell {

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

using namespace std;
using boost::make_iterator_range;
using boost::locale::to_lower;
using boost::locale::to_title;

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
	At_Scope_Exit<decltype(lname)> aname(lname);

#define ASE_INTERNAL2(ctr, ...)                                                \
	ASE_INTERNAL1(MACRO_CONCAT(Auto_func_, ctr),                           \
	              MACRO_CONCAT(Auto_instance_, ctr), __VA_ARGS__)

#define AT_SCOPE_EXIT(...) ASE_INTERNAL2(__COUNTER__, __VA_ARGS__)

/**
 * @brief Check spelling for a word.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
auto Dict_Base::spell_priv(std::wstring& s) const -> bool
{
	// do input conversion (iconv)
	input_substr_replacer.replace(s);

	// triming whitespace should be part of tokenization, not here
	// boost::trim(s, locale_aff);
	if (s.empty())
		return true;
	bool abbreviation = s.back() == '.';
	if (abbreviation) {
		// trim trailing periods
		auto i = s.find_last_not_of('.');
		// if i == npos, i + 1 == 0, so no need for extra if.
		s.erase(i + 1);
		if (s.empty())
			return true;
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

/**
 * @brief Checks recursively the spelling according to break patterns.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
auto Dict_Base::spell_break(std::wstring& s, size_t depth) const -> bool
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
		if (s.compare(0, pat.size(), pat) == 0) {
			auto substr = s.substr(pat.size());
			auto res = spell_break(substr);
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
			auto res = spell_break(substr);
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

/**
 * @brief Checks spelling according to casing of the provided word.
 *
 * @param s string to check spelling for.
 * @return The spelling result.
 */
auto Dict_Base::spell_casing(std::wstring& s) const -> const Flag_Set*
{
	auto casing_type = classify_casing(s, internal_locale);
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

/**
 * @brief Checks spelling for a word which is in all upper case.
 *
 * @param s string to check spelling for.
 * @return The flags of the corresponding dictionary word.
 */
auto Dict_Base::spell_casing_upper(std::wstring& s) const -> const Flag_Set*
{
	auto& loc = internal_locale;

	auto res = check_word(s);
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
		res = check_word(t);
		if (res)
			return res;
		part1 = to_title(part1, loc);
		t = part1 + part2;
		res = check_word(t);
		if (res)
			return res;
	}

	// handle sharp s for German
	if (checksharps && s.find(L"SS") != s.npos) {
		auto t = to_lower(s, loc);
		res = spell_sharps(t);
		if (!res)
			t = to_title(s, loc);
		res = spell_sharps(t);
		if (res)
			return res;
	}
	auto t = to_title(s, loc);
	res = check_word(t);
	if (res && !res->contains(keepcase_flag))
		return res;

	t = to_lower(s, loc);
	res = check_word(t);
	if (res && !res->contains(keepcase_flag))
		return res;
	return nullptr;
}

/**
 * @brief Checks spelling for a word which is in title casing.
 *
 * @param s string to check spelling for.
 * @return The flags of the corresponding dictionary word.
 */
auto Dict_Base::spell_casing_title(std::wstring& s) const -> const Flag_Set*
{
	auto& loc = internal_locale;

	// check title case
	auto res = check_word(s);

	// forbid bad capitalization
	if (res && res->contains(forbiddenword_flag))
		return nullptr;
	if (res)
		return res;

	// attempt checking lower case spelling
	auto t = to_lower(s, loc);
	res = check_word(t);

	// with CHECKSHARPS, ß is allowed too in KEEPCASE words with title case
	if (res && res->contains(keepcase_flag) &&
	    !(checksharps && (t.find(L'\xDF') != t.npos))) {
		res = nullptr;
	}
	return res;
}

/**
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
auto Dict_Base::spell_sharps(std::wstring& base, size_t pos, size_t n,
                             size_t rep) const -> const Flag_Set*
{
	const size_t MAX_SHARPS = 5;
	pos = base.find(L"ss", pos);
	if (pos != std::string::npos && n < MAX_SHARPS) {
		base[pos] = L'\xDF'; // ß
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
		return check_word(base);
	}
	return nullptr;
}

/**
 * @brief Low-level spell-cheking.
 *
 * Checks spelling for various unaffixed versions of the provided word.
 * Unaffixing is done by combinations of zero or more unsuffixing and
 * unprefixing operations.
 *
 * @param s string to check spelling for.
 * @return The flags of the corresponding dictionary word.
 */
auto Dict_Base::check_word(std::wstring& s) const -> const Flag_Set*
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
		auto ret3 = strip_suffix_only(s);
		if (ret3)
			return &ret3->second;
	}
	{
		auto ret2 = strip_prefix_only(s);
		if (ret2)
			return &ret2->second;
	}
	{
		auto ret4 = strip_prefix_then_suffix_commutative(s);
		if (ret4)
			return &ret4->second;
	}
	if (!complex_prefixes) {
		auto ret6 = strip_suffix_then_suffix(s);
		if (ret6)
			return &ret6->second;

		auto ret7 = strip_prefix_then_2_suffixes(s);
		if (ret7)
			return &ret7->second;

		auto ret8 = strip_suffix_prefix_suffix(s);
		if (ret8)
			return &ret8->second;

		// this is slow and unused so comment
		// auto ret9 = strip_2_suffixes_then_prefix(s);
		// if (ret9)
		//	return &ret9->second;
	}
	else {
		auto ret6 = strip_prefix_then_prefix(s);
		if (ret6)
			return &ret6->second;
		auto ret7 = strip_suffix_then_2_prefixes(s);
		if (ret7)
			return &ret7->second;

		auto ret8 = strip_prefix_suffix_prefix(s);
		if (ret8)
			return &ret8->second;

		// this is slow and unused so comment
		// auto ret9 = strip_2_prefixes_then_suffix(s);
		// if (ret9)
		//	return &ret9->second;
	}
	auto ret10 = check_compound(s);
	if (ret10)
		return &ret10->second;

	return nullptr;
}

template <class AffixT>
class To_Root_Unroot_RAII {
      private:
	using value_type = typename AffixT::value_type;
	basic_string<value_type>& word;
	const AffixT& affix;

      public:
	To_Root_Unroot_RAII(basic_string<value_type>& w, const AffixT& a)
	    : word(w), affix(a)
	{
		affix.to_root(word);
	}
	~To_Root_Unroot_RAII() { affix.to_derived(word); }
};

template <Affixing_Mode m>
auto Dict_Base::affix_NOT_valid(const Prefix<wchar_t>& e) const
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
auto Dict_Base::affix_NOT_valid(const Suffix<wchar_t>& e) const
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

auto prefix(const wstring& word, size_t len)
{
	return my_string_view<wchar_t>(word).substr(0, len);
}
auto prefix(wstring&& word, size_t len) = delete;
auto suffix(const wstring& word, size_t len)
{
	return my_string_view<wchar_t>(word).substr(word.size() - len);
}
auto suffix(wstring&& word, size_t len) = delete;

/**
 * @brief Iterator of prefix entres that match a word.
 *
 * Iterates all prefix entries where the appending member is prefix of a given
 * word.
 */
class Prefix_Iter {
	const Prefix_Table<wchar_t>& tbl;
	const wstring& word;
	size_t len;
	my_string_view<wchar_t> pfx;
	using iter = typename Prefix_Table<wchar_t>::iterator;
	iter a;
	iter b;
	bool valid;

      public:
	Prefix_Iter(const Prefix_Table<wchar_t>& tbl, const wstring& word)
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
 * Iterates all suffix entries where the appending member is suffix of a given
 * word.
 */
class Suffix_Iter {
	const Suffix_Table<wchar_t>& tbl;
	const wstring& word;
	size_t len;
	my_string_view<wchar_t> pfx;
	using iter = typename Suffix_Table<wchar_t>::iterator;
	iter a;
	iter b;
	bool valid;

      public:
	Suffix_Iter(const Suffix_Table<wchar_t>& tbl, const wstring& word)
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

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_only(std::wstring& word) const
    -> Affixing_Result<Prefix<wchar_t>>
{
	auto& dic = words;

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& e = *it;
		if (outer_affix_NOT_valid<m>(e))
			continue;
		if (is_circumfix(e))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, e);
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
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(e.cont_flags))
				continue;
			return {word_entry, e};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_suffix_only(std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>>
{
	auto& dic = words;

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
		auto& e = *it;
		if (outer_affix_NOT_valid<m>(e))
			continue;
		if (it.aff_len() != 0 && m == AT_COMPOUND_END &&
		    e.cont_flags.contains(compound_onlyin_flag))
			continue;
		if (is_circumfix(e))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, e);
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
			if (!is_valid_inside_compound<m>(word_flags) &&
			    !is_valid_inside_compound<m>(e.cont_flags))
				continue;
			return {word_entry, e};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_suffix(std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>
{
	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		auto ret = strip_pfx_then_sfx_2<m>(pe, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_then_sfx_2(const Prefix<wchar_t>& pe,
                                     std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>
{
	auto& dic = words;

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(se))
			continue;
		if (is_circumfix(pe) != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se);
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
auto Dict_Base::strip_suffix_then_prefix(std::wstring& word) const
    -> Affixing_Result<Prefix<wchar_t>, Suffix<wchar_t>>
{
	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
		auto& se = *it;
		if (se.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		auto ret = strip_sfx_then_pfx_2<m>(se, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_sfx_then_pfx_2(const Suffix<wchar_t>& se,
                                     std::wstring& word) const
    -> Affixing_Result<Prefix<wchar_t>, Suffix<wchar_t>>
{
	auto& dic = words;

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(pe))
			continue;
		if (is_circumfix(pe) != is_circumfix(se))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe);
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
auto Dict_Base::strip_prefix_then_suffix_commutative(std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>
{
	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& pe = *it;
		if (pe.cross_product == false)
			continue;
		if (affix_NOT_valid<m>(pe))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe);
		if (!pe.check_condition(word))
			continue;
		auto ret = strip_pfx_then_sfx_comm_2<m>(pe, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_then_sfx_comm_2(const Prefix<wchar_t>& pe,
                                          std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>
{
	auto& dic = words;
	auto has_needaffix_pe = pe.cont_flags.contains(need_affix_flag);
	auto is_circumfix_pe = is_circumfix(pe);

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
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
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se);
		if (!se.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
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
auto Dict_Base::strip_suffix_then_suffix(std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>, Suffix<wchar_t>>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!suffixes.has_continuation_flags())
		return {};

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
		auto& se1 = *it;

		// The following check is purely for performance, it does not
		// change correctness.
		if (!suffixes.has_continuation_flag(se1.flag))
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		auto ret = strip_sfx_then_sfx_2<FULL_WORD>(se1, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_sfx_then_sfx_2(const Suffix<wchar_t>& se1,
                                     std::wstring& word) const
    -> Affixing_Result<Suffix<wchar_t>, Suffix<wchar_t>>
{

	auto& dic = words;

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		if (is_circumfix(se2))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se2);
		if (!se2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, se2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {word_entry, se2, se1};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_prefix(std::wstring& word) const
    -> Affixing_Result<Prefix<wchar_t>, Prefix<wchar_t>>
{
	// The following check is purely for performance, it does not change
	// correctness.
	if (!prefixes.has_continuation_flags())
		return {};

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& pe1 = *it;
		// The following check is purely for performance, it does not
		// change correctness.
		if (!prefixes.has_continuation_flag(pe1.flag))
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		auto ret = strip_pfx_then_pfx_2<FULL_WORD>(pe1, word);
		if (ret)
			return ret;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_then_pfx_2(const Prefix<wchar_t>& pe1,
                                     std::wstring& word) const
    -> Affixing_Result<Prefix<wchar_t>, Prefix<wchar_t>>
{
	auto& dic = words;

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		if (is_circumfix(pe2))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe2);
		if (!pe2.check_condition(word))
			continue;
		for (auto& word_entry :
		     make_iterator_range(dic.equal_range(word))) {
			auto& word_flags = word_entry.second;
			if (!cross_valid_inner_outer(word_flags, pe2))
				continue;
			// badflag check
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {word_entry, pe2, pe1};
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_then_2_suffixes(std::wstring& word) const
    -> Affixing_Result<>
{
	if (!suffixes.has_continuation_flags())
		return {};

	for (auto i1 = Prefix_Iter(prefixes, word); i1; ++i1) {
		auto& pe1 = *i1;
		if (pe1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = Suffix_Iter(suffixes, word); i2; ++i2) {
			auto& se1 = *i2;
			if (se1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se1))
				continue;
			if (is_circumfix(pe1) != is_circumfix(se1))
				continue;
			To_Root_Unroot_RAII<Suffix<wchar_t>> yyy(word, se1);
			if (!se1.check_condition(word))
				continue;
			auto ret = strip_pfx_2_sfx_3<FULL_WORD>(pe1, se1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_pfx_2_sfx_3(const Prefix<wchar_t>& pe1,
                                  const Suffix<wchar_t>& se1,
                                  std::wstring& word) const -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
		auto& se2 = *it;
		if (!cross_valid_inner_outer(se2, se1))
			continue;
		if (affix_NOT_valid<m>(se2))
			continue;
		if (is_circumfix(se2))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se2);
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
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_suffix_prefix_suffix(std::wstring& word) const
    -> Affixing_Result<>
{
	if (!suffixes.has_continuation_flags() &&
	    !prefixes.has_continuation_flags())
		return {};

	for (auto i1 = Suffix_Iter(suffixes, word); i1; ++i1) {
		auto& se1 = *i1;
		if (se1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = Prefix_Iter(prefixes, word); i2; ++i2) {
			auto& pe1 = *i2;
			if (pe1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe1))
				continue;
			To_Root_Unroot_RAII<Prefix<wchar_t>> yyy(word, pe1);
			if (!pe1.check_condition(word))
				continue;
			auto ret = strip_s_p_s_3<FULL_WORD>(se1, pe1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_s_p_s_3(const Suffix<wchar_t>& se1,
                              const Prefix<wchar_t>& pe1,
                              std::wstring& word) const -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
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
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se2);
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
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_suffixes_then_prefix(std::wstring& word) const
    -> Affixing_Result<>
{
	if (!suffixes.has_continuation_flags() &&
	    !prefixes.has_continuation_flags())
		return {};

	for (auto i1 = Suffix_Iter(suffixes, word); i1; ++i1) {
		auto& se1 = *i1;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		if (is_circumfix(se1))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = Suffix_Iter(suffixes, word); i2; ++i2) {
			auto& se2 = *i2;
			if (se2.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se2))
				continue;
			To_Root_Unroot_RAII<Suffix<wchar_t>> yyy(word, se2);
			if (!se2.check_condition(word))
				continue;
			auto ret = strip_2_sfx_pfx_3<FULL_WORD>(se1, se2, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_sfx_pfx_3(const Suffix<wchar_t>& se1,
                                  const Suffix<wchar_t>& se2,
                                  std::wstring& word) const -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
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
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe1);
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
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			// needflag check here if needed
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_suffix_then_2_prefixes(std::wstring& word) const
    -> Affixing_Result<>
{
	if (!prefixes.has_continuation_flags())
		return {};

	for (auto i1 = Suffix_Iter(suffixes, word); i1; ++i1) {
		auto& se1 = *i1;
		if (se1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(se1))
			continue;
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se1);
		if (!se1.check_condition(word))
			continue;
		for (auto i2 = Prefix_Iter(prefixes, word); i2; ++i2) {
			auto& pe1 = *i2;
			if (pe1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe1))
				continue;
			if (is_circumfix(se1) != is_circumfix(pe1))
				continue;
			To_Root_Unroot_RAII<Prefix<wchar_t>> yyy(word, pe1);
			if (!pe1.check_condition(word))
				continue;
			auto ret = strip_sfx_2_pfx_3<FULL_WORD>(se1, pe1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_sfx_2_pfx_3(const Suffix<wchar_t>& se1,
                                  const Prefix<wchar_t>& pe1,
                                  std::wstring& word) const -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
		auto& pe2 = *it;
		if (!cross_valid_inner_outer(pe2, pe1))
			continue;
		if (affix_NOT_valid<m>(pe2))
			continue;
		if (is_circumfix(pe2))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe2);
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
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_prefix_suffix_prefix(std::wstring& word) const
    -> Affixing_Result<>
{
	if (!suffixes.has_continuation_flags() &&
	    !prefixes.has_continuation_flags())
		return {};

	for (auto i1 = Prefix_Iter(prefixes, word); i1; ++i1) {
		auto& pe1 = *i1;
		if (pe1.cross_product == false)
			continue;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = Suffix_Iter(suffixes, word); i2; ++i2) {
			auto& se1 = *i2;
			if (se1.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(se1))
				continue;
			To_Root_Unroot_RAII<Suffix<wchar_t>> yyy(word, se1);
			if (!se1.check_condition(word))
				continue;
			auto ret = strip_p_s_p_3<FULL_WORD>(pe1, se1, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_p_s_p_3(const Prefix<wchar_t>& pe1,
                              const Suffix<wchar_t>& se1,
                              std::wstring& word) const -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = Prefix_Iter(prefixes, word); it; ++it) {
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
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe2);
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
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			return {word_entry};
		}
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_prefixes_then_suffix(std::wstring& word) const
    -> Affixing_Result<>
{
	if (!suffixes.has_continuation_flags() &&
	    !prefixes.has_continuation_flags())
		return {};

	for (auto i1 = Prefix_Iter(prefixes, word); i1; ++i1) {
		auto& pe1 = *i1;
		if (outer_affix_NOT_valid<m>(pe1))
			continue;
		if (is_circumfix(pe1))
			continue;
		To_Root_Unroot_RAII<Prefix<wchar_t>> xxx(word, pe1);
		if (!pe1.check_condition(word))
			continue;
		for (auto i2 = Prefix_Iter(prefixes, word); i2; ++i2) {
			auto& pe2 = *i2;
			if (pe2.cross_product == false)
				continue;
			if (affix_NOT_valid<m>(pe2))
				continue;
			To_Root_Unroot_RAII<Prefix<wchar_t>> yyy(word, pe2);
			if (!pe2.check_condition(word))
				continue;
			auto ret = strip_2_pfx_sfx_3<FULL_WORD>(pe1, pe2, word);
			if (ret)
				return ret;
		}
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::strip_2_pfx_sfx_3(const Prefix<wchar_t>& pe1,
                                  const Prefix<wchar_t>& pe2,
                                  std::wstring& word) const -> Affixing_Result<>
{
	auto& dic = words;

	for (auto it = Suffix_Iter(suffixes, word); it; ++it) {
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
		To_Root_Unroot_RAII<Suffix<wchar_t>> xxx(word, se1);
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
			if (m == FULL_WORD &&
			    word_flags.contains(compound_onlyin_flag))
				continue;
			return {word_entry};
		}
	}

	return {};
}

auto match_compound_pattern(const Compound_Pattern<wchar_t>& p,
                            const wstring& word, size_t i,
                            Compounding_Result first, Compounding_Result second)
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

auto is_compound_forbidden_by_patterns(
    const vector<Compound_Pattern<wchar_t>>& patterns, const wstring& word,
    size_t i, Compounding_Result first, Compounding_Result second)
{
	return any_of(begin(patterns), end(patterns), [&](auto& p) {
		return match_compound_pattern(p, word, i, first, second);
	});
}

auto Dict_Base::check_compound(std::wstring& word) const -> Compounding_Result
{
	auto part = std::wstring();

	if (compound_flag || compound_begin_flag || compound_middle_flag ||
	    compound_last_flag) {
		auto ret = check_compound(word, 0, 0, part);
		if (ret)
			return ret;
	}
	if (!compound_rules.empty()) {
		auto words_data = vector<const Flag_Set*>();
		return check_compound_with_rules(word, words_data, 0, part);
	}

	return {};
}

template <Affixing_Mode m>
auto Dict_Base::check_compound(std::wstring& word, size_t start_pos,
                               size_t num_part, std::wstring& part) const
    -> Compounding_Result
{
	size_t min_length = 3;
	if (compound_min_length != 0)
		min_length = compound_min_length;
	if (word.size() < min_length * 2)
		return {};
	size_t max_length = word.size() - min_length;
	for (auto i = start_pos + min_length; i <= max_length; ++i) {

		auto part1_entry = check_compound_classic<m>(word, start_pos, i,
		                                             num_part, part);

		if (part1_entry)
			return part1_entry;

		part1_entry = check_compound_with_pattern_replacements<m>(
		    word, start_pos, i, num_part, part);

		if (part1_entry)
			return part1_entry;
	}
	return {};
}

template <Affixing_Mode m>
auto Dict_Base::check_compound_classic(std::wstring& word, size_t start_pos,
                                       size_t i, size_t num_part,
                                       std::wstring& part) const
    -> Compounding_Result
{
	part.assign(word, start_pos, i - start_pos);
	auto part1_entry = check_word_in_compound<m>(part);
	if (!part1_entry)
		return {};
	if (part1_entry->second.contains(forbiddenword_flag))
		return {};
	if (compound_check_triple) {
		auto triple = wstring(3, word[i]);
		if (word.compare(i - 1, 3, triple) == 0)
			return {};
		if (i >= 2 && word.compare(i - 2, 3, triple) == 0)
			return {};
	}
	if (compound_check_case &&
	    has_uppercase_at_compound_word_boundary(word, i, internal_locale))
		return {};

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

	return part1_entry;

try_recursive:
	part2_entry =
	    check_compound<AT_COMPOUND_MIDDLE>(word, i, num_part + 1, part);
	if (!part2_entry)
		goto try_simplified_triple;
	if (is_compound_forbidden_by_patterns(compound_patterns, word, i,
	                                      part1_entry, part2_entry))
		goto try_simplified_triple;
	// if (compound_check_duplicate && part1_entry == part2_entry)
	//	goto try_simplified_triple;

	return part1_entry;

try_simplified_triple:
	if (!compound_simplified_triple)
		return {};
	if (!(i >= 2 && word[i - 1] == word[i - 2]))
		return {};
	word.insert(i, 1, word[i - 1]);
	AT_SCOPE_EXIT(word.erase(i, 1));
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

	return part1_entry;

try_simplified_triple_recursive:
	part2_entry =
	    check_compound<AT_COMPOUND_MIDDLE>(word, i, num_part + 1, part);
	if (!part2_entry)
		return {};
	if (is_compound_forbidden_by_patterns(compound_patterns, word, i,
	                                      part1_entry, part2_entry))
		return {};
	// if (compound_check_duplicate && part1_entry == part2_entry)
	//	return {};

	return part1_entry;
}

template <Affixing_Mode m>
auto Dict_Base::check_compound_with_pattern_replacements(
    std::wstring& word, size_t start_pos, size_t i, size_t num_part,
    std::wstring& part) const -> Compounding_Result
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
			auto triple = wstring(3, word[i]);
			if (word.compare(i - 1, 3, triple) == 0)
				continue;
			if (i >= 2 && word.compare(i - 2, 3, triple) == 0)
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

		return part1_entry;

	try_recursive:
		part2_entry = check_compound<AT_COMPOUND_MIDDLE>(
		    word, i, num_part + 1, part);
		if (!part2_entry)
			goto try_simplified_triple;
		if (p.second_word_flag != 0 &&
		    !part2_entry->second.contains(p.second_word_flag))
			goto try_simplified_triple;
		// if (compound_check_duplicate && part1_entry == part2_entry)
		//	goto try_simplified_triple;

		return part1_entry;

	try_simplified_triple:
		if (!compound_simplified_triple)
			return {};
		if (!(i >= 2 && word[i - 1] == word[i - 2]))
			return {};
		word.insert(i, 1, word[i - 1]);
		AT_SCOPE_EXIT(word.erase(i, 1));
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

		return part1_entry;

	try_simplified_triple_recursive:
		part2_entry = check_compound<AT_COMPOUND_MIDDLE>(
		    word, i, num_part + 1, part);
		if (!part2_entry)
			continue;
		if (p.second_word_flag != 0 &&
		    !part2_entry->second.contains(p.second_word_flag))
			continue;
		// if (compound_check_duplicate && part1_entry == part2_entry)
		//	return {};

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
auto Dict_Base::check_word_in_compound(std::wstring& word) const
    -> Compounding_Result
{
	auto range = words.equal_range(word);
	for (auto& we : make_iterator_range(range)) {
		auto& word_flags = we.second;
		if (word_flags.contains(need_affix_flag))
			continue;
		if (word_flags.contains(compound_flag))
			return {&we};
		if (m == AT_COMPOUND_BEGIN &&
		    word_flags.contains(compound_begin_flag))
			return {&we};
		if (m == AT_COMPOUND_MIDDLE &&
		    word_flags.contains(compound_middle_flag))
			return {&we};
		if (m == AT_COMPOUND_END &&
		    word_flags.contains(compound_last_flag))
			return {&we};
	}
	auto x2 = strip_suffix_only<m>(word);
	if (x2)
		return {x2, is_modiying_affix(*get<1>(x2))};

	auto x1 = strip_prefix_only<m>(word);
	if (x1)
		return {x1, is_modiying_affix(*get<1>(x1))};

	auto x3 = strip_prefix_then_suffix_commutative<m>(word);
	if (x3)
		return {x3, is_modiying_affix(*get<1>(x3)) ||
		                is_modiying_affix(*get<2>(x3))};
	return {};
}

auto Dict_Base::check_compound_with_rules(
    std::wstring& word, std::vector<const Flag_Set*>& words_data,
    size_t start_pos, std::wstring& part) const -> Compounding_Result
{
	size_t min_length = 3;
	if (compound_min_length != 0)
		min_length = compound_min_length;
	if (word.size() < min_length * 2)
		return {};
	size_t max_length = word.size() - min_length;
	for (auto i = start_pos + min_length; i <= max_length; ++i) {

		part.assign(word, start_pos, i - start_pos);
		auto part1_entry = Word_List::const_pointer();
		auto range = words.equal_range(part);
		for (auto& we : make_iterator_range(range)) {
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
		for (auto& we : make_iterator_range(range)) {
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
			if (m)
				return {part1_entry};
		}
	try_recursive:
		part2_entry =
		    check_compound_with_rules(word, words_data, i, part);
		if (part2_entry)
			return {part2_entry};
	}
	return {};
}

auto Dict_Base::suggest_priv(std::wstring& word, List_WStrings& out) const
    -> void
{
	rep_suggest(word, out);
	map_suggest(word, out);
	extra_char_suggest(word, out);
	keyboard_suggest(word, out);
	bad_char_suggest(word, out);
	forgotten_char_suggest(word, out);
	phonetic_suggest(word, out);
}

auto Dict_Base::add_sug_if_correct(std::wstring& word, List_WStrings& out) const
    -> bool
{
	for (auto& o : out)
		if (o == word)
			return true;
	auto res = check_word(word);
	if (!res)
		return false;
	if (res->contains(forbiddenword_flag))
		return false;
	if (forbid_warn && res->contains(warn_flag))
		return false;
	out.push_back(word);
	return true;
}

auto Dict_Base::try_rep_suggestion(std::wstring& word, List_WStrings& out) const
    -> void
{
	if (add_sug_if_correct(word, out))
		return;

	auto i = size_t(0);
	auto j = word.find(' ');
	if (j == word.npos)
		return;
	auto part = wstring();
	for (; j != word.npos; i = j + 1, j = word.find(' ', i)) {
		part.assign(word, i, j - i);
		if (!check_word(part))
			return;
	}
	out.push_back(word);
}

auto Dict_Base::rep_suggest(std::wstring& word, List_WStrings& out) const
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
		if (word.compare(0, from.size(), from) == 0) {
			word.replace(0, from.size(), to);
			try_rep_suggestion(word, out);
			word.replace(0, to.size(), from);
		}
	}
	for (auto& r : reps.end_word_replacements()) {
		auto& from = r.first;
		auto& to = r.second;
		auto pos = word.size() - from.size();
		if (from.size() <= word.size() &&
		    word.compare(pos, word.npos, from) == 0) {
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

auto Dict_Base::extra_char_suggest(std::wstring& word, List_WStrings& out) const
    -> void
{
	for (auto i = word.size() - 1; i != size_t(-1); --i) {
		auto c = word[i];
		word.erase(i, 1);
		add_sug_if_correct(word, out);
		word.insert(i, 1, c);
	}
}

auto Dict_Base::map_suggest(std::wstring& word, List_WStrings& out,
                            size_t i) const -> void
{
	for (; i != word.size(); ++i) {
		for (auto& e : similarities) {
			auto j = e.chars.find(word[i]);
			if (j == word.npos)
				goto try_find_strings;
			for (auto c : e.chars) {
				if (c == e.chars[j])
					continue;
				word[i] = c;
				add_sug_if_correct(word, out);
				map_suggest(word, out, i + 1);
				word[i] = e.chars[j];
			}
			for (auto& r : e.strings) {
				word.replace(i, 1, r);
				add_sug_if_correct(word, out);
				map_suggest(word, out, i + r.size());
				word.replace(i, r.size(), 1, e.chars[j]);
			}
		try_find_strings:
			for (auto& f : e.strings) {
				if (word.compare(i, f.size(), f) != 0)
					continue;
				for (auto c : e.chars) {
					word.replace(i, f.size(), 1, c);
					add_sug_if_correct(word, out);
					map_suggest(word, out, i + 1);
					word.replace(i, 1, f);
				}
				for (auto& r : e.strings) {
					if (f == r)
						continue;
					word.replace(i, f.size(), r);
					add_sug_if_correct(word, out);
					map_suggest(word, out, i + r.size());
					word.replace(i, r.size(), f);
				}
			}
		}
	}
}

auto Dict_Base::keyboard_suggest(std::wstring& word, List_WStrings& out) const
    -> void
{
	auto& ct = use_facet<ctype<wchar_t>>(internal_locale);
	auto& kb = keyboard_closeness;
	for (size_t j = 0; j != word.size(); ++j) {
		auto c = word[j];
		auto upp_c = ct.toupper(c);
		if (upp_c != c) {
			word[j] = upp_c;
			add_sug_if_correct(word, out);
			word[j] = c;
		}
		for (auto i = kb.find(c); i != kb.npos; i = kb.find(c, i + 1)) {
			if (i != 0 && kb[i - 1] != '|') {
				word[j] = kb[i - 1];
				add_sug_if_correct(word, out);
				word[j] = c;
			}
			if (i + 1 != kb.size() && kb[i + 1] != '|') {
				word[j] = kb[i + 1];
				add_sug_if_correct(word, out);
				word[j] = c;
			}
		}
	}
}

auto Dict_Base::bad_char_suggest(std::wstring& word, List_WStrings& out) const
    -> void
{
	for (auto new_c : try_chars) {
		for (size_t i = 0; i != word.size(); ++i) {
			auto c = word[i];
			if (c == new_c)
				continue;
			word[i] = new_c;
			add_sug_if_correct(word, out);
			word[i] = c;
		}
	}
}

auto Dict_Base::forgotten_char_suggest(std::wstring& word,
                                       List_WStrings& out) const -> void
{
	for (auto new_c : try_chars) {
		for (auto i = word.size(); i != size_t(-1); --i) {
			word.insert(i, 1, new_c);
			add_sug_if_correct(word, out);
			word.erase(i, 1);
		}
	}
}

auto Dict_Base::phonetic_suggest(std::wstring& word, List_WStrings& out) const
    -> void
{
	using ShortStr = boost::container::small_vector<wchar_t, 64>;
	auto backup = ShortStr(&word[0], &word[word.size()]);
	boost::algorithm::to_upper(word, internal_locale);
	auto changed = phonetic_table.replace(word);
	if (changed) {
		boost::algorithm::to_lower(word, internal_locale);
		add_sug_if_correct(word, out);
	}
	word.assign(&backup[0], backup.size());
}

Dictionary::Dictionary(std::istream& aff, std::istream& dic)
{
	if (!parse_aff_dic(aff, dic))
		throw std::ios_base::failure("error parsing");
	external_locale_known_utf8 = is_locale_known_utf8(external_locale);
}

auto Dictionary::external_to_internal_encoding(const string& in,
                                               wstring& wide_out) const -> bool
{
	if (external_locale_known_utf8)
		return utf8_to_wide(in, wide_out);
	else
		return to_wide(in, external_locale, wide_out);
}

auto Dictionary::internal_to_external_encoding(string& out,
                                               wstring& wide_in) const -> bool
{
	if (external_locale_known_utf8)
		wide_to_utf8(wide_in, out);
	else
		return to_narrow(wide_in, out, external_locale);
	return true;
}

Dictionary::Dictionary()
{
	// ensures the internal locale is boost locale
	set_encoding_and_language("");
	external_locale_known_utf8 = is_locale_known_utf8(external_locale);
}

/**
 * @brief Create a dictionary from opened files as iostreams
 *
 * Prefer using load_from_path(). Use this if you have a specific use case,
 * like when .aff and .dic are in-memory buffers istringstream.
 *
 * @param aff The iostream of the .aff file
 * @param dic The iostream of the .dic file
 * @return Dictionary object
 */
auto Dictionary::load_from_aff_dic(std::istream& aff, std::istream& dic)
    -> Dictionary
{
	return Dictionary(aff, dic);
}

/**
 * @brief Create a dictionary from files
 * @param file path without extensions
 * @return Dictionary object
 */
auto Dictionary::load_from_path(const std::string& file_path_without_extension)
    -> Dictionary
{
	auto path = file_path_without_extension;
	path += ".aff";
	std::ifstream aff_file(path);
	if (aff_file.fail()) {
		auto err = "Aff file " + path + " not found";
		throw std::ios_base::failure(err);
	}
	path.replace(path.size() - 3, 3, "dic");
	std::ifstream dic_file(path);
	if (dic_file.fail()) {
		auto err = "Dic file " + path + " not found";
		throw std::ios_base::failure(err);
	}
	return load_from_aff_dic(aff_file, dic_file);
}

/**
 * @brief Imbues external locale object to set external encoding
 *
 * The locale must contain codecvt<wchar_t, char, mbstate_t> facet that can
 * convert the strings from the external locale to UTF-32 on non-Windows
 * platforms, and to UTF-16 on Windows. By external encoding it is meant the
 * encoding of strings in the client code that uses this library.
 *
 * You may safely use locales geenrated by Boost.Locale.
 *
 * @param loc locale object with valid codecvt<wchar_t, char, mbstate_t>
 */
auto Dictionary::imbue(const locale& loc) -> void
{
	external_locale = loc;
	external_locale_known_utf8 = is_locale_known_utf8(external_locale);
}

/**
 * @brief Checks if a given word is correct
 * @param word
 * @return true if correct, false otherwise
 */
auto Dictionary::spell(const std::string& word) const -> bool
{
	auto static thread_local wide_word = wstring();
	auto static thread_local narrow_word = string();
	auto ok_enc = external_to_internal_encoding(word, wide_word);
	if (unlikely(wide_word.size() > 180)) {
		wide_word.resize(180);
		wide_word.shrink_to_fit();
		return false;
	}
	if (unlikely(!ok_enc))
		return false;
	return spell_priv(wide_word);
}

/**
 * @brief Suggests correct words for a given incorrect word
 * @param word incorrect word
 * @param[out] out this object will be populated with the suggestions
 */
auto Dictionary::suggest(const string& word, List_Strings& out) const -> void
{
	auto static thread_local wide_word = wstring();
	auto static thread_local wide_list = List_WStrings();

	out.clear();
	auto ok_enc = external_to_internal_encoding(word, wide_word);
	if (unlikely(wide_word.size() > 180)) {
		wide_word.resize(180);
		wide_word.shrink_to_fit();
		return;
	}
	if (unlikely(!ok_enc))
		return;
	wide_list.clear();
	suggest_priv(wide_word, wide_list);
	for (auto& w : wide_list) {
		auto& o = out.emplace_back();
		internal_to_external_encoding(o, w);
	}
}
} // namespace nuspell
