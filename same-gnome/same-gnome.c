/*
 * Same-Gnome: the game.
 * (C) 1997 the Free Software Foundation
 *
 * Author: Miguel de Icaza.
 *         Federico Mena.
 *         Horacio Pe�a.
 *
 * The idea is originally from KDE's same game program.
 *
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <getopt.h>

#include <config.h>
#include <gnome.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-session.h>

#define STONE_SIZE 40
#define STONE_COLS  15
#define STONE_LINES 10
#define GAME_EVENTS (GDK_EXPOSURE_MASK              |\
		     GDK_BUTTON_PRESS_MASK          |\
		     GDK_ENTER_NOTIFY_MASK          |\
		     GDK_LEAVE_NOTIFY_MASK          |\
		     GDK_POINTER_MOTION_MASK)
	
GtkWidget *pref_dialog, *scorew;
GtkWidget *app, *draw_area, *vb;
GtkMenuFactory *mf;
GdkPixmap *stones, *mask;
int tagged_count = 0;
int ball_timeout_id = -1;
int old_x = -1, old_y = -1;
int score;
gchar *scenario;
char *session_id;
gint restarted;
gint debugging;

struct {
	char *scenario;
	int make_it_default;
	} selected_scenario = {0,0};

struct ball {
	int color;
	int tag;
	int frame;
} field [STONE_COLS][STONE_LINES];

int nstones;
int ncolors;
int sync_stones = 0;

#define mapx(x) (x)
#define mapy(y) (STONE_LINES-1-(y))

void
draw_ball (int x, int y)
{
	int bx, by;

	if (field [x][y].color){
		by = STONE_SIZE * (field [x][y].color - 1);
		bx = STONE_SIZE * (field [x][y].frame);
		
		gdk_draw_pixmap (draw_area->window,
				 draw_area->style->black_gc, stones,
				 bx, by, x * STONE_SIZE, y * STONE_SIZE,
				 STONE_SIZE, STONE_SIZE);
	} else {
		gdk_window_clear_area (draw_area->window, x * STONE_SIZE, y * STONE_SIZE,
				       STONE_SIZE, STONE_SIZE);
	}
}

void
paint (GdkRectangle *area)
{
	int x1, y1, x2, y2, x, y;
	
	x1 = area->x / STONE_SIZE;
	y1 = area->y / STONE_SIZE;
	x2 = (area->x + area->width) / STONE_SIZE;
	y2 = (area->y + area->height) / STONE_SIZE;

	for (x = x1; x <= x2; x++){
		for (y = y1; y <= y2; y++){
			draw_ball (x, y);
		}
	}
}

void
untag_all ()
{
	int x, y;

	for (x = 0; x < STONE_COLS; x++)
		for (y = 0; y < STONE_LINES; y++){
			field [x][y].tag   = 0;
			if (sync_stones){
				field [x][y].frame = 0;
				draw_ball (x, y);
			}
		}
}

int
flood_fill (int x, int y, int color)
{
	int c = 0;
	
	if (field [x][y].color != color)
		return c;
	
	if (field [x][y].tag)
		return c;

	c = 1;
	field [x][y].tag = 1;
	
	if (x+1 < STONE_COLS)
		c += flood_fill (x+1, y, color);
	if (x)
		c += flood_fill (x-1, y, color);
	if (y+1 < STONE_LINES)
		c += flood_fill (x, y+1, color);
	if (y)
		c += flood_fill (x, y-1, color);
	return c;
}

int
move_tagged_balls (void *data)
{
	int x, y;
	
	for (x = 0; x < STONE_COLS; x++)
		for (y = 0; y < STONE_LINES; y++){
			if (!field [x][y].tag)
				continue;
			field [x][y].frame = (field [x][y].frame + 1) % nstones;
			draw_ball (x, y);
		}
	gdk_flush ();
	return 1;
}

void
disable_timeout ()
{
	if (ball_timeout_id != -1){
		gtk_timeout_remove (ball_timeout_id);
		ball_timeout_id = -1;
	}
}

void
mark_balls (int x, int y)
{
	if (x == old_x && y == old_y)
		return;
	old_x = x;
	old_y = y;

	untag_all ();
	disable_timeout ();
	if (!field [x][y].color)
		return;
	
	tagged_count = flood_fill (x, y, field [x][y].color);
	
	if (tagged_count > 1)
		ball_timeout_id = gtk_timeout_add (100, move_tagged_balls, 0);
}

void
compress_column (int x)
{
	int y, ym;
	
	for (y = STONE_LINES - 1; y >= 0; y--){
		if (!field [mapx(x)][mapy(y)].tag)
			continue;
		for (ym = y; ym < STONE_LINES - 1; ym++)
			field [mapx(x)][mapy(ym)] = field [mapx(x)][mapy(ym+1)];
		field [mapx(x)][mapy(ym)].color = 0;
		field [mapx(x)][mapy(ym)].tag   = 0;
	}
}

void
compress_y ()
{
	int x;

	for (x = 0; x < STONE_COLS; x++)
		compress_column (x);
}

void
copy_col (int dest, int src)
{
	int y;
	
	for (y = 0; y < STONE_LINES; y++)
		field [mapx(dest)][mapy(y)] = field [mapx(src)][mapy(y)];
}

void
clean_last_col ()
{
	int y;

	for (y = 0; y < STONE_LINES; y++){
		field [mapx(STONE_COLS-1)][mapy(y)].color = 0;
		field [mapx(STONE_COLS-1)][mapy(y)].tag   = 0;
	}
}

void
compress_x ()
{
	int x, xm, l;

	for (x = 0; x < STONE_COLS; x++){
		for (l = STONE_COLS; field [mapx(x)][mapy(0)].color == 0 && l; l--){
			for (xm = x; xm < STONE_COLS-1; xm++)
				copy_col (xm, xm+1);
			clean_last_col ();
		} 
	}
}

void
set_score (int new_score)
{
	char b [20];
	
	score = new_score;
	sprintf (b, "%5d", score);
	gtk_label_set (GTK_LABEL(scorew), b);
}

void
show_scores ( gchar *title, guint pos )
{
	gnome_scores_display (title, "samegnome", NULL, pos);
}

void 
game_top_ten_callback(GtkWidget *widget, gpointer data)
{
	show_scores(_("The Same Gnome: High Scores"), 0);
}

void
end_of_game (char *title)
{
	int pos;

	pos = gnome_score_log(score, NULL, TRUE);
	show_scores(title, pos);
}

void
check_game_over (void)
{
	int cleared=1;
	int x,y;
	
	for(x = 0; x < STONE_COLS; x++)
		for(y = 0 ; y < STONE_LINES; y++) {
			if (!field [x][y].color)
				continue;
			cleared = 0;
			if(x+1 < STONE_COLS) 
				if(field[x][y].color == field[x+1][y].color)
					return;
			if(y+1 < STONE_LINES) 
				if(field[x][y].color == field[x][y+1].color)
					return;
		}
	if (cleared){
		set_score (score+1000);
		end_of_game (_("The Same Gnome: you win!"));
	}
	else
		end_of_game(_("The Same Gnome: the Game Is Over"));
}

void
kill_balls (int x, int y)
{
	if (!field [x][y].color)
		return;
	
	if (tagged_count < 2)
		return;

	set_score (score + (tagged_count - 2) * (tagged_count - 2));
	compress_y ();
	compress_x ();
	gtk_widget_draw (draw_area, NULL);
	check_game_over ();
}

gint
area_event (GtkWidget *widget, GdkEvent *event, void *d)
{
	switch (event->type){
	case GDK_EXPOSE: {
		GdkEventExpose *e = (GdkEventExpose *) event;
		paint (&e->area);
		return TRUE;
	}
	
	case GDK_BUTTON_PRESS: {
		int x, y;
		
		gtk_widget_get_pointer (widget, &x, &y);
		kill_balls (x / STONE_SIZE, y / STONE_SIZE);
		old_x = -1;
		old_y = -1;
	}

	case GDK_ENTER_NOTIFY:
	case GDK_MOTION_NOTIFY: {
		int x, y;
		
		gtk_widget_get_pointer (widget, &x, &y);
		mark_balls (x / STONE_SIZE, y / STONE_SIZE);
		return TRUE;
	}
	
	case GDK_LEAVE_NOTIFY: 
		disable_timeout ();
		untag_all ();
		return TRUE;

	default:
		return FALSE;
	}
}

void
fill_board (void)
{
	int x, y;

	for (x = 0; x < STONE_COLS; x++)
		for (y = 0; y < STONE_LINES; y++){
			field [x][y].color = 1 + (rand () % ncolors);
			field [x][y].tag   = 0;
			field [x][y].frame = sync_stones ? 0 : (rand () % nstones);
		}
}

void
new_game (void)
{
	fill_board ();
	set_score (0);
	gtk_widget_draw (draw_area, NULL);
}

void
configure_sync (char *fname)
{
	if (strstr (fname, "-sync.xpm"))
		sync_stones = 1;
	else
		sync_stones = 0;
}

void
load_scenario (char *fname)
{
	GtkStyle  *style;
	int width, height;
	char *tmp, *fn; 

	tmp = g_copy_strings ( "samegnome/", fname, NULL);

	fn = gnome_unconditional_pixmap_file ( tmp ); 
	g_free( tmp );

	if (!g_file_exists (fn)){
			printf (_("Could not find the \'%s\' theme for SameGnome\n"), fn);
			exit (1);
		}

	if(scenario) free(scenario);
	scenario = strdup(fname);

	style = gtk_widget_get_style (draw_area);
	configure_sync (fname);
	stones = gdk_pixmap_create_from_xpm (app->window, &mask, &style->bg [GTK_STATE_NORMAL], fn);
	g_free( fn );
	gdk_window_get_size (stones, &width, &height);
	nstones = width / STONE_SIZE;
/*	ncolors = height / STONE_SIZE; */
	ncolors = 3;
	gtk_widget_draw (draw_area, NULL);
}

void
set_selection (GtkWidget *widget, void *data)
{
	selected_scenario.scenario = data;
}

void
set_selection_def (GtkWidget *widget, gpointer *data)
{
	selected_scenario.make_it_default = GTK_TOGGLE_BUTTON (widget)->active;
}

void
create_same_board (char *fname)
{
	draw_area = gtk_drawing_area_new ();
	gtk_widget_set_events (draw_area, gtk_widget_get_events (draw_area) | GAME_EVENTS);

	gtk_box_pack_start_defaults (GTK_BOX(vb), draw_area);
	gtk_widget_realize (draw_area);
	gtk_style_set_background (draw_area->style,
				  draw_area->window,
				  GTK_STATE_NORMAL);
	gtk_widget_show (draw_area);

	load_scenario (fname);
	gtk_drawing_area_size (GTK_DRAWING_AREA (draw_area),
			       STONE_COLS  * STONE_SIZE,
			       STONE_LINES * STONE_SIZE);
	gtk_signal_connect (GTK_OBJECT(draw_area), "event", (GtkSignalFunc) area_event, 0);
}

void
game_new_callback (GtkWidget *widget, void *data)
{
	new_game ();
}

int
yes (GtkWidget *widget, void *data)
{
	selected_scenario.scenario = 0;
	return TRUE;
}

void
free_str (GtkWidget *widget, void *data)
{
	free (data);
}

void
fill_menu (GtkWidget *menu)
{
	struct dirent *e;
	char *dname = gnome_unconditional_pixmap_file ("samegnome");
	DIR *dir;
	
	dir = opendir (dname);

	if (!dir)
		return;
	
	while ((e = readdir (dir)) != NULL){
		GtkWidget *item;
		char *s = strdup (e->d_name);

		if (!strstr (e->d_name, ".xpm")) {
			free (s);
			continue;
			}
			
		item = gtk_menu_item_new_with_label (s);
		gtk_widget_show (item);
		gtk_menu_append (GTK_MENU(menu), item);
		gtk_signal_connect (GTK_OBJECT(item), "activate", (GtkSignalFunc)set_selection, s);
		gtk_signal_connect (GTK_OBJECT(item), "destroy", (GtkSignalFunc) free_str, s);
	}
	closedir (dir);
}

void
cancel (GtkWidget *widget, void *data)
{
	gtk_widget_destroy (pref_dialog);
	pref_dialog = 0;
}

void
load_scenario_callback (GtkWidget *widget, void *data)
{
	if (selected_scenario.scenario) {
		load_scenario (selected_scenario.scenario);
		new_game ();
		if (selected_scenario.make_it_default) {
			gnome_config_set_string (
				"/same-gnome/Preferences/Scenario", 
				selected_scenario.scenario);
			gnome_config_sync();
			}
		}
	cancel (0,0);
}

GnomeActionAreaItem sel_actions [] = {
	{ NULL, load_scenario_callback },
	{ NULL, cancel }
};

void
game_preferences_callback (GtkWidget *widget, void *data)
{
	GtkWidget *menu, *omenu, *l, *hb, *cb, *f, *fv;
	GtkDialog *d;

	if (pref_dialog)
		return;
	
	pref_dialog = gtk_dialog_new ();
	d = GTK_DIALOG(pref_dialog);
	gtk_signal_connect (GTK_OBJECT(app), "delete_event", (GtkSignalFunc)yes, NULL);

	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	fill_menu (menu);
	gtk_widget_show (omenu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU(omenu), menu);

	f = gtk_frame_new (_ ("Scenario"));
	gtk_container_border_width (GTK_CONTAINER (f), 5);

	hb = gtk_hbox_new (FALSE, FALSE);
	gtk_widget_show (hb);
	
	l = gtk_label_new (_("Select scenario:"));
	gtk_widget_show (l);
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_box_pack_start_defaults (GTK_BOX(hb), omenu);

	cb = gtk_check_button_new_with_label ( _("Make it the default scenario") );
	gtk_signal_connect (GTK_OBJECT(cb), "clicked", (GtkSignalFunc)set_selection_def, NULL);
	gtk_widget_show (cb);

	fv = gtk_vbox_new (0, 5);
	gtk_container_border_width (GTK_CONTAINER (fv), 5);
	gtk_widget_show (fv);
	
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);
	gtk_box_pack_start_defaults (GTK_BOX(fv), cb);
	gtk_box_pack_start_defaults (GTK_BOX(d->vbox), f);
	gtk_container_add (GTK_CONTAINER (f), fv);
	
	gtk_widget_show (f);
	
	sel_actions [0].label = _("Ok");
	sel_actions [1].label = _("Cancel");
	
	gnome_build_action_area (d, sel_actions, 2, 0);
	gtk_widget_show (pref_dialog);
}

int
game_quit_callback (GtkWidget *widget, void *data)
{
	GtkWidget *box;
	
	box = gnome_messagebox_new (_("Do you really want to quit?"),
				    GNOME_MESSAGEBOX_QUESTION,
				    _("Yes"), _("No"), NULL);
	gnome_messagebox_set_modal (GNOME_MESSAGEBOX (box));
	
	gtk_widget_destroy (app);
	gtk_main_quit ();

	return TRUE;
}

GtkMenuEntry same_menu [] = {
	{ N_("Game/New"),        "<control>N", game_new_callback, NULL },
	{ N_("Game/Scenario"),   "<control>S", game_preferences_callback, NULL },
	{ N_("Game/Top Ten"),    "<control>T", game_top_ten_callback, NULL },
	{ N_("Game/<separator>"), NULL, NULL, NULL },
	{ N_("Game/Exit"),       "<control>E", (GtkMenuCallback) game_quit_callback, NULL }, 
};

#define ELEMENTS(x) (sizeof (x) / sizeof (x [0]))

GtkMenuFactory *
create_menu ()
{
	GtkMenuFactory *subfactory;
	int i;
	
	for (i = 0; i < ELEMENTS(same_menu); i++)
		same_menu[i].path = _(same_menu[i].path);
	
	subfactory = gtk_menu_factory_new  (GTK_MENU_FACTORY_MENU_BAR);
	gtk_menu_factory_add_entries (subfactory, same_menu, ELEMENTS(same_menu));

	return subfactory;
}

GtkWidget *
create_main_window ()
{
	app = gnome_app_new ("samegnome", "Same Gnome");
	gtk_widget_realize (app);
	
	gtk_signal_connect (GTK_OBJECT(app), "delete_event", GTK_SIGNAL_FUNC(game_quit_callback), NULL);
	gtk_window_set_policy (GTK_WINDOW(app), 0, 0, 1);
	return app;
}

static int
save_state (gpointer client_data, GnomeSaveStyle save_style,
           int is_shutdown, GnomeInteractStyle interact_style,
           int is_fast)
{
	gchar *sess;
	gchar *buf;
	gchar *buf2;
	struct ball *f = (struct ball*) field;
	int i;

	gchar *argv[3];

	if (debugging)
		g_print ("Saving state\n");

	sess = g_copy_strings ("Saved-Session-",
				session_id,
				NULL);

	buf = g_copy_strings ("/same-gnome/",
				sess, 
				"/Score",
				NULL);
	gnome_config_set_int (buf, score);
	g_free(buf);

	buf = g_copy_strings ("/same-gnome/",
				sess, 
				"/nstones",
				NULL);
	gnome_config_set_int (buf, sync_stones ? 1 : nstones);
	g_free(buf);

	buf = g_copy_strings ("/same-gnome/",
				sess, 
				"/Field",
				NULL);

	buf2 = g_malloc(STONE_COLS*STONE_LINES+1);
	for ( i=0 ; i < (STONE_COLS*STONE_LINES) ; i++ ) 
		buf2[i]  =f[i].color + 'a';
	buf2[STONE_COLS*STONE_LINES] = '\0';

	gnome_config_set_string (buf, buf2);
	g_free(buf);
	g_free(buf2);

	gnome_config_sync();
	g_free(sess);

	argv[0] = (char*) client_data;
	argv[1] = "--session";
	argv[2] = session_id;

	gnome_session_set_restart_command (3, argv); /* probably it should
		remember command-line parameters (--debug, ...) */
	
	return(1);
}

void
restart (gchar *id)
{
	gchar *sess;
	gchar *buf;
	gchar *buf2;
	struct ball *f = (struct ball*) field;
	int i;

	if (debugging)
		g_print ("Retrieving state\n");

	sess = g_copy_strings ("Saved-Session-",
				id,
				NULL);

	buf = g_copy_strings ("/same-gnome/",
				sess, 
				"/Score",
				NULL);
	score = gnome_config_get_int_with_default (buf, 0);
	g_free(buf);

	buf = g_copy_strings ("/same-gnome/",
				sess, 
				"/nstones",
				NULL);
	nstones = gnome_config_get_int_with_default (buf, 0);
	g_free(buf);

	buf = g_copy_strings ("/same-gnome/",
				sess, 
				"/Field",
				NULL);
	buf2 = gnome_config_get_string_with_default (buf, NULL);

	if (buf2)
		for ( i=0 ; i < (STONE_COLS*STONE_LINES) ; i++ )
		{
		f[i].color = buf2[i] - 'a';
		f[i].tag   = 0;
		f[i].frame = nstones ? (rand () % nstones) : 0;
		}
	g_free(buf2);
	g_free(buf);

	if (debugging)
		g_print ("Buh!\n");

	g_free(sess);
	return(1);
}

gchar *
parse_args (int argc,char *argv[])
{
	gint ch;

	struct option options[] = {
		{ "debug",    	no_argument,		NULL,	'd'	},
		{ "help",    	no_argument,		NULL,	'h'	},
		{ "session", 	required_argument,	NULL,	'S'	},
		{ "scenario", 	required_argument,	NULL,	's'	},
		{ "version", 	no_argument,		NULL,	'v'	},
		{ NULL, 0, NULL, 0 }
		};

	gchar *id = NULL;
	gchar *fname = gnome_config_get_string ( "/same-gnome/Preferences/Scenario=stones.xpm" );
	
	debugging = 0;
	restarted = 0;

	score = 0;

	/* initialize getopt */
	optarg = NULL;
	optind = 0;
	optopt = 0;

	while( (ch = getopt_long(argc, argv, "dhsv", options, NULL)) != EOF ) 
	{
		switch(ch) 
		{
			case 'd':
				debugging = 1;
				g_print ("Debugging mode\n");
				break;
			case 'h':
				exit(0);
				break;
			case 'v':
				g_print (_("Gnome Mines version 0.1\n"));
				exit(0);
				break;
			case 'S':
				id = g_strdup (optarg);
				restart (id);
				restarted = 1;
				break;
			case 's':
				g_free (fname);
				fname = g_strdup (optarg);
				break;
			case ':':
			case '?':
				g_print (_("Options error\n"));
				exit(0);
				break;
		}
	}

	session_id = gnome_session_init (save_state, argv[0], NULL, NULL, id);
	if (debugging)
		g_print ("Session ID: %s\n", session_id);
	gnome_session_set_program (argv[0]);
	g_free(id);

	return fname;
}

int
main (int argc, char *argv [])
{
	GtkWidget *label, *hb;
	gchar *fname;
	
	gnome_init (&argc, &argv);
	fname = parse_args(argc, argv);
	textdomain (PACKAGE);

	if (argc > 1)
		fname = strdup (argv [1]);
	else {
		fname = gnome_config_get_string ( "/same-gnome/Preferences/Scenario=stones.xpm" );
	}

	srand (time (NULL));

	app = create_main_window ();
	vb = gtk_vbox_new (FALSE, 0);
	hb = gtk_hbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), vb);
	mf = create_menu ();
	gnome_app_set_menus (GNOME_APP (app), GTK_MENU_BAR (mf->widget));
	
	label = gtk_label_new (_("Score: "));
	scorew = gtk_label_new ("");
	set_score (score);
	gtk_box_pack_start_defaults (GTK_BOX(vb), hb);
	gtk_box_pack_end   (GTK_BOX(hb), scorew, 0, 0, 10);
	gtk_box_pack_end   (GTK_BOX(hb), label,  0, 0, 0);
	
	create_same_board (fname);
	if (!restarted)
		new_game ();
	
	free (fname);

	gtk_widget_show (app);
	gtk_widget_show (hb);
	gtk_widget_show (vb);
	gtk_widget_show (GTK_WIDGET(label));
	gtk_widget_show (GTK_WIDGET(scorew));

	gtk_main ();
	return 0;
}
