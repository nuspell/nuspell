/* Copyright 2016-2024 Dimitrij Mijoski
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

#include "finder.hxx"
#include "utils.hxx"

#include <algorithm>
#include <filesystem>
#include <set>

#if __has_include(<unistd.h>)
#include <unistd.h> // defines _POSIX_VERSION
#endif
#if _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace std;

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

using fs_path = std::filesystem::path;
using std::filesystem::directory_entry;
using std::filesystem::directory_iterator;
using std::filesystem::filesystem_error;

template <class CharT>
static auto& split_to_paths(basic_string_view<CharT> s, CharT sep,
                            vector<fs_path>& out)
{
	for (size_t i1 = 0;;) {
		auto i2 = s.find(sep, i1);
		out.emplace_back(s.substr(i1, i2 - i1));
		if (i2 == s.npos)
			break;
		i1 = i2 + 1;
	}
	return out;
}

/**
 * @brief Append the paths of the default directories to be searched for
 * dictionaries.
 * @param[out] paths vector that receives the directory paths
 */
auto append_default_dir_paths(vector<fs_path>& paths) -> void
{
#ifdef _WIN32
	const auto PATHSEP = L';';
	auto dicpath = _wgetenv(L"DICPATH");
#else
	const auto PATHSEP = ':';
	auto dicpath = getenv("DICPATH");
#endif
	if (dicpath && *dicpath)
		split_to_paths(basic_string_view(dicpath), PATHSEP, paths);

#ifdef _POSIX_VERSION
	auto xdg_data_home = getenv("XDG_DATA_HOME");
	if (xdg_data_home && *xdg_data_home)
		paths.push_back(fs_path(xdg_data_home) / "hunspell");
	else if (auto home = getenv("HOME"))
		paths.push_back(fs_path(home) / ".local/share/hunspell");

	auto xdg_data_dirs = getenv("XDG_DATA_DIRS");
	if (xdg_data_dirs && *xdg_data_dirs) {
		auto data_dirs = string_view(xdg_data_dirs);

		auto i = size(paths);
		split_to_paths(data_dirs, PATHSEP, paths);
		for (; i != size(paths); ++i)
			paths[i] /= "hunspell";

		i = size(paths);
		split_to_paths(data_dirs, PATHSEP, paths);
		for (; i != size(paths); ++i)
			paths[i] /= "myspell";
	}
	else {
		paths.push_back("/usr/local/share/hunspell");
		paths.push_back("/usr/share/hunspell");
		paths.push_back("/usr/local/share/myspell");
		paths.push_back("/usr/share/myspell");
	}
#if defined(__APPLE__) && defined(__MACH__)
	if (auto home = getenv("HOME"))
		paths.push_back(fs_path(home) / "Library/Spelling");
#endif
#endif
#if _WIN32
	for (auto& e : {L"LOCALAPPDATA", L"PROGRAMDATA"}) {
		auto p = _wgetenv(e);
		if (p)
			paths.push_back(fs_path(p) / L"hunspell");
	}
#endif
}

static auto append_lo_global(const fs_path& p, vector<fs_path>& paths) -> void
{
	for (auto& de : directory_iterator(p)) {
		if (!de.is_directory())
			continue;
		if (!begins_with(de.path().filename().string(), "dict-"))
			continue;
		paths.push_back(de);
	}
}

static auto append_lo_user(const directory_entry& de1,
                           vector<fs_path>& paths) -> void
{
	for (auto& de2 : directory_iterator(de1)) {
		try {
			if (!de2.is_directory())
				continue;
			if (de2.path().extension() != ".oxt")
				continue;
			for (auto& de3 : directory_iterator(de2)) {
				if (de3.is_directory() &&
				    begins_with(de3.path().filename().string(),
				                "dict")) {
					paths.push_back(de3);
				}
				else if (de3.is_regular_file() &&
				         de3.path().extension() == ".aff") {
					paths.push_back(de2);
					break;
				}
			}
		}
		catch (const filesystem_error&) {
		}
	}
}

/**
 * @brief Append the paths of the LibreOffice's directories to be searched for
 * dictionaries.
 *
 * @warning This function shall not be called from LibreOffice or modules that
 * may end up being used by LibreOffice. It is mainly intended to be used by
 * the CLI tool.
 *
 * @param[out] paths vector that receives the directory paths
 */
auto append_libreoffice_dir_paths(vector<fs_path>& paths) -> void
{
	using namespace filesystem;
	auto p1 = path();
	// add LibreOffice global paths
	try {
#if _WIN32
		auto lo_dir = wstring(MAX_PATH - 1, '*');
		// size in bytes including the zero teminator
		DWORD lo_dir_sz = (size(lo_dir) + 1) * sizeof(wchar_t);
		auto subkey = L"SOFTWARE\\LibreOffice\\UNO\\InstallPath";
		auto regerr = RegGetValueW(HKEY_LOCAL_MACHINE, subkey, nullptr,
		                           RRF_RT_REG_SZ, nullptr, data(lo_dir),
		                           &lo_dir_sz);
		lo_dir.resize(lo_dir_sz / sizeof(wchar_t) - 1);
		if (regerr == ERROR_MORE_DATA) {
			regerr = RegGetValueW(HKEY_LOCAL_MACHINE, subkey,
			                      nullptr, RRF_RT_REG_SZ, nullptr,
			                      data(lo_dir), &lo_dir_sz);
		}
		if (regerr == ERROR_SUCCESS) {
			p1 = lo_dir;
			p1.replace_filename("share\\extensions");
			append_lo_global(p1, paths);
		}
#elif defined(__APPLE__) && defined(__MACH__)
		p1 = "/Applications/LibreOffice.app/Contents/Resources/"
		     "extensions";
		append_lo_global(p1, paths);
#elif _POSIX_VERSION
		p1 = "/opt";
		for (auto& de1 : directory_iterator(p1)) {
			try {
				if (!de1.is_directory())
					continue;
				if (!begins_with(de1.path().filename().string(),
				                 "libreoffice "))
					continue;
				append_lo_global(
				    de1.path() / "share/extensions", paths);
			}
			catch (const filesystem_error&) {
			}
		}
#endif
	}
	catch (const filesystem_error&) {
	}

	// add LibreOffice user paths
	p1.clear();
	try {
#if _WIN32
		auto appdata = _wgetenv(L"APPDATA");
		if (!appdata)
			return;
		p1 = appdata;
#elif _POSIX_VERSION
		auto home = getenv("HOME");
		if (!home)
			return;
		p1 = home;
#if defined(__APPLE__) && defined(__MACH__)
		p1 /= "Library/Application Support";
#else
		p1 /= ".config";
#endif
#else
		return;
#endif
		p1 /= "libreoffice/4/user/uno_packages/cache/uno_packages";
		p1.make_preferred();
		for (auto& de1 : directory_iterator(p1)) {
			try {
				if (de1.is_directory())
					append_lo_user(de1, paths);
			}
			catch (const filesystem_error&) {
			}
		}
	}
	catch (const filesystem_error&) {
	}
}

/**
 * @brief Serach the directories for only one dictionary
 *
 * This function is more efficient than search_dirs_for_dicts() because it
 * does not iterate whole directories, it only checks the existance of .dic and
 * .aff files. Useful for some CLI tools. GUI apps generally need a list of all
 * dictionaries.
 *
 * @param[in] dir_paths list of directories
 * @param[in] dict_name_stem dictionary name, filename without extension (stem)
 * @return path to the .aff file of the dictionary or empty object if not found
 */
auto search_dirs_for_one_dict(const vector<fs_path>& dir_paths,
                              const fs_path& dict_name_stem) -> fs_path
{
	using std::filesystem::is_regular_file;
	auto fp = fs_path();
	for (auto& dir : dir_paths) {
		fp = dir;
		fp /= dict_name_stem;
		fp += ".dic";
		if (is_regular_file(fp)) {
			fp.replace_extension(".aff");
			if (is_regular_file(fp))
				return fp;
		}
	}
	fp.clear();
	return fp;
}

static auto search_dir_for_dicts(const fs_path& dir_path,
                                 vector<fs_path>& dict_list) -> void
try {
	using namespace filesystem;
	auto dics = set<path>();
	for (auto& entry : directory_iterator(dir_path)) {
		if (!entry.is_regular_file())
			continue;
		auto& fp = entry.path();
		auto ext = fp.extension();
		if (ext == ".dic" || ext == ".aff") {
			auto [it, inserted] = dics.insert(fp.stem());
			if (!inserted) // already existed
				dict_list.emplace_back(fp).replace_extension(
				    ".aff");
		}
	}
}
catch (const filesystem_error&) {
}

/**
 * @brief Search the directories for dictionaries.
 *
 * This function searches the directories for files that represent dictionaries
 * and for each found dictionary it appends the path of the .aff file to @p
 * dict_list. One dictionary consts of two files, .aff and .dic, and both need
 * to exist, but only the .aff is added.
 *
 * @param[in]  dir_paths list of paths to directories
 * @param[out] dict_list vector that receives the paths of the found
 * dictionaries
 */
auto search_dirs_for_dicts(const vector<fs_path>& dir_paths,
                           vector<fs_path>& dict_list) -> void
{
	for (auto& p : dir_paths)
		search_dir_for_dicts(p, dict_list);
}

/**
 * @brief Search the default directories for dictionaries.
 *
 * This is just a convenience that call two other functions.
 * @see append_default_dir_paths()
 * @see search_dirs_for_dicts()
 * @return vector with the paths of the .aff files of the found dictionaries
 */
auto search_default_dirs_for_dicts() -> vector<fs_path>
{
	auto dir_paths = vector<fs_path>();
	auto dict_list = vector<fs_path>();
	append_default_dir_paths(dir_paths);
	search_dirs_for_dicts(dir_paths, dict_list);
	return dict_list;
}

auto append_default_dir_paths(std::vector<std::string>& paths) -> void
{
	auto out = vector<fs_path>();
	append_default_dir_paths(out);
	transform(begin(out), end(out), back_inserter(paths),
	          [](const fs_path& p) { return p.string(); });
}

auto append_libreoffice_dir_paths(std::vector<std::string>& paths) -> void
{
	auto out = vector<fs_path>();
	append_libreoffice_dir_paths(out);
	transform(begin(out), end(out), back_inserter(paths),
	          [](const fs_path& p) { return p.string(); });
}

static auto
new_to_old_dict_list(vector<fs_path>& new_aff_paths,
                     vector<pair<string, string>>& dict_list) -> void
{
	transform(begin(new_aff_paths), end(new_aff_paths),
	          back_inserter(dict_list), [](const fs_path& p) {
		          return pair{p.stem().string(),
		                      fs_path(p).replace_extension().string()};
	          });
}

auto search_dir_for_dicts(const string& dir_path,
                          vector<pair<string, string>>& dict_list) -> void
{
	auto new_dict_list = vector<fs_path>();
	search_dir_for_dicts(dir_path, new_dict_list);
	new_to_old_dict_list(new_dict_list, dict_list);
}

auto search_dirs_for_dicts(const std::vector<string>& dir_paths,
                           std::vector<std::pair<string, string>>& dict_list)
    -> void
{
	auto new_dict_list = vector<fs_path>();
	for (auto& p : dir_paths)
		search_dir_for_dicts(p, new_dict_list);
	new_to_old_dict_list(new_dict_list, dict_list);
}

auto search_default_dirs_for_dicts(
    std::vector<std::pair<std::string, std::string>>& dict_list) -> void
{
	auto new_dir_paths = vector<fs_path>();
	auto new_dict_list = vector<fs_path>();
	append_default_dir_paths(new_dir_paths);
	search_dirs_for_dicts(new_dir_paths, new_dict_list);
	new_to_old_dict_list(new_dict_list, dict_list);
}

auto find_dictionary(
    const std::vector<std::pair<std::string, std::string>>& dict_list,
    const std::string& dict_name)
    -> std::vector<std::pair<std::string, std::string>>::const_iterator
{
	return find_if(begin(dict_list), end(dict_list),
	               [&](auto& e) { return e.first == dict_name; });
}

Dict_Finder_For_CLI_Tool::Dict_Finder_For_CLI_Tool() {}
auto Dict_Finder_For_CLI_Tool::get_dictionary_path(const std::string&) const
    -> std::string
{
	return {};
}

Dict_Finder_For_CLI_Tool_2::Dict_Finder_For_CLI_Tool_2()
{
	append_default_dir_paths(dir_paths);
	append_libreoffice_dir_paths(dir_paths);
	dir_paths.push_back(".");
}

/**
 * @internal
 * @brief Gets the dictionary path.
 *
 * If @p dict is a path that contains slash, the function returns the input
 * argument as is, otherwise searches the found dictionaries by their name
 * (stem) and returns their path.
 *
 * @param dict name (stem, filename without extension) or path with slash and
 * with .aff extension.
 * @return the path to dictionary or empty if does not exists.
 */
auto Dict_Finder_For_CLI_Tool_2::get_dictionary_path(const fs_path& dict) const
    -> fs_path
{
	if (dict.has_stem() && distance(begin(dict), end(dict)) == 1)
		return search_dirs_for_one_dict(dir_paths, dict);
	return dict;
}

NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
