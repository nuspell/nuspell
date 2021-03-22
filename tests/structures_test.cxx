/* Copyright 2018-2020 Sander van Geloven, Dimitrij Mijoski
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

#include <nuspell/structures.hxx>

#include <catch2/catch.hpp>

using namespace std;
using namespace nuspell;

TEST_CASE("String_Set::String_Set", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>(u16string(u"abc"));
	auto ss3 = String_Set<char16_t>(u"abc");

	CHECK(0 == ss1.size());
	CHECK(ss2 == ss3);
	CHECK(u"abc" == ss2.data());
}

TEST_CASE("String_Set::operator=", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>(u"abc");
	ss1 = u"abc";
	CHECK(ss1 == ss2);
	auto s = u16string(u"abc");
	ss1 = s;
	CHECK(ss1 == ss2);
}

TEST_CASE("String_Set::size", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	CHECK(ss1.empty());
	ss1.insert(u"abc");
	ss1.insert(u"def");
	ss1.insert(u"ghi");
	CHECK_FALSE(ss1.empty());
	CHECK(9 == ss1.size());
	CHECK(1024 < ss1.max_size());
	ss1.clear();
	CHECK(ss1.empty());
}

TEST_CASE("String_Set::begin end", "[structures]")
{
	using U16Str_Set = String_Set<char16_t>;
	auto ss1 = U16Str_Set();
	ss1.insert(u"aa");
	ss1.insert(u"bb");
	CHECK(ss1 == U16Str_Set(u"ab"));

	auto b = ss1.begin();
	auto e = ss1.end();
	b++;
	b++;
	CHECK(b == e);

	auto rb = ss1.rbegin();
	auto re = ss1.rend();
	rb++;
	rb++;
	CHECK(rb == re);

	auto cb = ss1.cbegin();
	auto ce = ss1.cend();
	cb++;
	cb++;
	CHECK(cb == ce);

	auto crb = ss1.crbegin();
	auto cre = ss1.crend();
	crb++;
	crb++;
	CHECK(crb == cre);

	auto lba = ss1.lower_bound('a');
	CHECK(lba == ss1.begin());
	CHECK(*lba == 'a');

	auto lbb = ss1.lower_bound('b');
	CHECK(lbb == ss1.begin() + 1);
	CHECK(*lbb == 'b');

	auto uba = ss1.upper_bound('a');
	CHECK(uba == ss1.begin() + 1);
	CHECK(*uba == 'b');

	auto ubb = ss1.upper_bound('b');
	CHECK(ubb == ss1.end());

	auto res = ss1.find('b');
	CHECK(res == e - 1);
	CHECK(*res == 'b');

	auto p = ss1.equal_range('b');
	CHECK(p.first == e - 1);
	CHECK(p.second == e);
}

TEST_CASE("String_Set manipulation", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>(u"abc");
	ss1 = u"abc";

	CHECK(ss1 == ss2);
	ss1.erase(ss1.begin());
	auto ss3 = String_Set<char16_t>(u"bc");
	CHECK(ss1 == ss3);

	ss1.clear();
	CHECK(ss1 != ss3);
	CHECK(ss1.empty());

	ss2.erase(ss2.begin(), ss2.end());
	CHECK(ss2.empty());

	ss2.insert(u"abc");
	auto res = ss2.insert(ss2.find('b'), 'x');
	CHECK(*(ss2.begin() + 3) == *(res));
}

TEST_CASE("String_Set comparison", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>();
	auto ss3 = String_Set<char16_t>();
	auto ss4 = String_Set<char16_t>();
	ss1.insert(u"abc");
	ss2.insert(u"abc");
	ss3.insert(u"abcd");
	ss4.insert(u"abcd");
	CHECK(ss1 == ss2);
	CHECK(ss3 == ss4);
	CHECK(ss1 != ss4);
	CHECK(ss3 != ss2);
	CHECK(ss1 < ss3);
	CHECK(ss4 > ss2);
	CHECK(ss1 <= ss2);
	CHECK(ss1 <= ss3);
	CHECK(ss3 >= ss4);
	CHECK(ss3 >= ss1);
	ss1.swap(ss3);
	CHECK(ss1 == ss4);
	CHECK(ss2 == ss3);
	CHECK(ss1 != ss2);
	CHECK(ss3 != ss4);
	CHECK(1 == ss3.count('c'));
	ss3.insert(u"c");
	ss3.insert(u"c");
	CHECK(1 == ss3.count('c'));
	CHECK(0 == ss3.count('z'));
}

TEST_CASE("Substr_Replacer", "[structures]")
{
	auto rep = Substr_Replacer({{"aa", "bb"},
	                            {"c", "d"},
	                            {"ee", "f"},
	                            {"g", "hh"},
	                            {"ii  ", ""},
	                            {"jj kk", "ll"},
	                            {"", "mm"},
	                            {" nn", ""}});
	CHECK(rep.replace_copy("aa XYZ c ee g ii jj kk nn") ==
	      "bb XYZ d f hh ii ll");
}

TEST_CASE("Break_Table", "[structures]")
{
	auto a = Break_Table();
	auto b = Break_Table({"--", "-"});
	auto c = Break_Table({"--", "-"});
	auto d = Break_Table();
	d = {"-", "--"};
	auto e = Break_Table();
	e = {"-", "--"};
	a = d;
}

TEST_CASE("Condition 1", "[structures]")
{
	auto c1 = Condition("");
	CHECK(c1.match_prefix(""));
	CHECK(c1.match_prefix("a"));

	auto c2 = Condition("a");
	CHECK(c2.match_prefix("a"));
	CHECK(c2.match_prefix("aa"));
	CHECK(c2.match_prefix("ab"));
	CHECK(c2.match_prefix("aba"));
	CHECK_FALSE(c2.match_prefix(""));
	CHECK_FALSE(c2.match_prefix("b"));
	CHECK_FALSE(c2.match_prefix("ba"));
	CHECK_FALSE(c2.match_prefix("bab"));

	auto c3 = Condition("ba");
	CHECK(c3.match_prefix("ba"));
	CHECK(c3.match_prefix("bab"));
	CHECK_FALSE(c3.match_prefix(""));
	CHECK_FALSE(c3.match_prefix("b"));
	CHECK_FALSE(c3.match_prefix("a"));
	CHECK_FALSE(c3.match_prefix("aa"));
	CHECK_FALSE(c3.match_prefix("ab"));
	CHECK_FALSE(c3.match_prefix("aba"));

	CHECK(c3.match_suffix("ba"));
	CHECK(c3.match_suffix("aba"));
	CHECK_FALSE(c3.match_suffix(""));
	CHECK_FALSE(c3.match_suffix("a"));
	CHECK_FALSE(c3.match_suffix("b"));
	CHECK_FALSE(c3.match_suffix("aa"));
	CHECK_FALSE(c3.match_suffix("ab"));
	CHECK_FALSE(c3.match_suffix("bab"));
}

TEST_CASE("Condition with wildcards", "[structures]")
{
	auto c1 = Condition(".");
	CHECK_FALSE(c1.match_prefix(""));
	CHECK(c1.match_prefix("a"));
	CHECK(c1.match_prefix("b"));
	CHECK(c1.match_prefix("aa"));
	CHECK(c1.match_prefix("ab"));
	CHECK(c1.match_prefix("ba"));
	CHECK(c1.match_prefix("bab"));
	CHECK(c1.match_prefix("aba"));

	auto c2 = Condition("..");
	CHECK_FALSE(c2.match_prefix(""));
	CHECK_FALSE(c2.match_prefix("a"));
	CHECK_FALSE(c2.match_prefix("b"));
	CHECK(c2.match_prefix("aa"));
	CHECK(c2.match_prefix("ab"));
	CHECK(c2.match_prefix("ba"));
	CHECK(c2.match_prefix("bab"));
	CHECK(c2.match_prefix("aba"));
}

TEST_CASE("Condition exceptions", "[structures]")
{
	auto cond1 = "]";
	CHECK_THROWS_AS(Condition(cond1), Condition_Exception);
	CHECK_THROWS_WITH(Condition(cond1),
	                  "closing bracket has no matching opening bracket");

	auto cond2 = "ab]";
	CHECK_THROWS_AS(Condition(cond2), Condition_Exception);
	CHECK_THROWS_WITH(Condition(cond2),
	                  "closing bracket has no matching opening bracket");

	auto cond3 = "[ab";
	CHECK_THROWS_AS(Condition(cond3), Condition_Exception);
	CHECK_THROWS_WITH(Condition(cond3),
	                  "opening bracket has no matching closing bracket");

	auto cond4 = "[";
	CHECK_THROWS_AS(Condition(cond4), Condition_Exception);
	CHECK_THROWS_WITH(Condition(cond4),
	                  "opening bracket has no matching closing bracket");

	auto cond5 = "[]";
	CHECK_THROWS_AS(Condition(cond5), Condition_Exception);
	CHECK_THROWS_WITH(Condition(cond5), "empty bracket expression");

	auto cond6 = "[^]";
	CHECK_THROWS_AS(Condition(cond6), Condition_Exception);
	CHECK_THROWS_WITH(Condition(cond6), "empty bracket expression");
}

TEST_CASE("Condition 2", "[structures]")
{
	auto c1 = Condition("[ab]");
	CHECK(c1.match_prefix("a"));
	CHECK(c1.match_prefix("b"));
	CHECK(c1.match_prefix("ax"));
	CHECK(c1.match_prefix("bb"));
	CHECK_FALSE(c1.match_prefix("c"));
	CHECK_FALSE(c1.match_prefix("cx"));

	auto c2 = Condition("[^ab]");
	CHECK_FALSE(c2.match_prefix("a"));
	CHECK_FALSE(c2.match_prefix("b"));
	CHECK_FALSE(c2.match_prefix("ax"));
	CHECK_FALSE(c2.match_prefix("bx"));
	CHECK(c2.match_prefix("c"));
	CHECK(c2.match_prefix("cx"));

	// not default regex behaviour for hyphen
	auto c3 = Condition("[a-c]");
	CHECK(c3.match_prefix("a"));
	CHECK(c3.match_prefix("-"));
	CHECK(c3.match_prefix("c"));
	CHECK_FALSE(c3.match_prefix("b"));

	// not default regex behaviour for hyphen
	auto c4 = Condition("[^a-c]");
	CHECK_FALSE(c4.match_prefix("a"));
	CHECK_FALSE(c4.match_prefix("-"));
	CHECK_FALSE(c4.match_prefix("c"));
	CHECK(c4.match_prefix("b"));
}

TEST_CASE("Condition unicode", "[structures]")
{
	auto c1 = Condition("áåĳßøæ");
	CHECK(c1.match_prefix("áåĳßøæ"));
	CHECK_FALSE(c1.match_prefix("ñ"));
}

TEST_CASE("Condition real-life examples", "[structures]")
{
	// found 2 times in affix files
	auto c1 = Condition("[áéiíóőuúüűy-àùø]");
	CHECK(c1.match_prefix("á"));
	CHECK(c1.match_prefix("é"));
	CHECK(c1.match_prefix("i"));
	CHECK(c1.match_prefix("í"));
	CHECK(c1.match_prefix("ó"));
	CHECK(c1.match_prefix("ő"));
	CHECK(c1.match_prefix("u"));
	CHECK(c1.match_prefix("ú"));
	CHECK(c1.match_prefix("ü"));
	CHECK(c1.match_prefix("ű"));
	CHECK(c1.match_prefix("y"));
	CHECK(c1.match_prefix("-"));
	CHECK(c1.match_prefix("à"));
	CHECK(c1.match_prefix("ù"));
	CHECK(c1.match_prefix("ø"));
	CHECK_FALSE(c1.match_prefix("ñ"));

	// found 850 times in affix files
	auto c2 = Condition("[cghjmsxyvzbdfklnprt-]");
	CHECK(c2.match_prefix("c"));
	CHECK(c2.match_prefix("-"));
	CHECK_FALSE(c2.match_prefix("ñ"));

	// found 744 times in affix files
	auto c3 = Condition("[áéiíóőuúüűy-àùø]");
	CHECK(c3.match_prefix("ő"));
	CHECK(c3.match_prefix("-"));
	CHECK_FALSE(c3.match_prefix("ñ"));

	// found 8 times in affix files
	auto c4 = Condition("[^-]");
	CHECK(c4.match_prefix("a"));
	CHECK(c4.match_prefix("b"));
	CHECK(c4.match_prefix("^"));
	CHECK_FALSE(c4.match_prefix("-"));

	// found 4 times in affix files
	auto c5 = Condition("[^cts]Z-");
	CHECK(c5.match_prefix("aZ-"));
	CHECK_FALSE(c5.match_prefix("cZ-"));
	CHECK_FALSE(c5.match_prefix("Z-"));

	// found 2 times in affix files
	auto c6 = Condition("[^cug^-]er");
	CHECK_FALSE(c6.match_prefix("^er"));
	CHECK(c6.match_prefix("_er"));

	// found 74 times in affix files
	auto c7 = Condition("[^дж]ерти");
	CHECK(c7.match_prefix("рерти"));
	CHECK(c7.match_prefix("рерти123"));
	CHECK(c7.match_suffix("123рерти"));

	CHECK_FALSE(c7.match_prefix("ерти"));

	CHECK_FALSE(c7.match_prefix("дерти"));
	CHECK_FALSE(c7.match_prefix("дерти123"));
	CHECK_FALSE(c7.match_suffix("123дерти"));

	CHECK_FALSE(c7.match_prefix("жерти"));
}

TEST_CASE("Prefix", "[structures]")
{
	auto pfx_tests = Prefix{u'U', true, "", "un", {}, Condition("wr.")};
	auto word = string("unwry");
	CHECK("wry" == pfx_tests.to_root(word));
	CHECK("wry" == word);

	word = string("unwry");
	CHECK("wry" == pfx_tests.to_root_copy(word));
	CHECK("unwry" == word);

	word = string("wry");
	CHECK("unwry" == pfx_tests.to_derived(word));
	CHECK("unwry" == word);

	word = string("wry");
	CHECK("unwry" == pfx_tests.to_derived_copy(word));
	CHECK("wry" == word);

	CHECK(pfx_tests.check_condition("wry"));
	CHECK_FALSE(pfx_tests.check_condition("unwry"));
}

TEST_CASE("Suffix", "[structures]")
{
	auto sfx_tests =
	    Suffix{u'T', true, "y", "ies", {}, Condition(".[^aeiou]y")};
	auto word = string("wries");
	CHECK("wry" == sfx_tests.to_root(word));
	CHECK("wry" == word);

	word = string("wries");
	CHECK("wry" == sfx_tests.to_root_copy(word));
	CHECK("wries" == word);

	word = string("wry");
	CHECK("wries" == sfx_tests.to_derived(word));
	CHECK("wries" == word);

	word = string("wry");
	CHECK("wries" == sfx_tests.to_derived_copy(word));
	CHECK("wry" == word);

	CHECK(sfx_tests.check_condition("wry"));
	CHECK_FALSE(sfx_tests.check_condition("ey"));
	CHECK_FALSE(sfx_tests.check_condition("wries"));
}

TEST_CASE("Prefix_Multiset")
{
	auto set =
	    Prefix_Multiset<string>{"",     "a",   "",   "ab",   "abx", "as",
	                            "asdf", "axx", "as", "bqwe", "ba",  "rqwe"};
	auto expected = vector<string>{"", "", "a", "as", "as", "asdf"};
	auto out = vector<string>();
	set.copy_all_prefixes_of("asdfg", back_inserter(out));
	CHECK(out == expected);

	auto word = string("asdfg");
	auto it = set.iterate_prefixes_of(word);
	out.assign(begin(it), end(it));
	REQUIRE(out == expected);
}

TEST_CASE("Suffix_Multiset")
{
	auto set =
	    Suffix_Multiset<string>{"",   "",   "a",   "b",   "b",   "ab",
	                            "ub", "zb", "aub", "uub", "xub", "huub"};
	auto expected = vector<string>{"", "", "b", "b", "ub", "uub", "huub"};
	auto out = vector<string>();
	auto word = string("ahahuub");
	set.copy_all_prefixes_of(word, back_inserter(out));
	CHECK(out == expected);

	auto it = set.iterate_prefixes_of(word);
	out.assign(begin(it), end(it));
	REQUIRE(out == expected);
}

TEST_CASE("String_Pair", "[structures]")
{
	auto x = String_Pair();
	CHECK(x.str() == "");
	CHECK(x.idx() == 0);
	CHECK(x.first() == "");
	CHECK(x.second() == "");

	x.first("123qwe");
	CHECK(x.str() == "123qwe");
	CHECK(x.idx() == 6);
	CHECK(x.first() == "123qwe");
	CHECK(x.second() == "");

	x.second("456z");
	CHECK(x.str() == "123qwe456z");
	CHECK(x.idx() == 6);
	CHECK(x.first() == "123qwe");
	CHECK(x.second() == "456z");

	x = String_Pair(string("6789"), string("zxcvbnm"));
	CHECK(x.str() == "6789zxcvbnm");
	CHECK(x.idx() == 4);
	CHECK(x.first() == "6789");
	CHECK(x.second() == "zxcvbnm");

	x = String_Pair("6789zxcvbnm", 4);
	CHECK(x.str() == "6789zxcvbnm");
	CHECK(x.idx() == 4);
	CHECK(x.first() == "6789");
	CHECK(x.second() == "zxcvbnm");

	CHECK_NOTHROW(String_Pair("6789", 4));
	CHECK_THROWS_AS(String_Pair("6789", 5), std::out_of_range);
	CHECK_THROWS_WITH(String_Pair("6789", 5), "word split is too long");
}

TEST_CASE("match_simple_regex", "[structures]")
{
	CHECK(match_simple_regex("abdff"s, "abc?de*ff"s));
	CHECK(match_simple_regex("abcdff"s, "abc?de*ff"s));
	CHECK(match_simple_regex("abdeeff"s, "abc?de*ff"s));
	CHECK(match_simple_regex("abcdeff"s, "abc?de*ff"s));
	CHECK_FALSE(match_simple_regex("abcdeeeefff"s, "abc?de*ff"s));
	CHECK_FALSE(match_simple_regex("abccdeeeeff"s, "abc?de*ff"s));
	CHECK_FALSE(match_simple_regex("qwerty"s, "abc?de*ff"s));
}

TEST_CASE("Similarity_Group", "[structures]")
{
	auto s1 = Similarity_Group();
	s1.parse("a(bb)");
	s1.parse("c(dd"); // non-regular
	s1.parse("e(f)");
	s1.parse(")"); // non-regular
	auto s2 = Similarity_Group();
	s2 = "(bb)a";
	s2 = "c(dd"; // non-regular
	s2 = "e";
	s2 = "f)"; // non-regular
	CHECK(s2.strings == s1.strings);
	CHECK(s2.chars == s1.chars);
	auto exp = string("acef)");
	CHECK(exp == s1.chars);
	auto v = vector<string>();
	v.push_back("bb");
	CHECK(v == s1.strings);
}

TEST_CASE("Phonetic_Table", "[structures]")
{
	auto p1 = pair<string, string>({"CC", "_"});
	auto p2 = pair<string, string>({"AA", "BB"});
	auto p3 = pair<string, string>({"AH(AEIOUY)-^", "*H"});
	auto p4 = pair<string, string>({"A(HR)", "_"});
	auto p5 = pair<string, string>({"CC<", "C"});
	auto p6 = pair<string, string>({"", "BB"});
	auto p7 = pair<string, string>({"MB$", "M"});
	auto p8 = pair<string, string>({"GG9", "K"});
	auto v1 =
	    vector<pair<string, string>>({p1, p2, p3, p4, p5, p6, p7, p8});
	auto f1 = Phonetic_Table(v1);
	auto f2 = Phonetic_Table();
	f2 = v1;
	auto f3 = Phonetic_Table(v1);
	auto f4 = Phonetic_Table();
	auto f5 = Phonetic_Table();

	auto p9 = pair<string, string>({"AA(", "N"}); // bad rule
	auto v2 = vector<pair<string, string>>({p9});
	f5 = v2;
	auto word = string("AA");
	auto exp = string("AA");
	CHECK_FALSE(f5.replace(word));

	f5 = v1;
	word = string("AA");
	exp = string("BB");
	CHECK(f5.replace(word));
	CHECK(exp == word);

	word = string("CCF");
	exp = string("F");
	CHECK(f5.replace(word));
	CHECK(exp == word);

	word = string("AABB");
	exp = string("BBBB");
	CHECK(f5.replace(word));
	CHECK(exp == word);

	word = string("ABBA");
	exp = string(word);
	CHECK_FALSE(f4.replace(word));
	CHECK(exp == word);
	CHECK_FALSE(f5.replace(word));
	CHECK(exp == word);
}
