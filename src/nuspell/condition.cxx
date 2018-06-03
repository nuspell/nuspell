/* Copyright 2018 Dimitrij Mijoski and Sander van Geloven
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
 * @file condition.cxx
 * Condition matching.
 */

#include "condition.hxx"
#include "string_utils.hxx"

#include <stdexcept>

namespace nuspell {

using namespace std;

/**
 * Constructs a Condition object.
 *
 * @param condition string containing simplifed regular expression to construct
 * this Condition object for.
 */
template <class CharT>
Condition<CharT>::Condition(const StrT& condition) : cond(condition)
{
	size_t i = 0;
	for (; i != cond.size();) {
		size_t j = cond.find_first_of(LITERAL(CharT, "[]."), i);
		if (i != j) {
			if (j == cond.npos) {
				spans.emplace_back(i, cond.size() - i, NORMAL);
				length += cond.size() - i;
				break;
			}
			spans.emplace_back(i, j - i, NORMAL);
			length += j - i;
			i = j;
		}
		if (cond[i] == '.') {
			spans.emplace_back(i, 1, DOT);
			++length;
			++i;
			continue;
		}
		if (cond[i] == ']') {
			auto what =
			    "Closing bracket has no matching opening bracket.";
			throw invalid_argument(what);
		}
		if (cond[i] == '[') {
			++i;
			if (i == cond.size()) {
				auto what = "Opening bracket has no matching "
				            "closing bracket.";
				throw invalid_argument(what);
			}
			Span_Type type;
			if (cond[i] == '^') {
				type = NONE_OF;
				++i;
			}
			else {
				type = ANY_OF;
			}
			j = cond.find(']', i);
			if (j == i) {
				auto what = "Empty bracket expression.";
				throw invalid_argument(what);
			}
			if (j == cond.npos) {
				auto what = "Opening bracket has no matching "
				            "closing bracket.";
				throw invalid_argument(what);
			}
			spans.emplace_back(i, j - i, type);
			++length;
			i = j + 1;
		}
	}
}

/**
 * Checks if provided string matched the condition.
 *
 * @param s string to check if it matches the condition.
 * @param pos start position for string, default is 0.
 * @param len length of string counting from the start position.
 * @return The valueof true when string matched condition.
 */
template <class CharT>
auto Condition<CharT>::match(const StrT& s, size_t pos, size_t len) const
    -> bool
{
	if (pos > s.size()) {
		throw out_of_range("pos out of bounds on s");
	}
	if (s.size() - pos < len)
		len = s.size() - pos;
	if (len != length)
		return false;

	size_t i = pos;
	for (auto& x : spans) {
		auto x_pos = get<0>(x);
		auto x_len = get<1>(x);
		auto x_type = get<2>(x);

		using tr = typename StrT::traits_type;
		switch (x_type) {
		case NORMAL:
			if (tr::compare(&s[i], &cond[x_pos], x_len) == 0)
				i += x_len;
			else
				return false;
			break;
		case DOT:
			++i;
			break;
		case ANY_OF:
			if (tr::find(cond.data() + x_pos, x_len, s[i]))
				++i;
			else
				return false;
			break;
		case NONE_OF:
			if (tr::find(cond.data() + x_pos, x_len, s[i]))
				return false;
			else
				++i;
			break;
		}
	}
	return true;
}

template <class CharT>
auto Condition<CharT>::match_prefix(const StrT& s) const -> bool
{
	return match(s, 0, length);
}

template <class CharT>
auto Condition<CharT>::match_suffix(const StrT& s) const -> bool
{
	if (length > s.size())
		return false;
	return match(s, s.size() - length, length);
}

template class Condition<char>;
template class Condition<wchar_t>;
} // namespace nuspell
