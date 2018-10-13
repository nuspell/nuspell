/* Copyright 2017-2018 Sander van Geloven
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

#include "../src/nuspell/finder.hxx"

using namespace std;
using namespace nuspell;

TEST_CASE("class finder", "[finder]")
{
	// Values of all sections below are highly system dependent!
	// Therefore, it is not possible to test these automatically.
	// Use this only for debugging and manual inspection.

	auto f = Finder::search_all_dirs_for_dicts();

	SECTION("get_paths")
	{
		auto ps = f.get_dir_paths();
		(void)ps;
	}

	SECTION("get_dictionaries")
	{
		auto ds = f.get_dictionaries();
		(void)ds;
	}

	SECTION("find")
	{
		auto i = f.find("");
		i = f.find("64852985806485298580");
		(void)i;
	}

	SECTION("equal_range")
	{
		auto r = f.equal_range("");
		r = f.equal_range("64852985806485298580");
		(void)r;
	}

	SECTION("get_dictionary")
	{
		auto s = f.get_dictionary("");
		s = f.get_dictionary("64852985806485298580");
		(void)s;
	}
}
