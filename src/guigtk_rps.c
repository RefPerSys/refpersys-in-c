/****************************************************************
 * file main_rps.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Description:
 *      This file is part of the Reflective Persistent System.
 *
 *      It has the GTK graphical user interface
 *
 *
 *      © Copyright 2019 - 2021 The Reflective Persistent System Team
 *      team@refpersys.org & http://refpersys.org/
 *
 * License:
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "Refpersys.h"
/* for mallopt: */
#include <malloc.h>
/* for KLIB,  Adelson-Velsky and Landis generic balanced trees from
    http://attractivechaos.github.io/klib/ see also
    https://en.wikipedia.org/wiki/AVL_tree */
#include "kavl.h"


GtkWidget *rpsgtk_topwin;
GtkWidget *rpsgtk_topvbox;
GtkWidget *rpsgtk_menubar;
GtkWidget *rpsgtk_menu_app;
GtkTextTagTable *rpsgtk_cmd_tagtable;
GtkTextTagTable *rpsgtk_output_tagtable;
GtkTextBuffer *rpsgtk_cmd_tbuf;
GtkTextBuffer *rpsgtk_output_tbuf;



void
rpsgui_initialize_command (void)
{
  rpsgtk_cmd_tagtable = gtk_text_tag_table_new ();
}				/* end rpsgui_initialize_command */

void
rpsgui_initialize_output (void)
{
  rpsgtk_output_tagtable = gtk_text_tag_table_new ();
}				/* end rpsgui_initialize_output */

void
rpsgui_initialize (void)
{
#warning rpsgui_initialize should use a GtkBuilder
  /* see https://docs.gtk.org/gtk3/class.Builder.html */
  rpsgtk_topwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (rpsgtk_topwin), 650, 555);
  char titlebuf[80];
  memset (titlebuf, 0, sizeof (titlebuf));
  snprintf (titlebuf, sizeof (titlebuf), "refpersys p.%d [%s] %s",
	    (int) getpid (), rps_hostname (), _rps_git_short_id);
  gtk_window_set_title (GTK_WINDOW (rpsgtk_topwin), titlebuf);
  rpsgtk_topvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (rpsgtk_topwin), rpsgtk_topvbox);
  rpsgtk_menubar = gtk_menu_bar_new ();
  gtk_container_add (GTK_CONTAINER (rpsgtk_topvbox), rpsgtk_menubar);
  rpsgtk_menu_app = gtk_menu_item_new_with_label ("App");
  gtk_container_add (GTK_CONTAINER (rpsgtk_menubar), rpsgtk_menu_app);
  gtk_widget_show_all (GTK_WIDGET (rpsgtk_topwin));
}				/* end rpsgui_initialize */


void
rpsgui_finalize (void)
{
}				/* end rpsgui_finalize */

void
rps_run_gui (int *pargc, char **argv)
{
  gtk_init (pargc, &argv);
  rpsgui_initialize ();
  gtk_main ();
  rpsgui_finalize ();
}				/* end rps_run_gui */
