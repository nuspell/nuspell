nuspell(1) -- spell checker
===========================

## SYNOPSIS


`nuspell` [-s] [-d _dict_NAME_] [-i _ENCODING_] [_FILE_]...  
`nuspell` -l|-G [-L] [-s] [-d _dict_NAME_] [-i _ENCODING_] [_FILE_]...  
`nuspell` -D|-h|--help|-v|--version


## DESCRIPTION

Nuspell checks spelling of each _FILE_.
Without _FILE_, checks standard input.

## OPTIONS

  - `-d` _di\_CT_:
    use _di\_CT_ dictionary. Only one dictionary is currently supported.
  - `-D`:
    print search paths and available dictionaries and exit
  - `-i` _ENCODING_:
    input/output encoding, default is active locale
  - `-l`:
    print only misspelled words or lines
  - `-G`:
    print only correct words or lines
  - `-L`:
    lines mode
  - `-s`:
    use simple whitespace text segmentation to extract words instead of the
    default Unicode text segmentation. It is not recommended to use this.
  - `-h, --help`:
    display this help and exit
  - `-v, --version`:
    print version number and exit

## ENVIRONMENT

  - DICPATH:
    Dictionary path.
    
## RETURN VALUES

Returns error if the argument syntax is invalid or some file can not be opened.
Otherwise, spell checking has occurred and returns success.
    
## BUGS

Bug reports: <https://github.com/nuspell/nuspell/issues>

## EXAMPLES

    nuspell -d en_US file.txt

## AUTHORS

Dimitrij Mijoski and Sander van Geloven

## COPYRIGHT

Copyright 2016-2019 Dimitrij Mijoski, Sander van Geloven
    
## SEE ALSO

Full documentation: <https://github.com/nuspell/nuspell/wiki>

Home page: <http://nuspell.github.io/>
