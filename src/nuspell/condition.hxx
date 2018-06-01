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

#ifndef NUSPELL_CONDITION_HXX
#define NUSPELL_CONDITION_HXX

#include <string>
#include <tuple>
#include <vector>

namespace nuspell {

enum Condition_Type {
	NORMAL /**< normal character */,
	DOT /**< wildcard character */,
	ANY_OF /**< set of possible characters */,
	NONE_OF /**< set of excluding characters */
};

/**
 * A class providing implementation for limited regular expression matching.
 */
template <class CharT>
class Condition {
      public:
	using StrT = std::basic_string<CharT>;
	template <class T, class U, class V>
	using tuple = std::tuple<T, U, V>;
	template <class T>
	using vector = std::vector<T>;

      private:
	const StrT cond;
	vector<tuple<size_t, size_t, Condition_Type>> spans; // pos, len, type
	size_t length;

      public:
	Condition(const StrT& condition);
	auto match(const StrT& s, const size_t& pos = 0, const size_t& len) const -> bool; // could not use "len = npos"
};
} // namespace nuspell
#endif // NUSPELL_CONDITION_HXX
