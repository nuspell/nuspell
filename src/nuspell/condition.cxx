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

#include "condition.hxx"
#include "string_utils.hxx"

#include <stdexcept>

namespace nuspell {

using namespace std;

/** Constructs a Condition object.
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
			auto what = string("Cannot construct Condition without "
			                   "opening bracket for condition ");
			throw invalid_argument(what);
		}
		if (cond[i] == '[') {
			++i;
			if (i == cond.size()) {
				auto what =
				    string("Cannot construct Condition without "
				           "closing bracket for condition ");
				throw invalid_argument(what);
			}
			Condition_Type type;
			if (cond[i] == '^') {
				type = NONE_OF;
				++i;
			}
			else {
				type = ANY_OF;
			}
			j = cond.find(']', i);
			if (j == i) {
				auto what =
				    string("Cannot construct Condition with "
				           "empty brackets for condition ");
				throw invalid_argument(what);
			}
			if (j == cond.npos) {
				auto what =
				    string("Cannot construct Condition without "
				           "closing bracket for condition ");
				throw invalid_argument(what);
			}
			spans.emplace_back(i, j - i, type);
			++length;
			i = j + 1;
		}
	}
}

/** Checks if provided string matched the condition.
 *
 * @param s string to check if it matches the condition.
 * @param pos start position for string, default is 0.
 * @param len length of string counting from the start position. Question:
 * correct?
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
		size_t x_pos =
		    get<0>(x); // can this also be x->pos and x-> len ?
		size_t x_len = get<1>(x);
		Condition_Type x_type = get<2>(x);

		switch (x_type) {
		case NORMAL:
			if (s.compare(i, x_len, cond, x_pos, x_len) == 0)
				i += x_len;
			else
				return false;
			break;
		case DOT:
			++i;
			break;
		case ANY_OF:

			if (StrT::traits_type::find(cond.data() + x_pos, x_len,
			                            s[i]) != nullptr)
				++i;
			else
				return false;
			break;
		case NONE_OF:
			if (StrT::traits_type::find(cond.data() + x_pos, x_len,
			                            s[i]) != nullptr)
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
