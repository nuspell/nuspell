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

TEST_CASE("get_all_paths", "[finder]")
{
	// values are highly system dependent!
	// use only for debugging or code coverage
	auto f = Finder::search_dictionaries_in_all_paths();
	auto ps = f.get_all_paths();
	if (ps.empty()) {
		CHECK(0 == ps.size());
        }
        else {
		auto amount = 0;
                for (auto& p : ps) {
			auto dummy = string(p);
			dummy.clear();
			++amount;
                }
		CHECK(amount == ps.size());
        }
}

TEST_CASE("get_all_dictionaries", "[finder]")
{
	// values are highly system dependent!
	// use only for debugging or code coverage
	auto f = Finder::search_dictionaries_in_all_paths();
	auto ds = f.get_all_dictionaries();
	if (ds.empty()) {
		CHECK(0 == ds.size());
        }
        else {
		auto amount = 0;
                for (auto& d : ds) {
			auto dummy = string(d.first);
			dummy.clear();
			dummy = string(d.second);
			dummy.clear();
			++amount;
                }
		CHECK(amount == ds.size());
        }
}

TEST_CASE("find", "[finder]")
{
	// values are highly system dependent!
	// use only for debugging or code coverage
	auto f = Finder::search_dictionaries_in_all_paths();
	auto res = f.find("");
	CHECK(0 != typeid(res).name());
	res = f.find("64852985806485298580");
	CHECK(0 != typeid(res).name());
}

TEST_CASE("equal_range", "[finder]")
{
	// values are highly system dependent!
	// use only for debugging or code coverage
	auto f = Finder::search_dictionaries_in_all_paths();
	auto res = f.equal_range("");
	CHECK(0 != typeid(res.first).name());
	CHECK(0 != typeid(res.second).name());
	res = f.equal_range("64852985806485298580");
	CHECK(0 != typeid(res.first).name());
	CHECK(0 != typeid(res.second).name());
}

TEST_CASE("get_dictionary", "[finder]")
{
	// values are highly system dependent!
	// use only for debugging or code coverage
	auto f = Finder::search_dictionaries_in_all_paths();
	auto res = f.get_dictionary("");
	CHECK(0 == res.size());
	res = f.get_dictionary("64852985806485298580");
	CHECK(0 == res.size());
}
