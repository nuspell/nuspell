SUFFIXES = .1 .3 .5 .1.md .3.md .5.md

dist_man_MANS = nuspell.1

RONN = ronn -r $<
.1.md.1:
	$(RONN)

.3.md.3:
	$(RONN)

.5.md.5:
	$(RONN)

EXTRA_DIST = nuspell.1.md
