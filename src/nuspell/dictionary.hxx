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

#include "aff_data.hxx"
#include "dic_data.hxx"

#include <fstream>
#include <locale>

namespace hunspell {

enum Spell_result {
	bad_word,
	good_word,
	affixed_good_word,
	compound_good_word
};

class Dictionary {
	using string = std::string;
	using u16string = std::u16string;

	Aff_data aff_data;
	Dic_data dic_data;

      private:
	template <class CharT>
	auto spell_priv(const std::basic_string<CharT> s) -> Spell_result
	{
		if (dic_data.words.count(
		        to_narrow_boost(s, aff_data.locale_aff)))
			return good_word;
		return bad_word;
	}

      public:
	Dictionary()
	    : // we explicity do value init so content is zeroed
	      aff_data(),
	      dic_data()
	{
	}
	explicit Dictionary(const string& dict_file_path)
	    : aff_data(), dic_data()
	{
		std::ifstream aff_file(dict_file_path + ".aff");
		std::ifstream dic_file(dict_file_path + ".dic");
		aff_data.parse(aff_file);
		dic_data.parse(dic_file, aff_data);
	}

	auto spell_dict_encoding(const std::string& word) -> Spell_result;

	auto spell_c_locale(const std::string& word) -> Spell_result;

	auto spell(const std::string& word, std::locale loc = std::locale())
	    -> Spell_result
	{
		auto f = [this](auto&& a) {
			return this->spell_priv(std::forward<decltype(a)>(a));
		};
		return convert_and_call(LocaleInput{}, word, loc,
		                        aff_data.locale_aff, f);
	}
	auto spell_u8(const std::string& word) -> Spell_result;
	auto spell(const std::wstring& word) -> Spell_result;
	auto spell(const std::u16string& word) -> Spell_result;
	auto spell(const std::u32string& word) -> Spell_result;
};
}