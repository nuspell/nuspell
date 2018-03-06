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

#ifndef NUSPELL_STRUCTURES_HXX
#define NUSPELL_STRUCTURES_HXX

#include <algorithm>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/range/iterator_range_core.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>

namespace nuspell {

template <class Container>
void sort_uniq(Container& c)
{
	auto first = begin(c);
	auto last = end(c);
	sort(first, last);
	c.erase(unique(first, last), last);
}

/**
 * @brief A Set class backed by a string. Very useful for small sets.
 */
template <class CharT>
class String_Set {
      private:
	std::basic_string<CharT> d;
	auto sort_uniq() -> void { nuspell::sort_uniq(d); }

      public:
	using StrT = std::basic_string<CharT>;
	using key_type = typename StrT::value_type;
	// using key_compare = Compare;
	using value_type = typename StrT::value_type;
	// using value_compare = Compare;
	using allocator_type = typename StrT::allocator_type;
	using pointer = typename StrT::pointer;
	using const_pointer = typename StrT::const_pointer;
	using reference = typename StrT::reference;
	using const_reference = typename StrT::const_reference;
	using size_type = typename StrT::size_type;
	using difference_type = typename StrT::difference_type;
	using iterator = typename StrT::iterator;
	using const_iterator = typename StrT::const_iterator;
	using reverse_iterator = typename StrT::reverse_iterator;
	using const_reverse_iterator = typename StrT::const_reverse_iterator;

	String_Set() = default;
	String_Set(const StrT& s) : d(s) { sort_uniq(); }
	String_Set(StrT&& s) : d(std::move(s)) { sort_uniq(); }
	template <class InputIterator>
	String_Set(InputIterator first, InputIterator last) : d(first, last)
	{
		sort_uniq();
	}
	auto operator=(const StrT& s) -> String_Set&
	{
		d = s;
		sort_uniq();
		return *this;
	}
	auto operator=(StrT&& s) -> String_Set&
	{
		d = std::move(s);
		sort_uniq();
		return *this;
	}

	// non standard underlying storage access:
	auto data() const { return d; }
	operator const StrT&() const noexcept { return d; }

	// iterators:
	iterator begin() noexcept { return d.begin(); }
	const_iterator begin() const noexcept { return d.begin(); }
	iterator end() noexcept { return d.end(); }
	const_iterator end() const noexcept { return d.end(); }

	reverse_iterator rbegin() noexcept { return d.rbegin(); }
	const_reverse_iterator rbegin() const noexcept { return d.rbegin(); }
	reverse_iterator rend() noexcept { return d.rend(); }
	const_reverse_iterator rend() const noexcept { return d.rend(); }

	const_iterator cbegin() const noexcept { return d.cbegin(); }
	const_iterator cend() const noexcept { return d.cend(); }
	const_reverse_iterator crbegin() const noexcept { return d.crbegin(); }
	const_reverse_iterator crend() const noexcept { return d.crend(); }

	// capacity:
	bool empty() const noexcept { return d.empty(); }
	size_type size() const noexcept { return d.size(); }
	size_type max_size() const noexcept { return d.max_size(); }

	// modifiers:
	// template <class... Args>
	// pair<iterator, bool> emplace(Args&&... args);
	// template <class... Args>
	// iterator emplace_hint(const_iterator hint, Args&&... args);

	/**
	 * Inserts a value.
	 *
	 * @param x the value to insert.
	 * @return The pair holding the post-insert iterator and a boolean
	 * representing insert succes.
	 */
	std::pair<iterator, bool> insert(const value_type& x)
	{
		auto it = std::lower_bound(begin(), end(), x);
		if (it != end() && *it == x) {
			return {it, false};
		}
		auto ret = d.insert(it, x);
		return {ret, true};
	}
	// std::pair<iterator, bool> insert(value_type&& x);
	iterator insert(iterator hint, const value_type& x)
	{
		if (hint == end() || x < *hint) {
			if (hint == begin() || *(hint - 1) < x) {
				return d.insert(hint, x);
			}
		}
		return insert(x).first;
	}

	// iterator insert(const_iterator hint, value_type&& x);
	template <class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		d.insert(d.end(), first, last);
		sort_uniq();
	}
	// void insert(std::initializer_list<value_type>);

	iterator erase(iterator position) { return d.erase(position); }
	size_type erase(const key_type& x)
	{
		auto i = d.find(x);
		if (i != d.npos) {
			d.erase(i, 1);
			return true;
		}
		return false;
	}
	iterator erase(iterator first, iterator last)
	{
		return d.erase(first, last);
	}
	void swap(String_Set& s) { d.swap(s.d); }
	void clear() noexcept { d.clear(); }

	// non standrd modifiers:
	auto insert(const StrT& s) -> void
	{
		d += s;
		sort_uniq();
	}
	auto operator+=(const StrT& s) -> String_Set
	{
		insert(s);
		return *this;
	}

	// observers:
	// key_compare   key_comp() const;
	// value_compare value_comp() const;

	// set operations:
      private:
	auto lookup(const key_type& x) const
	{
		auto i = d.find(x);
		if (i != d.npos)
			i = d.size();
		return i;
	}

      public:
	iterator find(const key_type& x) { return begin() + lookup(x); }
	const_iterator find(const key_type& x) const
	{
		return begin() + lookup(x);
	}
	size_type count(const key_type& x) const { return d.find(x) != d.npos; }

	iterator lower_bound(const key_type& x)
	{
		return std::lower_bound(begin(), end(), x);
	}

	const_iterator lower_bound(const key_type& x) const
	{
		return std::lower_bound(begin(), end(), x);
	}
	iterator upper_bound(const key_type& x)
	{
		return std::upper_bound(begin(), end(), x);
	}

	const_iterator upper_bound(const key_type& x) const
	{
		return std::upper_bound(begin(), end(), x);
	}
	std::pair<iterator, iterator> equal_range(const key_type& x)
	{
		return std::equal_range(begin(), end(), x);
	}

	std::pair<const_iterator, const_iterator>
	equal_range(const key_type& x) const
	{
		return std::equal_range(begin(), end(), x);
	}

	// non standard set operations:
	bool exists(const key_type& x) const { return count(x); }

	// compare
	bool operator<(const String_Set& rhs) const { return d < rhs.d; }
	bool operator<=(const String_Set& rhs) const { return d <= rhs.d; }
	bool operator==(const String_Set& rhs) const { return d == rhs.d; }
	bool operator!=(const String_Set& rhs) const { return d != rhs.d; }
	bool operator>=(const String_Set& rhs) const { return d >= rhs.d; }
	bool operator>(const String_Set& rhs) const { return d > rhs.d; }
};

extern template class String_Set<char>;
extern template class String_Set<wchar_t>;
extern template class String_Set<char16_t>;

template <class CharT>
auto swap(String_Set<CharT>& a, String_Set<CharT>& b)
{
	a.swap(b);
}

using Flag_Set = String_Set<char16_t>;

template <class CharT>
class Substr_Replacer {
      public:
	using StrT = std::basic_string<CharT>;
	using Table_Pairs = std::vector<std::pair<StrT, StrT>>;

      private:
	Table_Pairs table;
	void sort_uniq();

      public:
	Substr_Replacer() = default;
	Substr_Replacer(const Table_Pairs& v);
	Substr_Replacer(Table_Pairs&& v);

	auto operator=(const Table_Pairs& v) -> Substr_Replacer&;
	auto operator=(Table_Pairs&& v) -> Substr_Replacer&;
	template <class Range>
	auto operator=(const Range& range) -> Substr_Replacer&
	{
		table.assign(std::begin(range), std::end(range));
		sort_uniq();
		return *this;
	}

	auto replace(StrT& s) const -> StrT&;
	auto replace_copy(StrT s) const -> StrT;
};
using Substring_Replacer = Substr_Replacer<char>;
using WSubstring_Replacer = Substr_Replacer<wchar_t>;

template <class CharT>
class Break_Table {
      public:
	using StrT = std::basic_string<CharT>;
	using Table_Str = std::vector<StrT>;
	using iterator = typename Table_Str::iterator;
	using const_iterator = typename Table_Str::const_iterator;

      private:
	Table_Str table;
	iterator start_word_breaks_last_it;
	iterator end_word_breaks_last_it;

	auto order_entries() -> void;

      public:
	Break_Table() = default;
	Break_Table(const Table_Str& v);
	Break_Table(Table_Str&& v);

	auto operator=(const Table_Str& v) -> Break_Table&;
	auto operator=(Table_Str&& v) -> Break_Table&;
	template <class Range>
	auto operator=(const Range& range) -> Break_Table&
	{
		table.assign(std::begin(range), std::end(range));
		order_entries();
		return *this;
	}

	auto start_word_breaks() const -> boost::iterator_range<const_iterator>
	{
		return {std::begin(table),
		        const_iterator(start_word_breaks_last_it)};
	}
	auto end_word_breaks() const -> boost::iterator_range<const_iterator>
	{
		return {const_iterator(start_word_breaks_last_it),
		        const_iterator(end_word_breaks_last_it)};
	}
	auto middle_word_breaks() const -> boost::iterator_range<const_iterator>
	{
		return {const_iterator(end_word_breaks_last_it),
		        std::end(table)};
	}
};

template <class CharT>
class Char_Eraser {
      public:
	using SetT = String_Set<CharT>;
	using StrT = std::basic_string<CharT>;

      private:
	SetT erase_chars;

      public:
	auto operator=(const SetT& e) -> Char_Eraser&;
	auto operator=(SetT&& e) -> Char_Eraser&;

	auto erase(StrT& s) const -> StrT&;
	auto erase_copy(const StrT& s) const -> StrT;
};

template <class CharT>
class Prefix_Entry {
      public:
	using StrT = std::basic_string<CharT>;
	using RegexT = std::basic_regex<CharT>;

	const char16_t flag;
	const bool cross_product;
	const StrT stripping;
	const StrT appending;
	const RegexT condition;

	Prefix_Entry() = default;
	Prefix_Entry(char16_t flag, bool cross_product, const StrT& strip,
	             const StrT& append, StrT condition);

	auto to_root(StrT& word) const -> StrT&;
	auto to_root_copy(StrT word) const -> StrT;

	auto to_derived(StrT& word) const -> StrT&;
	auto to_derived_copy(StrT word) const -> StrT;

	auto check_condition(const StrT& word) const -> bool;
};

template <class CharT>
class Suffix_Entry {
      public:
	using StrT = std::basic_string<CharT>;
	using RegexT = std::basic_regex<CharT>;

	const char16_t flag;
	const bool cross_product;
	const StrT stripping;
	const StrT appending;
	const RegexT condition;

	Suffix_Entry() = default;
	Suffix_Entry(char16_t flag, bool cross_product, const StrT& strip,
	             const StrT& append, StrT condition);

	auto to_root(StrT& word) const -> StrT&;
	auto to_root_copy(StrT word) const -> StrT;

	auto to_derived(StrT& word) const -> StrT&;
	auto to_derived_copy(StrT word) const -> StrT;

	auto check_condition(const StrT& word) const -> bool;
};

using boost::multi_index_container;
using boost::multi_index::indexed_by;
using boost::multi_index::member;
using boost::multi_index::hashed_non_unique;

template <class CharT>
using Prefix_Table = multi_index_container<
    Prefix_Entry<CharT>,
    indexed_by<hashed_non_unique<
        member<Prefix_Entry<CharT>, const std::basic_string<CharT>,
               &Prefix_Entry<CharT>::appending>>>>;

template <class CharT>
using Suffix_Table = multi_index_container<
    Suffix_Entry<CharT>,
    indexed_by<hashed_non_unique<
        member<Suffix_Entry<CharT>, const std::basic_string<CharT>,
               &Suffix_Entry<CharT>::appending>>>>;
}
#endif
