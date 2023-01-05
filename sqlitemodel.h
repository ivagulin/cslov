#ifndef __SQLITE_MODEL_H__
#define __SQLITE_MODEL_H__

#include <gtk/gtktreemodel.h>
#include <sqlite3.h>

typedef struct _SqliteModel SqliteModel;
typedef struct _SqliteModelClass SqliteModelClass;

GtkType sqlite_model_get_type (void);
SqliteModel* sqlite_model_new (char** table, int rows, int columns);

struct _SqliteModel
{
	GObject parent;

	gint stamp;
	gint columns;
	gint rows;
	char** table;
};

struct _SqliteModelClass
{
	GObjectClass parent_class;	
};

#endif /* __SQLITE_MODEL_H__ */
