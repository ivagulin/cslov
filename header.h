#ifndef MAIN
#define MAIN

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* From callback */
extern int  get_word_id(const char* word, const char* table);
extern void load_word_list(GtkTreeView* tv, const char* table);
extern int  load_dicts_list(GtkComboBox *combo);
extern void load_description(GtkTextBuffer* buffer, int row, char* table_name);
extern int  init_db(const char*);
extern const char* db_path;

/* From main */
extern void status(const char*);
extern void report_error(const char *, ...);

//config vars
extern const int english_dict;
extern const int min_id;
extern const int word_retries;

#endif
