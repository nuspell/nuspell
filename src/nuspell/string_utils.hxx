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
 * @file
 * @brief String algorithms, private header.
 */

#ifndef NUSPELL_STRING_UTILS_HXX
#define NUSPELL_STRING_UTILS_HXX

#include <algorithm>
#include <iterator>
#include <locale>
#include <stack>
#include <string>
#include <vector>

#ifdef __has_include
#if __has_include(<experimental/string_view>)
#if !defined(_LIBCPP_VERSION) || _LIBCPP_VERSION < 7000
#include <experimental/string_view>
#if defined(__cpp_lib_experimental_string_view) || defined(_LIBCPP_VERSION)
#define NUSPELL_STR_VIEW_NS std::experimental
#endif
#endif
#endif
#endif

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 7000
#include <string_view>
#define NUSPELL_STR_VIEW_NS std
#endif

#ifndef NUSPELL_STR_VIEW_NS
#define NUSPELL_STR_VIEW_NS boost
#include <boost/functional/hash.hpp>
#include <boost/utility/string_view.hpp>
template <class CharT>
struct std::hash<boost::basic_string_view<CharT>> {
	auto operator()(boost::basic_string_view<CharT> s) const
	{
		return boost::hash_range(begin(s), end(s));
	}
};
#endif

namespace nuspell {
#define NUSPELL_LITERAL(T, x) ::nuspell::literal_choose<T>(x, L##x)

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

template <class CharT>
using my_string_view = NUSPELL_STR_VIEW_NS::basic_string_view<CharT>;
using string_view = my_string_view<char>;
using wstring_view = my_string_view<wchar_t>;

/**
 * @brief Splits string on set of single char seperators.
 *
 * Consecutive separators are treated as separate and will emit empty strings.
 *
 * @param s string to split.
 * @param sep seperator(s) to split on.
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
 * @brief Splits string on single char seperator.
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
 * @brief Splits on whitespace.
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
 * @brief Splits on whitespace, outputs to vector of strings.
 *
 * See split_on_whitespace().
 *
 * @param s string to split.
 * @param[out] v vector with separated strings. The vector is first cleared.
 * @param loc locale object that takes care of what is whitespace.
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

template <class CharT>
auto& replace_char(std::basic_string<CharT>& s, CharT from, CharT to)
{
	for (auto i = s.find(from); i != s.npos; i = s.find(from, i + 1)) {
		s[i] = to;
	}
	return s;
}

/**
 * @brief Tests if word is a number.
 *
 * Allow numbers with dots ".", dashes "-" and commas ",", but forbids double
 * separators such as "..", "--" and ".,".  This implementation increases
 * performance over the regex implementation in the standard library.
 */
template <class CharT>
auto is_number(const std::basic_string<CharT>& s) -> bool
{
	if (s.empty())
		return false;

	auto it = begin(s);
	if (s[0] == '-')
		++it;
	while (it != end(s)) {
		auto next = find_if(it, end(s),
		                    [](auto c) { return c < '0' || c > '9'; });
		if (next == it)
			return false;
		if (next == end(s))
			return true;
		it = next;
		auto c = *it;
		if (c == '.' || c == ',' || c == '-')
			++it;
		else
			return false;
	}
	return false;
}

template <class DataIter, class PatternIter, class FuncEq = std::equal_to<>>
auto match_simple_regex(DataIter data_first, DataIter data_last,
                        PatternIter pat_first, PatternIter pat_last,
                        FuncEq eq = FuncEq())
{
	auto s = std::stack<std::pair<DataIter, PatternIter>>();
	s.emplace(data_first, pat_first);
	auto data_it = DataIter();
	auto pat_it = PatternIter();
	while (!s.empty()) {
		std::tie(data_it, pat_it) = s.top();
		s.pop();
		if (pat_it == pat_last) {
			if (data_it == data_last)
				return true;
			else
				return false;
		}
		auto node_type = *pat_it;
		if (pat_it + 1 == pat_last)
			node_type = 0;
		else
			node_type = *(pat_it + 1);
		switch (node_type) {
		case '?':
			s.emplace(data_it, pat_it + 2);
			if (data_it != data_last && eq(*data_it, *pat_it))
				s.emplace(data_it + 1, pat_it + 2);
			break;
		case '*':
			s.emplace(data_it, pat_it + 2);
			if (data_it != data_last && eq(*data_it, *pat_it))
				s.emplace(data_it + 1, pat_it);

			break;
		default:
			if (data_it != data_last && eq(*data_it, *pat_it))
				s.emplace(data_it + 1, pat_it + 1);
			break;
		}
	}
	return false;
}

template <class DataRange, class PatternRange, class FuncEq = std::equal_to<>>
auto match_simple_regex(const DataRange& data, const PatternRange& pattern,
                        FuncEq eq = FuncEq())
{
	using namespace std;
	return match_simple_regex(begin(data), end(data), begin(pattern),
	                          end(pattern), eq);
}

} // namespace nuspell
#endif // NUSPELL_STRING_UTILS_HXX
