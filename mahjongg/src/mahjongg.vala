public class Mahjongg
{
    private Settings settings;

    private GnomeGamesSupport.Scores highscores;

    private List<Map> maps = null;

    private Gtk.Window window;
    private GameView game_view;
    private Gtk.Statusbar statusbar;
    private Gtk.UIManager ui_manager;
    private Gtk.Label tiles_label;
    private Gtk.Toolbar toolbar;
    private Gtk.Label moves_label;
    private GnomeGamesSupport.Clock game_clock;
    private Gtk.Dialog? preferences_dialog = null;

    private GnomeGamesSupport.PauseAction pause_action;
    private Gtk.Action hint_action;
    private Gtk.Action redo_action;
    private Gtk.Action undo_action;
    private Gtk.Action restart_action;
    private GnomeGamesSupport.FullscreenAction fullscreen_action;
    private GnomeGamesSupport.FullscreenAction leave_fullscreen_action;

    public Mahjongg ()
    {
        settings = new Settings ("org.gnome.mahjongg");

        load_maps ();

        highscores = new GnomeGamesSupport.Scores ("mahjongg",
                                                   new GnomeGamesSupport.ScoresCategory[0],
                                                   null, null, 0,
                                                   GnomeGamesSupport.ScoreStyle.TIME_ASCENDING);
        foreach (var map in maps)
        {
            var display_name = dpgettext2 (null, "mahjongg map name", map.name);
            highscores.add_category (map.score_name, display_name);
        }

        window = new Gtk.Window (Gtk.WindowType.TOPLEVEL);
        window.title = _("Mahjongg");
        window.set_default_size (530, 440);
        GnomeGamesSupport.settings_bind_window_state ("/org/gnome/mahjongg/", window);

        var status_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 10);

        var group_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        var label = new Gtk.Label (_("Tiles Left:"));
        group_box.pack_start (label, false, false, 0);
        var spacer = new Gtk.Label (" ");
        group_box.pack_start (spacer, false, false, 0);
        tiles_label = new Gtk.Label ("");
        group_box.pack_start (tiles_label, false, false, 0);
        status_box.pack_start (group_box, false, false, 0);

        group_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        label = new Gtk.Label (_("Moves Left:"));
        group_box.pack_start (label, false, false, 0);
        spacer = new Gtk.Label (" ");
        group_box.pack_start (spacer, false, false, 0);
        moves_label = new Gtk.Label ("");
        group_box.pack_start (moves_label, false, false, 0);
        status_box.pack_start (group_box, false, false, 0);

        group_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
        var game_clock_label = new Gtk.Label (_("Time:"));
        group_box.pack_start (game_clock_label, false, false, 0);
        spacer = new Gtk.Label (" ");
        group_box.pack_start (spacer, false, false, 0);
        game_clock = new GnomeGamesSupport.Clock ();
        group_box.pack_start (game_clock, false, false, 0);
        status_box.pack_start (group_box, false, false, 0);

        /* show the status bar items */
        statusbar = new Gtk.Statusbar ();
        ui_manager = new Gtk.UIManager ();

        GnomeGamesSupport.stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);

        create_menus (ui_manager);
        window.add_accel_group (ui_manager.get_accel_group ());
        var box = ui_manager.get_widget ("/MainMenu");

        window.delete_event.connect (window_delete_event_cb);

        game_view = new GameView ();
        game_view.button_press_event.connect (view_button_press_event);        
        game_view.set_size_request (320, 200);

        toolbar = (Gtk.Toolbar) ui_manager.get_widget ("/Toolbar");
        toolbar.get_style_context ().add_class (Gtk.STYLE_CLASS_PRIMARY_TOOLBAR);

        var vbox = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);

        vbox.pack_start (box, false, false, 0);
        vbox.pack_start (toolbar, false, false, 0);
        vbox.pack_start (game_view, true, true, 0);

        statusbar.pack_end (status_box, false, false, 6);
        vbox.pack_end (statusbar, false, false, 0);

        window.add (vbox);

        settings.changed.connect (conf_value_changed_cb);

        new_game ();

        game_view.grab_focus ();
    }

    public void start ()
    {
        window.show_all ();

        leave_fullscreen_action.set_visible_policy (GnomeGamesSupport.VisiblePolicy.ON_FULLSCREEN);
        conf_value_changed_cb (settings, "tileset");
        conf_value_changed_cb (settings, "bgcolour");
        conf_value_changed_cb (settings, "show-toolbar");
    }

    private void update_ui ()
    {
        pause_action.sensitive = game_view.game.move_number > 1;
        restart_action.sensitive = game_view.game.move_number > 1;

        if (game_view.paused)
        {
            hint_action.sensitive = false;
            undo_action.sensitive = false;
            redo_action.sensitive = false;
        }
        else
        {
            hint_action.sensitive = game_view.game.moves_left > 0;
            undo_action.sensitive = game_view.game.can_undo;
            redo_action.sensitive = game_view.game.can_redo;
        }

        moves_label.set_text ("%2u".printf (game_view.game.moves_left));
        tiles_label.set_text ("%3d".printf (game_view.game.visible_tiles));
    }

    private void theme_changed_cb (Gtk.ComboBox widget)
    {
        Gtk.TreeIter iter;
        widget.get_active_iter (out iter);
        string theme;
        widget.model.get (iter, 1, out theme);
        settings.set_string ("tileset", theme);
    }

    private void conf_value_changed_cb (Settings settings, string key)
    {
        if (key == "tileset")
        {
            var theme = settings.get_string ("tileset");
            game_view.theme = load_theme_texture (theme);
            if (game_view.theme == null)
            {
                warning ("Unable to load theme %s, falling back to default", theme);
                game_view.theme = load_theme_texture ("postmodern.svg", true);
            }
        }
        else if (key == "show-toolbar")
        {
            toolbar.visible = settings.get_boolean ("show-toolbar");
        }
        else if (key == "bgcolour")
        {
            game_view.set_background (settings.get_string ("bgcolour"));
        }
        else if (key == "mapset")
        {
            /* Prompt user if already made a move */
            if (game_view.game.started)
            {
                var dialog = new Gtk.MessageDialog (window,
                                                    Gtk.DialogFlags.MODAL,
                                                    Gtk.MessageType.QUESTION,
                                                    Gtk.ButtonsType.NONE,
                                                    "%s", _("Do you want to start a new game with this map?"));
                dialog.format_secondary_text (_("If you continue playing the next game will use the new map."));
                dialog.add_buttons (_("_Continue playing"), Gtk.ResponseType.REJECT,
                                    _("Use _new map"), Gtk.ResponseType.ACCEPT,
                                    null);
                dialog.set_default_response (Gtk.ResponseType.ACCEPT);
                var response = dialog.run ();
                if (response == Gtk.ResponseType.ACCEPT)
                    new_game ();
                dialog.destroy ();
            }
            else
                new_game ();
        }
    }

    private GnomeGamesSupport.Preimage? load_theme_texture (string filename, bool fail_on_error = false)
    {
        var pixmap_directory = GnomeGamesSupport.runtime_get_directory (GnomeGamesSupport.RuntimeDirectory.GAME_PIXMAP_DIRECTORY);
        var path = Path.build_filename (pixmap_directory, filename);
        try
        {
            return new GnomeGamesSupport.Preimage.from_file (path);
        }
        catch (Error e)
        {
            warning ("Failed to load theme %s: %s", filename, path);
            return null;
        }
    }

    private bool view_button_press_event (Gtk.Widget widget, Gdk.EventButton event)
    {
        /* Cancel pause on click */
        if (pause_action.get_is_paused ())
        {
            pause_action.set_is_paused (false);
            return true;
        }

        return false;
    }

    private void show_toolbar_cb (Gtk.Action action)
    {
        var toggle_action = (Gtk.ToggleAction) action;
        settings.set_boolean ("show-toolbar", toggle_action.active);
    }

    private void background_changed_cb (Gtk.ColorButton widget)
    {
        Gdk.Color colour;
        widget.get_color (out colour);
        settings.set_string ("bgcolour", "#%04x%04x%04x".printf (colour.red, colour.green, colour.blue));
    }

    private void map_changed_cb (Gtk.ComboBox widget)
    {
        settings.set_string ("mapset", maps.nth_data (widget.active).name);
    }

    private void moved_cb ()
    {
        /* Start game once moved */
        if (game_clock.get_seconds () == 0)
            game_clock.start ();

        update_ui ();

        if (game_view.game.complete)
        {
            game_clock.stop ();

            var seconds = game_clock.get_seconds ();

            var p = highscores.add_time_score ((seconds / 60) * 1.0 + (seconds % 60) / 100.0);
            var scores_dialog = new GnomeGamesSupport.ScoresDialog (window, highscores, _("Mahjongg Scores"));
            scores_dialog.set_category_description (_("Map:"));
            var title = _("Puzzle solved!");
            var message = _("You didn't make the top ten, better luck next time.");
            if (p == 1)
                message = _("Your score is the best!");
            else if (p > 1)
                message = _("Your score has made the top ten.");
            scores_dialog.set_message ("<b>%s</b>\n\n%s".printf (title, message));
            scores_dialog.set_buttons (GnomeGamesSupport.ScoresButtons.QUIT_BUTTON | GnomeGamesSupport.ScoresButtons.NEW_GAME_BUTTON);
            if (p > 0)
                scores_dialog.set_hilight (p);

            switch (scores_dialog.run ())
            {
            case Gtk.ResponseType.REJECT:
                Gtk.main_quit ();
                break;
            default:
                new_game ();
                break;
            }
            scores_dialog.destroy ();
        }
        else if (!game_view.game.can_move)
        {
            var dialog = new Gtk.MessageDialog (window, Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                                Gtk.MessageType.INFO,
                                                Gtk.ButtonsType.NONE,
                                                "%s", _("There are no more moves."));
            dialog.format_secondary_text (_("Each puzzle has at least one solution.  You can undo your moves and try and find the solution for a time penalty, restart this game or start an new one."));
            dialog.add_buttons (Gtk.Stock.UNDO, Gtk.ResponseType.REJECT,
                                _("_Restart"), Gtk.ResponseType.CANCEL,
                                _("_New game"), Gtk.ResponseType.ACCEPT);

            dialog.set_default_response (Gtk.ResponseType.ACCEPT);
            switch (dialog.run ())
            {
            case Gtk.ResponseType.REJECT:
                undo_cb ();
                break;
            case Gtk.ResponseType.CANCEL:
                restart_game ();
                break;
            default:
            case Gtk.ResponseType.ACCEPT:
                new_game ();
                break;
            }
            dialog.destroy ();
        }
    }

    private void properties_cb ()
    {
        if (preferences_dialog != null)
        {
            preferences_dialog.present ();
            return;
        }

        preferences_dialog = new Gtk.Dialog.with_buttons (_("Mahjongg Preferences"),
                                                   window,
                                                   Gtk.DialogFlags.DESTROY_WITH_PARENT,
                                                   Gtk.Stock.CLOSE,
                                                   Gtk.ResponseType.CLOSE, null);
        preferences_dialog.set_border_width (5);
        var dialog_content_area = (Gtk.Box) preferences_dialog.get_content_area ();
        dialog_content_area.set_spacing (2);
        preferences_dialog.set_resizable (false);
        preferences_dialog.set_default_response (Gtk.ResponseType.CLOSE);
        preferences_dialog.response.connect (preferences_dialog_response_cb);

        var top_table = new Gtk.Table (4, 1, false);
        top_table.border_width = 5;
        top_table.set_row_spacings (18);
        top_table.set_col_spacings (0);

        var frame = new GnomeGamesSupport.Frame (_("Tiles"));
        top_table.attach_defaults (frame, 0, 1, 0, 1);

        var table = new Gtk.Table (2, 2, false);
        table.set_row_spacings (6);
        table.set_col_spacings (12);

        var label = new Gtk.Label.with_mnemonic (_("_Tile set:"));
        label.set_alignment (0, 0.5f);
        table.attach (label, 0, 1, 0, 1, Gtk.AttachOptions.FILL, 0, 0, 0);

        var themes = load_themes ();
        var theme_combo = new Gtk.ComboBox ();
        var theme_store = new Gtk.ListStore (2, typeof (string), typeof (string));
        theme_combo.model = theme_store;
        var renderer = new Gtk.CellRendererText ();
        theme_combo.pack_start (renderer, true);
        theme_combo.add_attribute (renderer, "text", 0);
        foreach (var theme in themes)
        {
            var tokens = theme.split (".", -1);
            var name = tokens[0];

            Gtk.TreeIter iter;
            theme_store.append (out iter);
            theme_store.set (iter, 0, name, 1, theme, -1);

            if (theme == settings.get_string ("tileset"))
                theme_combo.set_active_iter (iter);
        }
        theme_combo.changed.connect (theme_changed_cb);
        table.attach_defaults (theme_combo, 1, 2, 0, 1);
        label.set_mnemonic_widget (theme_combo);

        frame.add (table);

        frame = new GnomeGamesSupport.Frame (_("Maps"));
        top_table.attach_defaults (frame, 0, 1, 1, 2);

        table = new Gtk.Table (1, 2, false);
        table.set_row_spacings (6);
        table.set_col_spacings (12);

        label = new Gtk.Label.with_mnemonic (_("_Select map:"));
        label.set_alignment (0, 0.5f);
        table.attach (label, 0, 1, 0, 1, Gtk.AttachOptions.FILL, 0, 0, 0);

        var map_combo = new Gtk.ComboBox ();
        var map_store = new Gtk.ListStore (2, typeof (string), typeof (string));
        map_combo.model = map_store;
        renderer = new Gtk.CellRendererText ();
        map_combo.pack_start (renderer, true);
        map_combo.add_attribute (renderer, "text", 0);
        foreach (var map in maps)
        {
            var display_name = dpgettext2 (null, "mahjongg map name", map.name);

            Gtk.TreeIter iter;
            map_store.append (out iter);
            map_store.set (iter, 0, display_name, 1, map, -1);

            if (settings.get_string ("mapset") == map.name)
                map_combo.set_active_iter (iter);
        }
        map_combo.changed.connect (map_changed_cb);
        table.attach_defaults (map_combo, 1, 2, 0, 1);
        label.set_mnemonic_widget (map_combo);

        frame.add (table);

        frame = new GnomeGamesSupport.Frame (_("Colors"));
        top_table.attach_defaults (frame, 0, 1, 2, 3);

        table = new Gtk.Table (1, 2, false);
        table.set_row_spacings (6);
        table.set_col_spacings (12);

        label = new Gtk.Label.with_mnemonic (_("_Background color:"));
        label.set_alignment (0, 0.5f);
        table.attach (label, 0, 1, 0, 1, Gtk.AttachOptions.FILL, 0, 0, 0);

        var widget = new Gtk.ColorButton ();
        widget.set_color (game_view.background_color);
        widget.color_set.connect (background_changed_cb);
        table.attach_defaults (widget, 1, 2, 0, 1);
        label.set_mnemonic_widget (widget);

        frame.add (table);

        dialog_content_area.pack_start (top_table, true, true, 0);

        preferences_dialog.show_all ();
    }

    private void preferences_dialog_response_cb (Gtk.Dialog dialog, int response)
    {
        preferences_dialog.destroy ();
        preferences_dialog = null;
    }

    private List<string> load_themes ()
    {
        List<string> themes = null;

        var path = GnomeGamesSupport.runtime_get_directory (GnomeGamesSupport.RuntimeDirectory.GAME_PIXMAP_DIRECTORY);
        Dir dir;
        try
        {
            dir = Dir.open (path);
        }
        catch (FileError e)
        {
            return themes;
        }

        while (true)
        {
            var s = dir.read_name ();
            if (s == null)
                break;

            if (s.has_suffix (".xpm") || s.has_suffix (".svg") || s.has_suffix (".gif") ||
                s.has_suffix (".png") || s.has_suffix (".jpg") || s.has_suffix (".xbm"))
                themes.append (s);
        }

        return themes;
    }

    private void hint_cb ()
    {
        var matches = game_view.game.find_matches (game_view.game.selected_tile);
        var n_matches = matches.length ();

        /* No match, just flash the selected tile */
        if (n_matches == 0)
        {
            if (game_view.game.selected_tile == null)
                return;
            game_view.game.set_hint (game_view.game.selected_tile, null);
        }
        else
        {
            var n = Random.int_range (0, (int) n_matches);
            var match = matches.nth_data (n);
            game_view.game.set_hint (match.tile0, match.tile1);
        }

        /* 30s penalty */
        game_clock.start ();
        game_clock.add_seconds (30);
    }

    private void about_cb ()
    {
        string[] authors =
        {
            _("Main game:"),
            "Francisco Bustamante",
            "Max Watson",
            "Heinz Hempe",
            "Michael Meeks",
            "Philippe Chavin",
            "Callum McKenzie",
            "Robert Ancell",
            "",
            _("Maps:"),
            "Rexford Newbould",
            "Krzysztof Foltman",
            null
        };

        string[] artists =
        {
            _("Tiles:"),
            "Jonathan Buzzard",
            "Jim Evans",
            "Richard Hoelscher",
            "Gonzalo Odiard",
            "Max Watson",
            null
        };

        string[] documenters =
        {
            "Eric Baudais",
            null
        };

        Gtk.show_about_dialog (window,
                               "program-name", _("Mahjongg"),
                               "version", VERSION,
                               "comments",
                               _("A matching game played with Mahjongg tiles.\n\nMahjongg is a part of GNOME Games."),
                               "copyright", "Copyright \xc2\xa9 1998-2008 Free Software Foundation, Inc.",
                               "license", GnomeGamesSupport.get_license (_("Mahjongg")),
                               "wrap-license", true,
                               "authors", authors,
                               "artists", artists,
                               "documenters", documenters,
                               "translator-credits", _("translator-credits"),
                               "logo-icon-name", "gnome-mahjongg",
                               "website", "http://www.gnome.org/projects/gnome-games",
                               "website-label", _("GNOME Games web site"),
                               null);
    }

    private void pause_cb (GnomeGamesSupport.PauseAction action)
    {
        game_view.paused = action.get_is_paused ();
        game_view.game.set_hint (null, null);
        game_view.game.selected_tile = null;

        if (game_view.paused)
            game_clock.stop ();
        else
            game_clock.start ();

        update_ui ();
    }

    private void scores_cb (Gtk.Action action)
    {
        var map_scores_dialog = new GnomeGamesSupport.ScoresDialog (window, highscores, _("Mahjongg Scores"));
        map_scores_dialog.set_category_description (_("Map:"));
        map_scores_dialog.run ();
        map_scores_dialog.destroy ();
    }

    private void new_game_cb (Gtk.Action action)
    {
        new_game ();
    }

    private void restart_game_cb (Gtk.Action action)
    {
        game_view.game.reset ();
        game_view.queue_draw ();
    }

    private bool window_delete_event_cb (Gdk.EventAny event)
    {
        Gtk.main_quit ();
        return true;
    }

    private void quit_cb ()
    {
        Gtk.main_quit ();
    }

    private void redo_cb (Gtk.Action action)
    {
        if (game_view.paused)
            return;

        game_view.game.redo ();
        update_ui ();
    }

    private void undo_cb ()
    {
        game_view.game.undo ();
        update_ui ();
    }

    private void restart_game ()
    {
        game_view.game.reset ();

        /* Prepare clock */
        game_clock.stop ();
        game_clock.reset ();

        update_ui ();
    }

    private void new_game ()
    {
        Map? map = null;
        foreach (var m in maps)
        {
            if (m.name == settings.get_string ("mapset"))
            {
                map = m;
                break;
            }
        }
        if (map == null)
            map = maps.nth_data (0);

        game_view.game = new Game (map);
        game_view.game.moved.connect (moved_cb);
        highscores.set_category (game_view.game.map.score_name);

        /* Set window title */
        var display_name = dpgettext2 (null, "mahjongg map name", game_view.game.map.name);
        /* Translators: This is the window title for Mahjongg which contains the map name, e.g. 'Mahjongg - Red Dragon' */
        window.set_title (_("Mahjongg - %s").printf (display_name));

        /* Prepare clock */
        game_clock.stop ();
        game_clock.reset ();

        update_ui ();
    }

    private void help_cb (Gtk.Action action)
    {
        GnomeGamesSupport.help_display (window, "mahjongg", null);
    }

    private const Gtk.ActionEntry actions[] =
    {
        {"GameMenu", null, N_("_Game")},
        {"SettingsMenu", null, N_("_Settings")},
        {"HelpMenu", null, N_("_Help")},
        {"NewGame", GnomeGamesSupport.STOCK_NEW_GAME, null, null, N_("Start a new game"), new_game_cb},
        {"RestartGame", GnomeGamesSupport.STOCK_RESTART_GAME, null, null, N_("Restart the current game"), restart_game_cb},
        {"UndoMove", GnomeGamesSupport.STOCK_UNDO_MOVE, null, null, N_("Undo the last move"), undo_cb},
        {"RedoMove", GnomeGamesSupport.STOCK_REDO_MOVE, null, null, N_("Redo the last move"), redo_cb},
        {"Hint", GnomeGamesSupport.STOCK_HINT, null, null, N_("Show a hint"), hint_cb},
        {"Scores", GnomeGamesSupport.STOCK_SCORES, null, null, null, scores_cb},
        {"Quit", Gtk.Stock.QUIT, null, null, null, quit_cb},
        {"Preferences", Gtk.Stock.PREFERENCES, null, null, null, properties_cb},
        {"Contents", GnomeGamesSupport.STOCK_CONTENTS, null, null, null, help_cb},
        {"About", Gtk.Stock.ABOUT, null, null, null, about_cb}
    };

    private const Gtk.ToggleActionEntry toggle_actions[] =
    {
        {"ShowToolbar", null, N_("_Toolbar"), null, N_("Show or hide the toolbar"), show_toolbar_cb}
    };

    private const string ui_description =
      "<ui>" +
      "  <menubar name='MainMenu'>" +
      "    <menu action='GameMenu'>" +
      "      <menuitem action='NewGame'/>" +
      "      <menuitem action='RestartGame'/>" +
      "      <menuitem action='PauseGame'/>" +
      "      <separator/>" +
      "      <menuitem action='UndoMove'/>" +
      "      <menuitem action='RedoMove'/>" +
      "      <menuitem action='Hint'/>" +
      "      <separator/>" +
      "      <menuitem action='Scores'/>" +
      "      <separator/>" +
      "      <menuitem action='Quit'/>" +
      "    </menu>" +
      "    <menu action='SettingsMenu'>" +
      "      <menuitem action='Fullscreen'/>" +
      "      <menuitem action='ShowToolbar'/>" +
      "      <separator/>" +
      "      <menuitem action='Preferences'/>" +
      "    </menu>" +
      "    <menu action='HelpMenu'>" +
      "      <menuitem action='Contents'/>" +
      "      <menuitem action='About'/>" +
      "    </menu>" +
      "  </menubar>" +
      "  <toolbar name='Toolbar'>" +
      "    <toolitem action='NewGame'/>" +
      "    <toolitem action='UndoMove'/>" +
      "    <toolitem action='Hint'/>" +
      "    <toolitem action='PauseGame'/>" +
      "    <toolitem action='LeaveFullscreen'/>" +
      "  </toolbar>" +
      "</ui>";

    private void create_menus (Gtk.UIManager ui_manager)
    {
        var action_group = new Gtk.ActionGroup ("group");

        action_group.set_translation_domain (GETTEXT_PACKAGE);
        action_group.add_actions (actions, this);
        action_group.add_toggle_actions (toggle_actions, this);

        ui_manager.insert_action_group (action_group, 0);
        try
        {
            ui_manager.add_ui_from_string (ui_description, -1);
        }
        catch (Error e)
        {
        }
        restart_action = action_group.get_action ("RestartGame");
        pause_action = new GnomeGamesSupport.PauseAction ("PauseGame");
        pause_action.is_important = true;
        pause_action.state_changed.connect (pause_cb);
        action_group.add_action_with_accel (pause_action, null);
        hint_action = action_group.get_action ("Hint");
        hint_action.is_important = true;
        undo_action = action_group.get_action ("UndoMove");
        undo_action.is_important = true;
        redo_action = action_group.get_action ("RedoMove");
        var show_toolbar_action = (Gtk.ToggleAction) action_group.get_action ("ShowToolbar");

        fullscreen_action = new GnomeGamesSupport.FullscreenAction ("Fullscreen", window);
        action_group.add_action_with_accel (fullscreen_action, null);

        leave_fullscreen_action = new GnomeGamesSupport.FullscreenAction ("LeaveFullscreen", window);
        action_group.add_action_with_accel (leave_fullscreen_action, null);

        show_toolbar_action.active = settings.get_boolean ("show-toolbar");
    }

    private void load_maps ()
    {
        maps = null;

        /* Add the builtin map */
        maps.append (new Map.builtin ());

        var path = GnomeGamesSupport.runtime_get_directory (GnomeGamesSupport.RuntimeDirectory.GAME_GAMES_DIRECTORY);
        var filelist = new GnomeGamesSupport.FileList ("*.map", ".", path, null);
        for (var i = 0; i < filelist.length (); i++)
        {
            var filename = filelist.get_nth (i);

            var loader = new MapLoader ();
            try
            {
                loader.load (filename);
            }
            catch (Error e)
            {
                warning ("Could not load map %s: %s\n", filename, e.message);
                continue;
            }
            foreach (var map in loader.maps)
                maps.append (map);
        }
    }

    public static int main (string[] args)
    {
        Gtk.init (ref args);

        var context = new OptionContext ("");
        context.set_translation_domain (GETTEXT_PACKAGE);
        context.add_group (Gtk.get_option_group (true));

        try
        {
            context.parse (ref args);
        }
        catch (Error e)
        {
            stdout.printf ("%s\n", e.message);
            return Posix.EXIT_FAILURE;
        }

        if (!GnomeGamesSupport.runtime_init ("mahjongg"))
            return Posix.EXIT_FAILURE;
#if ENABLE_SETGID
        GnomeGamesSupport.setgid_io_init ();
#endif
        GnomeGamesSupport.stock_init ();

        Environment.set_application_name (_("Mahjongg"));
        Gtk.Window.set_default_icon_name ("gnome-mahjongg");

        var app = new Mahjongg ();
        app.start ();

        Gtk.main ();

        Settings.sync();

        GnomeGamesSupport.runtime_shutdown ();

        return Posix.EXIT_SUCCESS;
    }
}
