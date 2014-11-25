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

static void ui_connect_to_server(GtkWidget *widget, gpointer data) {
	start_report_thread();
}

static void ui_add_consumer_process(GtkWidget *widget, gpointer data) {
	monitor_new_process();
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

	settingsMenuItem = gtk_builder_get_object(builder, "connectToServer");
	g_signal_connect(settingsMenuItem, "activate", G_CALLBACK(ui_connect_to_server), NULL);

	settingsMenuItem = gtk_builder_get_object(builder, "startConsumer");
	g_signal_connect(settingsMenuItem, "activate", G_CALLBACK(ui_add_consumer_process), NULL);

	gtk_main();

	return 0;
}
/*
int message_buffer_append(int bufId, char *message) {
	GtkTextIter iter;
	char namebuf[32];
	sprintf_s(namebuf, sizeof(namebuf), "textbuffer%d", (bufId + 1));
	buffers[bufId] = gtk_builder_get_object(builder, namebuf);
	gtk_text_buffer_get_iter_at_offset(buffers[bufId], &iter, -1);
	gtk_text_buffer_insert(buffers[bufId], &iter, message, -1);
}
*/

// @see: http://stackoverflow.com/questions/26052313/gtk-warning-invalid-text-buffer-iterator-when-writing-to-the-same-text-view-mul
struct DispatchData {
	GtkTextBuffer *buffer;
	char *output_str;
};
struct DispatchData *data[3];

static gboolean display_status_textbuffer(struct DispatchData *data)
{
	//printf("put: %s\n", data->output_str);
	gtk_text_buffer_set_text(data->buffer, data->output_str, strlen(data->output_str));
	g_free(data);
	return G_SOURCE_REMOVE;
}

int message_buffer_set(int bufId, char *message) {
	char namebuf[32];
	sprintf_s(namebuf, sizeof(namebuf), "textbuffer%d", (bufId + 1));
	buffers[bufId] = (GtkTextBuffer *)gtk_builder_get_object(builder, namebuf);

	data[bufId] = g_new0(struct DispatchData, 1);
	data[bufId]->output_str = message;
	printf("put: %s\n", message);
	data[bufId]->buffer = buffers[bufId];
	gdk_threads_add_idle(display_status_textbuffer, data[bufId]);
	return 0;
}