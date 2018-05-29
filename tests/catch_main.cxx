/* Copyright 2017 Dimitrij Mijoski
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

#include "string_utils.hxx"

namespace nuspell {

using namespace std;

namespace {
template <class CharT>

/**
 * Tests with a regular expression if word is a number.
 */
auto is_number_regex = basic_regex<CharT>(LITERAL(CharT, R"(-?\d([,.-]?\d)*)"));
}

/**
 * Tests if word is a number.
 *
 * Allow numbers with dots ".", dashes "-" and commas ",", but forbid double
 * separators such as "..", "--" and ".,".
 */
template <class CharT>
auto is_number(const std::basic_string<CharT>& s) -> bool
{
#if DEV_IS_NUMBER_REGEX
	return std::regex_match(s, is_number_regex<CharT>);
#else
	// see also strings_utils_test.cpp
	enum { NBEGIN, NNUM, NSEP };
	int nstate = NBEGIN;
	size_t i;

	for (i = 0; (i < s.size()); i++) {
		if ((s[i] <= '9') && (s[i] >= '0')) {
			nstate = NNUM;
		}
		else if ((s[i] == ',') || (s[i] == '.') || (s[i] == '-')) {
			if ((nstate == NSEP) || (i == 0))
				break;
			nstate = NSEP;
		}
		else
			break;
	}
	if ((i == s.size()) && (nstate == NNUM))
		return true;

	return false;
#endif
}
template auto is_number<>(const string& s) -> bool;
template auto is_number<>(const wstring& s) -> bool;
} // namespace nuspell
