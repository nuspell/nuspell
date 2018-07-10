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

/**
 * @file structures.hxx
 * Data structures.
 */

#ifndef NUSPELL_STRUCTURES_HXX
#define NUSPELL_STRUCTURES_HXX

#include "condition.hxx"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/range/iterator_range_core.hpp>

#ifdef __has_include
#if __has_include(<experimental/string_view>)
#if !defined(_LIBCPP_VERSION) || _LIBCPP_VERSION < 7000
#include <experimental/string_view>
#if defined(__cpp_lib_experimental_string_view) || defined(_LIBCPP_VERSION)
#define NUSPELL_STR_VIEW_NS std::experimental
#endif
#endif
#endif
#endif

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 7000
#include <string_view>
#define NUSPELL_STR_VIEW_NS std
#endif

#ifndef NUSPELL_STR_VIEW_NS
#include <boost/utility/string_view.hpp>
#endif

namespace nuspell {

/**
 * @brief A Set class backed by a string. Very useful for small sets.
 *
 * Has the same interface as std::set.
 */
template <class CharT>
class String_Set {
      private:
	std::basic_string<CharT> d;
	auto sort_uniq() -> void
	{
		auto first = begin();
		auto last = end();
		using t = traits_type;
		sort(first, last, t::lt);
		d.erase(unique(first, last, t::eq), last);
	}
	struct Char_Traits_Less_Than {
		auto operator()(CharT a, CharT b)
		{
			return traits_type::lt(a, b);
		}
	};

      public:
	using StrT = std::basic_string<CharT>;
	using traits_type = typename StrT::traits_type;

	using key_type = typename StrT::value_type;
	using key_compare = Char_Traits_Less_Than;
	using value_type = typename StrT::value_type;
	using value_compare = key_compare;
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
	String_Set(StrT&& s) : d(move(s)) { sort_uniq(); }
	template <class InputIterator>
	String_Set(InputIterator first, InputIterator last) : d(first, last)
	{
		sort_uniq();
	}
	String_Set(std::initializer_list<value_type> il) : d(il)
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
		d = move(s);
		sort_uniq();
		return *this;
	}
	auto operator=(std::initializer_list<value_type> il) -> String_Set&
	{
		d = il;
		sort_uniq();
		return *this;
	}

	// non standard underlying storage access:
	auto& data() const { return d; }
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
	std::pair<iterator, bool> insert(const value_type& x)
	{
		auto it = lower_bound(x);
		if (it != end() && *it == x) {
			return {it, false};
		}
		auto ret = d.insert(it, x);
		return {ret, true};
	}
	// std::pair<iterator, bool> insert(value_type&& x);
	iterator insert(iterator hint, const value_type& x)
	{
		if (hint == end() || traits_type::lt(x, *hint)) {
			if (hint == begin() ||
			    traits_type::lt(*(hint - 1), x)) {
				return d.insert(hint, x);
			}
		}
		return insert(x).first;
	}

	// iterator insert(const_iterator hint, value_type&& x);
	template <class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		d.insert(end(), first, last);
		sort_uniq();
	}
	void insert(std::initializer_list<value_type> il)
	{
		d.insert(end(), il);
		sort_uniq();
	}
	template <class... Args>
	std::pair<iterator, bool> emplace(Args&&... args)
	{
		return insert(CharT(args...));
	}

	template <class... Args>
	iterator emplace_hint(iterator hint, Args&&... args)
	{
		return insert(hint, CharT(args...));
	}

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
	key_compare key_comp() const { return Char_Traits_Less_Than(); }
	value_compare value_comp() const { return key_comp(); }

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
		return std::lower_bound(begin(), end(), x, traits_type::lt);
	}

	const_iterator lower_bound(const key_type& x) const
	{
		return std::lower_bound(begin(), end(), x, traits_type::lt);
	}
	iterator upper_bound(const key_type& x)
	{
		return std::upper_bound(begin(), end(), x, traits_type::lt);
	}

	const_iterator upper_bound(const key_type& x) const
	{
		return std::upper_bound(begin(), end(), x, traits_type::lt);
	}
	std::pair<iterator, iterator> equal_range(const key_type& x)
	{
		return std::equal_range(begin(), end(), x, traits_type::lt);
	}

	std::pair<const_iterator, const_iterator>
	equal_range(const key_type& x) const
	{
		return std::equal_range(begin(), end(), x, traits_type::lt);
	}

	// non standard set operations:
	bool contains(const key_type& x) const { return count(x); }

	// compare
	bool operator<(const String_Set& rhs) const { return d < rhs.d; }
	bool operator<=(const String_Set& rhs) const { return d <= rhs.d; }
	bool operator==(const String_Set& rhs) const { return d == rhs.d; }
	bool operator!=(const String_Set& rhs) const { return d != rhs.d; }
	bool operator>=(const String_Set& rhs) const { return d >= rhs.d; }
	bool operator>(const String_Set& rhs) const { return d > rhs.d; }
};
// extern template class String_Set<char>;
// extern template class String_Set<wchar_t>;
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
	void sort_uniq(); // implemented in cxx

      public:
	Substr_Replacer() = default;
	Substr_Replacer(const Table_Pairs& v) : table(v) { sort_uniq(); }
	Substr_Replacer(const Table_Pairs&& v) : table(move(v)) { sort_uniq(); }

	auto& operator=(const Table_Pairs& v)
	{
		table = v;
		sort_uniq();
		return *this;
	}
	auto& operator=(const Table_Pairs&& v)
	{
		table = move(v);
		sort_uniq();
		return *this;
	}

	template <class Range>
	auto operator=(const Range& range) -> Substr_Replacer&
	{
		table.assign(std::begin(range), std::end(range));
		sort_uniq();
		return *this;
	}

	auto replace(StrT& s) const -> StrT&; // implemented in cxx
	auto replace_copy(StrT s) const -> StrT
	{
		replace(s);
		return s;
	}
};
extern template class Substr_Replacer<char>;
extern template class Substr_Replacer<wchar_t>;

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

	auto order_entries() -> void; // implemented in cxx

      public:
	Break_Table() = default;
	Break_Table(const Table_Str& v) : table(v) { order_entries(); }

	Break_Table(Table_Str&& v) : table(move(v)) { order_entries(); }

	auto& operator=(const Table_Str& v)
	{
		table = v;
		order_entries();
		return *this;
	}

	auto& operator=(Table_Str&& v)
	{
		table = move(v);
		order_entries();
		return *this;
	}

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
extern template class Break_Table<char>;
extern template class Break_Table<wchar_t>;

struct identity {
	template <class T>
	constexpr auto&& operator()(T&& t)
	{
		return std::forward<T>(t);
	}
};

template <class Value, class Key = Value, class KeyExtract = identity,
          class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class Hash_Multiset {
      private:
	using bucket_type = boost::container::small_vector<Value, 1>;
	static constexpr float max_load_fact = 7.0 / 8.0;
	std::vector<bucket_type> data;
	size_t sz = 0;
	size_t max_load_factor_capacity = 0;
	KeyExtract key_extract = {};
	Hash hash = {};
	KeyEqual equal = {};

      public:
	using key_type = Key;
	using value_type = Value;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using hasher = Hash;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename bucket_type::pointer;
	using const_pointer = typename bucket_type::const_pointer;
	using local_iterator = typename bucket_type::iterator;
	using local_const_iterator = typename bucket_type::const_iterator;

	Hash_Multiset() : data(16) {}

	auto size() const { return sz; }
	auto empty() const { return size() == 0; }

	auto rehash(size_t count)
	{
		if (empty()) {
			size_t capacity = 16;
			while (capacity <= count)
				capacity <<= 1;
			data.resize(capacity);
			max_load_factor_capacity =
			    std::ceil(capacity * max_load_fact);
			return;
		}
		if (count < size() / max_load_fact)
			count = size() / max_load_fact;
		auto n = Hash_Multiset();
		n.rehash(count);
		for (auto& b : data) {
			for (auto& x : b) {
				n.insert(x);
			}
		}
		data.swap(n.data);
		sz = n.sz;
		max_load_factor_capacity = n.max_load_factor_capacity;
	}

	auto reserve(size_t count) -> void
	{
		rehash(std::ceil(count / max_load_fact));
	}

	auto insert(const_reference value)
	{
		using namespace std;
		if (sz == max_load_factor_capacity) {
			reserve(sz + 1);
		}
		auto&& key = key_extract(value);
		auto h = hash(key);
		auto h_mod = h & (data.size() - 1);
		auto& bucket = data[h_mod];
		if (bucket.size() == 0 || bucket.size() == 1 ||
		    equal(key, key_extract(bucket.back()))) {
			bucket.push_back(value);
			++sz;
			return end(bucket) - 1;
		}
		auto last =
		    std::find_if(rbegin(bucket), rend(bucket), [&](auto& x) {
			    return equal(key, key_extract(x));
		    });
		if (last != rend(bucket)) {
			auto ret = bucket.insert(last.base(), value);
			++sz;
			return ret;
		}

		bucket.push_back(value);
		++sz;
		return end(bucket) - 1;
	}
	template <class... Args>
	auto emplace(Args&&... a)
	{
		return insert(value_type(std::forward<Args>(a)...));
	}

	// Note, leaks non-const iterator. do not modify
	// the key part of the returned value(s).
	template <class CompatibleKey>
	auto equal_range_nonconst_unsafe(const CompatibleKey& key)
	    -> std::pair<local_iterator, local_iterator>
	{
		using namespace std;
		if (data.empty())
			return {};
		auto h = hash(key);
		auto h_mod = h & (data.size() - 1);
		auto& bucket = data[h_mod];
		if (bucket.empty())
			return {};
		if (bucket.size() == 1) {
			if (equal(key, key_extract(bucket.front())))
				return {begin(bucket), end(bucket)};
			return {};
		}
		auto first =
		    std::find_if(begin(bucket), end(bucket), [&](auto& x) {
			    return equal(key, key_extract(x));
		    });
		if (first == end(bucket))
			return {};
		auto next = first + 1;
		if (next == end(bucket) || !equal(key, key_extract(*next)))
			return {first, next};
		auto last =
		    std::find_if(rbegin(bucket), rend(bucket), [&](auto& x) {
			    return equal(key, key_extract(x));
		    });
		return {first, last.base()};
	}

	template <class CompatibleKey>
	auto equal_range(const CompatibleKey& key) const
	    -> std::pair<local_const_iterator, local_const_iterator>
	{
		using namespace std;
		if (data.empty())
			return {};
		auto h = hash(key);
		auto h_mod = h & (data.size() - 1);
		auto& bucket = data[h_mod];
		if (bucket.empty())
			return {};
		if (bucket.size() == 1) {
			if (equal(key, key_extract(bucket.front())))
				return {begin(bucket), end(bucket)};
			return {};
		}
		auto first =
		    std::find_if(begin(bucket), end(bucket), [&](auto& x) {
			    return equal(key, key_extract(x));
		    });
		if (first == end(bucket))
			return {};
		auto next = first + 1;
		if (next == end(bucket) || !equal(key, key_extract(*next)))
			return {first, next};
		auto last =
		    std::find_if(rbegin(bucket), rend(bucket), [&](auto& x) {
			    return equal(key, key_extract(x));
		    });
		return {first, last.base()};
	}
};

template <class CharT>
class Prefix {
      public:
	using StrT = std::basic_string<CharT>;
	using CondT = Condition<CharT>;

	char16_t flag = 0;
	bool cross_product = false;
	StrT stripping;
	StrT appending;
	Flag_Set cont_flags;
	CondT condition;

	Prefix() = default;
	Prefix(char16_t flag, bool cross_product, const StrT& strip,
	       const StrT& append, const Flag_Set& cont_flags,
	       const StrT& condition)
	    : flag(flag), cross_product(cross_product), stripping(strip),
	      appending(append), cont_flags(cont_flags), condition(condition)
	{
	}

	auto to_root(StrT& word) const -> StrT&
	{
		return word.replace(0, appending.size(), stripping);
	}
	auto to_root_copy(StrT word) const -> StrT
	{
		to_root(word);
		return word;
	}

	auto to_derived(StrT& word) const -> StrT&
	{
		return word.replace(0, stripping.size(), appending);
	}
	auto to_derived_copy(StrT word) const -> StrT
	{
		to_derived(word);
		return word;
	}

	auto check_condition(const StrT& word) const -> bool
	{
		return condition.match_prefix(word);
	}
};

template <class CharT>
class Suffix {
      public:
	using StrT = std::basic_string<CharT>;
	using CondT = Condition<CharT>;

	char16_t flag = 0;
	bool cross_product = false;
	StrT stripping;
	StrT appending;
	Flag_Set cont_flags;
	CondT condition;

	Suffix() = default;
	Suffix(char16_t flag, bool cross_product, const StrT& strip,
	       const StrT& append, const Flag_Set& cont_flags,
	       const StrT& condition)
	    : flag(flag), cross_product(cross_product), stripping(strip),
	      appending(append), cont_flags(cont_flags), condition(condition)
	{
	}

	auto to_root(StrT& word) const -> StrT&
	{
		return word.replace(word.size() - appending.size(),
		                    appending.size(), stripping);
	}
	auto to_root_copy(StrT word) const -> StrT
	{
		to_root(word);
		return word;
	}

	auto to_derived(StrT& word) const -> StrT&
	{
		return word.replace(word.size() - stripping.size(),
		                    stripping.size(), appending);
	}
	auto to_derived_copy(StrT word) const -> StrT
	{
		to_derived(word);
		return word;
	}

	auto check_condition(const StrT& word) const -> bool
	{
		return condition.match_suffix(word);
	}
};
extern template class Prefix<char>;
extern template class Prefix<wchar_t>;
extern template class Suffix<char>;
extern template class Suffix<wchar_t>;

using boost::multi_index_container;
using boost::multi_index::hashed_non_unique;
using boost::multi_index::indexed_by;
using boost::multi_index::member;

#ifdef NUSPELL_STR_VIEW_NS
template <class CharT>
using my_string_view = NUSPELL_STR_VIEW_NS::basic_string_view<CharT>;
template <class CharT>
using sv_hash = std::hash<my_string_view<CharT>>;
#else
template <class CharT>
using my_string_view = boost::basic_string_view<CharT>;
template <class CharT>
struct sv_hash {
	auto operator()(boost::basic_string_view<CharT> s) const
	{
		return boost::hash_range(begin(s), end(s));
	}
};
#endif

template <class CharT>
struct sv_eq {
	auto operator()(my_string_view<CharT> l, my_string_view<CharT> r) const
	{
		return l == r;
	}
};

template <class CharT, class AffixT>
using Affix_Table_Base =
    Hash_Multiset<AffixT, std::basic_string<CharT>,
                  member<AffixT, std::basic_string<CharT>, &AffixT::appending>,
                  sv_hash<CharT>, sv_eq<CharT>>;

template <class CharT, class AffixT>
class Affix_Table : private Affix_Table_Base<CharT, AffixT> {
      private:
	Flag_Set all_cont_flags;

      public:
	using base = Affix_Table_Base<CharT, AffixT>;
	using iterator = typename base::local_const_iterator;
	template <class... Args>
	auto emplace(Args&&... a)
	{
		auto it = base::emplace(std::forward<Args>(a)...);
		all_cont_flags += it->cont_flags;
		return it;
	}
	auto equal_range(my_string_view<CharT> appending) const
	{
		return base::equal_range(appending);
	}
	auto has_continuation_flags() const
	{
		return all_cont_flags.size() != 0;
	}
	auto has_continuation_flag(char16_t flag) const
	{
		return all_cont_flags.contains(flag);
	}
};

template <class CharT>
using Prefix_Table = Affix_Table<CharT, Prefix<CharT>>;

template <class CharT>
using Suffix_Table = Affix_Table<CharT, Suffix<CharT>>;

template <class CharT>
class String_Pair {
	using StrT = std::basic_string<CharT>;
	size_t i = 0;
	StrT s;

      public:
	String_Pair() = default;
	template <class Str1>
	String_Pair(Str1&& str, size_t i) : i(i), s(std::forward<Str1>(str))
	{
		if (i > s.size())
			throw std::out_of_range("Word split is too long.");
	}
	template <
	    class Str1, class Str2,
	    class = std::enable_if_t<
	        std::is_same<std::remove_reference_t<Str1>, StrT>::value &&
	        std::is_same<std::remove_reference_t<Str2>, StrT>::value>>
	String_Pair(Str1&& first, Str2&& second)
	    : i(first.size()) /* must be init before s, before we move
	                         from first */
	      ,
	      s(std::forward<Str1>(first) + std::forward<Str2>(second))

	{
	}
	auto first() const { return my_string_view<CharT>(s).substr(0, i); }
	auto second() const { return my_string_view<CharT>(s).substr(i); }
	auto first(my_string_view<CharT> x)
	{
		s.replace(0, i, x.data(), x.size());
		i = x.size();
	}
	auto second(my_string_view<CharT> x)
	{
		s.replace(i, s.npos, x.data(), x.size());
	}
	auto& str() const { return s; }
	auto idx() const { return i; }
};
template <class CharT>
struct Compound_Pattern {
	using StrT = std::basic_string<CharT>;

	String_Pair<CharT> begin_end_chars;
	StrT replacement;
	char16_t first_word_flag = 0;
	char16_t second_word_flag = 0;
	bool match_first_only_unaffixed_or_zero_affixed = false;
};
} // namespace nuspell
#endif // NUSPELL_STRUCTURES_HXX
