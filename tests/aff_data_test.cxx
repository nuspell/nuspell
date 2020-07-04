/* Copyright 2019-2020 Dimitrij Mijoski
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

#include <nuspell/aff_data.hxx>

#include <catch2/catch.hpp>
#include <iostream>
#include <sstream>

using namespace std;
using namespace nuspell;

TEST_CASE("class Encoding")
{
	auto e = Encoding();
	CHECK("" == e.value());
	CHECK("ISO8859-1" == e.value_or_default());
	CHECK_FALSE(e.is_utf8());

	e = Encoding("uTF8");
	CHECK("UTF-8" == e.value());
	CHECK("UTF-8" == e.value_or_default());
	CHECK(e.is_utf8());

	e = "MICROSOFT-CP1251";
	CHECK("CP1251" == e.value());
	CHECK("CP1251" == e.value_or_default());
	CHECK_FALSE(e.is_utf8());
}

TEST_CASE("Aff_Data::parse() error 1")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
# missing data, option header incomplete
BREAK
BREAK -
BREAK ^-
)";
	auto in = istringstream(str);
	CHECK_FALSE(Aff_Data().parse_aff(in));

	cerr.rdbuf(old);
}

TEST_CASE("Aff_Data::parse() error 2")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
# bad entry number, array-command header is corrupt
BREAK a
BREAK -
BREAK ^-
)";
	auto in = istringstream(str);
	CHECK_FALSE(Aff_Data().parse_aff(in));

	cerr.rdbuf(old);
}
#if 0
TEST_CASE("Aff_Data::parse() error 3", "[!mayfail]")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
# multiple table definitions
BREAK 2
BREAK -
BREAK ^-
BREAK 2
BREAK -
BREAK ^-
)";
	auto in = istringstream(str);
	CHECK_FALSE(Aff_Data().parse_aff(in));

	cerr.rdbuf(old);
}

TEST_CASE("Aff_Data::parse() error 4", "[!mayfail]")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
# table is corrupt, corrupted option name
BREAK 2
BREac -
BREAK ^-
)";
	auto in = istringstream(str);
	CHECK_FALSE(Aff_Data().parse_aff(in));

	cerr.rdbuf(old);
}

TEST_CASE("Aff_Data::parse() error 5", "[!mayfail]")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
# table is corrupt, too many definitions
BREAK 3
BREAK -
BREAK ^-
BREAK _
BREAK -$
)";
	auto in = istringstream(str);
	CHECK_FALSE(Aff_Data().parse_aff(in));

	cerr.rdbuf(old);
}

TEST_CASE("Aff_Data::parse() error 6", "[!mayfail]")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
# table is corrupt, too few definitions
BREAK 3
BREAK -
BREAK ^-
)";
	auto in = istringstream(str);
	CHECK_FALSE(Aff_Data().parse_aff(in));

	cerr.rdbuf(old);
}
#endif

TEST_CASE("Aff_Data::parse() utf8 long flag warning")
{
	auto cerr_buf = stringbuf();
	auto old = cerr.rdbuf(&cerr_buf);

	auto str = R"(
SET UTF-8
TRY անմիողտեըցկչյԱՆՄԻՈՂՏԵԸՑԿՉՅ
FLAG long

SFX AB Y 1
SFX AB 	0	ից	.

SFX DE Y 1
SFX DE 	0	իգ	.

SFX խխ Y 1
SFX խխ 	0	ագ	.
)";
	auto in = istringstream(str);
	CHECK(Aff_Data().parse_aff(in));

	auto warn_str = cerr_buf.str();
	using namespace Catch::Matchers;
	CHECK_THAT(warn_str,
	           StartsWith("Nuspell warning") && !Contains("error"));

	cerr.rdbuf(old);
}
