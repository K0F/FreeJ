#include <SDL/SDL.h>
#include <pthread.h>
#include <context.h>
#include <plugger.h>
#include <jutils.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#include <config.h>

/* widget -> callback connection define */
#define CONNECT(w,s,c) \
  wg = glade_xml_get_widget(gtk, w); \
  if(wg) g_signal_connect((gpointer) wg, s , \
			  G_CALLBACK( c ),NULL);

/* gtk_ctrl global variables */
static Context *env;
static GladeXML *gtk;
static GtkWidget *wg;
static GtkTreeIter iter;
static GtkWidget *layer_list;
static GtkListStore *layer_model;
static GtkWidget *effect_list;
static GtkListStore *effect_model;
static GtkMenu *menu_effect;

static Layer *laysel;
static Filter *effsel;

static bool quit;

pthread_t _thread;
pthread_attr_t _attr;

/* layer's list model columns */
enum {
  LAYER_ACTIVE,
  LAYER_BLIT,
  LAYER_NAME,
  LAYER_FILE,
  LAYER_COLS
};

/* effect's list model columns */
enum {
  EFFECT_ACTIVE,
  EFFECT_NAME,
  EFFECT_COLS
};

void on_fullscreen(GtkWidget *widget, gpointer *data) {
  func("%s(%p,%p)",__FUNCTION__,widget,data);
  SDL_WM_ToggleFullScreen(env->surf);
}
void on_about(GtkWidget *widget, gpointer *data) {
  func("%s(%p,%p)",__FUNCTION__,widget,data);
  env->osd->credits();
}
void on_select_layer(GtkWidget *widget, gpointer *data) {
  Layer *lay;
  char **sel = gtk_file_selection_get_selections(GTK_FILE_SELECTION(data));
  for(int c=0;sel[c];c++) {
    func("%s : creating %s",__FUNCTION__,sel[c]);
    lay = create_layer(sel[c]);
    if(lay) env->add_layer(lay);
  }
}
void on_add_layer(GtkWidget *widget, gpointer *data) {
  func("%s(%p,%p)",__FUNCTION__,widget,data);
  /* create a file selector dialog */
  GtkWidget *fs = gtk_file_selection_new("Add a new Layer from file:");
  /* register callback when ok is pressed */
  g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),"clicked",
		   G_CALLBACK(on_select_layer), (gpointer)fs);
  /* destroy the fileselector when a button is pressed */
  g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),"clicked",
			   G_CALLBACK(gtk_widget_destroy),(gpointer)fs);
  g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),"clicked",
			   G_CALLBACK(gtk_widget_destroy),(gpointer)fs);
  gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(fs),true);
  /* show the widget */
  gtk_widget_show(fs);
}
void on_osd(GtkWidget *widget, gpointer *data) {
  func("%s(%p,%p)",__FUNCTION__,widget,data);
  //  bool res = gtk_toggle_button_get_active
  //    ((GtkToggleButton*)widget);
  env->osd->active();
}
void on_overwrite(GtkWidget *widget, gpointer *data) {
  func("%s(%p,%p)",__FUNCTION__,widget,data);
  env->clear_all = !env->clear_all;
}
void on_quit(GtkWidget *widget, gpointer *data) {
  func("%s(%p,%p)",__FUNCTION__,widget,data);
  quit = true;
  env->quit = true;
}

/* =================== LAYER LIST */
void init_layer_list() {
  
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  layer_model =  gtk_list_store_new(LAYER_COLS,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING, /* name */
				    G_TYPE_STRING, /* blit */
				    G_TYPE_STRING); /* file */
  layer_list = glade_xml_get_widget(gtk,"treeview_layer");
  /* register the model on the list view */
  gtk_tree_view_set_model((GtkTreeView*)layer_list,GTK_TREE_MODEL(layer_model));
  /* we are discarding the reference to the model now */
  g_object_unref(G_OBJECT(layer_model));
  
  /* initialize tree view columns and renderers */
  rend = gtk_cell_renderer_toggle_new();
  col = gtk_tree_view_column_new_with_attributes
    ("ON",rend,"active",LAYER_ACTIVE,NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(layer_list),col);

  rend = gtk_cell_renderer_text_new();
  //  g_object_set(G_OBJECT(rend),"background","lightorange",NULL);
  col = gtk_tree_view_column_new_with_attributes
    ("Name",rend,"text",LAYER_NAME,NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(layer_list),col);

  rend = gtk_cell_renderer_text_new();
  //  g_object_set(G_OBJECT(rend),"background","lightgray",NULL);
  col = gtk_tree_view_column_new_with_attributes
    ("Blit",rend,"text",LAYER_BLIT,NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(layer_list),col);  

  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes
    ("File",rend,"text",LAYER_FILE,NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(layer_list),col);  

}
void update_layer_list() {
  /* fill up the list of layers allready loaded */
  gtk_list_store_clear(layer_model);
  Layer *lay = (Layer*)env->layers.begin();
  while(lay) {
    gtk_list_store_append(layer_model,&iter);
    gtk_list_store_set(layer_model,&iter,
		       LAYER_ACTIVE,!lay->active,
		       LAYER_NAME,lay->getname(),
		       LAYER_BLIT,lay->get_blit(),
		       LAYER_FILE,lay->get_filename(),
		       -1);
    lay = (Layer*)lay->next;
  }
}

void init_effect_list() {
  /* =================== EFFECT LIST */

  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;

  effect_model = gtk_list_store_new(EFFECT_COLS,
				    G_TYPE_BOOLEAN, /* name */
				    G_TYPE_STRING); /* on */
  effect_list = glade_xml_get_widget(gtk,"treeview_effect");
  /* register the model on the list view */
  gtk_tree_view_set_model((GtkTreeView*)effect_list,GTK_TREE_MODEL(effect_model));
  /* we can discard the reference to the model now */
  g_object_unref(G_OBJECT(effect_model));
  /* initialize tree view columns and renderers */
  rend = gtk_cell_renderer_toggle_new();
  col = gtk_tree_view_column_new_with_attributes
    ("ON",rend,"active",EFFECT_ACTIVE,NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(effect_list),col);
  rend = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes
    ("Name",rend,"text",EFFECT_NAME,NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(effect_list),col);
}

void update_effect_list() {
  gtk_list_store_clear(effect_model);
  if(!laysel) return;
  Filter *filt = (Filter*)laysel->filters.begin();
  while(filt) {
    gtk_list_store_append(effect_model,&iter);
    gtk_list_store_set(effect_model,&iter,
		       EFFECT_ACTIVE,!filt->active,
		       EFFECT_NAME,filt->getname(),
		       -1);
    filt = (Filter*)filt->next;
  }
}
void on_select_effect(char *name) {
  gtk_menu_set_active(menu_effect,0);
  if(!laysel) {
    error("no layer selected for effect %s",name); return; }
  for(int c; env->plugger->plugs[c] ; c++) {
    if(strcmp(env->plugger->plugs[c]->getname(),name)==0)
      laysel->add_filter(env->plugger->plugs[c]);
  }
}

void init_effect_menu() {
  GtkOptionMenu *option_menu_effect;
  GtkWidget *item;

  option_menu_effect = (GtkOptionMenu*) glade_xml_get_widget(gtk,"menu_effect");
  menu_effect = (GtkMenu*)gtk_menu_new();
  gtk_menu_set_title(menu_effect,"Add effect");
  item = gtk_menu_item_new_with_label("Add effect");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_effect),item);
  for(int c=0;env->plugger->plugs[c];c++) {
    item = gtk_menu_item_new_with_label(env->plugger->plugs[c]->getname());
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_effect),item);
    g_signal_connect_swapped(G_OBJECT(item),"activate",
			     G_CALLBACK(on_select_effect),
			     (gpointer)env->plugger->plugs[c]->getname());
  }
  gtk_option_menu_set_menu(option_menu_effect,(GtkWidget*)menu_effect);
}

void *gtk_run(void *arg) {
  while(!quit) {
    update_layer_list();
    update_effect_list();
    gdk_threads_enter();
    while(gtk_events_pending())
      gtk_main_iteration();
    gdk_threads_leave();
    jsleep(0,100);
  }
  return(NULL);
}
void gtk_thread_start() {
  pthread_create(&_thread, &_attr, gtk_run, NULL);
}

void gtk_ctrl_quit() {
  quit = true;
  jsleep(0,500);
  if(pthread_attr_destroy(&_attr) == -1)
    error("error destroying POSIX thread attribute");
}

bool gtk_ctrl_init(Context *nenv, int *argc, char **argv) {
  env = nenv;
  gtk_init(argc, &argv);
  gtk = glade_xml_new("../freej-gtk2.glade",NULL,NULL);
  
  /* connect signal handlers 
     glade_xml_signal_autoconnect(gtk); */

  /* signal to glib we're going to use threads */
  g_thread_init(NULL);

  /* connect signals to callbacks */
  CONNECT("fullscreen","activate",on_fullscreen);
  CONNECT("about","activate",on_about);
  CONNECT("add_layer","activate",on_add_layer);
  CONNECT("osd_switch","toggled",on_osd);
  CONNECT("overwrite","toggled",on_overwrite);
  CONNECT("quit","activate",on_quit);
  CONNECT("main_win","destroy",on_quit);

  init_layer_list();
  init_effect_list();
  init_effect_menu();

  quit = false;
  if(pthread_attr_init (&_attr) == -1)
    error("error initializing POSIX thread attribute");
  /* sets the thread as detached
     see: man pthread_attr_init(3) */
  pthread_attr_setdetachstate(&_attr,PTHREAD_CREATE_DETACHED);


  gtk_thread_start();
  return true;
}
