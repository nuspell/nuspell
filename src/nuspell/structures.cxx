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

#include "structures.hxx"

#include <algorithm>
#include <type_traits>

namespace hunspell {

using namespace std;

void Flag_Set::sort_uniq()
{
	auto first = flags.begin();
	auto last = flags.end();
	sort(first, last);
	flags.erase(unique(first, last), last);
}

Flag_Set::Flag_Set(const std::u16string& s) : flags(s) { sort_uniq(); }

Flag_Set::Flag_Set(std::u16string&& s) : flags(move(s)) { sort_uniq(); }

auto Flag_Set::operator=(const std::u16string& s) -> Flag_Set&
{
	flags = s;
	sort_uniq();
	return *this;
}

auto Flag_Set::operator=(std::u16string&& s) -> Flag_Set&
{
	flags = move(s);
	sort_uniq();
	return *this;
}

auto Flag_Set::insert(const std::u16string& s) -> void
{
	flags += s;
	sort_uniq();
}

bool Flag_Set::erase(char16_t flag)
{
	auto i = flags.find(flag);
	if (i != flags.npos) {
		flags.erase(i, 1);
		return true;
	}
	return false;
}

static_assert(is_move_constructible<Flag_Set>::value == true,
              "Flag Set Not move constructable");
static_assert(is_move_assignable<Flag_Set>::value == true,
              "Flag Set Not move assingable");
}
