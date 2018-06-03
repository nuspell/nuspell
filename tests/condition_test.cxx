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

#include "catch.hpp"

#include <iostream>

#include "../src/nuspell/condition.hxx"
#include <boost/locale.hpp>

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method match characters with standard character <char>",
          "[condition]")
{
	auto c1 = Condition<char>("");
	CHECK(true == c1.match(""));
	CHECK(false == c1.match("a"));

	auto c2 = Condition<char>("a");
	CHECK(false == c2.match(""));
	CHECK(true == c2.match("a"));
	CHECK(false == c2.match("aa"));
	CHECK(false == c2.match("ab"));
	CHECK(false == c2.match("aba"));
	CHECK(false == c2.match("b"));
	CHECK(false == c2.match("ba"));
	CHECK(false == c2.match("bab"));

	CHECK(false == c2.match_prefix(""));
	CHECK(true == c2.match_prefix("a"));
	CHECK(true == c2.match_prefix("aa"));
	CHECK(true == c2.match_prefix("ab"));
	CHECK(true == c2.match_prefix("aba"));
	CHECK(false == c2.match_prefix("b"));
	CHECK(false == c2.match_prefix("ba"));
	CHECK(false == c2.match_prefix("bab"));

	CHECK(false == c2.match_suffix(""));
	CHECK(true == c2.match_suffix("a"));
	CHECK(true == c2.match_suffix("aa"));
	CHECK(false == c2.match_suffix("ab"));
	CHECK(true == c2.match_suffix("aba"));
	CHECK(false == c2.match_suffix("b"));
	CHECK(true == c2.match_suffix("ba"));
	CHECK(false == c2.match_suffix("bab"));

	auto c3 = Condition<char>("ba");
	CHECK(false == c3.match(""));
	CHECK(false == c3.match("b"));
	CHECK(true == c3.match("ba"));
	CHECK(false == c3.match("bab"));
	CHECK(false == c3.match("a"));
	CHECK(false == c3.match("aa"));
	CHECK(false == c3.match("ab"));
	CHECK(false == c3.match("aba"));

	CHECK(false == c3.match_prefix(""));
	CHECK(false == c3.match_prefix("b"));
	CHECK(true == c3.match_prefix("ba"));
	CHECK(true == c3.match_prefix("bab"));
	CHECK(false == c3.match_prefix("a"));
	CHECK(false == c3.match_prefix("aa"));
	CHECK(false == c3.match_prefix("ab"));
	CHECK(false == c3.match_prefix("aba"));

	CHECK(false == c3.match_suffix(""));
	CHECK(false == c3.match_suffix("b"));
	CHECK(true == c3.match_suffix("ba"));
	CHECK(false == c3.match_suffix("bab"));
	CHECK(false == c3.match_suffix("a"));
	CHECK(false == c3.match_suffix("aa"));
	CHECK(false == c3.match_suffix("ab"));
	CHECK(true == c3.match_suffix("aba"));
}

TEST_CASE("method match wildcards with wide character <wchar_t>", "[condition]")
{
	auto c1 = Condition<wchar_t>(L".");
	CHECK(false == c1.match_prefix(L""));
	CHECK(true == c1.match_prefix(L"a"));
	CHECK(true == c1.match_prefix(L"b"));
	CHECK(true == c1.match_prefix(L"aa"));
	CHECK(true == c1.match_prefix(L"ab"));
	CHECK(true == c1.match_prefix(L"ba"));
	CHECK(true == c1.match_prefix(L"bab"));
	CHECK(true == c1.match_prefix(L"aba"));

	auto c2 = Condition<wchar_t>(L"..");
	CHECK(false == c2.match_prefix(L""));
	CHECK(false == c2.match_prefix(L"a"));
	CHECK(false == c2.match_prefix(L"b"));
	CHECK(true == c2.match_prefix(L"aa"));
	CHECK(true == c2.match_prefix(L"ab"));
	CHECK(true == c2.match_prefix(L"ba"));
	CHECK(true == c2.match_prefix(L"bab"));
	CHECK(true == c2.match_prefix(L"aba"));
}

TEST_CASE("method match selections with runtime exceptions", "[condition]")
{
	auto cond1 = L"]";
	CHECK_THROWS_AS([&]() { auto c1 = Condition<wchar_t>(cond1); }(),
	                std::invalid_argument);
	CHECK_THROWS_WITH([&]() { auto c1 = Condition<wchar_t>(cond1); }(),
	                  "Closing bracket has no matching opening bracket.");

	auto cond2 = L"ab]";
	CHECK_THROWS_AS([&]() { auto c2 = Condition<wchar_t>(cond2); }(),
	                std::invalid_argument);
	CHECK_THROWS_WITH([&]() { auto c2 = Condition<wchar_t>(cond2); }(),
	                  "Closing bracket has no matching opening bracket.");

	auto cond3 = L"[ab";
	CHECK_THROWS_AS([&]() { auto c3 = Condition<wchar_t>(cond3); }(),
	                std::invalid_argument);
	CHECK_THROWS_WITH([&]() { auto c3 = Condition<wchar_t>(cond3); }(),
	                  "Opening bracket has no matching closing bracket.");

	auto cond4 = L"[";
	CHECK_THROWS_AS([&]() { auto c4 = Condition<wchar_t>(cond4); }(),
	                std::invalid_argument);
	CHECK_THROWS_WITH([&]() { auto c4 = Condition<wchar_t>(cond4); }(),
	                  "Opening bracket has no matching closing bracket.");

	auto cond5 = L"[]";
	CHECK_THROWS_AS([&]() { auto c5 = Condition<wchar_t>(cond5); }(),
	                std::invalid_argument);
	CHECK_THROWS_WITH([&]() { auto c5 = Condition<wchar_t>(cond5); }(),
	                  "Empty bracket expression.");

	auto cond6 = L"[^]";
	CHECK_THROWS_AS([&]() { auto c6 = Condition<wchar_t>(cond6); }(),
	                std::invalid_argument);
	CHECK_THROWS_WITH([&]() { auto c6 = Condition<wchar_t>(cond6); }(),
	                  "Empty bracket expression.");
}

TEST_CASE("method match selections with standard character <char>",
          "[condition]")
{
	auto c1 = Condition<char>("[ab]");
	CHECK(true == c1.match("a"));
	CHECK(true == c1.match("b"));
	CHECK(false == c1.match("c"));

	auto c2 = Condition<char>("[^ab]");
	CHECK(false == c2.match("a"));
	CHECK(false == c2.match("b"));
	CHECK(true == c2.match("c"));

	// not default regex behaviour for hyphen
	auto c3 = Condition<char>("[a-c]");
	CHECK(true == c3.match("a"));
	CHECK(true == c3.match("-"));
	CHECK(true == c3.match("c"));
	CHECK(false == c3.match("b"));

	// not default regex behaviour for hyphen
	auto c4 = Condition<char>("[^a-c]");
	CHECK(false == c4.match("a"));
	CHECK(false == c4.match("-"));
	CHECK(false == c4.match("c"));
	CHECK(true == c4.match("b"));
}

TEST_CASE("method match diacritics and ligatues", "[condition]")
{
	auto c1 = Condition<wchar_t>(L"áåĳßøæ");
	CHECK(true == c1.match(L"áåĳßøæ"));
	CHECK(false == c1.match(L"ñ"));
}

TEST_CASE("method match real-life combinations", "[condition]")
{
	// found 2 times in affix files
	auto c1 = Condition<wchar_t>(L"[áéiíóőuúüűy-àùø]");
	CHECK(true == c1.match(L"á"));
	CHECK(true == c1.match(L"é"));
	CHECK(true == c1.match(L"i"));
	CHECK(true == c1.match(L"í"));
	CHECK(true == c1.match(L"ó"));
	CHECK(true == c1.match(L"ő"));
	CHECK(true == c1.match(L"u"));
	CHECK(true == c1.match(L"ú"));
	CHECK(true == c1.match(L"ü"));
	CHECK(true == c1.match(L"ű"));
	CHECK(true == c1.match(L"y"));
	CHECK(true == c1.match(L"-"));
	CHECK(true == c1.match(L"à"));
	CHECK(true == c1.match(L"ù"));
	CHECK(true == c1.match(L"ø"));
	CHECK(false == c1.match(L"ñ"));

	// found 850 times in affix files
	auto c2 = Condition<wchar_t>(L"[cghjmsxyvzbdfklnprt-]");
	CHECK(true == c2.match(L"c"));
	CHECK(true == c2.match(L"-"));
	CHECK(false == c2.match(L"ñ"));

	// found 744 times in affix files
	auto c3 = Condition<wchar_t>(L"[áéiíóőuúüűy-àùø]");
	CHECK(true == c3.match(L"ő"));
	CHECK(true == c3.match(L"-"));
	CHECK(false == c3.match(L"ñ"));

	// found 8 times in affix files
	auto c4 = Condition<wchar_t>(L"[^-]");
	CHECK(true == c4.match(L"a"));
	CHECK(true == c4.match(L"b"));
	CHECK(true == c4.match(L"^"));
	CHECK(false == c4.match(L"-"));

	// found 4 times in affix files
	auto c5 = Condition<wchar_t>(L"[^cts]Z-");
	CHECK(true == c5.match(L"aZ-"));
	CHECK(false == c5.match(L"cZ-"));
	CHECK(false == c5.match(L"Z-"));

	// found 2 times in affix files
	auto c6 = Condition<wchar_t>(L"[^cug^-]er");
	CHECK(false == c6.match(L"^er"));
	CHECK(true == c6.match(L"_er"));

	// found 74 times in affix files
	auto c7 = Condition<wchar_t>(L"[^дж]ерти");
	CHECK(true == c7.match(L"рерти"));
	CHECK(true == c7.match_prefix(L"рерти123"));
	CHECK(true == c7.match_suffix(L"123рерти"));

	CHECK(false == c7.match(L"ерти"));

	CHECK(false == c7.match(L"дерти"));
	CHECK(false == c7.match_prefix(L"дерти123"));
	CHECK(false == c7.match_suffix(L"123дерти"));

	CHECK(false == c7.match(L"жерти"));
}
