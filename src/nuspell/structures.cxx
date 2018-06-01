/* Copyright 2018 Dimitrij Mijoski
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

#include "structures.hxx"

#include <boost/utility/string_view.hpp>

namespace nuspell {

using namespace std;
using boost::basic_string_view;

template class String_Set<char>;
template class String_Set<wchar_t>;
template class String_Set<char16_t>;

template <class CharT>
void Substr_Replacer<CharT>::sort_uniq()
{
	auto first = table.begin();
	auto last = table.end();
	sort(first, last, [](auto& a, auto& b) { return a.first < b.first; });
	auto it = unique(first, last,
	                 [](auto& a, auto& b) { return a.first == b.first; });
	table.erase(it, last);

	// remove empty key ""
	if (!table.empty() && table.front().first.empty())
		table.erase(table.begin());
}

namespace {
template <class CharT>
struct Comparer_Str_Rep {

	using StrT = basic_string<CharT>;
	using StrViewT = basic_string_view<CharT>;

	auto static cmp_prefix_of(const StrT& p, StrViewT of)
	{
		return p.compare(0, p.npos, of.data(), 0,
		                 min(p.size(), of.size()));
	}
	auto operator()(const pair<StrT, StrT>& a, StrViewT b)
	{
		return cmp_prefix_of(a.first, b) < 0;
	}
	auto operator()(StrViewT a, const pair<StrT, StrT>& b)
	{
		return cmp_prefix_of(b.first, a) > 0;
	}
	auto eq(const pair<StrT, StrT>& a, StrViewT b)
	{
		return cmp_prefix_of(a.first, b) == 0;
	}
};

template <class CharT>
auto find_match(const typename Substr_Replacer<CharT>::Table_Pairs& t,
                basic_string_view<CharT> s)
{
	Comparer_Str_Rep<CharT> csr;
	auto it = begin(t);
	auto last_match = end(t);
	for (;;) {
		auto it2 = upper_bound(it, end(t), s, csr);
		if (it2 == it) {
			// not found, s is smaller that the range
			break;
		}
		--it2;
		if (csr.eq(*it2, s)) {
			// Match found. Try another search maybe for
			// longer.
			last_match = it2;
			it = ++it2;
		}
		else {
			// not found, s is greater that the range
			break;
		}
	}
	return last_match;
}
} // namespace

template <class CharT>
auto Substr_Replacer<CharT>::replace(StrT& s) const -> StrT&
{

	if (table.empty())
		return s;
	for (size_t i = 0; i < s.size(); /*no increment here*/) {
		auto substr = basic_string_view<CharT>(&s[i], s.size() - i);
		auto it = find_match(table, substr);
		if (it != end(table)) {
			auto& match = *it;
			// match found. match.first is the found string,
			// match.second is the replacement.
			s.replace(i, match.first.size(), match.second);
			i += match.second.size();
			continue;
		}
		++i;
	}
	return s;
}

template class Substr_Replacer<char>;
template class Substr_Replacer<wchar_t>;

template <class CharT>
auto Break_Table<CharT>::order_entries() -> void
{
	using boost::make_iterator_range;

	auto it = remove_if(begin(table), end(table), [](auto& s) {
		return s.empty() ||
		       (s.size() == 1 && (s[0] == '^' || s[0] == '$'));
	});
	table.erase(it, end(table));

	auto is_start_word_break = [=](auto& x) { return x[0] == '^'; };
	auto is_end_word_break = [=](auto& x) { return x.back() == '$'; };
	start_word_breaks_last_it =
	    partition(begin(table), end(table), is_start_word_break);

	for (auto& e :
	     make_iterator_range(begin(table), start_word_breaks_last_it)) {
		e.erase(0, 1);
	}

	end_word_breaks_last_it =
	    partition(start_word_breaks_last_it, end(table), is_end_word_break);
	for (auto& e : make_iterator_range(start_word_breaks_last_it,
	                                   end_word_breaks_last_it)) {
		e.pop_back();
	}
}
template class Break_Table<char>;
template class Break_Table<wchar_t>;

/**
 * Constructs a prefix entry.
 *
 * @note Do not provide a string "0" for the parameter stripping.
 * In such case, only an emptry string "" should be used. This is handled by the
 * function nuspell::parse_affix and should not be double checked here for
 * reasons of optimization.
 *
 * @param flag
 * @param cross_product
 * @param strip
 * @param append
 * @param condition
 */
template <class CharT>
Prefix<CharT>::Prefix(char16_t flag, bool cross_product, const StrT& strip,
                      const StrT& append, const Flag_Set& cont_flags,
                      const StrT& condition)
    : flag{flag}, cross_product{cross_product}, stripping{strip},
      appending{append}, cont_flags{cont_flags}, condition{condition}
{
}

/**
 * Converts a word into a root according to this prefix entry.
 *
 * The conversion of the word is done by removing at the beginning of the word
 * what (could have been) appended and subsequently adding at the beginning
 * (what could have been) stripped. This method does the reverse of the
 * derive method.
 *
 * @param word the word which is converted into a root inplace.
 * @return @p word, modified as described.
 */
template <class CharT>
auto Prefix<CharT>::to_root(StrT& word) const -> StrT&
{
	return word.replace(0, appending.size(), stripping);
}

/**
 * Converts a word into a root according to this prefix entry.
 *
 * @param the word which is converted into a root as a new copy.
 * @return The resulting root.
 */
template <class CharT>
auto Prefix<CharT>::to_root_copy(StrT word) const -> StrT
{
	to_root(word);
	return word;
}

/**
 * Converts a root word into a derived word according to this prefix entry.
 *
 * The conversion of the word is done by replacing at the beginning of the word
 * what to strip with what to append.
 *
 * @param word the root word which is converted to a derived word inplace.
 * @return @p word, modified as described.
 */
template <class CharT>
auto Prefix<CharT>::to_derived(StrT& word) const -> StrT&
{
	return word.replace(0, stripping.size(), appending);
}

/**
 * Converts a root word into a derived word according to this prefix entry.
 *
 * @param word the root word which is converted to a derived as a new copy.
 * @return The resulting derived word.
 */
template <class CharT>
auto Prefix<CharT>::to_derived_copy(StrT word) const -> StrT
{
	to_derived(word);
	return word;
}

/**
 * Checks of the condition of this prefix entry matches the supplied word.
 *
 * @note In regular expressions, dots in groups are not metacharacters.
 *
 * @param word to check against the condition.
 * @return The result of the check.
 */
template <class CharT>
auto Prefix<CharT>::check_condition(const StrT& word) const -> bool
{
	return condition.match_prefix(word);
}

/**
 * Constructs a suffix entry.
 *
 * @note Do not provide a string "0" for the parameter stripping.
 * In such case, only an emptry string "" should be used. This is handled by the
 * function nuspell::parse_affix and should not be double checked here for
 * reasons of optimization.
 *
 * @param flag
 * @param cross_product
 * @param strip
 * @param append
 * @param condition
 */
template <class CharT>
Suffix<CharT>::Suffix(char16_t flag, bool cross_product, const StrT& strip,
                      const StrT& append, const Flag_Set& cont_flags,
                      const StrT& condition)
    : flag{flag}, cross_product{cross_product}, stripping{strip},
      appending{append}, cont_flags{cont_flags}, condition{condition}
{
}

/**
 * Converts a word into a root according to this suffix entry.
 *
 * The conversion of the word is done by removing at the end of the word what
 * (could have been) appended and subsequently adding at the end (what could
 * have been) stripped. This method does the reverse of the derive method.
 *
 * @param word the word which is converted into a root inplace.
 * @return @p word, modified as described.
 */
template <class CharT>
auto Suffix<CharT>::to_root(StrT& word) const -> StrT&
{
	return word.replace(word.size() - appending.size(), appending.size(),
	                    stripping);
}

/**
 * Converts a word into a root according to this suffix entry.
 *
 * @param word the word which is converted into a root as new copy.
 * @return The resulting root.
 */
template <class CharT>
auto Suffix<CharT>::to_root_copy(StrT word) const -> StrT
{
	return to_root(word);
}

/**
 * Converts a root word into a derived word according to this suffix entry.
 *
 * The conversion of the word is done by replacing at the end of the word
 * what to strip with what to append.
 *
 * @param word the root word which is converted to a derived word inplace.
 * @return @p word, modified as described.
 */
template <class CharT>
auto Suffix<CharT>::to_derived(StrT& word) const -> StrT&
{
	return word.replace(word.size() - stripping.size(), stripping.size(),
	                    appending);
}

/**
 * Converts a root word into a derived word according to this suffix entry.
 *
 * @param word the root word which is converted to a derived word as a new copy.
 * @return The resulting derived word.
 */
template <class CharT>
auto Suffix<CharT>::to_derived_copy(StrT word) const -> StrT
{
	to_derived(word);
	return word;
}

/**
 * Checks of the condition of this suffix entry matches the supplied word.
 *
 * @param word to check against the condition.
 * @return The resulting of the check.
 */
template <class CharT>
auto Suffix<CharT>::check_condition(const StrT& word) const -> bool
{
	return condition.match_suffix(word);
}

template class Prefix<char>;
template class Prefix<wchar_t>;
template class Suffix<char>;
template class Suffix<wchar_t>;
} // namespace nuspell
