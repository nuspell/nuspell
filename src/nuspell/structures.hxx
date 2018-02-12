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

#include <string>
#include <utility>
#include <vector>

namespace hunspell {

/**
 * @brief A Set class backed by a string. Very useful for small sets.
 */
class Flag_Set {
	std::u16string flags;
	auto sort_uniq() -> void;

      public:
	// creation
	Flag_Set() = default;
	Flag_Set(const std::u16string& s);
	Flag_Set(std::u16string&& s);
	auto operator=(const std::u16string& s) -> Flag_Set&;
	auto operator=(std::u16string&& s) -> Flag_Set&;

	// insert
	// auto insert(char16_t flag) -> It;

	// bulk insert
	// auto insert(It first, It last) -> void;
	auto insert(const std::u16string& s) -> void;
	auto operator+=(const std::u16string& s) -> Flag_Set&
	{
		insert(s);
		return *this;
	}

	// erase
	auto erase(char16_t flag) -> bool;

	// bulk erase
	// auto clear() { return flags.clear(); }

	// access
	auto size() const noexcept { return flags.size(); }
	auto data() const noexcept -> const std::u16string& { return flags; }
	operator const std::u16string&() const noexcept { return flags; }
	auto empty() const noexcept -> bool { return flags.empty(); }
	auto exists(char16_t flag) const -> bool
	{
		// This method is most commonly called (on hotpath)
		// put it in header so it is inlined.
		// Flags are short strings.
		// Optimized linear search should be better for short strings.
		return flags.find(flag) != flags.npos;
	}
	auto count(char16_t flag) const -> size_t { return exists(flag); }

	auto begin() const noexcept { return flags.begin(); }
	auto end() const noexcept { return flags.end(); }
	auto friend swap(Flag_Set& a, Flag_Set& b) { a.flags.swap(b.flags); }
};

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

	auto replace(StrT& s) const -> StrT&;
	auto replace_copy(StrT s) const -> StrT;
};
using Substring_Replacer = Substr_Replacer<char>;
using WSubstring_Replacer = Substr_Replacer<wchar_t>;
}
#endif
