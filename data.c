#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

#include "header.h"
#include "sqlitemodel.h"

static sqlite3 *connect;

const char* db_path = "/var/lib/cslov/ptkdic.db";

int init_db(const char* pname)
{
	const char* local_path = basename((char*)db_path);
	const char* pname_path = g_strdup_printf("%s/%s", dirname((char*)pname), basename((char*)db_path));
	const char* absolute_path = db_path;
	const char *path = 0;

	if(access(local_path, R_OK) == 0)
		path = local_path;
	else if(access(pname_path, R_OK) == 0){
		path = pname_path;
	}
	else if(access(absolute_path, R_OK) == 0)
		path = absolute_path;
	else{
		return SQLITE_CANTOPEN;
	}

	return sqlite3_open_v2(path, &connect, SQLITE_OPEN_READONLY, NULL);
}

static int set_description(void *TextBuffer, int argc, char **argv, char **azColName)
{
	GtkTextBuffer *buffer = TextBuffer;
	gtk_text_buffer_set_text ( buffer, argv[0], -1);
	return 0;
}

void load_description(GtkTextBuffer* buffer, int row, char* table_name)
{
    char q[0x200];
    sprintf(q, "SELECT art_txt FROM %s WHERE art_id=%i LIMIT 1", table_name, row+min_id);

	char *error;
	if ( sqlite3_exec(connect, q, set_description, buffer, &error) != SQLITE_OK ){
		report_error(error);
		free(error);
		return;
	}
}

static char* strip_string(char* string, int n)
{
	string[strlen(string) - n] = 0;
	return string;
}

int get_word_id( const char *word, const char *table_name )
{
	char **table;
	int rows, columns, res;
	int word_id = -1, iters=0;
	char *work_word = g_strdup(word);
	
	while( -1==word_id && iters++<3 ){
		char query[200];
		sprintf(query, "SELECT art_id FROM %s WHERE word LIKE  '%s%%' LIMIT 1", table_name, work_word );

		res = sqlite3_get_table(connect, query, &table, &rows, &columns, NULL);
		if(rows>0){
			status("найдено");
			word_id = strtol( table[1], NULL, 10 )-min_id;
			break;
		}
		strip_string(work_word, 1);
	}
	
	if(1!=iters)
		status("не точно");
	if(-1==word_id)
		status("не найдено");
	g_free(work_word);

	return word_id;
}

static int dicts_loaded = 0;

static int add_dict(void *obj, int argc, char **argv, char **azColName)
{
	GtkComboBox* combo = obj;
	dicts_loaded++;
	gtk_combo_box_append_text(combo, argv[0]);
	return 0;
}

int load_dicts_list(GtkComboBox* combo)
{
	sqlite3_exec(connect, "SELECT name FROM sqlite_master WHERE type = \"table\" ", add_dict, combo, NULL);
	return dicts_loaded != 0;
}

double timeval()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1000000.;
}

void load_word_list(GtkTreeView* tv, const char* table_name)
{
	SqliteModel *model;
	char **table, *error;
	int cols, rows, res;
	double start, end;

	start = timeval();
	
	/* execute query */
    char *query = g_strdup_printf("SELECT art_id,word FROM %s ", table_name);
	res = sqlite3_get_table(connect, query, &table, &rows, &cols, &error);
	free(query);
	if ( res != SQLITE_OK ){
	    report_error(error);
		free(error);
	    return;
	}

	/* convert mysql result to mysqlmodel */
	model = sqlite_model_new(table, rows, cols);
	
	end = timeval();
	printf("Load time[%s]: %0.0f \n", table_name, end - start);
	
	/* set model to tree view */
	gtk_tree_view_set_model(tv, GTK_TREE_MODEL(model) );
	g_object_unref( G_OBJECT(model) );

	/* set initial selection */
	GtkTreePath *path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, 0);
	gtk_tree_view_set_cursor(tv, path, NULL, FALSE);
}
