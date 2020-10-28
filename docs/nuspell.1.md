---
title: NUSPELL
section: 1
header: User Commands
footer: Nuspell vX.Y  # override this on the command line in CMake
author:
- Dimitrij Mijoski
- Sander van Geloven
date: 2020-10-28
---

# NAME

nuspell - Command-line tool for spellchecking.

# SYNOPSIS

**nuspell** \[**-s**\] \[**-d** _dict\_NAME_\] \[**-i** _ENCODING_\] \[_FILE_\]...  
**nuspell** **-l|-G** \[**-L**\] \[**-s**\] \[**-d** _dict\_NAME_\] \[**-i** _ENCODING_\] \[_FILE_\]...  
**nuspell** **-D|-h|\--help|-v|\--version**


# DESCRIPTION

Nuspell checks spelling of each _FILE_.
Without _FILE_, checks standard input.

# OPTIONS

-d _di\_CT_
:   use _di\_CT_ dictionary. Only one dictionary is currently supported.

-D
:   print search paths and available dictionaries and exit

-i _ENCODING_
:   input/output encoding, default is active locale

-l
:   print only misspelled words or lines

-G
:   print only correct words or lines

-L
:   lines mode

-s
:   use simple white-space text segmentation to extract words instead of the
    default Unicode text segmentation. It is not recommended to use this.

-h, \--help
:   display this help and exit

-v, \--version
:   print version number and exit

# ENVIRONMENT

DICPATH
:   Path to additional directory to search for dictionaries.
    
# RETURN VALUES

Returns error if the argument syntax is invalid or some file can not be opened.
Otherwise, spell checking has occurred and returns success.
    
# BUGS

Bug reports: <https://github.com/nuspell/nuspell/issues>

# EXAMPLES

    nuspell -d en_US file.txt

# COPYRIGHT

Copyright 2016-2020 Dimitrij Mijoski, Sander van Geloven
    
# SEE ALSO

Full documentation: <https://github.com/nuspell/nuspell/wiki>

Home page: <http://nuspell.github.io/>
