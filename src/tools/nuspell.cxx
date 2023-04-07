/* Copyright 2016-2023 Dimitrij Mijoski
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

#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>

#include <cassert>
#include <clocale>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unicode/brkiter.h>
#include <unicode/ucnv.h>

#include <getopt.h>

#if __has_include(<unistd.h>)
#include <unistd.h> // defines _POSIX_VERSION
#endif
#ifdef _POSIX_VERSION
#include <langinfo.h>
#endif
#ifdef _WIN32
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// manually define if not supplied by the build system
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown.version"
#endif

using namespace std;
using nuspell::Dictionary, nuspell::Dictionary_Loading_Error,
    nuspell::Dict_Finder_For_CLI_Tool_2;
namespace {
enum Mode { NORMAL, HELP, VERSION, LIST_DICTS };
auto print_help(const char* program_name) -> void
{
	auto p = string_view(program_name);
	auto& o = cout;
	o << "Usage:\n"
	  << p << " [-d dict_NAME] [OPTION]... [FILE...]\n"
	  << p << " -D|--help|--version\n"
	  << R"(
Check spelling of each FILE. If no FILE is specified, check standard input.
The text in the input is first segmented into words with an algorithm
that recognizes punctuation and then each word is checked.

  -d, --dictionary=di_CT    use di_CT dictionary (only one is supported)
  -D, --list-dictionaries   print search paths and available dictionaries
  --encoding=enc            set both input and output encoding
  --input-encoding=enc      input encoding, default is active locale
  --output-encoding=enc     output encoding, default is active locale
  --help                    print this help
  --version                 print version number

One dictionary consists of two files with extensions .dic and .aff.
The -d option accepts either dictionary name without filename extension or a
path with slash (and with extension) to the .aff file of the dictionary. When
just a name is given, it will be searched among the list of dictionaries in the
default directories (see option -D). When a path to .aff is given, only the
dictionary under the path is considered.

The following environment variables can have effect:

  DICTIONARY - same as -d,
  DICPATH    - additional directory path to search for dictionaries.

Example:
)"
	  << "    " << p << " -d en_US file.txt\n"
	  << "    " << p << " -d ../../subdir/di_CT.aff\n"
	  << R"(
Bug reports: <https://github.com/nuspell/nuspell/issues>
Full documentation: <https://github.com/nuspell/nuspell/wiki>
Home page: <http://nuspell.github.io/>
)";
}

auto ver_str = "nuspell " PROJECT_VERSION R"(
Copyright (C) 2016-2023 Dimitrij Mijoski
License LGPLv3+: GNU LGPL version 3 or later <http://gnu.org/licenses/lgpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by Dimitrij Mijoski.
)";

auto print_version() -> void { cout << ver_str; }

auto list_dictionaries(const Dict_Finder_For_CLI_Tool_2& f) -> void
{
	if (empty(f.get_dir_paths())) {
		cout << "No search paths available" << '\n';
	}
	else {
		cout << "Search paths:" << '\n';
		for (auto& p : f.get_dir_paths()) {
			cout << p.string() << '\n';
		}
	}
	auto dicts = vector<filesystem::path>();
	nuspell::search_dirs_for_dicts(f.get_dir_paths(), dicts);
	if (empty(dicts)) {
		cout << "No dictionaries available\n";
	}
	else {
		stable_sort(begin(dicts), end(dicts));
		cout << "Available dictionaries:\n";
		for (auto& d : dicts) {
			cout << left << setw(15) << d.stem().string() << ' '
			     << d.string() << '\n';
		}
	}
}

auto from_utf8(string_view source, string& dest, UConverter* ucnv,
               UErrorCode& uerr)
{
	dest.resize(dest.capacity());
	auto len =
	    ucnv_fromAlgorithmic(ucnv, UCNV_UTF8, dest.data(), dest.size(),
	                         source.data(), source.size(), &uerr);
	dest.resize(len);
	if (uerr == U_BUFFER_OVERFLOW_ERROR) {
		uerr = U_ZERO_ERROR;
		ucnv_fromAlgorithmic(ucnv, UCNV_UTF8, dest.data(), dest.size(),
		                     source.data(), source.size(), &uerr);
	}
}

auto to_unicode_string(string_view source, icu::UnicodeString& dest,
                       UConverter* ucnv, UErrorCode& uerr)
{
	auto buf = dest.getBuffer(-1);
	auto len = ucnv_toUChars(ucnv, buf, dest.getCapacity(), source.data(),
	                         source.size(), &uerr);
	if (uerr == U_BUFFER_OVERFLOW_ERROR) {
		uerr = U_ZERO_ERROR;
		dest.releaseBuffer(0);
		buf = dest.getBuffer(len);
		if (!buf)
			throw bad_alloc();
		len = ucnv_toUChars(ucnv, buf, dest.getCapacity(),
		                    source.data(), source.size(), &uerr);
	}
	dest.releaseBuffer(len);
}

auto process_word_utf8_output_enc(const Dictionary& dic, string_view word,
                                  vector<string>& suggestions, ostream& out)
{
	auto correct = dic.spell(word);
	if (correct) {
		out << "* OK\n";
		return;
	}
	dic.suggest(word, suggestions);
	if (suggestions.empty()) {
		out << "# Wrong: " << word << ". No suggestions.\n";
		return;
	}
	out << "& Wrong: " << word << ". How about: ";
	out << suggestions[0];
	for_each(begin(suggestions) + 1, end(suggestions),
	         [&](auto& sug) { out << ", " << sug; });
	out << '\n';
}

auto process_word_any_output_enc(const Dictionary& dic, string_view u8word,
                                 vector<string>& suggestions, ostream& out,
                                 UConverter* out_cnv, UErrorCode& uerr)
{
	auto correct = dic.spell(u8word);
	if (correct) {
		out << "* OK\n";
		return;
	}
	dic.suggest(u8word, suggestions);
	auto encoded_out = string();
	from_utf8(u8word, encoded_out, out_cnv, uerr);
	if (suggestions.empty()) {
		out << "# Wrong: " << encoded_out << ". No suggestions.\n";
		return;
	}
	out << "& Wrong:  " << encoded_out << ". How about: ";
	from_utf8(suggestions[0], encoded_out, out_cnv, uerr);
	out << encoded_out;
	for_each(begin(suggestions) + 1, end(suggestions),
	         [&](const string& u8sug) {
		         out << ", ";
		         from_utf8(u8sug, encoded_out, out_cnv, uerr);
		         out << encoded_out;
	         });
	out << '\n';
}

auto is_word_break(int32_t typ)
{
	return (UBRK_WORD_NUMBER <= typ && typ < UBRK_WORD_NUMBER_LIMIT) ||
	       (UBRK_WORD_LETTER <= typ && typ < UBRK_WORD_LETTER_LIMIT) ||
	       (UBRK_WORD_KANA <= typ && typ < UBRK_WORD_KANA_LIMIT) ||
	       (UBRK_WORD_IDEO <= typ && typ < UBRK_WORD_IDEO_LIMIT);
}

auto process_line_utf8_input_enc(const Dictionary& dic, const string& line,
                                 UText* utext, icu::BreakIterator* ubrkiter,
                                 vector<string>& suggestions, ostream& out,
                                 UConverter* out_cnv, UErrorCode& uerr)
{
	utext_openUTF8(utext, line.data(), line.size(), &uerr);
	ubrkiter->setText(utext, uerr);
	auto is_utf8_out = ucnv_getType(out_cnv) == UCNV_UTF8;
	for (auto i = ubrkiter->first(), prev = 0; i != ubrkiter->DONE;
	     prev = i, i = ubrkiter->next()) {
		auto typ = ubrkiter->getRuleStatus();
		if (is_word_break(typ)) {
			auto word = string_view(line).substr(prev, i - prev);
			if (is_utf8_out)
				process_word_utf8_output_enc(dic, word,
				                             suggestions, out);
			else
				process_word_any_output_enc(
				    dic, word, suggestions, out, out_cnv, uerr);
		}
	}
	assert(U_SUCCESS(uerr));
}

auto process_line_any_input_enc(const Dictionary& dic, const string& line,
                                icu::UnicodeString& uline, UConverter* in_cnv,
                                icu::BreakIterator* ubrkiter, UErrorCode& uerr,
                                string& u8word, vector<string>& suggestions,
                                ostream& out, UConverter* out_cnv)
{
	to_unicode_string(line, uline, in_cnv, uerr);
	ubrkiter->setText(uline);
	/* size_t orig_prev = 0, orig_i = 0;
	auto src = line.c_str();
	auto src_end = src + line.size(); */
	auto is_utf8_out = ucnv_getType(out_cnv) == UCNV_UTF8;
	ucnv_resetToUnicode(in_cnv);
	for (auto i = ubrkiter->first(), prev = 0; i != ubrkiter->DONE;
	     prev = i, i = ubrkiter->next() /*, orig_prev = orig_i*/) {

		/* for (auto j = prev; j != i; ++j) {
		        auto cp =
		            ucnv_getNextUChar(in_cnv, &src, src_end, &uerr);

		        // U_IS_SURROGATE(uline[j]) or
		        // U_IS_LEAD(uline[j]) can work too
		        j += !U_IS_BMP(cp);
		}
		orig_i = distance(line.c_str(), src); */

		auto typ = ubrkiter->getRuleStatus();
		if (is_word_break(typ)) {
			auto uword = uline.tempSubStringBetween(prev, i);
			u8word.clear();
			uword.toUTF8String(u8word);
			/* auto enc_word = string_view(line).substr(
			    orig_prev, orig_i - orig_prev); */

			if (is_utf8_out)
				process_word_utf8_output_enc(dic, u8word,
				                             suggestions, out);
			else
				process_word_any_output_enc(dic, u8word,
				                            suggestions, out,
				                            out_cnv, uerr);
		}
	}
	assert(U_SUCCESS(uerr));
}

auto process_text(const Dictionary& dic, istream& in, UConverter* in_cnv,
                  ostream& out, UConverter* out_cnv, UErrorCode& uerr)
{
	auto line = string();
	auto suggestions = vector<string>();
	auto wrong_words = vector<string_view>();

	// TODO: try to use Locale constructed from dictionary name.
	auto ubrkiter = unique_ptr<icu::BreakIterator>(
	    icu::BreakIterator::createWordInstance(icu::Locale(), uerr));
	auto utext = icu::LocalUTextPointer(
	    utext_openUTF8(nullptr, line.data(), line.size(), &uerr));
	auto uline = icu::UnicodeString();
	auto u8word = string();
	auto is_utf8 = ucnv_getType(in_cnv) == UCNV_UTF8;

	if (&in == &cin)
		out << "Enter some text: ";
	while (getline(in, line)) {
		wrong_words.clear();
		if (is_utf8)
			process_line_utf8_input_enc(dic, line, utext.getAlias(),
			                            ubrkiter.get(), suggestions,
			                            out, out_cnv, uerr);
		else
			process_line_any_input_enc(dic, line, uline, in_cnv,
			                           ubrkiter.get(), uerr, u8word,
			                           suggestions, out, out_cnv);

		out << '\n'; // In NORMAL mode put empty line for each input
		             // line.
	}
}
} // namespace
int main(int argc, char* argv[])
{
	auto mode_int = int(Mode::NORMAL);
	auto program_name = "nuspell";
	auto dictionary = string();
	auto input_enc = string();
	auto output_enc = string();

	if (argc > 0 && argv[0])
		program_name = argv[0];

	ios_base::sync_with_stdio(false);

	auto optstring = "d:D";
	option longopts[] = {
	    {"help", no_argument, &mode_int, Mode::HELP},
	    {"version", no_argument, &mode_int, Mode::VERSION},
	    {"list-dictionaries", no_argument, &mode_int, Mode::LIST_DICTS},
	    {"dictionary", required_argument, nullptr, 'd'},
	    {"encoding", required_argument, nullptr, 'e'},
	    {"input-encoding", required_argument, nullptr, 'i'},
	    {"output-encoding", required_argument, nullptr, 'o'},
	    {}};
	int longindex;
	int c;
	while ((c = getopt_long(argc, argv, optstring, longopts, &longindex)) !=
	       -1) {
		switch (c) {
		case 0:
			// check longopts[longindex] if needed
			break;
		case 'd':
			dictionary = optarg;
			break;
		case 'D':
			mode_int = Mode::LIST_DICTS;
			break;
		case 'e':
			input_enc = optarg;
			output_enc = optarg;
			break;
		case 'i':
			input_enc = optarg;
			break;
		case 'o':
			output_enc = optarg;
			break;
		case '?':
			return EXIT_FAILURE;
		}
	}
	auto mode = static_cast<Mode>(mode_int);
	if (mode == Mode::VERSION) {
		print_version();
		return 0;
	}
	else if (mode == Mode::HELP) {
		print_help(program_name);
		return 0;
	}
	auto f = Dict_Finder_For_CLI_Tool_2();
	if (mode == Mode::LIST_DICTS) {
		list_dictionaries(f);
		return 0;
	}

	char* loc_str = nullptr;
#if _WIN32
	loc_str = setlocale(LC_CTYPE, nullptr); // will return "C"

	/* On Windows, the console is a buggy thing. If the default C locale is
	active, then the encoding of the strings gotten from C or C++ stdio
	(fgets, scanf, cin) is GetConsoleCP(). Stdout accessed via standard
	functions (printf, cout) expects encoding of GetConsoleOutputCP() which
	is the same as GetConsoleCP() unless manually changed. By default both
	are the active OEM encoding, unless changed with the command chcp, or by
	calling the Set functions.

	If we call setlocale(LC_CTYPE, ""), or let's say setlocale(LC_CTYPE,
	".1251"), then stdin will still return in the encoding GetConsoleCP(),
	but stdout functions like printf now will expect a different encoding,
	the one set via setlocale. Because of this mess don't change locale with
	setlocale on Windows.

	When stdin or stout are redirected from/to file or another terminal like
	the one in MSYS2, they are read/written as-is. Then we will assume UTF-8
	encoding. */
#else
	loc_str = setlocale(LC_CTYPE, "");
	if (!loc_str) {
		clog << "WARNING: Can not set to system locale, fall back to "
		        "\"C\".\n";
		loc_str = setlocale(LC_CTYPE, nullptr); // will return "C"
	}
#endif
#if _POSIX_VERSION
	auto enc_str = nl_langinfo(CODESET);
	if (input_enc.empty())
		input_enc = enc_str;
	if (output_enc.empty())
		output_enc = enc_str;
#elif _WIN32
	if (optind == argc && _isatty(_fileno(stdin)))
		input_enc = "cp" + to_string(GetConsoleCP());
	else if (input_enc.empty())
		input_enc = "UTF-8";
	if (_isatty(_fileno(stdout)))
		output_enc = "cp" + to_string(GetConsoleOutputCP());
	else if (output_enc.empty())
		output_enc = "UTF-8";
#endif
	auto loc_str_sv = string_view(loc_str);
	clog << "INFO: Locale LC_CTYPE=" << loc_str_sv
	     << ", Input encoding=" << input_enc
	     << ", Output encoding=" << output_enc << endl;

	if (dictionary.empty()) {
		auto denv = getenv("DICTIONARY");
		if (denv)
			dictionary = denv;
	}
	if (dictionary.empty()) {
		// infer dictionary from locale
		auto idx = min(loc_str_sv.find('.'), loc_str_sv.find('@'));
		dictionary = loc_str_sv.substr(0, idx);
	}
	if (dictionary.empty()) {
		clog << "ERROR: No dictionary provided and can not infer from "
		        "OS locale\n";
		return EXIT_FAILURE;
	}
	auto filename = f.get_dictionary_path(dictionary);
	if (filename.empty()) {
		clog << "ERROR: Dictionary " << dictionary << " not found\n";
		return EXIT_FAILURE;
	}
	clog << "INFO: Pointed dictionary " << filename.string() << endl;
	auto dic = Dictionary();
	try {
		dic.load_aff_dic_internal(filename, clog);
	}
	catch (const Dictionary_Loading_Error& e) {
		clog << "ERROR: " << e.what() << '\n';
		return EXIT_FAILURE;
	}
	// ICU reports all types of errors, logic errors and runtime errors
	// using this enum. We should not check for logic errors, they should
	// not happened. Optionally, only assert that they are not there can be
	// used. We should check for runtime errors.
	// The encoding conversion is a common case where runtime error can
	// happen, but by default ICU uses Unicode replacement character on
	// errors and reprots success. This can be changed, but there is no need
	// for that.
	auto uerr = U_ZERO_ERROR;
	auto inp_enc_cstr = input_enc.c_str();
	if (input_enc.empty()) {
		inp_enc_cstr = nullptr;
		clog << "WARNING: using default ICU encoding converter for IO"
		     << endl;
	}
	auto in_ucnv =
	    icu::LocalUConverterPointer(ucnv_open(inp_enc_cstr, &uerr));
	if (U_FAILURE(uerr)) {
		clog << "ERROR: Invalid encoding " << input_enc << ".\n";
		return EXIT_FAILURE;
	}

	auto out_ucnv_smart_ptr = icu::LocalUConverterPointer();
	auto out_ucnv = in_ucnv.getAlias(); // often output_enc is same as input
	if (output_enc != input_enc) {
		auto output_enc_cstr = output_enc.c_str();
		if (output_enc.empty()) {
			output_enc_cstr = nullptr;
			clog << "WARNING: using default ICU encoding converter "
			        "for IO"
			     << endl;
		}
		out_ucnv = ucnv_open(output_enc_cstr, &uerr);
		out_ucnv_smart_ptr.adoptInstead(out_ucnv);
		if (U_FAILURE(uerr)) {
			clog << "ERROR: Invalid encoding " << output_enc
			     << ".\n";
			return EXIT_FAILURE;
		}
	}

	if (optind == argc) {
		process_text(dic, cin, in_ucnv.getAlias(), cout, out_ucnv,
		             uerr);
	}
	else {
		for (; optind != argc; ++optind) {
			auto file_name = argv[optind];
			ifstream in(file_name);
			if (!in.is_open()) {
				clog << "ERROR: Can't open " << file_name
				     << '\n';
				return EXIT_FAILURE;
			}
			process_text(dic, in, in_ucnv.getAlias(), cout,
			             out_ucnv, uerr);
		}
	}
}
