/* We should probably be using the GIR files, but I can't get them to work in
 * Vala.  This works for now but means it needs to be updated when the library
 * changes -- Robert Ancell */

[CCode (cprefix = "Games", lower_case_cprefix = "games_")]
namespace GnomeGamesSupport
{
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_SCORES;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_PAUSE_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RESUME_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_FULLSCREEN;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_LEAVE_FULLSCREEN;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_NEW_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_START_NEW_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_NETWORK_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_NETWORK_LEAVE;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_PLAYER_LIST;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RESTART_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_UNDO_MOVE;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_REDO_MOVE;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_HINT;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_END_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_CONTENTS;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RESET;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_TELEPORT;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RTELEPORT;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_DEAL_CARDS;

    [CCode (cheader_filename = "games-stock.h")]
    public static void stock_init ();
    [CCode (cheader_filename = "games-stock.h")]
    public static void stock_prepare_for_statusbar_tooltips (Gtk.UIManager ui_manager, Gtk.Widget statusbar);
    [CCode (cheader_filename = "games-stock.h")]
    public static string get_license (string game_name);

    [CCode (cheader_filename = "games-setgid-io.h", cname = "setgid_io_init")]
    public static void setgid_io_init ();

    [CCode (cprefix = "GAMES_RUNTIME_", cheader_filename = "games-runtime.h")]
    public enum RuntimeDirectory
    {
        PREFIX,
        DATA_DIRECTORY,
        COMMON_DATA_DIRECTORY,
        PKG_DATA_DIRECTORY,
        SCORES_DIRECTORY,
        LOCALE_DIRECTORY,
        COMMON_PIXMAP_DIRECTORY,
        PRERENDERED_CARDS_DIRECTORY,
        SCALABLE_CARDS_DIRECTORY,
        ICON_THEME_DIRECTORY,
        PIXMAP_DIRECTORY,
        SOUND_DIRECTORY,
        GAME_DATA_DIRECTORY,
        GAME_GAMES_DIRECTORY,
        GAME_PIXMAP_DIRECTORY,
        GAME_THEME_DIRECTORY,
        GAME_HELP_DIRECTORY,
        LAST_DIRECTORY,
        FIRST_DERIVED_DIRECTORY
    }

    [CCode (cheader_filename = "games-runtime.h")]
    public static bool runtime_init (string name);
    [CCode (cheader_filename = "games-runtime.h")]
    public static void runtime_shutdown ();
    [CCode (cheader_filename = "games-runtime.h")]
    public static unowned string runtime_get_directory (RuntimeDirectory directory);
    [CCode (cheader_filename = "games-runtime.h")]
    public static unowned string runtime_get_file (RuntimeDirectory directory, string name);
    [CCode (cheader_filename = "games-runtime.h")]
    public static int runtime_get_gpl_version  ();
    [CCode (cheader_filename = "games-runtime.h")]
    public static bool runtime_is_system_prefix ();
    
    [CCode (cheader_filename = "games-help.h")]
    public static void help_display (Gtk.Widget window, string doc_module, string? section);
    [CCode (cheader_filename = "games-help.h")]
    public static bool help_display_full (Gtk.Widget window, string doc_module, string? section) throws GLib.Error;
    
    [CCode (cheader_filename = "games-settings.h")]    
    public static void settings_bind_window_state (string path, Gtk.Window window);

    [CCode (cheader_filename = "games-clock.h")]
    public class Clock : Gtk.Label
    {
        public Clock ();
        public void start ();
        public bool is_started ();
        public void stop ();
        public void reset ();
        public time_t get_seconds ();
        public void add_seconds (time_t seconds);
        public void set_updated (bool do_update);
    }
    
    [CCode (cheader_filename = "games-pause-action.h")]
    public class PauseAction : Gtk.Action
    {
        public signal void state_changed ();
        public PauseAction (string name);
        public bool get_is_paused ();
    }

    [CCode (cprefix = "GAMES_FULLSCREEN_ACTION_VISIBLE_")]
    public enum VisiblePolicy
    {
        ALWAYS,
        ON_FULLSCREEN,
        ON_UNFULLSCREEN
    }

    [CCode (cheader_filename = "games-fullscreen-action.h")]
    public class FullscreenAction : Gtk.Action
    {
        public FullscreenAction (string name, Gtk.Window window);
        public void set_visible_policy (VisiblePolicy visible_policy);
        public VisiblePolicy get_visible_policy ();
        public void set_is_fullscreen (bool is_fullscreen);
        public bool get_is_fullscreen ();
    }

    [CCode (cprefix = "GAMES_SCORES_STYLE_", cheader_filename = "games-score.h")]
    public enum ScoreStyle
    {
        PLAIN_DESCENDING,
        PLAIN_ASCENDING,
        TIME_DESCENDING,
        TIME_ASCENDING
    }
    
    [CCode (cheader_filename = "games-scores.h", destroy_function = "")]
    public struct ScoresCategory
    {
        string key;
        string name;
    }

    [CCode (cheader_filename = "games-score.h")]
    public class Score : GLib.Object
    {
        public Score ();
    }

    [CCode (cheader_filename = "games-scores.h")]
    public class Scores : GLib.Object
    {
        public Scores (string app_name, ScoresCategory[] categories, string? categories_context, string? categories_domain, int default_category_index, ScoreStyle style);
        public void set_category (string category);
        public int add_score (Score score);
        public int add_plain_score (uint32 value);
        public int add_time_score (double value);
        public void update_score (string new_name);
        public void update_score_name (string new_name, string old_name);
        public ScoreStyle get_style ();
        public unowned string get_category ();
        public void add_category (string key, string name);
    }
    
    [CCode (cprefix = "GAMES_SCORES_", cheader_filename = "games-scores-dialog.h")]
    public enum ScoresButtons
    {
        CLOSE_BUTTON,
        NEW_GAME_BUTTON,
        UNDO_BUTTON,
        QUIT_BUTTON
    }

    [CCode (cheader_filename = "games-scores-dialog.h")]
    public class ScoresDialog : Gtk.Dialog
    {
        public ScoresDialog (Gtk.Window parent_window, Scores scores, string title);
        public void set_category_description (string description);
        public void set_hilight (uint pos);
        public void set_message (string message);
        public void set_buttons (uint buttons);
    }

    [CCode (cheader_filename = "games-frame.h")]
    public class Frame : Gtk.Box
    {
        public Frame (string label);
        void set_label (string label);
    }

    [CCode (cheader_filename = "games-preimage.h")]
    public class Preimage : GLib.Object
    {
        public Preimage ();
        public Preimage.from_file (string filename) throws GLib.Error;
        public void set_font_options (Cairo.FontOptions font_options);
        public Gdk.Pixbuf render (int width, int height);
        public void render_cairo (Cairo.Context cr, int width, int height);
        public Gdk.Pixbuf render_sub (string node, int width, int height, double xoffset, double yoffset, double xzoom, double yzoom);
        public void render_cairo_sub (Cairo.Context cr, string node, int width, int height, double xoffset, double yoffset, double xzoom, double yzoom);
        public bool is_scalable ();
        public int get_width ();
        public int get_height ();
        public Gdk.Pixbuf render_unscaled_pixbuf ();
    }

    [CCode (cheader_filename = "games-files.h")]
    public class FileList : GLib.Object
    {
        public FileList (string glob, ...);
        public FileList.images (string path1, ...);
        public void transform_basename ();
        public size_t length ();
        public void for_each (GLib.Func<string> function);
        public string find (GLib.CompareFunc function);
        public unowned string? get_nth (int n);
        public Gtk.Widget create_widget (string selection, uint flags);
    }
}

