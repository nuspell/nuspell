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

#include "string_utils.hxx"

namespace nuspell {

using namespace std;

namespace {
template <class CharT>
auto is_number_regex = basic_regex<CharT>(LITERAL(CharT, R"(-?\d([,.-]?\d)*)"));
}

/**
 * Tests if word is a number.
 */
template <class CharT>
auto is_number(const std::basic_string<CharT>& s) -> bool
{
	return std::regex_match(s, is_number_regex<CharT>);
}
template auto is_number<>(const string& s) -> bool;
template auto is_number<>(const wstring& s) -> bool;
}
