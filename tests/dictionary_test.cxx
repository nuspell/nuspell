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

#include "../src/nuspell/dictionary.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("class dictionary", "[dictionary]")
{
	// To run especially this test in the IDE:
	// set executable ch.catch
	// and set working directory to %{buildDir}/tests
	// (Probably move this info to the wiki and/or README.)
	auto dictionary = Dictionary("v1cmdline/base");

	SECTION("method spell")
	{
		// correct word without affixes
		CHECK(GOOD_WORD == dictionary.spell("sawyer"));
		// correct word with unused prefix U
		CHECK(GOOD_WORD == dictionary.spell("created"));
		// correct word with used suffix D
		// CHECK(GOOD_WORD == dictionary.spell("looked"));
		// incorrect word with used suffix D
		// CHECK(BAD_WORD == dictionary.spell("loooked"));
	}
}
