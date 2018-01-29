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

#include <boost/utility/string_ref.hpp>

namespace hunspell {

using namespace std;

template <class Container>
void sort_uniq(Container& c)
{
	auto first = begin(c);
	auto last = end(c);
	sort(first, last);
	c.erase(unique(first, last), last);
}

void Flag_Set::sort_uniq() { hunspell::sort_uniq(flags); }

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

auto Flag_Set::erase(char16_t flag) -> bool
{
	auto i = flags.find(flag);
	if (i != flags.npos) {
		flags.erase(i, 1);
		return true;
	}
	return false;
}

static_assert(is_move_constructible<Flag_Set>::value,
              "Flag Set Not move constructable");
static_assert(is_move_assignable<Flag_Set>::value,
              "Flag Set Not move assingable");

void Substring_Replacer::sort_uniq()
{
	auto first = table.begin();
	auto last = table.end();
	sort(first, last, [](auto& a, auto& b) { return a.first < b.first; });
	auto it = unique(first, last,
	                 [](auto& a, auto& b) { return a.first == b.first; });
	table.erase(it, last);

	// remove empty key ""
	if (!table.empty() && table.front().first.empty())
		table.erase(table.begin());
}

Substring_Replacer::Substring_Replacer(const Table_Pairs& v) : table(v)
{
	sort_uniq();
}

Substring_Replacer::Substring_Replacer(Table_Pairs&& v) : table(move(v))
{
	sort_uniq();
}
auto Substring_Replacer::operator=(const Table_Pairs& v) -> Substring_Replacer&
{
	table = v;
	sort_uniq();
	return *this;
}
auto Substring_Replacer::operator=(Table_Pairs&& v) -> Substring_Replacer&
{
	table = move(v);
	sort_uniq();
	return *this;
}

auto cmp_prefix_of(const string& p, boost::string_ref of)
{
	return p.compare(0, p.npos, of.data(), 0, min(p.size(), of.size()));
}

struct Comparer_Str_Rep {

	auto operator()(const pair<string, string>& a, boost::string_ref b)
	{
		return cmp_prefix_of(a.first, b) < 0;
	}
	auto operator()(boost::string_ref a, const pair<string, string>& b)
	{
		return cmp_prefix_of(b.first, a) > 0;
	}
	auto eq(const pair<string, string>& a, boost::string_ref b)
	{
		return cmp_prefix_of(a.first, b) == 0;
	}
};

auto find_match(const Substring_Replacer::Table_Pairs& t, boost::string_ref s)
{
	Comparer_Str_Rep csr;
	auto it = begin(t);
	auto last_match = end(t);
	for (;;) {
		auto it2 = upper_bound(it, end(t), s, csr);
		if (it2 == it) {
			// not found, s is smaller that the range
			break;
		}
		--it2;
		if (csr.eq(*it2, s)) {
			// Match found. Try another search maybe for
			// longer.
			last_match = it2;
			it = ++it2;
		}
		else {
			// not found, s is greater that the range
			break;
		}
	}
	return last_match;
}

auto Substring_Replacer::replace(string& s) const -> string&
{

	if (table.empty())
		return s;
	for (size_t i = 0; i < s.size(); /*no increment here*/) {
		auto substr = boost::string_ref(s).substr(i);
		auto it = find_match(table, substr);
		if (it != end(table)) {
			auto& match = *it;
			// match found. match.first is the found string,
			// match.second is the replacement.
			s.replace(i, match.first.size(), match.second);
			i += match.second.size();
			continue;
		}
		++i;
	}
	return s;
}

auto Substring_Replacer::replace_copy(const string& s) const -> string
{
	auto ret = s;
	replace(ret);
	return ret;
}

static_assert(is_move_constructible<Substring_Replacer>::value,
              "Substring_Replacer Not move constructable");
static_assert(is_move_assignable<Substring_Replacer>::value,
              "Substring_Replacer Not move assingable");
}
