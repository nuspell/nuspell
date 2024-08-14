---
title: NUSPELL
section: 1
header: User Commands
footer: Nuspell vX.Y  # override this on the command line in CMake
author: Dimitrij Mijoski
date: 2024-07-03 # This date should be changed when significant changes in this
                 # document are made. It is not release date or build date.
---

# NAME

nuspell - Command-line tool for spellchecking.

# SYNOPSIS

**nuspell** \[**-d** _dict_NAME_\] \[_OPTION_\]... \[_FILE_\]...  
**nuspell** **-D|\--help|\--version**

# DESCRIPTION

Check spelling of each _FILE_. If no _FILE_ is specified, check standard input.
The text in the input is first segmented into words with an algorithm
that recognizes punctuation and then each word is checked.

# OPTIONS

__-d, \--dictionary=__*di_CT*
:  Use _di_CT_ dictionary. Only one is supported. A dictionary consists of two
   files with extensions .dic and .aff. The **-d** option accepts either
   dictionary name without filename extension, usually a language tag, or a
   path (with slash) to the .aff file including the filename extension. When
   just a name is given, it will be searched among the list of dictionaries in
   the default directories (see option **-D**). When a path to .aff is given,
   only the dictionary under the path is considered. When **-d** is not present,
   the CLI tools tries to load a dictionary using the language tag from the
   active locale.

**-D, \--list-dictionaries**
:  Print search paths and available dictionaries and exit.

__\--encoding=__*ENC*
:  Set both input and output encoding.

__\--input-encoding=__*ENC*
:  Set input encoding, default is active locale.

__\--output-encoding=__*ENC*
:  Set output encoding, default is active locale.

**\--help**
:  Print short help.

**\--version**
:  Print version number.

# EXIT STATUS

Returns error if the argument syntax is invalid, if the dictionary can not be
loaded or if some input file can not be opened. Otherwise, spell checking has
occurred and returns success.

# ENVIRONMENT

**DICTIONARY**
:  Specify dictionary, same as option **-d**.

**DICPATH**
:  Path to additional directory to search for dictionaries.

# CAVEATS

The CLI tool is primarily intended to be used interactively by human. The input
is plain text and the output is mostly plain text with some symbols and words
that are meant to be read by human and not by machine. The format of the output
is not strictly defined and may change, thus it is not machine-readable. Other
programs should use the C++ library directly which has stable API.

# EXAMPLES

    nuspell -d en_US file.txt
    nuspell -d ../../subdir/di_CT.aff

# REPORTING BUGS

Bug reports: <https://github.com/nuspell/nuspell/issues>

# COPYRIGHT

Copyright 2016-2024 Nuspell authors.

# SEE ALSO

Full documentation: <https://github.com/nuspell/nuspell/wiki>

Home page: <http://nuspell.github.io/>
