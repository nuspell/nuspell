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

namespace nuspell {

using namespace std;

/** Constructs a Condition object.
 *
 * @param condition string containing simplifed regular expression to construct this
 * Condition object for.
 */
template <class CharT>
Condition<CharT>::Condition(const StrT& condition) : cond(condition), length(0)
{
	spans = vector<tuple<size_t, size_t, Condition_Type>>();
	// TODO Throw exception when condition has empty selections "[]" or empty
	// exceptions "[^]".

	size_t i = 0;
	for (; i != cond.size();) {
		size_t j = cond.find_any_of(LITERAL(CharT, "[]."), i);
		if (i != j) {
			if (j == cond.npos) {
				spans.push_back(i, cond.size() - i, NORMAL);
				break;
			}
			spans.push_back(i, j - i, NORMAL);
			i = j;
		}
		if (cond[i] == '.') {
			spans.push_back(i, 1, DOT);
			++i;
			continue;
		}
		if (cond[i] == ']') {
			auto what = std::string("Cannot construct Condition without opening bracket for condition ") + cond;
			throw std::invalid_argument(what);
		}
		if (cond[i] == '[') {
			++i;
			if (i == cond.size()) {
				auto what = std::string("Cannot construct Condition without closing bracket for condition ") + cond;
				throw std::invalid_argument(what);
			}
			if (cond[i] == '^') {
				type = NONE_OF;
				++i;
			} else {
				type = ANY_OF;
			}
			j = cond.find(']', i);
			if (j == i) {
				auto what = std::String("Cannot construct Condition with empty brackets for condition ") + cond;
				throw std::invalid_argument(what);
			}
			if ( j == cond.npos) {
				auto what = std::String("Cannot construct Condition without closing bracket for condition ") + cond;
				throw std::invalid_argument(what);
			}
			spans.push_back(i, j - i, type);
			i = j + 1;
		}
	}
	for (std::vector<tuple<size_t, size_t, Condition_Type>>::const_iterator x = spans.begin(); x != spans.end(); ++x) {
		size_t x_len = std::get<1>(x);
		length += x_len;
	}
}

/** Checks if provided string matched the condition.
 *
 * @param s string to check if it matches the condition.
 * @param pos start position for string, default is 0.
 * @param len length of string counting from the start position. Question: correct?
 * @return The valueof true when string matched condition.
 */
template <class CharT>
auto Condition<CharT>::match(const StrT& s, const size_t& pos, const size_t& len) const -> bool
{
	if (pos >= s.size()) {
		//Question: should we calculate more precise what length of condition is?
		auto what = std::string("Cannot match Condition when start position ") + pos + " for string " + s + " is greater than length of condition " + cond;
		throw std::length_error(what);
	}
	if (s.size() - pos < len)
		len = s.size() - pos;
	if (len != length)
		return false;

	size_t i = pos;

	for (std::vector<tuple<size_t, size_t, Condition_Type>>::const_iterator x = spans.begin(); x != spans.end(); ++x) {
		size_t x_pos = std::get<0>(x);
		size_t x_len = std::get<1>(x);
		Condition_Type x_type = std::get<2>(x);

		switch(x_type) { // [2] is type
		case NORMAL:
			if (s.compare(i, x_len, cond, x_pos, x_len) == 0)
				i += x.len;
			else
				return false;
			break;
		case DOT:
			++i;
			break;
		case ANY_OF:
			if (cond.find(s[i], x_pos, x_len) != s.npos)
				++i;
			else
				return false;
			break;
		case NONE_OF:
			if (cond.find(s[i], x_pos, x_len) != s.npos)
				return false;
			else
				++i;
			break;
		}
	}
	return true;
}

template class Condition<char>;
template class Condition<wchar_t>;
template class Condition<char16_t>;
} // namespace nuspell
