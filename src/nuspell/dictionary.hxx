/* Copyright 2016-2020 Dimitrij Mijoski
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
 * @file
 * @brief Dictionary spelling.
 */

#ifndef NUSPELL_DICTIONARY_HXX
#define NUSPELL_DICTIONARY_HXX

#include "aff_data.hxx"

#include <locale>

NUSPELL_MSVC_PRAGMA_WARNING(push)
NUSPELL_MSVC_PRAGMA_WARNING(disable : 4251 4275)

namespace nuspell {
inline namespace v4 {

enum Affixing_Mode {
	FULL_WORD,
	AT_COMPOUND_BEGIN,
	AT_COMPOUND_END,
	AT_COMPOUND_MIDDLE
};

struct Affixing_Result_Base {
	Word_List::const_pointer root_word = {};

	operator Word_List::const_pointer() const { return root_word; }
	auto& operator*() const { return *root_word; }
	auto operator->() const { return root_word; }
};

template <class T1 = void, class T2 = void>
struct Affixing_Result : Affixing_Result_Base {
	const T1* a = {};
	const T2* b = {};

	Affixing_Result() = default;
	Affixing_Result(Word_List::const_reference r, const T1& a, const T2& b)
	    : Affixing_Result_Base{&r}, a{&a}, b{&b}
	{
	}
};
template <class T1>
struct Affixing_Result<T1, void> : Affixing_Result_Base {
	const T1* a = {};

	Affixing_Result() = default;
	Affixing_Result(Word_List::const_reference r, const T1& a)
	    : Affixing_Result_Base{&r}, a{&a}
	{
	}
};

template <>
struct Affixing_Result<void, void> : Affixing_Result_Base {
	Affixing_Result() = default;
	Affixing_Result(Word_List::const_reference r) : Affixing_Result_Base{&r}
	{
	}
};

struct Compounding_Result {
	Word_List::const_pointer word_entry = {};
	unsigned char num_words_modifier = {};
	signed char num_syllable_modifier = {};
	bool affixed_and_modified = {}; /**< non-zero affix */
	operator Word_List::const_pointer() const { return word_entry; }
	auto& operator*() const { return *word_entry; }
	auto operator->() const { return word_entry; }
};

struct NUSPELL_EXPORT Dict_Base : public Aff_Data {

	enum Forceucase : bool {
		FORBID_BAD_FORCEUCASE = false,
		ALLOW_BAD_FORCEUCASE = true
	};

	enum Hidden_Homonym : bool {
		ACCEPT_HIDDEN_HOMONYM = false,
		SKIP_HIDDEN_HOMONYM = true
	};

	enum High_Quality_Sugs : bool {
		ALL_LOW_QUALITY_SUGS = false,
		HAS_HIGH_QUALITY_SUGS = true
	};

	auto spell_priv(std::wstring& s) const -> bool;
	auto spell_break(std::wstring& s, size_t depth = 0) const -> bool;
	auto spell_casing(std::wstring& s) const -> const Flag_Set*;
	auto spell_casing_upper(std::wstring& s) const -> const Flag_Set*;
	auto spell_casing_title(std::wstring& s) const -> const Flag_Set*;
	auto spell_sharps(std::wstring& base, size_t n_pos = 0, size_t n = 0,
	                  size_t rep = 0) const -> const Flag_Set*;

	auto check_word(std::wstring& s, Forceucase allow_bad_forceucase = {},
	                Hidden_Homonym skip_hidden_homonym = {}) const
	    -> const Flag_Set*;
	auto check_simple_word(std::wstring& word,
	                       Hidden_Homonym skip_hidden_homonym = {}) const
	    -> const Flag_Set*;

	template <Affixing_Mode m>
	auto affix_NOT_valid(const Prefix<wchar_t>& a) const;
	template <Affixing_Mode m>
	auto affix_NOT_valid(const Suffix<wchar_t>& a) const;
	template <Affixing_Mode m, class AffixT>
	auto outer_affix_NOT_valid(const AffixT& a) const;
	template <class AffixT>
	auto is_circumfix(const AffixT& a) const;
	template <Affixing_Mode m>
	auto is_valid_inside_compound(const Flag_Set& flags) const;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_prefix_only(std::wstring& s,
	                       Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Prefix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_suffix_only(std::wstring& s,
	                       Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Suffix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto
	strip_prefix_then_suffix(std::wstring& s,
	                         Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>;

	template <Affixing_Mode m>
	auto strip_pfx_then_sfx_2(const Prefix<wchar_t>& pe, std::wstring& s,
	                          Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto
	strip_suffix_then_prefix(std::wstring& s,
	                         Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Prefix<wchar_t>, Suffix<wchar_t>>;

	template <Affixing_Mode m>
	auto strip_sfx_then_pfx_2(const Suffix<wchar_t>& se, std::wstring& s,
	                          Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<Prefix<wchar_t>, Suffix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_prefix_then_suffix_commutative(
	    std::wstring& word, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_pfx_then_sfx_comm_2(const Prefix<wchar_t>& pe,
	                               std::wstring& word,
	                               Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<Suffix<wchar_t>, Prefix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto
	strip_suffix_then_suffix(std::wstring& s,
	                         Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Suffix<wchar_t>, Suffix<wchar_t>>;

	template <Affixing_Mode m>
	auto strip_sfx_then_sfx_2(const Suffix<wchar_t>& se1, std::wstring& s,
	                          Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<Suffix<wchar_t>, Suffix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto
	strip_prefix_then_prefix(std::wstring& s,
	                         Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<Prefix<wchar_t>, Prefix<wchar_t>>;

	template <Affixing_Mode m>
	auto strip_pfx_then_pfx_2(const Prefix<wchar_t>& pe1, std::wstring& s,
	                          Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<Prefix<wchar_t>, Prefix<wchar_t>>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_prefix_then_2_suffixes(
	    std::wstring& s, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m>
	auto strip_pfx_2_sfx_3(const Prefix<wchar_t>& pe1,
	                       const Suffix<wchar_t>& se1, std::wstring& s,
	                       Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_suffix_prefix_suffix(
	    std::wstring& s, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m>
	auto strip_s_p_s_3(const Suffix<wchar_t>& se1,
	                   const Prefix<wchar_t>& pe1, std::wstring& word,
	                   Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_2_suffixes_then_prefix(
	    std::wstring& s, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m>
	auto strip_2_sfx_pfx_3(const Suffix<wchar_t>& se1,
	                       const Suffix<wchar_t>& se2, std::wstring& word,
	                       Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_suffix_then_2_prefixes(
	    std::wstring& s, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m>
	auto strip_sfx_2_pfx_3(const Suffix<wchar_t>& se1,
	                       const Prefix<wchar_t>& pe1, std::wstring& s,
	                       Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_prefix_suffix_prefix(
	    std::wstring& word, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m>
	auto strip_p_s_p_3(const Prefix<wchar_t>& pe1,
	                   const Suffix<wchar_t>& se1, std::wstring& word,
	                   Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD>
	auto strip_2_prefixes_then_suffix(
	    std::wstring& word, Hidden_Homonym skip_hidden_homonym = {}) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m>
	auto strip_2_pfx_sfx_3(const Prefix<wchar_t>& pe1,
	                       const Prefix<wchar_t>& pe2, std::wstring& word,
	                       Hidden_Homonym skip_hidden_homonym) const
	    -> Affixing_Result<>;

	auto check_compound(std::wstring& word,
	                    Forceucase allow_bad_forceucase) const
	    -> Compounding_Result;

	template <Affixing_Mode m = AT_COMPOUND_BEGIN>
	auto check_compound(std::wstring& word, size_t start_pos,
	                    size_t num_part, std::wstring& part,
	                    Forceucase allow_bad_forceucase) const
	    -> Compounding_Result;

	template <Affixing_Mode m = AT_COMPOUND_BEGIN>
	auto check_compound_classic(std::wstring& word, size_t start_pos,
	                            size_t i, size_t num_part,
	                            std::wstring& part,
	                            Forceucase allow_bad_forceucase) const
	    -> Compounding_Result;

	template <Affixing_Mode m = AT_COMPOUND_BEGIN>
	auto check_compound_with_pattern_replacements(
	    std::wstring& word, size_t start_pos, size_t i, size_t num_part,
	    std::wstring& part, Forceucase allow_bad_forceucase) const
	    -> Compounding_Result;

	template <Affixing_Mode m>
	auto check_word_in_compound(std::wstring& s) const
	    -> Compounding_Result;

	auto calc_num_words_modifier(const Prefix<wchar_t>& pfx) const
	    -> unsigned char;

	template <Affixing_Mode m>
	auto calc_syllable_modifier(Word_List::const_reference we) const
	    -> signed char;

	template <Affixing_Mode m>
	auto calc_syllable_modifier(Word_List::const_reference we,
	                            const Suffix<wchar_t>& sfx) const
	    -> signed char;

	auto count_syllables(const std::wstring& word) const -> size_t;

	auto check_compound_with_rules(std::wstring& word,
	                               std::vector<const Flag_Set*>& words_data,
	                               size_t start_pos, std::wstring& part,
	                               Forceucase allow_bad_forceucase) const

	    -> Compounding_Result;

	auto suggest_priv(std::wstring& word, List_WStrings& out) const -> void;

	auto suggest_low(std::wstring& word, List_WStrings& out) const
	    -> High_Quality_Sugs;

	auto add_sug_if_correct(std::wstring& word, List_WStrings& out) const
	    -> bool;

	auto uppercase_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto rep_suggest(std::wstring& word, List_WStrings& out) const -> void;

	auto try_rep_suggestion(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto is_rep_similar(std::wstring& word) const -> bool;

	auto map_suggest(std::wstring& word, List_WStrings& out,
	                 size_t i = 0) const -> void;

	auto adjacent_swap_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto distant_swap_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto keyboard_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto extra_char_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto forgotten_char_suggest(std::wstring& word,
	                            List_WStrings& out) const -> void;

	auto move_char_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto bad_char_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto doubled_two_chars_suggest(std::wstring& word,
	                               List_WStrings& out) const -> void;

	auto two_words_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto phonetic_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto ngram_suggest(std::wstring& word, List_WStrings& out) const
	    -> void;

	auto expand_root_word_for_ngram(Word_List::const_reference root,
	                                std::wstring_view wrong,
	                                List_WStrings& expanded_list,
	                                std::vector<bool>& cross_affix) const
	    -> void;

      public:
	Dict_Base()
	    : Aff_Data() // we explicity do value init so content is zeroed
	{
	}
};

/**
 * @brief The only important public exception
 */
class NUSPELL_EXPORT Dictionary_Loading_Error : public std::runtime_error {
      public:
	using std::runtime_error::runtime_error;
};

/**
 * @brief The only important public class
 */
class NUSPELL_EXPORT Dictionary : private Dict_Base {
	std::locale external_locale;
	bool external_locale_known_utf8;

	Dictionary(std::istream& aff, std::istream& dic);
	auto external_to_internal_encoding(std::string_view in,
	                                   std::wstring& wide_out) const
	    -> bool;

	auto internal_to_external_encoding(std::wstring_view wide_in,
	                                   std::string& out) const -> bool;

      public:
	Dictionary();
	auto static load_from_aff_dic(std::istream& aff, std::istream& dic)
	    -> Dictionary;
	auto static load_from_path(
	    const std::string& file_path_without_extension) -> Dictionary;
	auto imbue(const std::locale& loc) -> void;
	auto imbue_utf8() -> void;
	auto spell(std::string_view word) const -> bool;
	auto suggest(std::string_view word, std::vector<std::string>& out) const
	    -> void;
};
} // namespace v4
} // namespace nuspell
NUSPELL_MSVC_PRAGMA_WARNING(pop)
#endif // NUSPELL_DICTIONARY_HXX
