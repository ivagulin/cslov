#include "sqlitemodel.h"
#include <stdio.h>
#include <stdlib.h>

void sqlite_model_init (SqliteModel* model);
void sqlite_model_class_init (SqliteModelClass* model);
void sqlite_model_tree_model_init (GtkTreeModelIface* iface);
void sqlite_model_finalize(GObject *obj);

GtkTreeModelFlags sqlite_model_get_flags(GtkTreeModel* model);
gint sqlite_model_get_n_columns(GtkTreeModel* model);
GType sqlite_model_get_column_type(GtkTreeModel* model, gint column);
gboolean sqlite_model_get_iter(GtkTreeModel* model, GtkTreeIter *iter, GtkTreePath *path);
GtkTreePath* sqlite_model_get_path(GtkTreeModel* model, GtkTreeIter* iter);
void sqlite_model_get_value(GtkTreeModel* model, GtkTreeIter *iter, gint column, GValue *value);
gboolean sqlite_model_iter_next(GtkTreeModel* model, GtkTreeIter* iter);
gint sqlite_model_nth_child(GtkTreeModel* model, GtkTreeIter* parent, GtkTreeIter* iter, gint n);

GType sqlite_model_get_type ()
{
	static GType sqlite_model_type = 0;

	if(!sqlite_model_type){
		static const GTypeInfo sqlite_model_info = {
			sizeof (SqliteModelClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) sqlite_model_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (SqliteModel),
			0,
			(GInstanceInitFunc) sqlite_model_init,
		};

		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) sqlite_model_tree_model_init,
			NULL,
			NULL
		};
	
		sqlite_model_type = g_type_register_static(G_TYPE_OBJECT, "SqliteModel", &sqlite_model_info, 0);
		g_type_add_interface_static(sqlite_model_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
	}

	return sqlite_model_type;
}

void sqlite_model_init (SqliteModel* model)
{
	model->stamp = g_random_int ();
	model->columns = 0;
	model->rows = 0;
}

void sqlite_model_finalize(GObject* obj)
{
	SqliteModel *model = (SqliteModel*)obj;
	int cell_count = (model->rows + 1) * model->columns ;
	sqlite3_free_table(model->table);
}

static GObjectClass *parent_class = NULL;
void sqlite_model_class_init (SqliteModelClass *class)
{
	GObjectClass *object_class = (GObjectClass*) class;
	
	parent_class = g_type_class_peek_parent (class);
	
	object_class->finalize = sqlite_model_finalize;
}

void sqlite_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = sqlite_model_get_flags;
  iface->get_n_columns = sqlite_model_get_n_columns;
  iface->get_column_type = sqlite_model_get_column_type;
  iface->get_iter = sqlite_model_get_iter;
  iface->get_path = sqlite_model_get_path;
  iface->get_value = sqlite_model_get_value;
  iface->iter_next = sqlite_model_iter_next;
  iface->iter_nth_child = sqlite_model_nth_child;

  /* null methods 
  iface->iter_children = sqlite_model_null;
  iface->iter_has_child = sqlite_model_null;
  iface->iter_n_children = sqlite_model_null;
  iface->iter_nth_child = sqlite_model_null;
  iface->iter_parent = sqlite_model_null;
  */
}

SqliteModel* sqlite_model_new (char** table, int rows, int columns)
{
	SqliteModel *retval;

	retval = g_object_new (sqlite_model_get_type(), NULL);

	retval->table = table;
	retval->rows = rows;
	retval->columns = columns;
	
	return retval;
}

GtkTreeModelFlags sqlite_model_get_flags(GtkTreeModel* model)
{
	return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}

gint sqlite_model_get_n_columns(GtkTreeModel* obj)
{
	SqliteModel *model = (SqliteModel*)obj;

	return (gint) model->columns;
}

GType sqlite_model_get_column_type(GtkTreeModel* model, gint column)
{
	SqliteModel *m = (SqliteModel*)model;

	if( column >= m->columns )
		return G_TYPE_NONE;

	return G_TYPE_STRING;
}

gboolean sqlite_model_get_iter(GtkTreeModel* model, GtkTreeIter *iter, GtkTreePath *path)
{
	SqliteModel *sqlite_model = (SqliteModel*) model;
	gint i;

	i = gtk_tree_path_get_indices (path)[0];

	if ( i >= sqlite_model->rows )
		return FALSE;

	iter->stamp = sqlite_model->stamp;
	iter->user_data = (gpointer)i;

	return TRUE;
}

GtkTreePath* sqlite_model_get_path(GtkTreeModel* model, GtkTreeIter* iter)
{
	GtkTreePath *path;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, (guint)iter->user_data);
  
	return path;
}

void sqlite_model_get_value(GtkTreeModel* obj, GtkTreeIter *iter, gint column, GValue *value)
{
	SqliteModel *model = (SqliteModel*) obj;
	int row = (gint)iter->user_data;

	g_value_init (value, G_TYPE_STRING);
	g_value_set_static_string(value, model->table[(row+1)*model->columns+column]);
}

gboolean sqlite_model_iter_next(GtkTreeModel* obj, GtkTreeIter* iter)
{
	SqliteModel* model = (SqliteModel*)obj;
	int row = (gint)iter->user_data + 1;

	if ( row >= model->rows )
		return FALSE;
	
	iter->user_data++;

	return TRUE;
}

gint sqlite_model_nth_child(GtkTreeModel* model, GtkTreeIter* parent, GtkTreeIter* iter, gint n)
{
	return FALSE;
}
