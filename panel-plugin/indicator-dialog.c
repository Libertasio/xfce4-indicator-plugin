/*
 *  Copyright (C) 2012-2013 Andrzej <ndrwrdck@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



/*
 *  This file implements a preferences dialog. The class extends GtkBuilder.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <exo/exo.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "indicator-dialog.h"
#include "indicator-dialog_ui.h"



static void              indicator_dialog_build               (IndicatorDialog          *dialog);
static void              indicator_dialog_response            (GtkWidget                *window,
                                                               gint                      response_id,
                                                               IndicatorDialog          *dialog);



struct _IndicatorDialogClass
{
  GtkBuilderClass   __parent__;
};

struct _IndicatorDialog
{
  GtkBuilder        __parent__;

  GObject          *dialog;
  GObject          *store;
  IndicatorConfig  *config;
};



enum
{
  COLUMN_PIXBUF,
  COLUMN_TITLE,
  COLUMN_HIDDEN,
  COLUMN_VISIBLE
};



G_DEFINE_TYPE (IndicatorDialog, indicator_dialog, GTK_TYPE_BUILDER)



static void
indicator_dialog_class_init (IndicatorDialogClass *klass)
{
}



static void
indicator_dialog_init (IndicatorDialog *dialog)
{
  dialog->dialog = NULL;
  dialog->store  = NULL;
  dialog->config = NULL;
}




static void
indicator_dialog_add_indicator (IndicatorDialog *dialog,
                                GdkPixbuf       *pixbuf,
                                const gchar     *name,
                                gboolean         hidden,
                                gboolean         visible)
{
  GtkTreeIter   iter;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));
  g_return_if_fail (name == NULL || g_utf8_validate (name, -1, NULL));

  /* insert in the store */
  gtk_list_store_append (GTK_LIST_STORE (dialog->store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (dialog->store), &iter,
                      COLUMN_PIXBUF, pixbuf,
                      COLUMN_TITLE, name,
                      COLUMN_HIDDEN, hidden,
                      COLUMN_VISIBLE, visible,
                      -1);
}



static void
indicator_dialog_update_indicator_names (IndicatorDialog *dialog)
{
  GList        *li;
  const gchar  *name;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (XFCE_IS_INDICATOR_CONFIG (dialog->config));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  for(li = indicator_config_get_known_indicators (dialog->config); li != NULL; li = li->next)
    {
      name = li->data;
      indicator_dialog_add_indicator
        (dialog,
         NULL,
         name,
         indicator_config_is_blacklisted (dialog->config, name),
         indicator_config_is_whitelisted (dialog->config, name));
    }
}



static void
indicator_dialog_hidden_toggled (GtkCellRendererToggle *renderer,
                                 const gchar           *path_string,
                                 IndicatorDialog       *dialog)
{
  GtkTreeIter   iter;
  gboolean      hidden;
  gchar        *name;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (XFCE_IS_INDICATOR_CONFIG (dialog->config));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (dialog->store), &iter, path_string))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->store), &iter,
                          COLUMN_HIDDEN, &hidden,
                          COLUMN_TITLE, &name, -1);

      /* insert value (we need to update it) */
      hidden = !hidden;

      /* update box and store with new state */
      indicator_config_blacklist_set (dialog->config, name, hidden);
      gtk_list_store_set (GTK_LIST_STORE (dialog->store), &iter, 2, hidden, -1);

      g_free (name);
    }
}



static void
indicator_dialog_visible_toggled (GtkCellRendererToggle *renderer,
                                  const gchar           *path_string,
                                  IndicatorDialog       *dialog)
{
  GtkTreeIter   iter;
  gboolean      visible;
  gchar        *name;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (XFCE_IS_INDICATOR_CONFIG (dialog->config));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (dialog->store), &iter, path_string))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dialog->store), &iter,
                          COLUMN_VISIBLE, &visible,
                          COLUMN_TITLE, &name, -1);

      /* insert value (we need to update it) */
      visible = !visible;

      /* update box and store with new state */
      indicator_config_whitelist_set (dialog->config, name, visible);
      gtk_list_store_set (GTK_LIST_STORE (dialog->store), &iter, 3, visible, -1);

      g_free (name);
    }
}




static void
indicator_dialog_swap_rows (IndicatorDialog  *dialog,
                            GtkTreeIter      *iter_prev,
                            GtkTreeIter      *iter)
{
  GdkPixbuf    *pixbuf1, *pixbuf2;
  gchar        *name1, *name2;
  gboolean      hidden1, hidden2;
  gboolean      visible1, visible2;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (XFCE_IS_INDICATOR_CONFIG (dialog->config));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  gtk_tree_model_get (GTK_TREE_MODEL (dialog->store), iter_prev,
                      COLUMN_PIXBUF,  &pixbuf1,
                      COLUMN_TITLE,   &name1,
                      COLUMN_HIDDEN,  &hidden1,
                      COLUMN_VISIBLE, &visible1, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (dialog->store), iter,
                      COLUMN_PIXBUF,  &pixbuf2,
                      COLUMN_TITLE,   &name2,
                      COLUMN_HIDDEN,  &hidden2,
                      COLUMN_VISIBLE, &visible2, -1);
  gtk_list_store_set (GTK_LIST_STORE (dialog->store), iter_prev,
                      COLUMN_PIXBUF, pixbuf2,
                      COLUMN_TITLE, name2,
                      COLUMN_HIDDEN, hidden2,
                      COLUMN_VISIBLE, visible2, -1);
  gtk_list_store_set (GTK_LIST_STORE (dialog->store), iter,
                      COLUMN_PIXBUF, pixbuf1,
                      COLUMN_TITLE, name1,
                      COLUMN_HIDDEN, hidden1,
                      COLUMN_VISIBLE, visible1, -1);

  /* do a matching operation on IndicatorConfig */
  indicator_config_swap_known_indicators (dialog->config, name1, name2);
}





static gboolean
indicator_dialog_iter_equal (GtkTreeIter *iter1,
                             GtkTreeIter *iter2)
{
  return (iter1->user_data  == iter2->user_data  &&
          iter1->user_data2 == iter2->user_data2 &&
          iter1->user_data3 == iter2->user_data3);
}





static void
indicator_dialog_item_up_clicked (GtkWidget       *button,
                                  IndicatorDialog *dialog)
{
  GObject            *treeview;
  GtkTreeSelection   *selection;
  GtkTreeIter         iter, iter_prev, iter_tmp;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  treeview = gtk_builder_get_object (GTK_BUILDER (dialog), "indicators-treeview");
  g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  /* gtk_tree_model_iter_previous available from Gtk3 */
  /* so we have to search for it starting from the first iter */
  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->store), &iter_prev))
    return;

  iter_tmp = iter_prev;
  while (!indicator_dialog_iter_equal (&iter_tmp, &iter))
    {
      iter_prev = iter_tmp;
      if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->store), &iter_tmp))
        return;
    }

  indicator_dialog_swap_rows (dialog, &iter_prev, &iter);
  gtk_tree_selection_select_iter (selection, &iter_prev);
}





static void
indicator_dialog_item_down_clicked (GtkWidget       *button,
                                    IndicatorDialog *dialog)
{
  GObject            *treeview;
  GtkTreeSelection   *selection;
  GtkTreeIter         iter, iter_next;

  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  treeview = gtk_builder_get_object (GTK_BUILDER (dialog), "indicators-treeview");
  g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  iter_next = iter;
  if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->store), &iter_next))
    return;

  indicator_dialog_swap_rows (dialog, &iter, &iter_next);
  gtk_tree_selection_select_iter (selection, &iter_next);
}





static void
indicator_dialog_clear_clicked (GtkWidget       *button,
                                IndicatorDialog *dialog)
{
  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));
  g_return_if_fail (XFCE_IS_INDICATOR_CONFIG (dialog->config));
  g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  if (xfce_dialog_confirm (GTK_WINDOW (gtk_widget_get_toplevel (button)),
                           GTK_STOCK_CLEAR, NULL, NULL,
                           _("Are you sure you want to clear the list of "
                             "known indicators?")))
    {
      gtk_list_store_clear (GTK_LIST_STORE (dialog->store));

      indicator_config_names_clear (dialog->config);
    }
}





static void
indicator_dialog_build (IndicatorDialog *dialog)
{
  GtkBuilder  *builder = GTK_BUILDER (dialog);
  GObject     *object;
  GError      *error = NULL;

  if (xfce_titled_dialog_get_type () == 0)
    return;

  /* load the builder data into the object */
  if (gtk_builder_add_from_string (builder, indicator_dialog_ui,
                                   indicator_dialog_ui_length, &error))
    {

      dialog->dialog = gtk_builder_get_object (builder, "dialog");
      g_return_if_fail (XFCE_IS_TITLED_DIALOG (dialog->dialog));
      g_signal_connect (G_OBJECT (dialog->dialog), "response",
                        G_CALLBACK (indicator_dialog_response), dialog);

      object = gtk_builder_get_object (builder, "size-max");
      g_return_if_fail (GTK_IS_WIDGET (object));
      exo_mutual_binding_new (G_OBJECT (dialog->config), "icon-size-max",
                              G_OBJECT (object), "value");

      object = gtk_builder_get_object (builder, "checkbutton-align-left");
      g_return_if_fail (GTK_IS_WIDGET (object));
      exo_mutual_binding_new (G_OBJECT (dialog->config), "align-left",
                              G_OBJECT (object), "active");

      object = gtk_builder_get_object (builder, "checkbutton-whitelist");
      g_return_if_fail (GTK_IS_WIDGET (object));
      exo_mutual_binding_new (G_OBJECT (dialog->config), "mode-whitelist",
                              G_OBJECT (object), "active");

      dialog->store = gtk_builder_get_object (builder, "indicators-store");
      g_return_if_fail (GTK_IS_LIST_STORE (dialog->store));
      indicator_dialog_update_indicator_names (dialog);

      object = gtk_builder_get_object (builder, "hidden-toggle");
      g_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (object));
      g_signal_connect (G_OBJECT (object), "toggled",
                        G_CALLBACK (indicator_dialog_hidden_toggled), dialog);

      object = gtk_builder_get_object (builder, "visible-toggle");
      g_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (object));
      g_signal_connect (G_OBJECT (object), "toggled",
                        G_CALLBACK (indicator_dialog_visible_toggled), dialog);

      object = gtk_builder_get_object (builder, "item-up");
      g_return_if_fail (GTK_IS_BUTTON (object));
      g_signal_connect (G_OBJECT (object), "clicked",
                        G_CALLBACK (indicator_dialog_item_up_clicked), dialog);

      object = gtk_builder_get_object (builder, "item-down");
      g_return_if_fail (GTK_IS_BUTTON (object));
      g_signal_connect (G_OBJECT (object), "clicked",
                        G_CALLBACK (indicator_dialog_item_down_clicked), dialog);

      object = gtk_builder_get_object (builder, "indicators-clear");
      g_return_if_fail (GTK_IS_BUTTON (object));
      g_signal_connect (G_OBJECT (object), "clicked",
                        G_CALLBACK (indicator_dialog_clear_clicked), dialog);
    }
  else
    {
      g_critical ("Faild to construct the builder: %s.",
                  error->message);
      g_error_free (error);
    }
}


static void
indicator_dialog_response (GtkWidget                *window,
                           gint                      response_id,
                           IndicatorDialog          *dialog)
{
  g_return_if_fail (GTK_IS_DIALOG (window));
  g_return_if_fail (XFCE_IS_INDICATOR_DIALOG (dialog));

  if (response_id == GTK_RESPONSE_HELP)
    {
      xfce_dialog_show_help (GTK_WINDOW (window), "xfce4-indicator", "dialog", NULL);
    }
  else
    {
      gtk_widget_destroy (window);

      g_object_unref (G_OBJECT (dialog));
    }
}



void
indicator_dialog_show (GdkScreen       *screen,
                       IndicatorConfig *config)
{
  static IndicatorDialog *dialog = NULL;

  g_return_if_fail (GDK_IS_SCREEN (screen));
  g_return_if_fail (XFCE_IS_INDICATOR_CONFIG (config));

  if (dialog == NULL)
    {
      dialog = g_object_new (XFCE_TYPE_INDICATOR_DIALOG, NULL);
      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer *) &dialog);
      dialog->config = config;
      indicator_dialog_build (XFCE_INDICATOR_DIALOG (dialog));
      gtk_widget_show (GTK_WIDGET (dialog->dialog));
    }
  else
    {
      gtk_window_present (GTK_WINDOW (dialog->dialog));
    }

  gtk_window_set_screen (GTK_WINDOW (dialog->dialog), screen);
}
