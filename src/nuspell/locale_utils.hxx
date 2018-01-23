/* Copyright 2016-2017 Dimitrij Mijoski
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

/*!
 * \file locale_utils.hxx
 * String transformations that depend on locales and enocdings.
 */

#ifndef LOCALE_UTILS_HXX
#define LOCALE_UTILS_HXX

#include <boost/locale.hpp>
#include <locale>
#include <string>

namespace hunspell {

auto decode_utf8(const std::string& s) -> std::u32string;
auto validate_utf8(const std::string& s) -> bool;

auto is_ascii(char c) -> bool;
auto is_all_ascii(const std::string& s) -> bool;
auto ascii_to_ucs2_skip_invalid(const std::string& s) -> std::u16string;

auto latin1_to_ucs2(const std::string& s) -> std::u16string;
auto latin1_to_u32(const std::string& s) -> std::u32string;

auto is_bmp(char32_t c) -> bool;
// auto is_non_bmp(char32_t c) -> bool;
auto is_all_bmp(const std::u32string& s) -> bool;
auto u32_to_ucs2_skip_non_bmp(const std::u32string& s) -> std::u16string;

// put template function definitions bellow the declarations above
// otherwise doxygen has bugs when generating call graphs

class LocaleInput {
};
class Utf8Input {
};
class SameAsDictInput {
};
class WideInput {
};

inline auto to_wide_std(const std::string& in, const std::locale& inloc)
{
	using namespace std;
	auto& cvt = use_facet<codecvt<wchar_t, char, mbstate_t>>(inloc);
	auto wide = std::wstring();
	wide.resize(in.size(), L'\0');
	auto state = mbstate_t{};
	auto char_ptr = in.c_str();
	auto wchar_ptr = &wide[0];
	cvt.in(state, in.c_str(), in.c_str() + in.size(), char_ptr, &wide[0],
	       &wide[wide.size()], wchar_ptr);
	wide.erase(wchar_ptr - &wide[0]);
	return wide;
}

template <class CharT>
auto to_narrow_boost(const std::basic_string<CharT>& w_in,
                     const std::locale& outloc)
{
	using namespace boost::locale::conv;
	return from_utf(w_in, outloc);
}

template <>
inline auto to_narrow_boost(const std::string& w_in,
                            const std::locale& /*outloc*/)
{
	return w_in;
}

template <class Str, class Callback>
auto convert_and_call(LocaleInput, Str&& in, const std::locale& inloc,
                      const std::locale& outloc, Callback func)
{
	using namespace std;
	using info_t = boost::locale::info;
	using namespace boost::locale::conv;
	if (has_facet<boost::locale::info>(inloc)) {
		auto& in_info = use_facet<info_t>(inloc);
		if (in_info.utf8())
			return convert_and_call(Utf8Input{}, forward<Str>(in),
			                        inloc, outloc, func);

		auto& out_info = use_facet<info_t>(outloc);
		if (out_info.utf8()) {
			auto w_out = to_utf<wchar_t>(in, inloc);
			return func(move(w_out));
		}
		else {
			auto in_enc = in_info.encoding();
			auto out_enc = out_info.encoding();
			if (in_enc == out_enc) {
				return func(forward<Str>(in));
			}
			else {
				auto out = between(in, out_enc, in_enc);
				return func(move(out));
			}
		}
	}
	// else
	auto wide = to_wide_std(in, inloc);
	return convert_and_call(WideInput{}, move(wide), inloc, outloc, func);
}

template <class Str, class Callback>
auto convert_and_call(Utf8Input, Str&& in, const std::locale& /*inloc*/,
                      const std::locale& outloc, Callback func)
{
	using namespace std;
	using info_t = boost::locale::info;
	using namespace boost::locale::conv;
	auto& out_info = use_facet<info_t>(outloc);
	if (out_info.utf8()) {
		auto w_out = utf_to_utf<wchar_t>(in);
		return func(move(w_out));
	}
	else {
		auto out = from_utf(in, outloc);
		return func(move(out));
	}
}

template <class Str, class Callback>
auto convert_and_call(SameAsDictInput, Str&& in, const std::locale& /*inloc*/,
                      const std::locale& outloc, Callback func)
{
	using namespace std;
	using info_t = boost::locale::info;
	using namespace boost::locale::conv;

	auto& out_info = use_facet<info_t>(outloc);
	if (out_info.utf8()) {
		auto w_out = utf_to_utf<wchar_t>(in);
		return func(move(w_out));
	}
	else {
		return func(forward<Str>(in));
	}
}

template <class WStr, class Callback>
auto convert_and_call(WideInput, WStr&& w_in, const std::locale& /*inloc*/,
                      const std::locale& outloc, Callback func)
{
	using namespace std;
	using info_t = boost::locale::info;
	using namespace boost::locale::conv;
	auto& out_info = use_facet<info_t>(outloc);
	if (out_info.utf8()) {
		return func(forward<WStr>(w_in));
	}
	else {
		auto out = from_utf(w_in, outloc);
		return func(move(out));
	}
}
}
#endif // LOCALE_UTILS_HXX
