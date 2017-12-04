#include <gtk/gtk.h>

static void printBuffer (GtkTextBuffer *buffer){
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
  g_print ("%s\n", gtk_text_buffer_get_text(buffer, &start, &end, TRUE));
}

static void activate (GtkApplication* app, gpointer user_data){
	GtkWidget *window;	
	GtkWidget *view;
	GtkTextBuffer *buffer;
	GtkWidget *button;
  GtkWidget *button_box;
  GtkWidget *box;

	window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Inspect");
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 400);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 50);
	gtk_container_add (GTK_CONTAINER (window), box);

  view = gtk_text_view_new ();
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_set_text (buffer, "A EU VO GOSA", 12);
	gtk_container_add (GTK_CONTAINER (box), view);

	button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_container_add (GTK_CONTAINER (box), button_box);

  button = gtk_button_new_with_label ("Send");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (printBuffer), buffer);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_container_add (GTK_CONTAINER (button_box), button);

	gtk_widget_show_all (window);
}

int main (int argc, char **argv){
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.bff", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}