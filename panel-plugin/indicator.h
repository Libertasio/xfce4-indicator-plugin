/*  Copyright (c) 2009 Mark Trompell <mark@foresightlinux.org>
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

#ifndef __INDICATOR_H__
#define __INDICATOR_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include "indicator-box.h"

G_BEGIN_DECLS
typedef struct _IndicatorPluginClass IndicatorPluginClass;
typedef struct _IndicatorPlugin      IndicatorPlugin;

#define XFCE_TYPE_INDICATOR_PLUGIN            (indicator_get_type ())
#define XFCE_INDICATOR_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_INDICATOR_PLUGIN, IndicatorPlugin))
#define XFCE_INDICATOR_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_INDICATOR_PLUGIN, IndicatorPluginClass))
#define XFCE_IS_INDICATOR_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_INDICATOR_PLUGIN))
#define XFCE_IS_INDICATOR_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_INDICATOR_PLUGIN))
#define XFCE_INDICATOR_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_INDICATOR_PLUGIN, IndicatorPluginClass))

GType indicator_get_type      (void) G_GNUC_CONST;

void  indicator_register_type (XfcePanelTypeModule *type_module);

#ifndef INDICATOR_OBJECT_SIGNAL_ENTRY_SCROLLED
#define INDICATOR_OBJECT_SIGNAL_ENTRY_SCROLLED "scroll-entry"
#endif

void                indicator_save             (XfcePanelPlugin    *plugin,
                                                IndicatorPlugin    *indicator);

XfceIndicatorBox   *indicator_get_buttonbox    (IndicatorPlugin    *plugin);

G_END_DECLS

#endif /* !__INDICATOR_H__ */
