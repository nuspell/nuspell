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

#include "../src/nuspell/string_utils.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method split_on_any_of", "[string_utils]")
{
	auto in = "^abc;.qwe/zxc/"s;
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_on_any_of(in, ".;^/"s, back_inserter(out));
	CHECK(exp == out);
}

TEST_CASE("method split", "[string_utils]")
{
	// tests split(), also tests split_v

	auto in = ";abc;;qwe;zxc;"s;
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_v(in, ';', out);
	CHECK(exp == out);

	in = "<>1<>234<>qwe<==><><>";
	exp = {"", "1", "234", "qwe<==>", "", ""};
	split_v(in, "<>", out);
	CHECK(exp == out);
}

TEST_CASE("method split_first", "[string_utils]")
{
	auto in = "first\tsecond"s;
	auto first = "first"s;
	auto out = split_first(in, '\t');
	CHECK(first == out);

	in = "first"s;
	out = split_first(in, '\t');
	CHECK(first == out);

	in = "\tsecond"s;
	first = ""s;
	out = split_first(in, '\t');
	CHECK(first == out);

	in = ""s;
	first = ""s;
	out = split_first(in, '\t');
	CHECK(first == out);
}

TEST_CASE("method split_on_whitespace", "[string_utils]")
{
	// also tests _v variant

	auto in = "   qwe ert  \tasd "s;
	auto exp = vector<string>{"qwe", "ert", "asd"};
	auto out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);

	in = "   \t\r\n  "s;
	exp = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);
}

#if 0
TEST_CASE("method segmentation_word_break", "[string_utils]")
{
	auto in = "\x0d"s;
	auto exp = "CR"s;
	auto out = string();
	segmentation_word_break(in, out);
	CHECK(exp == out);

	in = "\x0b"s;
	exp = "Newline"s;
	out = string();
	segmentation_word_break(in, out);
	CHECK(exp == out);

	in = "\x30a2"s;
	exp = "Katakana"s;
	out = string();
	segmentation_word_break(in, out);
	CHECK(exp == out);

	in = "A\x30a2"s;
	exp = "Katakana"s;
	out = string();
	segmentation_word_break(in, out, 1);
	CHECK(exp == out);
}

TEST_CASE("method segmentation_preprocess_boundaries", "[string_utils]")
{
	auto in = "\r\n"s;
	auto exp =
	    vector<pair<int, string>>{make_pair(0, "CR"s), make_pair(1, "LF"s)};
	auto out = vector<pair<int, string>>();
	segmentation_preprocess_boundaries(in, out);
	CHECK(exp == out);

	in = "A\u0308A"s;
	exp = vector<pair<int, string>>{make_pair(0, "ALetter"s),
	                                make_pair(2, "ALetter"s)};
	out = vector<pair<int, string>>();
	segmentation_preprocess_boundaries(in, out);
	CHECK(exp == out);

	in = "\n\u2060"s;
	exp = vector<pair<int, string>>{make_pair(0, "LF"s),
	                                make_pair(1, "Format"s)};
	out = vector<pair<int, string>>();
	segmentation_preprocess_boundaries(in, out);
	CHECK(exp == out);

	in = "\x01\u0308\x01"s;
	exp = vector<pair<int, string>>{make_pair(0, "Other"s),
	                                make_pair(2, "Other"s)};
	out = vector<pair<int, string>>();
	segmentation_preprocess_boundaries(in, out);
	CHECK(exp == out);
}

TEST_CASE("method segmentation_word_breakables", "[string_utils]")
{
	auto in = "ABC"s;
	auto exp = vector<int>{1, 0, 0};
	auto out = vector<int>();
	segmentation_word_breakables(in, out);
	CHECK(exp == out);

	in = "Hello, world."s;
	// Perhaps use value of 2 for non-ALetters such as whitespace, numbers
	// and punctuation? This differs compared to reference implementation.
	exp = vector<int>{1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1};
	out = vector<int>();
	segmentation_word_breakables(in, out);
	CHECK(exp == out);

	in = "\x01\u0308\x01"s;
	exp = vector<int>{1, 0, 1};
	out = vector<int>();
	segmentation_word_breakables(in, out);
	CHECK(exp == out);
}

TEST_CASE("method segmentation_words", "[string_utils]")
{
	auto in = "The quick (“brown”) fox can’t jump 32.3 feet, right?"s;
	auto exp = vector<string>{
	    "The"s,  " "s, "quick"s, " "s, "("s,     "“"s,     "brown"s, "”"s,
	    ")"s,    " "s, "fox"s,   " "s, "can’t"s, " "s,     "jump"s,  " "s,
	    "32.3"s, " "s, "feet"s,  ","s, " "s,     "right"s, "?"s};
	auto out = vector<string>();
	segmentation_words(in, out);
	CHECK(exp == out);

	in = ""s;
	exp = vector<string>{};
	out = vector<string>();
	segmentation_words(in, out);
	CHECK(exp == out);
}

TEST_CASE("method parse_text", "[string_utils]")
{
	auto in =
	    "The quick (“brown”) fox can’t jump 32.3 feet, right? Or can it?\n\nLet's see."s;
	// This is the result we eventually aim for. Grouped non-ALettrs and
	// boolean for is the string needs spell checking, e.g. at least one
	// character is an ALetter. This could also be not part of the output
	// and tested just before checking is done.
	auto exp = vector<pair<bool, string>>{
	    make_pair(true, "The"s),   make_pair(false, " "s),
	    make_pair(true, "quick"s), make_pair(false, " (“"s),
	    make_pair(true, "brown"s), make_pair(false, "”) "s),
	    make_pair(true, "fox"s),   make_pair(false, " "s),
	    make_pair(true, "can’t"s), make_pair(false, " "s),
	    make_pair(true, "jump"s),  make_pair(false, " 32.3 "s),
	    make_pair(true, "feet"s),  make_pair(false, ", "s),
	    make_pair(true, "right"s), make_pair(false, "?"s),
	    make_pair(true, "Or"s),    make_pair(false, " "s),
	    make_pair(true, "can"s),   make_pair(false, " "s),
	    make_pair(true, "it"s),    make_pair(false, "?\n\n"s),
	    make_pair(true, "Let's"s), make_pair(false, " "s),
	    make_pair(true, "see"s),   make_pair(false, "."s)};
	auto out = vector<pair<bool, string>>();
	parse_text(in, out);
	CHECK(exp == out);

	in = ""s;
	exp = vector<pair<bool, string>>{};
	out = vector<pair<bool, string>>();
	parse_text(in, out);
	CHECK(exp == out);
}
#endif

TEST_CASE("method parse_on_whitespace", "[string_utils]")
{
	// also tests _v variant

	auto in = "   qwe ert  \tasd "s;
	auto exp = vector<string>{"   ", "qwe", " ", "ert", "  \t", "asd", " "};
	auto out = vector<string>();
	parse_on_whitespace_v(in, out);
	CHECK(exp == out);

	in = "   \t\r\n  "s;
	exp = vector<string>{in};
	parse_on_whitespace_v(in, out);
	CHECK(exp == out);
}

TEST_CASE("method is_number", "[string_utils]")
{
	CHECK_FALSE(is_number(""s));
	CHECK_FALSE(is_number("a"s));
	CHECK_FALSE(is_number("1a"s));
	CHECK_FALSE(is_number("a1"s));
	CHECK_FALSE(is_number(".a"s));
	CHECK_FALSE(is_number("a."s));
	CHECK_FALSE(is_number(",a"s));
	CHECK_FALSE(is_number("a,"s));
	CHECK_FALSE(is_number("-a"s));
	CHECK_FALSE(is_number("a-"s));

	CHECK_FALSE(is_number("1..1"s));
	CHECK_FALSE(is_number("1.,1"s));
	CHECK_FALSE(is_number("1.-1"s));
	CHECK_FALSE(is_number("1,.1"s));
	CHECK_FALSE(is_number("1,,1"s));
	CHECK_FALSE(is_number("1,-1"s));
	CHECK_FALSE(is_number("1-.1"s));
	CHECK_FALSE(is_number("1-,1"s));
	CHECK_FALSE(is_number("1--1"s));

	CHECK(is_number("1,1111"s));
	CHECK(is_number("-1,1111"s));
	CHECK(is_number("1,1111.00"s));
	CHECK(is_number("-1,1111.00"s));
	CHECK(is_number("1.1111"s));
	CHECK(is_number("-1.1111"s));
	CHECK(is_number("1.1111,00"s));
	CHECK(is_number("-1.1111,00"s));

	// below needs extra review

	CHECK(is_number("1"s));
	CHECK(is_number("-1"s));
	CHECK_FALSE(is_number("1-"s));

	CHECK_FALSE(is_number("1."s));
	CHECK_FALSE(is_number("-1."s));
	CHECK_FALSE(is_number("1.-"s));

	CHECK_FALSE(is_number("1,"s));
	CHECK_FALSE(is_number("-1,"s));
	CHECK_FALSE(is_number("1,-"s));

	CHECK(is_number("1.1"s));
	CHECK(is_number("-1.1"s));
	CHECK_FALSE(is_number("1.1-"s));

	CHECK(is_number("1,1"s));
	CHECK(is_number("-1,1"s));
	CHECK_FALSE(is_number("1,1-"s));

	CHECK_FALSE(is_number(".1"s));
	CHECK_FALSE(is_number("-.1"s));
	CHECK_FALSE(is_number(".1-"s));

	CHECK_FALSE(is_number(",1"s));
	CHECK_FALSE(is_number("-,1"s));
	CHECK_FALSE(is_number(",1-"s));
}

TEST_CASE("function match_simple_regex", "[string_utils]")
{
	CHECK(match_simple_regex("abdff"s, "abc?de*ff"s));
	CHECK(match_simple_regex("abcdff"s, "abc?de*ff"s));
	CHECK(match_simple_regex("abdeeff"s, "abc?de*ff"s));
	CHECK(match_simple_regex("abcdeff"s, "abc?de*ff"s));
	CHECK_FALSE(match_simple_regex("abcdeeeefff"s, "abc?de*ff"s));
	CHECK_FALSE(match_simple_regex("abccdeeeeff"s, "abc?de*ff"s));
	CHECK_FALSE(match_simple_regex("qwerty"s, "abc?de*ff"s));
}
