#include "monitor.h"

GtkTextBuffer *buffers[3];
GtkBuilder *builder;

GObject *window, *settingsDialog, *settingsMenuItem;
GtkWidget *console1, *console2, *console3;
GtkWidget *hbox1;

static void ui_show_settings_dialog(GtkWidget *widget, gpointer data) {
	settingsDialog = gtk_builder_get_object(builder, "settingsDialog");
	gtk_dialog_run(GTK_DIALOG(settingsDialog));

}

int start_ui() {
	/* Construct a GtkBuilder instance and load our UI description */
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "builder.ui", NULL);

	/* Connect signal handlers to the constructed widgets. */
	window = gtk_builder_get_object(builder, "window1");
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	settingsMenuItem = gtk_builder_get_object(builder, "settingsMenuItem");
	g_signal_connect(settingsMenuItem, "activate", G_CALLBACK(ui_show_settings_dialog), NULL);

	gtk_main();
}

int message_buffer(int bufId, char *message) {
	GtkTextIter iter;
	char namebuf[32];
	sprintf_s(namebuf, sizeof(namebuf), "textbuffer%d", (bufId + 1));
	buffers[bufId] = gtk_builder_get_object(builder, namebuf);
	gtk_text_buffer_get_iter_at_offset(buffers[bufId], &iter, -1);
	gtk_text_buffer_insert(buffers[bufId], &iter, message, -1);
}