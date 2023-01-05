#include "header.h"
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

//config vars
const int start_dict_index =  0;
const int min_id = 11;
const int word_retries = 4;

//other vars
char *table_name = 0;

//Widgets
GtkWidget *sw_text;
GtkWidget *entry;
GtkWidget *window;
GtkWidget *dictionary_menu;
GtkWidget *status_bar;
GtkWidget *tv;
GtkWidget *dialog;
GtkTextBuffer *text;
GtkWidget *text_view;
 
/* Focus on functions */
static void focus_on_list( ){
    gtk_widget_grab_focus (tv);
}

static void focus_on_find( ){
    gtk_widget_grab_focus (entry);
}

static void focus_on_description( ){
    gtk_widget_grab_focus (text_view);
}

/* Menu callbacks */
static void call_show_menu_dict()
{
	gtk_widget_show_all(dialog);
    if ( gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_OK){
		char *new_dict = gtk_combo_box_get_active_text((GtkComboBox*)dictionary_menu);
		load_word_list(GTK_TREE_VIEW(tv), new_dict);
	}
	gtk_widget_hide(dialog);
}

static void call_show_about()
{
	char* authors[] = {"Igor Vagulin", NULL};
    gtk_show_about_dialog ( (GtkWindow*) window, "name", "cslov", "authors", authors, NULL);
}

/* show error in dialog box or on stderr if graphics not started.  */
void report_error(const char *message, ...)
{
	va_list args;
	va_start(args, message);
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Error occured", GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_container_add( GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), gtk_label_new(g_strdup_vprintf(message, args)));
	g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
	gtk_widget_show_all(dialog);
	gtk_dialog_run( GTK_DIALOG(dialog) );
}

static GtkItemFactoryEntry menu_items[] = {
    { "/_File",           NULL,          NULL, 0, "<Branch>" },
	{ "/File/Dictionary", "<control>D",  call_show_menu_dict, 0, "<Item>" },
    { "/File/Quit",       "<control>Q",  gtk_main_quit, 0, "<Item>" },
    { "/Fo_cus",          NULL,          NULL, 0, "<Branch>" },
    { "/Focus/Find",     "<control>F",  focus_on_find, 0, "<Item>" },
    { "/Focus/List",     "<control>L",  focus_on_list, 0, "<Item>" },
    { "/Focus/Description", "<control>E",  focus_on_description, 0, "<Item>" },
    { "/_Help",          NULL,          NULL, 0, "<LastBranch>" },
    { "/Help/_About",    NULL,          call_show_about, 0,  "<Item>" },
};

/* create main menu from menu items array */
void widgets_menu_create( GtkWidget  *window, GtkWidget **menubar )
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

	accel_group = gtk_accel_group_new ();

	/* Create menu bar by item factory from menu array */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

	/* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */ 
	if (menubar)
		*menubar = gtk_item_factory_get_widget (item_factory, "<main>");
}

/* print msg in status bar */
void status(const char *msg)
{
	if ( ! GTK_IS_STATUSBAR(status_bar) )
		{
			fprintf(stderr, "%s \n", msg);
			return;
		}
    gtk_statusbar_push (GTK_STATUSBAR(status_bar), 0, msg);
}

void util_select_row(int row)
{
	if ( row<=0 )
		return;
	
	GtkTreePath *path = gtk_tree_path_new();
	gtk_tree_path_append_index (path, row);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(tv), path, NULL, 1, 0.5, 0.5);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW(tv), path, NULL, 0);
	
	gboolean retval;
	g_signal_emit_by_name( tv, "move-cursor", GTK_MOVEMENT_VISUAL_POSITIONS, tv, &retval);
}

/* key events (like escape or up down) processing */
gboolean call_key_pressed (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	switch(event->keyval){
	case GDK_Escape:
		gtk_entry_set_text(GTK_ENTRY(entry), "");
		return 1;
	case GDK_Up:
	case GDK_Down:
		{
			int *indices;
			GtkTreePath* path;
			
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tv), &path, NULL);
			if ( path )
				{
					indices = gtk_tree_path_get_indices(path);
					if (event->keyval == GDK_Up)  --*indices;
					if (event->keyval == GDK_Down) ++*indices;
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv), path, NULL, FALSE);
					gtk_tree_path_free(path);
				}
		}
		return 1;	
	default:
		return 0;
	}
}

/* if text in entry changed start search in dict for word in entry */
gboolean call_key_released (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    static char old_word[0x200];    
    if( !strcmp( GTK_ENTRY(entry)->text, old_word) )
	    return 0;
	
    strcpy(old_word, GTK_ENTRY(entry)->text);
	g_signal_emit_by_name( entry, "activate");
	
	return 0;
}

/* callback for enter pressed in search entry box */
void call_entry_activated(GtkEntry *entry, gpointer user_data){
	char* table = gtk_combo_box_get_active_text((GtkComboBox*)dictionary_menu);
	int word_id = get_word_id( GTK_ENTRY(entry)->text, table );
	util_select_row(word_id);
	g_free(table);
}

/* word selection in treeview processing */
void call_row_selected(GtkTreeView *treeview, GtkMovementStep arg1, gint arg2, gpointer user_data)
{
	int *indices;
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(tv), &path, NULL);

	if(!path)
		return;

	indices = gtk_tree_path_get_indices(path);
	load_description( text, indices[0], gtk_combo_box_get_active_text((GtkComboBox*)dictionary_menu) );
	gtk_tree_path_free(path);
}

/* connect widgets to signal handlers */
void widgets_connect()
{
    g_signal_connect ( entry, "activate", G_CALLBACK (call_entry_activated), NULL);
    g_signal_connect ( window, "delete_event", G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect ( entry, "key-release-event", G_CALLBACK(call_key_released), NULL);
	g_signal_connect ( entry, "key-press-event", G_CALLBACK(call_key_pressed), NULL);

	GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	g_signal_connect ( G_OBJECT(select), "changed", G_CALLBACK(call_row_selected), NULL);
}

/* Widgets setup :) */
void widgets_setup()
{
    //Append column 
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("word", gtk_cell_renderer_text_new(), "text", 1, NULL );
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (GTK_TREE_VIEW(tv), column);
	
	//Setup selection
	GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);

	//TreeView
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(tv), 1);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(tv), 0);

    //text
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

    //window
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw_text), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw_text), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_window_set_default_size (GTK_WINDOW (window), 600, 400);

    //other
    gtk_widget_grab_focus (entry);
	gtk_statusbar_set_has_resize_grip( GTK_STATUSBAR(status_bar), 0 );
}

/* creation of all widgets in main window */
void widgets_create()
{
    GtkWidget *hpaned = gtk_hpaned_new();
    GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
    GtkWidget *sw = gtk_scrolled_window_new( NULL , NULL );
    GtkWidget *word_box = gtk_vbox_new(FALSE, 2);
    GtkWidget *menubar;


    /* Create widgets */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    entry = gtk_entry_new();
    sw_text = gtk_scrolled_window_new(NULL, NULL);
    text_view = gtk_text_view_new();
    dictionary_menu = gtk_combo_box_new_text();
    status_bar = gtk_statusbar_new ();
    text=gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
    dialog = gtk_dialog_new_with_buttons ("Pic The Dictionary", (GtkWindow*)window, 
										  GTK_DIALOG_MODAL,  GTK_STOCK_OK, GTK_RESPONSE_OK, 
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);


    /* create menu */
    widgets_menu_create(window, &menubar);

    /* Pack Widgets */
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_container_add (GTK_CONTAINER (sw_text), text_view);

    //dialog
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), dictionary_menu);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    //vbox
    gtk_box_pack_end (GTK_BOX(vbox), status_bar, FALSE, FALSE, 1);
    gtk_box_pack_end (GTK_BOX(vbox), hpaned, TRUE, TRUE, 1);
    gtk_box_pack_end (GTK_BOX(vbox), menubar, FALSE, FALSE, 1);

    //wordbox
    gtk_box_pack_end (GTK_BOX(word_box), entry, FALSE, TRUE, 1);
    gtk_box_pack_end (GTK_BOX(word_box), sw, 1, 1, 1);
	
    //list and textview in hpanned
    gtk_paned_add2 ( (GtkPaned*)hpaned, sw_text);
    gtk_paned_add1 ( (GtkPaned*)hpaned, word_box);
    
    //clist
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
    tv = gtk_tree_view_new();
    gtk_container_add( GTK_CONTAINER(sw), tv);
}

/* try to extract word from OS text buffer */
gchar* get_clipboard_text()
{
	GtkClipboard* cb = gtk_clipboard_get ( GDK_SELECTION_PRIMARY );

    if( ! gtk_clipboard_wait_is_text_available (cb) )
		return NULL;

	gchar* ct = gtk_clipboard_wait_for_text (cb);
	gchar* str = g_locale_from_utf8(ct, -1, NULL, NULL, NULL);
	g_free(ct);

	if(!str)
		return NULL;
	
	gchar* p = str;
	while(!isalpha(*p) && *p)
		p++;
	strcpy(str, p);
		
	while(isalpha(*p))
		p++;
	*p = 0;

	ct = g_locale_to_utf8(str, -1, NULL, NULL, NULL);
	g_free(str);
		
	return ct;
}

int main(int argc, const char* argv[])
{
	gchar* ct;
    
	gtk_init (&argc, &argv);

	widgets_create();
	widgets_setup();
	widgets_connect();

	/* connect to db */
	char self_name[1024];
	readlink("/proc/self/exe", self_name, sizeof(self_name));
	int init_db_code = init_db(self_name);
	if ( init_db_code != SQLITE_OK ){
		report_error("Failed to open database %s: %s", db_path, sqlite3_errstr(init_db_code));
		return 1;
	}

	/* get dictionary list */
	if( ! load_dicts_list(GTK_COMBO_BOX(dictionary_menu)) ){
		report_error("Dicts loading failed");
		return 1;
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(dictionary_menu), start_dict_index );
	load_word_list(GTK_TREE_VIEW(tv), gtk_combo_box_get_active_text((GtkComboBox*)dictionary_menu) );
	
    /* Finish init */
	gtk_widget_show_all (window);	

	/* Try to extract word from OS text buffer and set it */
	if( (ct = get_clipboard_text()) ){
		gtk_entry_set_text( GTK_ENTRY(entry), ct );
		g_signal_emit_by_name( entry, "activate", NULL);
		g_free(ct);
	}
	
    /* Going in the main loop */
    gtk_main();

    return 0;
}

#ifdef WIN32
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	return  main(__argc, __argv);
}
#endif
