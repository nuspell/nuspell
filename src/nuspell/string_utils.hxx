/* Copyright 2018 Dimitrij Mijoski, Sander van Geloven
 * Copyright 2016-2017 Dimitrij Mijoski
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
 * @file string_utils.hxx
 * String algorithms.
 */

#ifndef NUSPELL_STRING_UTILS_HXX
#define NUSPELL_STRING_UTILS_HXX

#include <algorithm>
#include <iterator>
#include <locale>
#include <string>
#include <vector>

namespace nuspell {
#define LITERAL(T, x) ::nuspell::literal_choose<T>(x, L##x)

template <class CharT>
auto constexpr literal_choose(const char* narrow, const wchar_t* wide);
template <>
auto constexpr literal_choose<char>(const char* narrow, const wchar_t*)
{
	return narrow;
}
template <>
auto constexpr literal_choose<wchar_t>(const char*, const wchar_t* wide)
{
	return wide;
}

/**
 * Splits string on set of single char seperators.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep seperator to split on.
 * @param out start of the output range where separated strings are
 * appended.
 * @return The end of the output range where separated strings are appended.
 */
template <class CharT, class SepT, class OutIt>
auto split_on_any_of(const std::basic_string<CharT>& s, const SepT& sep,
                     OutIt out)
{
	using size_type = typename std::basic_string<CharT>::size_type;
	size_type i1 = 0;
	size_type i2;
	do {
		i2 = s.find_first_of(sep, i1);
		*out++ = s.substr(i1, i2 - i1);
		i1 = i2 + 1; // we can only add +1 if sep is single char.

		// i2 gets s.npos after the last separator.
		// Length of i2-i1 will always go past the end. That is defined.
	} while (i2 != s.npos);
	return out;
}

/**
 * Splits string on single char seperator.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep char that acts as separator to split on.
 * @param out start of the output range where separated strings are
 * appended.
 * @return The iterator that indicates the end of the output range.
 */
template <class CharT, class OutIt>
auto split(const std::basic_string<CharT>& s, CharT sep, OutIt out)
{
	return split_on_any_of(s, sep, out);
}

/**
 * Splits string on string separator.
 *
 * @param s string to split.
 * @param sep seperator to split on.
 * @param out start of the output range where separated strings are
 * appended.
 * @return The end of the output range where separated strings are appended.
 */
template <class CharT, class OutIt>
auto split(const std::basic_string<CharT>& s,
           const std::basic_string<CharT>& sep, OutIt out)
{
	using size_type = typename std::basic_string<CharT>::size_type;
	size_type i1 = 0;
	size_type i2;
	do {
		i2 = s.find(sep, i1);
		*out++ = s.substr(i1, i2 - i1);
		i1 = i2 + sep.size();
	} while (i2 != s.npos);
	return out;
}

/**
 * Splits string on string separator.
 *
 * @param s string to split.
 * @param sep seperator to split on.
 * @param out start of the output range where separated strings are
 * appended.
 * @return The end of the output range where separated strings are appended.
 */
template <class CharT, class OutIt>
auto split(const std::basic_string<CharT>& s, const CharT* sep, OutIt out)
{
	return split(s, std::basic_string<CharT>(sep), out);
}

/**
 * Splits string on seperator, output to vector of strings.
 *
 * See split().
 *
 * @param s string to split.
 * @param sep separator to split on.
 * @param[out] v vector with separated strings. The vector is first cleared.
 */
template <class CharT, class CharOrStr>
auto split_v(const std::basic_string<CharT>& s, const CharOrStr& sep,
             std::vector<std::basic_string<CharT>>& v)
{
	v.clear();
	split(s, sep, std::back_inserter(v));
}

/**
 * Gets the first token of a splitted string.
 *
 * @param s string to split.
 * @param sep char or string that acts as separator to split on.
 * @return The string that has been split off.
 */
template <class CharT, class CharOrStr>
auto split_first(const std::basic_string<CharT>& s, const CharOrStr& sep)
    -> std::basic_string<CharT>
{
	auto index = s.find(sep);
	return s.substr(0, index);
}

/**
 * Splits on whitespace.
 *
 * Consecutive whitespace is treated as single separator. Behaves same as
 * Python's split called without separator argument.
 *
 * @param s string to split.
 * @param out start of the output range where separated strings are
 * appended.
 * @param loc locale object that takes care of what is whitespace.
 * @return The iterator that indicates the end of the output range.
 */
template <class CharT, class OutIt>
auto split_on_whitespace(const std::basic_string<CharT>& s, OutIt out,
                         const std::locale& loc = std::locale()) -> OutIt
{
	auto& f = std::use_facet<std::ctype<CharT>>(loc);
	auto isspace = [&](auto& c) { return f.is(std::ctype_base::space, c); };
	auto i1 = begin(s);
	auto endd = end(s);
	do {
		auto i2 = std::find_if_not(i1, endd, isspace);
		if (i2 == endd)
			break;
		auto i3 = std::find_if(i2, endd, isspace);
		*out++ = std::basic_string<CharT>(i2, i3);
		i1 = i3;
	} while (i1 != endd);
	return out;
}

/**
 * Splits on whitespace, outputs to vector of strings.
 *
 * See split_on_whitespace().
 *
 * @param s string to split.
 * @param[out] v vector with separated strings. The vector is first cleared.
 * @param loc input locale.
 */
template <class CharT>
auto split_on_whitespace_v(const std::basic_string<CharT>& s,
                           std::vector<std::basic_string<CharT>>& v,
                           const std::locale& loc = std::locale()) -> void
{
	v.clear();
	split_on_whitespace(s, back_inserter(v), loc);
}

template <class CharT>
auto& erase_chars(std::basic_string<CharT>& s,
                  const std::basic_string<CharT>& erase_chars)
{
	if (erase_chars.empty())
		return s;
	auto is_erasable = [&](CharT c) {
		return erase_chars.find(c) != erase_chars.npos;
	};
	auto it = remove_if(begin(s), end(s), is_erasable);
	s.erase(it, end(s));
	return s;
}

/**
 * Tests if word is a number.
 *
 * Allow numbers with dots ".", dashes "-" and commas ",", but forbids double
 * separators such as "..", "--" and ".,".  This results in increase of
 * performance over an implementation with <regex> use.
 */
template <class CharT>
auto is_number(const std::basic_string<CharT>& s) -> bool
{
	if (s.empty())
		return false;

	size_t i = 0;
	if (s[0] == '-')
		++i;
	for (; i != s.size();) {
		auto next =
		    s.find_first_not_of(LITERAL(CharT, "0123456789"), i);
		if (next == i)
			return false;
		if (next == s.npos)
			return true;
		i = next;
		auto c = s[i];
		if (c == '.' || c == ',' || c == '-')
			++i;
		else
			return false;
	}
	return false;
}
} // namespace nuspell
#endif // NUSPELL_STRING_UTILS_HXX
