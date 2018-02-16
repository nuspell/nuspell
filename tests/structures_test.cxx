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

#include "catch.hpp"

#include <iostream>

#include "../src/nuspell/structures.hxx"

using namespace std;
using namespace hunspell;

TEST_CASE("class Suffix_Entry", "[structures]")
{
	const char16_t* flag = u"T";
	auto sfx =
	    hunspell::Suffix_Entry(*u"T", true, "y"s, "ies"s, ".[^aeiou]y"s);

	SECTION("method to_root")
	{
		auto word = "berries"s;
		CHECK("berry"s == sfx.to_root(word));
		CHECK("berry"s == word);
	}

	SECTION("method to_root_copy")
	{
		auto word = "berries"s;
		CHECK("berry"s == sfx.to_root_copy(word));
		CHECK("berries"s == word);
	}

	SECTION("method to_derived")
	{
		auto word = "berry"s;
		CHECK("berries"s == sfx.to_derived(word));
		CHECK("berries"s == word);
	}

	SECTION("method to_derived_copy")
	{
		auto word = "berry"s;
		CHECK("berries"s == sfx.to_derived_copy(word));
		CHECK("berry"s == word);
	}

	SECTION("method check_condition")
	{
		CHECK(true == sfx.check_condition("berry"s));
		CHECK(false == sfx.check_condition("hey"s));
	}
}
