SUBDIRS =

if BUILD_HELP
SUBDIRS += help
endif

NULL =

mapdir = $(pkgdatadir)/mahjongg/games
map_DATA = \
	mahjongg.map	\
	$(NULL)

pixmapdir = $(pkgdatadir)/mahjongg/pixmaps
pixmap_DATA = \
	smooth.png	\
	postmodern.svg	\
	$(NULL)

bin_PROGRAMS = mahjongg

mahjongg_SOURCES = 	\
	drawing.c	\
	drawing.h	\
	mahjongg.c	\
	mahjongg.h	\
	maps.c		\
	maps.h		\
	solubility.c	\
	solubility.h	\
	translatable_game_names.h	\
	$(NULL)

mahjongg_CPPFLAGS = \
	-I$(top_srcdir)					\
	$(AM_CPPFLAGS)

mahjongg_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)
		
mahjongg_LDADD = \
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS)	\
	$(INTLLIBS)	\
	$(NULL)

if HAVE_GNOME
mahjongg_CFLAGS += $(GNOME_CFLAGS)
mahjongg_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
mahjongg_CFLAGS += $(RSVG_CFLAGS)
mahjongg_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
mahjongg_CFLAGS += $(GHTREAD_CFLAGS)
mahjongg_LDADD += $(GTHREAD_LIBS)
endif

gsettings_in_file = org.gnome.mahjongg.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

man_MANS = mahjongg.6

desktop_in_files = mahjongg.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

EXTRA_DIST = \
	$(pixmap_DATA)		\
	$(gsettings_in_file)	\
	$(man_MANS)	\
	$(map_DATA)		\
	$(NULL)

CLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)

install-scorefiles-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-for i in easy difficult confounding pyramid tictactoe cloud dragon bridges ziggurat; do \
		touch $(DESTDIR)$(scoredir)/mahjongg.$$i.scores; \
		chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/mahjongg.$$i.scores; \
		chmod 664 $(DESTDIR)$(scoredir)/mahjongg.$$i.scores; \
	done

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/mahjongg && chmod 2555 $(DESTDIR)$(bindir)/mahjongg ;\
	fi

install-data-local: install-scorefiles-local

@INTLTOOL_DESKTOP_RULE@

-include $(top_srcdir)/git.mk
