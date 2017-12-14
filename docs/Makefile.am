
dist_man_MANS = nuspell.1

if HAVE_RONN
SUFFIXES = .md
.md:
	ronn -r $<
endif

EXTRA_DIST = nuspell.1.md README.md
