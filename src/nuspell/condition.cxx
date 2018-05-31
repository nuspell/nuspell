/* Copyright 2018 Sander van Geloven
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
 * @param cond string containing simplifed regular expression to construct this
 * Condition object for.
 */
template <class CharT>
Condition<CharT>::Condition(const StrT& cond) : c(cond)
{
	// TODO Throw exception when cond has empty selections "[]" or empty
	// exceptions "[^]".
}

/** Checks if provided string matched the condition.
 *
 * @param s string to check if it matches the condition.
 * @return The valueof true when string matched condition.
 */
template <class CharT>
auto Condition<CharT>::match(const StrT& s) const -> bool
{
	return false;
}

template class Condition<char>;
template class Condition<wchar_t>;
template class Condition<char16_t>;
} // namespace nuspell
