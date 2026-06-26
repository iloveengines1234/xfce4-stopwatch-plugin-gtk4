/* Copyright (c) Natanael Copa <ncopa@alpinelinux.org>
 * SPDX-License-Identifier: MIT
 */

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>

#include "stopwatchtimer.h"
#include "stopwatch.h"

static void stopwatch_construct (XfcePanelPlugin *plugin);
XFCE_PANEL_PLUGIN_REGISTER (stopwatch_construct);

static gint
update_start_stop_image (GtkToggleButton *button) {
    int active;
    const char *icon_names[2] = { "media-playback-start", "media-playback-pause" };

    active = gtk_toggle_button_get_active (button);

    /* GTK4: Instead of gtk_button_set_image, we configure the child of the button directly */
    GtkWidget *image = gtk_image_new_from_icon_name (icon_names[active & 1]);
    gtk_button_set_child (GTK_BUTTON (button), image);
    
    return active;
}

void
stopwatch_save (XfcePanelPlugin *plugin, StopwatchPlugin *stopwatch)
{
    XfceRc *rc;
    gchar *filename;
    gint64 start, end;
    gchar buf[32];

    filename = xfce_panel_plugin_save_location (plugin, TRUE);

    if (G_UNLIKELY (filename == NULL)) {
        g_debug ("Failed to get config file location");
        return;
    }

    rc = xfce_rc_simple_open (filename, FALSE);
    g_free (filename);

    if (G_UNLIKELY (rc == NULL)) {
        return;
    }

    stopwatch_timer_get_state (stopwatch->timer, &start, &end);
    g_snprintf (buf, sizeof(buf), "%" G_GINT64_FORMAT, start);
    xfce_rc_write_entry (rc, "start_time", buf);
    g_snprintf (buf, sizeof(buf), "%" G_GINT64_FORMAT, end);
    xfce_rc_write_entry (rc, "end_time", buf);
    xfce_rc_close (rc);
}

static void
stopwatch_load (StopwatchPlugin *stopwatch)
{
    XfceRc *rc;
    gchar *filename;
    gint64 start, end;
    const gchar *value;

    filename = xfce_panel_plugin_save_location (stopwatch->plugin, TRUE);

    if (G_UNLIKELY (filename == NULL)) {
        g_debug ("Failed to get config file location");
        return;
    }

    rc = xfce_rc_simple_open (filename, TRUE);
    g_free (filename);

    if (G_UNLIKELY (rc == NULL)) {
        return;
    }

    value = xfce_rc_read_entry (rc, "start_time", "0");
    start = (gint64) g_ascii_strtoll (value, NULL, 10);

    value = xfce_rc_read_entry (rc, "end_time", "0");
    end = (gint64) g_ascii_strtoll (value, NULL, 10);

    xfce_rc_close (rc);

    stopwatch_timer_set_state (stopwatch->timer, start, end);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stopwatch->button),
                                  stopwatch_timer_is_active(stopwatch->timer));
}

static void
stopwatch_toggle (GtkToggleButton *button, gpointer user_data) {
    StopwatchPlugin *stopwatch = (StopwatchPlugin *)user_data;
    gboolean active = update_start_stop_image (button);
    if (active) {
        stopwatch_timer_start(stopwatch->timer);
    } else {
        stopwatch_timer_stop(stopwatch->timer);
    }
    
    if (stopwatch->menuitem_reset) {
        gtk_widget_set_sensitive (stopwatch->menuitem_reset, !active);
    }
    stopwatch_save (stopwatch->plugin, stopwatch);
}

static gboolean
stopwatch_update_display (gpointer ptr)
{
    gchar buf[16];
    StopwatchPlugin *stopwatch = (StopwatchPlugin *)ptr;
    gint64 elapsed = (gint64) stopwatch_timer_elapsed (stopwatch->timer) / 1000000;
    guint seconds = elapsed % 60;
    guint minutes = (elapsed / 60) % 60;
    guint hours = (elapsed / (60 * 60));

    g_snprintf (buf, sizeof(buf), "%02d:%02d:%02d", hours, minutes, seconds);
    gtk_label_set_text (GTK_LABEL (stopwatch->label), buf);
    return TRUE;
}

static StopwatchPlugin *
stopwatch_new (XfcePanelPlugin *plugin)
{
    StopwatchPlugin *stopwatch;
    XfcePanelPluginMode mode;
    GtkOrientation orientation;

    stopwatch = g_slice_new0(StopwatchPlugin);
    stopwatch->plugin = plugin;
    stopwatch->timer = stopwatch_timer_new();

    mode = xfce_panel_plugin_get_mode (plugin);
    orientation = (mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

    /* GTK4: GtkEventBox is dropped. We pass elements directly via a horizontal or vertical layout container */
    stopwatch->box = gtk_box_new (orientation, 2);
    gtk_box_set_homogeneous (GTK_BOX (stopwatch->box), FALSE);

    GtkWidget *spacer_start = gtk_label_new (NULL);
    gtk_widget_set_hexpand(spacer_start, TRUE);
    gtk_widget_set_vexpand(spacer_start, TRUE);
    gtk_box_append (GTK_BOX (stopwatch->box), spacer_start);

    stopwatch->label = gtk_label_new (NULL);
    gtk_label_set_selectable (GTK_LABEL (stopwatch->label), FALSE);
    
    /* GTK4 handling of label orientation strings uses modern layout configurations */
    if (orientation == GTK_ORIENTATION_VERTICAL) {
        gtk_label_set_vertical_alignment(GTK_LABEL(stopwatch->label), TRUE);
    }
    
    gtk_widget_set_halign (stopwatch->label, GTK_ALIGN_CENTER);
    gtk_box_append (GTK_BOX (stopwatch->box), stopwatch->label);

    stopwatch->button = gtk_toggle_button_new ();
    gtk_widget_set_focusable (stopwatch->button, FALSE);
    gtk_widget_set_halign (stopwatch->button, GTK_ALIGN_CENTER);
    gtk_box_append (GTK_BOX (stopwatch->box), stopwatch->button);

    GtkWidget *spacer_end = gtk_label_new (NULL);
    gtk_widget_set_hexpand(spacer_end, TRUE);
    gtk_widget_set_vexpand(spacer_end, TRUE);
    gtk_box_append (GTK_BOX (stopwatch->box), spacer_end);

    stopwatch_load (stopwatch);
    update_start_stop_image (GTK_TOGGLE_BUTTON (stopwatch->button));
    stopwatch_update_display(stopwatch);

    g_signal_connect (stopwatch->button, "toggled", G_CALLBACK (stopwatch_toggle), stopwatch);

    stopwatch->timeout_id = g_timeout_add_seconds (1, stopwatch_update_display, stopwatch);

    return stopwatch;
}

static void
stopwatch_free (XfcePanelPlugin *plugin, StopwatchPlugin *stopwatch)
{
    g_source_remove(stopwatch->timeout_id);
    g_slice_free (StopwatchPlugin, stopwatch);
}

static void
stopwatch_mode_changed (XfcePanelPlugin *plugin,
                        XfcePanelPluginMode mode,
                        StopwatchPlugin *stopwatch)
{
    GtkOrientation orientation = (mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

    gtk_orientable_set_orientation (GTK_GUIDE(stopwatch->box) ? GTK_ORIENTABLE (stopwatch->box) : GTK_ORIENTABLE(stopwatch->box), orientation);
    if (orientation == GTK_ORIENTATION_VERTICAL) {
         gtk_label_set_vertical_alignment(GTK_LABEL(stopwatch->label), TRUE);
    } else {
         gtk_label_set_vertical_alignment(GTK_LABEL(stopwatch->label), FALSE);
    }
}

static gboolean
stopwatch_size_changed (XfcePanelPlugin *plugin,
                        gint size,
                        StopwatchPlugin *stopwatch)
{
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    } else {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);
    }

    return TRUE;
}

static void
stopwatch_reset (GsimpleAction *action, GVariant *parameter, gpointer user_data)
{
    StopwatchPlugin *stopwatch = (StopwatchPlugin *)user_data;
    stopwatch_timer_reset (stopwatch->timer);
    stopwatch_update_display(stopwatch);
}

static void
stopwatch_show_about (XfcePanelPlugin *plugin, StopwatchPlugin *stopwatch)
{
    const gchar *auth[] = {
        "Diego Ongaro <ongardie@gmail.com>",
        "Natanael Copa <ncopa@alpinelinux.org>",
        NULL
    };

    gtk_show_about_dialog (NULL,
        "logo-icon-name", "xfce4-stopwatch-plugin",
        "license", xfce_get_license_text (XFCE_LICENSE_TEXT_BSD),
        "version", VERSION_FULL,
        "program-name", PACKAGE_NAME,
        "comments", _("Time yourself"),
        "website", PACKAGE_URL,
        "copyright", "Copyright \302\251 2021-" COPYRIGHT_YEAR " The Xfce development team",
        "authors", auth, NULL);
}

static void
stopwatch_construct (XfcePanelPlugin *plugin)
{
    StopwatchPlugin *stopwatch;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    stopwatch = stopwatch_new (plugin);
    
    /* GTK4: We assign the core box directly to our panel allocation frame */
    xfce_panel_plugin_set_child(plugin, stopwatch->box);
    xfce_panel_plugin_add_action_widget (plugin, stopwatch->box);

    /* libxfce4panel-3.0 context menus rely on GActions instead of GtkMenuItems */
    GSimpleActionGroup *group = g_simple_action_group_new();
    GSimpleAction *reset_action = g_simple_action_new("reset", NULL);
    g_signal_connect(reset_action, "activate", G_CALLBACK(stopwatch_reset), stopwatch);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(reset_action));
    
    gtk_widget_insert_action_group(GTK_WIDGET(plugin), "custom", G_ACTION_GROUP(group));
    xfce_panel_plugin_menu_append_action(plugin, "custom.reset", _("Reset"));
    xfce_panel_plugin_menu_show_about (plugin);

    g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (stopwatch_free), stopwatch);
    g_signal_connect (G_OBJECT (plugin), "mode-changed", G_CALLBACK (stopwatch_mode_changed), stopwatch);
    g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK (stopwatch_size_changed), stopwatch);
    g_signal_connect (G_OBJECT (plugin), "save", G_CALLBACK (stopwatch_save), stopwatch);
    g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (stopwatch_show_about), stopwatch);
}