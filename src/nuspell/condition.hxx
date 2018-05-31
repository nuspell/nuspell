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

#ifndef NUSPELL_CONDITION_HXX
#define NUSPELL_CONDITION_HXX

#include <string>

namespace nuspell {

/**
 * A class providing implementation for limited regular expression matching.
 */
template <class CharT>
class Condition {
      public:
	using StrT = std::basic_string<CharT>;

      private:
	const StrT c;

      public:
	Condition(const StrT& cond);
	auto match(const StrT& s) const -> bool;
};
} // namespace nuspell
#endif // NUSPELL_CONDITION_HXX
