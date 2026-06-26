/*
 * Copyright (C) Natanael Copa <ncopa@alpinelinux.org>
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef __STOPWATCH_H__
#define __STOPWATCH_H__

G_BEGIN_DECLS

#include "stopwatchtimer.h"

typedef struct {
    XfcePanelPlugin *plugin;
    GtkWidget   *box;             /* GTK4: Direct container wrapper, ebox removed */
    GtkWidget   *button;
    GtkWidget   *label;
    GtkWidget   *menuitem_reset;  /* Retained or can map to your GSimpleAction reference if needed later */
    StopwatchTimer  *timer;
    guint       timeout_id;

} StopwatchPlugin;

void stopwatch_save (XfcePanelPlugin *plugin, StopwatchPlugin *stopwatch);

G_END_DECLS
#endif