/* Copyright 2021 Dimitrij Mijoski
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
#ifndef NUSPELL_UNICODE_HXX
#define NUSPELL_UNICODE_HXX
#include <string_view>
#include <unicode/utf16.h>
#include <unicode/utf8.h>

template <class CharT>
struct UTF8_Traits {
	static_assert(sizeof(CharT) == 1);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = U8_MAX_LENGTH;

	struct Encoded_CP {
		CharT seq[max_width];
		size_t size = 0;
		Encoded_CP(char32_t cp) { U8_APPEND_UNSAFE(seq, size, cp); }
	};
	UTF8_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
#if U_ICU_VERSION_MAJOR_NUM <= 60
		auto s_ptr = s.data();
		int32_t idx = i;
		int32_t len = s.size();
		U8_NEXT(s_ptr, idx, len, c);
		i = idx;
#else
		auto len = s.size();
		U8_NEXT(s, i, len, c);
#endif
		return c;
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool { return cp < 0; }
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		U8_NEXT_UNSAFE(s, i, c);
		return c;
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
	auto static move_back_valid_cp(String_View s, size_t& i) -> void
	{
		U8_BACK_1_UNSAFE(s, i);
	}
};

template <class CharT>
struct UTF16_Traits {
	static_assert(sizeof(CharT) == 2);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = U16_MAX_LENGTH;
	struct Encoded_CP {
		CharT seq[max_width];
		size_t size = 0;
		Encoded_CP(char32_t cp) { U16_APPEND_UNSAFE(seq, size, cp); }
	};
	UTF16_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		auto len = s.size();
		int32_t c;
		U16_NEXT(s, i, len, c);
		return c;
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return U_IS_SURROGATE(cp);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		int32_t c;
		U16_NEXT_UNSAFE(s, i, c);
		return c;
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
	auto static move_back_valid_cp(String_View s, size_t& i) -> void
	{
		U16_BACK_1_UNSAFE(s, i);
	}
};

template <class CharT>
struct UTF32_Traits {
	static_assert(sizeof(CharT) == 4);

	using String_View = std::basic_string_view<CharT>;

	static constexpr size_t max_width = 1;
	struct Encoded_CP {
		CharT seq[1];
		static constexpr size_t size = 1;
		Encoded_CP(char32_t cp) { seq[0] = cp; }
	};
	UTF32_Traits() = delete;
	auto static decode(String_View s, size_t& i) -> int32_t
	{
		return s[i++];
	}
	auto static is_decoded_cp_error(int32_t cp) -> bool
	{
		return !(0 <= cp && cp <= 0x10ffff);
	}
	auto static decode_valid(String_View s, size_t& i) -> int32_t
	{
		return s[i++];
	}
	auto static encode_valid(char32_t cp) -> Encoded_CP { return cp; }
	auto static move_back_valid_cp(String_View, size_t& i) -> void { --i; }
};

template <class CharT>
struct UTF_Traits;

template <>
struct UTF_Traits<char> : UTF8_Traits<char> {
};
template <>
struct UTF_Traits<char16_t> : UTF16_Traits<char16_t> {
};
#if U_SIZEOF_WCHAR_T == 4
template <>
struct UTF_Traits<wchar_t> : UTF32_Traits<wchar_t> {
};
#elif U_SIZEOF_WCHAR_T == 2
template <>
struct UTF_Traits<wchar_t> : UTF16_Traits<wchar_t> {
};
#endif
#endif // NUSPELL_UNICODE_HXX
