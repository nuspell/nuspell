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

/*!
 * \file string_utils.hxx
 * String algorithms not dependent on locale.
 */

#ifndef HUNSPELL_STRING_UTILS_HXX
#define HUNSPELL_STRING_UTILS_HXX

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <iterator>
#include <locale>
#include <string>
#include <vector>

namespace hunspell {

/*!
 * Splits string on set of single char seperators.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * \param s a string to split.
 * \param sep char or string holding all separators to split on.
 * \param out start of the output range where separated strings are appended.
 * \return iterator that indicates the end of the output range.
 */
template <class ChT, class Tr, class Al, class SepT, class OutIt>
auto split_on_any_of(const std::basic_string<ChT, Tr, Al>& s, const SepT& sep,
                     OutIt out)
{
	using size_type = typename std::basic_string<ChT, Tr, Al>::size_type;
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

/*!
 * Splits string on single char seperator.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * \param s a string to split.
 * \param sep char that acts as separator to split on.
 * \param out start of the output range where separated strings are appended.
 * \return iterator that indicates the end of the output range.
 */
template <class ChT, class Tr, class Al, class OutIt>
auto split(const std::basic_string<ChT, Tr, Al>& s, ChT sep, OutIt out)
{
	return split_on_any_of(s, sep, out);
}

/*!
 * \brief Split string on string separator.
 * \param s
 * \param sep
 * \param out
 */
template <class ChT, class Tr, class Al, class OutIt>
auto split(const std::basic_string<ChT, Tr, Al>& s,
           const std::basic_string<ChT, Tr, Al>& sep, OutIt out)
{
	using size_type = typename std::basic_string<ChT, Tr, Al>::size_type;
	size_type i1 = 0;
	size_type i2;
	do {
		i2 = s.find(sep, i1);
		*out++ = s.substr(i1, i2 - i1);
		i1 = i2 + sep.size();
	} while (i2 != s.npos);
	return out;
}

/*!
 * \brief Split string on string separator.
 * \param s
 * \param sep
 * \param out
 */
template <class ChT, class Tr, class Al, class OutIt>
auto split(const std::basic_string<ChT, Tr, Al>& s, const ChT* sep, OutIt out)
{
	return split(s, std::basic_string<ChT, Tr, Al>(sep), out);
}

/*!
 * Splits string on seperator, output to vector of strings.
 *
 * See split().
 *
 * \param s in string
 * \param sep in separator
 * \param v out vector. The vector is first cleared.
 */
template <class ChT, class Tr, class Al, class CharOrStr>
auto split_v(const std::basic_string<ChT, Tr, Al>& s, const CharOrStr& sep,
             std::vector<std::basic_string<ChT, Tr, Al>>& v)
{
	v.clear();
	split(s, sep, std::back_inserter(v));
}

/*!
 * Gets the first token of a splitted string.
 *
 * \param s a string to split.
 * \param sep char or string that acts as separator to split on.
 * \return The string that has been split off.
 */
template <class CharT, class CharOrStr>
auto split_first(const std::basic_string<CharT>& s, const CharOrStr& sep)
    -> std::basic_string<CharT>
{
	auto index = s.find(sep);
	return s.substr(0, index);
}

/*!
 * Splits on whitespace.
 *
 * Consecutive whitespace is treated as single separator. Behaves same as
 * Python's split called without separator argument.
 *
 * \param s a string to split.
 * \param out start of the output range where separated strings are appended.
 * \param loc locale object that takes care of what is whitespace
 * \return iterator that indicates the end of the output range.
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

/*!
 * Splits on whitespace, outputs to vector of strings.
 *
 * See split_on_whitespace().
 *
 * \param s in string
 * \param v out vector. The vector is first cleared.
 * \param loc in locale
 */
template <class CharT>
auto split_on_whitespace_v(const std::basic_string<CharT>& s,
                           std::vector<std::basic_string<CharT>>& v,
                           const std::locale& loc = std::locale()) -> void
{
	v.clear();
	split_on_whitespace(s, back_inserter(v), loc);
}

/*!
 * Convert entire string to upper case.
 *
 * \param s in string to convert to upper case.
 * \param loc in locale.
 * \return converted string.
 */
template <class CharT>
auto to_upper(std::basic_string<CharT> s,
              const std::locale& loc = std::locale())
{
	return boost::to_upper_copy(s, loc);
}

template <class CharT>
auto to_upper1(std::basic_string<CharT> s,
               const std::locale& loc = std::locale())
{
	auto& f = std::use_facet<std::ctype<CharT>>(loc);
	// FIXME	std::transform(s.begin(), s.end(), s.begin(),
	// f.toupper);
	return s;
}

template <class CharT>
auto to_upper2(std::basic_string<CharT> s,
               const std::locale& loc = std::locale())
{
	auto& f = std::use_facet<std::ctype<CharT>>(loc);
	// FIXME	return string(f.toupper(s.data(), s.data() + s.size()));
	return s;
}

template <class CharT>
auto to_upper3(std::basic_string<CharT> s,
               const std::locale& loc = std::locale())
{
	// TODO Implement using locale.
	return boost::to_upper(s);
}

/*!
 * Capitalize a string by converting the first character to upper case.
 *
 * A special case is made for Dutch when boolean d is set to true. When the
 * first two characters are "ij", both will be converted to upper case.
 *
 * \param s in string to capitalize.
 * \param d in boolean indicating capitalizating for Dutch ij digraph/ligature.
 * \param loc in locale.
 * \return capitalized string.
 */
template <class CharT>
auto capitalize(std::basic_string<CharT> s, bool d = false,
                const std::locale& loc = std::locale())
{
	auto length = s.length();
	if (length < 1)
		return s;
	auto& f = std::use_facet<std::ctype<CharT>>(loc);
	s[0] = f.toupper(s[0]); // TODO Optimization candidate for Boost
	                        // boost::locale::to_title
	if (d && length > 1 && 'i' == f.tolower(s[0]) && 'j' == f.tolower(s[1]))
		s[1] = f.toupper(s[1]); // TODO Optimization candidate for Boost
		                        // boost::locale::to_title
	return s;
}

template <class CharT>
auto capitalize1(std::basic_string<CharT> s, bool d = false,
                 const std::locale& loc = std::locale())
{
	auto length = s.length();
	if (length < 1)
		return s;
	// FIXME	s[0] = boost::to_upper(s[0], loc);
	if (d && length > 1 && 'i' == boost::to_lower(s[0]) &&
	    'j' == boost::to_lower(s[1]))
		// FIXME		s[1] = boost::to_upper(s[1], loc);
		return s;
}
}
#endif
