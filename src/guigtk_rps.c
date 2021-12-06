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


GtkWidget *guigtk_topwin;
GtkWidget *guigtk_topvbox;
GtkWidget *guigtk_menubar;
GtkWidget *guigtk_menu_app;
void
rpsgui_initialize (void)
{
  guigtk_topwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (guigtk_topwin), 450, 333);
  char titlebuf[80];
  memset (titlebuf, 0, sizeof (titlebuf));
  snprintf (titlebuf, sizeof (titlebuf), "refpersys p.%d [%s] %s",
	    (int) getpid (), rps_hostname (), _rps_git_short_id);
  gtk_window_set_title (GTK_WINDOW (guigtk_topwin), titlebuf);
  guigtk_topvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (guigtk_topwin), guigtk_topvbox);
  guigtk_menubar = gtk_menu_bar_new ();
  gtk_container_add (GTK_CONTAINER (guigtk_topvbox), guigtk_menubar);
  guigtk_menu_app = gtk_menu_item_new_with_label ("App");
  gtk_container_add (GTK_CONTAINER (guigtk_menubar), guigtk_menu_app);
  gtk_widget_show_all (GTK_WIDGET (guigtk_topwin));
}				/* end rpsgui_initialize_windows */

void
rps_run_gui (int *pargc, char **argv)
{
  gtk_init (pargc, &argv);
  rpsgui_initialize ();
  gtk_main ();
}				/* end rps_run_gui */
