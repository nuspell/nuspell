/* Copyright 2018 Dimitrij Mijoski, Sander van Geloven
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

/**
 * @file structures.cxx
 * Data structures.
 */

#include "structures.hxx"
#include "string_utils.hxx"

namespace nuspell {

using namespace std;

// template class String_Set<char>;
// template class String_Set<wchar_t>;
template class String_Set<char16_t>;

template <class CharT>
void Substr_Replacer<CharT>::sort_uniq()
{
	auto first = begin(table);
	auto last = end(table);
	sort(first, last, [](auto& a, auto& b) { return a.first < b.first; });
	auto it = unique(first, last,
	                 [](auto& a, auto& b) { return a.first == b.first; });
	table.erase(it, last);

	// remove empty key ""
	if (!table.empty() && table.front().first.empty())
		table.erase(begin(table));
}

namespace {
template <class CharT>
struct Comparer_Str_Rep {

	using StrT = basic_string<CharT>;
	using StrViewT = my_string_view<CharT>;

	auto static cmp_prefix_of(const StrT& p, StrViewT of)
	{
		return p.compare(0, p.npos, of.data(),
		                 min(p.size(), of.size()));
	}
	auto operator()(const pair<StrT, StrT>& a, StrViewT b)
	{
		return cmp_prefix_of(a.first, b) < 0;
	}
	auto operator()(StrViewT a, const pair<StrT, StrT>& b)
	{
		return cmp_prefix_of(b.first, a) > 0;
	}
	auto static eq(const pair<StrT, StrT>& a, StrViewT b)
	{
		return cmp_prefix_of(a.first, b) == 0;
	}
};

template <class CharT>
auto find_match(const typename Substr_Replacer<CharT>::Table_Pairs& t,
                my_string_view<CharT> s)
{
	Comparer_Str_Rep<CharT> csr;
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
} // namespace

template <class CharT>
auto Substr_Replacer<CharT>::replace(StrT& s) const -> StrT&
{

	if (table.empty())
		return s;
	for (size_t i = 0; i < s.size(); /*no increment here*/) {
		auto substr = my_string_view<CharT>(&s[i], s.size() - i);
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

template class Substr_Replacer<char>;
template class Substr_Replacer<wchar_t>;

template <class CharT>
auto Break_Table<CharT>::order_entries() -> void
{
	auto it = remove_if(begin(table), end(table), [](auto& s) {
		return s.empty() ||
		       (s.size() == 1 && (s[0] == '^' || s[0] == '$'));
	});
	table.erase(it, end(table));

	auto is_start_word_break = [=](auto& x) { return x[0] == '^'; };
	auto is_end_word_break = [=](auto& x) { return x.back() == '$'; };
	auto start_word_breaks_last =
	    partition(begin(table), end(table), is_start_word_break);
	start_word_breaks_last_idx = start_word_breaks_last - begin(table);

	for_each(begin(table), start_word_breaks_last,
	         [](auto& e) { e.erase(0, 1); });

	auto end_word_breaks_last =
	    partition(start_word_breaks_last, end(table), is_end_word_break);
	end_word_breaks_last_idx = end_word_breaks_last - begin(table);

	for_each(start_word_breaks_last, end_word_breaks_last,
	         [](auto& e) { e.pop_back(); });
}
template class Break_Table<char>;
template class Break_Table<wchar_t>;

template <class CharT>
auto Condition<CharT>::construct() -> void
{
	size_t i = 0;
	for (; i != cond.size();) {
		size_t j = cond.find_first_of(LITERAL(CharT, "[]."), i);
		if (i != j) {
			if (j == cond.npos) {
				spans.emplace_back(i, cond.size() - i, NORMAL);
				length += cond.size() - i;
				break;
			}
			spans.emplace_back(i, j - i, NORMAL);
			length += j - i;
			i = j;
		}
		if (cond[i] == '.') {
			spans.emplace_back(i, 1, DOT);
			++length;
			++i;
			continue;
		}
		if (cond[i] == ']') {
			auto what =
			    "closing bracket has no matching opening bracket";
			throw invalid_argument(what);
		}
		if (cond[i] == '[') {
			++i;
			if (i == cond.size()) {
				auto what = "opening bracket has no matching "
				            "closing bracket";
				throw invalid_argument(what);
			}
			Span_Type type;
			if (cond[i] == '^') {
				type = NONE_OF;
				++i;
			}
			else {
				type = ANY_OF;
			}
			j = cond.find(']', i);
			if (j == i) {
				auto what = "empty bracket expression";
				throw invalid_argument(what);
			}
			if (j == cond.npos) {
				auto what = "opening bracket has no matching "
				            "closing bracket";
				throw invalid_argument(what);
			}
			spans.emplace_back(i, j - i, type);
			++length;
			i = j + 1;
		}
	}
}

/**
 * Checks if provided string matched the condition.
 *
 * @param s string to check if it matches the condition.
 * @param pos start position for string, default is 0.
 * @param len length of string counting from the start position.
 * @return The valueof true when string matched condition.
 */
template <class CharT>
auto Condition<CharT>::match(const StrT& s, size_t pos, size_t len) const
    -> bool
{
	if (pos > s.size()) {
		throw out_of_range("position on the string is out of bounds");
	}
	if (s.size() - pos < len)
		len = s.size() - pos;
	if (len != length)
		return false;

	size_t i = pos;
	for (auto& x : spans) {
		auto x_pos = get<0>(x);
		auto x_len = get<1>(x);
		auto x_type = get<2>(x);

		using tr = typename StrT::traits_type;
		switch (x_type) {
		case NORMAL:
			if (tr::compare(&s[i], &cond[x_pos], x_len) == 0)
				i += x_len;
			else
				return false;
			break;
		case DOT:
			++i;
			break;
		case ANY_OF:
			if (tr::find(&cond[x_pos], x_len, s[i]))
				++i;
			else
				return false;
			break;
		case NONE_OF:
			if (tr::find(&cond[x_pos], x_len, s[i]))
				return false;
			else
				++i;
			break;
		}
	}
	return true;
}
template class Condition<char>;
template class Condition<wchar_t>;

template class Prefix<char>;
template class Prefix<wchar_t>;
template class Suffix<char>;
template class Suffix<wchar_t>;

auto Compound_Rule_Table::fill_all_flags() -> void
{
	for (auto& f : rules) {
		all_flags += f;
	}
	all_flags.erase(u'?');
	all_flags.erase(u'*');
}

struct Out_Iter_One_Bool {
	bool* value = nullptr;
	auto& operator++() { return *this; }
	auto& operator++(int) { return *this; }
	auto& operator*() { return *this; }
	auto& operator=(char16_t)
	{
		*value = true;
		return *this;
	}
};

auto Compound_Rule_Table::has_any_of_flags(const Flag_Set& f) const -> bool
{
	auto has_intersection = false;
	auto out_it = Out_Iter_One_Bool{&has_intersection};
	set_intersection(begin(all_flags), end(all_flags), begin(f), end(f),
	                 out_it);
	return has_intersection;
}

auto match_compund_rule(const std::vector<const Flag_Set*>& words_data,
                        const u16string& pattern)
{
	return match_simple_regex(
	    words_data, pattern,
	    [](const Flag_Set* d, char16_t p) { return d->contains(p); });
}

auto Compound_Rule_Table::match_any_rule(
    const std::vector<const Flag_Set*> data) const -> bool
{
	return any_of(begin(rules), end(rules), [&](const u16string& p) {
		return match_compund_rule(data, p);
	});
}

template <class CharT>
auto Replacement_Table<CharT>::order_entries() -> void
{
	auto it = remove_if(begin(table), end(table), [](auto& p) {
		auto& s = p.first;
		return s.empty() ||
		       (s.size() == 1 && (s[0] == '^' || s[0] == '$'));
	});
	table.erase(it, end(table));

	auto is_start_word_pat = [=](auto& x) { return x.first[0] == '^'; };
	auto is_end_word_pat = [=](auto& x) { return x.first.back() == '$'; };

	auto start_word_reps_last =
	    partition(begin(table), end(table), is_start_word_pat);
	start_word_reps_last_idx = start_word_reps_last - begin(table);
	for_each(begin(table), start_word_reps_last,
	         [](auto& e) { e.first.erase(0, 1); });

	auto whole_word_reps_last =
	    partition(begin(table), start_word_reps_last, is_end_word_pat);
	whole_word_reps_last_idx = whole_word_reps_last - begin(table);
	for_each(begin(table), whole_word_reps_last,
	         [](auto& e) { e.first.pop_back(); });

	auto end_word_reps_last =
	    partition(start_word_reps_last, end(table), is_end_word_pat);
	end_word_reps_last_idx = end_word_reps_last - begin(table);
	for_each(start_word_reps_last, end_word_reps_last,
	         [](auto& e) { e.first.pop_back(); });
}
template class Replacement_Table<char>;
template class Replacement_Table<wchar_t>;

template <class CharT>
auto Similarity_Group<CharT>::parse(const StrT& s) -> void
{
	auto i = size_t(0);
	for (;;) {
		auto j = s.find('(', i);
		chars.append(s, i, j - i);
		if (j == s.npos)
			break;
		i = j + 1;
		j = s.find(')', i);
		if (j == s.npos)
			break;
		auto len = j - i;
		if (len == 1)
			chars += s[i];
		else if (len > 1)
			strings.push_back(s.substr(i, len));
		i = j + 1;
	}
}
template struct Similarity_Group<char>;
template struct Similarity_Group<wchar_t>;

template <class CharT>
auto Phonetic_Table<CharT>::order() -> void
{
	stable_sort(begin(table), end(table), [](auto& pair1, auto& pair2) {
		if (pair2.first.empty())
			return false;
		if (pair1.first.empty())
			return true;
		return pair1.first[0] < pair2.first[0];
	});
	auto it = find_if_not(begin(table), end(table),
	                      [](auto& p) { return p.first.empty(); });
	table.erase(begin(table), it);
	for (auto& r : table) {
		if (r.second == LITERAL(CharT, "_"))
			r.second.clear();
	}
}

template <class CharT>
auto Phonetic_Table<CharT>::match(const StrT& data, size_t i,
                                  const StrT& pattern, bool at_begin)
    -> Phonet_Match_Result
{
	auto ret = Phonet_Match_Result();
	auto j = pattern.find_first_of(LITERAL(CharT, "(<-0123456789^$"));
	if (j == pattern.npos)
		j = pattern.size();
	if (data.compare(i, j, pattern, 0, j) == 0)
		ret.count_matched = j;
	else
		return {};
	if (j == pattern.size())
		return ret;
	if (pattern[j] == '(') {
		auto k = pattern.find(')', j);
		if (k == pattern.npos)
			return {}; // bad rule
		auto x = char_traits<CharT>::find(&pattern[j + 1], k - (j + 1),
		                                  data[i + j]);
		if (!x)
			return {};
		j = k + 1;
		ret.count_matched += 1;
	}
	if (j == pattern.size())
		return ret;
	if (pattern[j] == '<') {
		ret.go_back_after_replace = true;
		++j;
	}
	auto k = pattern.find_first_not_of('-', j);
	if (k == pattern.npos) {
		k = pattern.size();
		ret.go_back_before_replace = k - j;
		if (ret.go_back_before_replace >= ret.count_matched)
			return {}; // bad rule
		return ret;
	}
	else {
		ret.go_back_before_replace = k - j;
		if (ret.go_back_before_replace >= ret.count_matched)
			return {}; // bad rule
	}
	j = k;
	if (pattern[j] >= '0' && pattern[j] <= '9') {
		ret.priority = pattern[j] - '0';
		++j;
	}
	if (j == pattern.size())
		return ret;
	if (pattern[j] == '^') {
		if (!at_begin)
			return {};
		++j;
	}
	if (j == pattern.size())
		return ret;
	if (pattern[j] == '^') {
		ret.treat_next_as_begin = true;
		++j;
	}
	if (j == pattern.size())
		return ret;
	if (pattern[j] != '$')
		return {}; // bad rule, no other char is allowed at this point
	if (i + ret.count_matched == data.size())
		return ret;
	return {};
}

template <class CharT>
auto Phonetic_Table<CharT>::replace(StrT& word) const -> bool
{
	using boost::make_iterator_range;
	struct Cmp {
		auto operator()(CharT c, const pair<StrT, StrT>& s)
		{
			return c < s.first[0];
		}
		auto operator()(const pair<StrT, StrT>& s, CharT c)
		{
			return s.first[0] < c;
		}
	};
	if (table.empty())
		return false;
	auto ret = false;
	auto treat_next_as_begin = true;
	size_t count_go_backs_after_replace = 0; // avoid infinite loop
	for (size_t i = 0; i != word.size(); ++i) {
		auto rules =
		    equal_range(begin(table), end(table), word[i], Cmp());
		for (auto& r : make_iterator_range(rules)) {
			auto rule = &r;
			auto m1 = match(word, i, r.first, treat_next_as_begin);
			if (!m1)
				continue;
			if (!m1.go_back_before_replace) {
				auto j = i + m1.count_matched - 1;
				auto rules2 = equal_range(
				    begin(table), end(table), word[j], Cmp());
				for (auto& r2 : make_iterator_range(rules2)) {
					auto m2 =
					    match(word, j, r2.first, false);
					if (m2 && m2.priority >= m1.priority) {
						i = j;
						rule = &r2;
						m1 = m2;
						break;
					}
				}
			}
			word.replace(
			    i, m1.count_matched - m1.go_back_before_replace,
			    rule->second);
			treat_next_as_begin = m1.treat_next_as_begin;
			if (m1.go_back_after_replace &&
			    count_go_backs_after_replace < 100) {
				count_go_backs_after_replace++;
			}
			else {
				i += rule->second.size();
			}
			--i;
			ret = true;
			break;
		}
	}
	return ret;
}
template class Phonetic_Table<char>;
template class Phonetic_Table<wchar_t>;

} // namespace nuspell
