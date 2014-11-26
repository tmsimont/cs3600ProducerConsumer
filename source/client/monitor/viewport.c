/**
 * File: viewport.c
 * Author: Trevor Simonton
 *
 * The viewport.c file implements the GTK+ library
 * to create GUI objects for the Monitor process
 *
 */
#include "monitor.h"

// primary report text pane text buffers
GtkTextBuffer *buffers[3];

GtkBuilder *builder;
GObject *window, *settingsDialog, *settingsMenuItem;
GtkWidget *console1, *console2, *console3;
GtkWidget *hbox1;

/**
 * Button callback to display settings dialog box
 */
static void ui_show_settings_dialog(GtkWidget *widget, gpointer data) {
	settingsDialog = gtk_builder_get_object(builder, "settingsDialog");
	gtk_dialog_run(GTK_DIALOG(settingsDialog));

}

/**
 * Button callback to connect to the Linux server
 */
static void ui_connect_to_server(GtkWidget *widget, gpointer data) {
	start_report_thread();
}

/**
 * Button callback to spawn a new consumer process
 */
static void ui_add_consumer_process(GtkWidget *widget, gpointer data) {
	monitor_new_process();
}

/**
 * Initialize GTK+ elements, wire up events and begin GTK+ main loop
 */
int start_ui() {
	// Construct a GtkBuilder instance and load our UI description
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "builder.ui", NULL);
	
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

/**
 * The following functions and objects are used to dispatch a text buffer update 
 * in a separate GTK+ thread. This is necessary to protect memory during
 * GTK GUI updates.
 * @see: http://stackoverflow.com/questions/26052313/gtk-warning-invalid-text-buffer-iterator-when-writing-to-the-same-text-view-mul
 */
struct DispatchData {
	GtkTextBuffer *buffer0;
	GtkTextBuffer *buffer1;
	GtkTextBuffer *buffer2;
	char *output_str_c;
	char *output_str_b;
	char *output_str_p;
};
struct DispatchData *data;
static gboolean display_status_textbuffer(struct DispatchData *data) {

	DWORD waitResult;
	printf("ui wait\n");
	waitResult = WaitForSingleObject(reportReadyMutex, INFINITE);
	switch (waitResult) {
		// The thread got ownership of the mutex
		case WAIT_OBJECT_0:
			__try {
				printf("ui got\n");
				gtk_text_buffer_set_text(data->buffer0, data->output_str_c, strlen(data->output_str_c));
				gtk_text_buffer_set_text(data->buffer1, data->output_str_b, strlen(data->output_str_b));
				gtk_text_buffer_set_text(data->buffer2, data->output_str_p, strlen(data->output_str_p));
				g_free(data);
				reportReady = 1;
			}

			__finally {
				printf("ui release\n");
				// Release ownership of the mutex object
				if (!ReleaseMutex(reportReadyMutex))	{
					printf("mutex error2\n%d\n", GetLastError());
				}
			}
			break;
		// The thread got ownership of an abandoned mutex
		case WAIT_ABANDONED:
			printf("mutex error3\n%d\n", GetLastError());
			return FALSE;
	}

	return G_SOURCE_REMOVE;
}

int viewport_update_panes(char *consumer, char *buffer, char *producer) {
	char namebuf[32];
	int bufId;

	for (bufId = 0; bufId < 3; bufId++) {
		sprintf_s(namebuf, sizeof(namebuf), "textbuffer%d", (bufId + 1));
		buffers[bufId] = (GtkTextBuffer *)gtk_builder_get_object(builder, namebuf);
	}

	data = g_new0(struct DispatchData, 1);
	data->output_str_c = consumer;
	data->output_str_b = buffer;
	data->output_str_p = producer;
	data->buffer0 = buffers[0];
	data->buffer1 = buffers[1];
	data->buffer2 = buffers[2];

	gdk_threads_add_idle(display_status_textbuffer, data);
	return 0;
}