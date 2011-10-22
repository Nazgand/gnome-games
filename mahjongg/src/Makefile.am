bin_PROGRAMS = mahjongg

mahjongg_SOURCES = 	\
	config.vapi	\
	game.vala	\
	game-view.vala	\
	mahjongg.vala	\
	map.vala	\
	$(NULL)

mahjongg_VALAFLAGS = \
	--pkg posix \
	--pkg gtk+-3.0 \
	--vapidir $(top_srcdir)/libgames-support \
	--pkg GnomeGamesSupport-1.0

mahjongg_CFLAGS = \
	-I$(top_srcdir)/libgames-support \
	-DVERSION=\"$(VERSION)\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)

if ENABLE_SETGID
mahjongg_VALAFLAGS += -D ENABLE_SETGID
endif
		
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

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/mahjongg && chmod 2555 $(DESTDIR)$(bindir)/mahjongg ;\
	fi

-include $(top_srcdir)/git.mk
