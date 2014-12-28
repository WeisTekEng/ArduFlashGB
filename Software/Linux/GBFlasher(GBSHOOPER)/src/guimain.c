/*
============================================================================
Name        : guimain.c
Author      : David Pello
Version     :
Copyright   : (C) David Pello 2012
Description : Ladecadence.net GameBoy FlashCart interface
============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <gtk/gtk.h>


#ifndef __BUILD_WINDOWS__
#include <ftdi.h>
#include <unistd.h>
#include <inttypes.h>
#endif

#include "gbshooper.h"
#include "communications.h"
#include "flashcart.h"

/* Callback function in which reacts to the "response" signal from the user in
 * the message dialog window.
 * This function is used to destroy the dialog window.
 */
static void on_close (GtkDialog *dialog,
						gint       response_id,
						gpointer   user_data)
{
  /*This will cause the dialog to be destroyed*/
  gtk_widget_destroy (GTK_WIDGET (dialog));

}



/* Callback function for the response signal "activate" related to the
 * SimpleAction "about_action".
 * This function is used to cause the about dialog window to popup.
 */
static void
about_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *about_dialog;
   uint8_t erc;
   status_t status;
   gchar* status_text;
   
   // check hardware
   erc = gbs_status(&status);
   if (erc != STAT_OK)
   {	
	 status_text = g_strdup_printf("Hardware not detected\n");
   }
   else 
   {
  	 status_text = g_strdup_printf("Hardware OK\nFirmware version %c.%c",
  												status.version_mayor,
  												status.version_minor);
   }

   about_dialog = gtk_about_dialog_new ();

   /* Lists of authors/ documentators to be used later, they must be initialized
    * in a null terminated array of strings.
    */
   const gchar *authors[] = {"David Pello", NULL};
   const gchar *documenters[] = {"David Pello", NULL};

   /* We fill in the information for the about dialog */
   gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (about_dialog),
   					"GTK+ GBShooper");
   gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (about_dialog),
   					"Copyright \xc2\xa9 2013 David Pello");
   gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (about_dialog), authors);
   gtk_about_dialog_set_documenters (GTK_ABOUT_DIALOG (about_dialog),
   					documenters);
   gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (about_dialog),
   					"Ladecadence.net");
   gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (about_dialog),
   					"http://ladecadence.net");
   gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG (about_dialog), status_text);

  /* We do not wish to show the title, which in this case would be
   * "AboutDialog Example". We have to reset the title of the messagedialog
   * window after setting the program name.
   */
  gtk_window_set_title (GTK_WINDOW (about_dialog), "");

  /* To close the aboutdialog when "close" is clicked we connect the response
   * signal to on_close
   */
  g_signal_connect (GTK_DIALOG (about_dialog), "response",
                    G_CALLBACK (on_close), NULL);

  /* Show the about dialog */
  gtk_widget_show (about_dialog);
  g_free(status_text);
}

static void
status_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *status_dialog;
   uint8_t erc;
   status_t status;
	
   status_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
   												GTK_MESSAGE_INFO,
   												GTK_BUTTONS_CLOSE,
   												"Status");
  erc = gbs_status(&status);
  if (erc != STAT_OK)
  {
	gtk_message_dialog_format_secondary_text((GtkMessageDialog*)status_dialog,
											 "Hardware error\n");
  }
  else {
  	gtk_message_dialog_format_secondary_text((GtkMessageDialog*)status_dialog,
  												"Hardware OK\n\
Firmware version %c.%c",
  												status.version_mayor,
  												status.version_minor);
  }

  gtk_dialog_run (GTK_DIALOG (status_dialog));
  gtk_widget_destroy (status_dialog);
}


static void
header_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *header_dialog;
   uint8_t erc;
   rom_header_t header;
	
   header_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
   												GTK_MESSAGE_INFO,
   												GTK_BUTTONS_CLOSE,
   												"Cart Info");

  erc = gbs_read_header(&header);
  if (erc != STAT_OK)
  {
	gtk_message_dialog_format_secondary_text((GtkMessageDialog*)header_dialog,
											 "Hardware error\n");
  }
  else {
  	gtk_message_dialog_format_secondary_text((GtkMessageDialog*)header_dialog,
  												"Cartridge Name: %s\n\
Cartridge Type: %s\n\
ROM Size: %s\n\
RAM Size: %s\n",
  												header.title,
  												header.cart,
  												header.rom_size,
  												header.ram_size);
  	
  }

  gtk_dialog_run (GTK_DIALOG (header_dialog));
  gtk_widget_destroy (header_dialog);

}

static void
erase_rom_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *erase_rom_window, *vbox, *progress_bar,
   			 *info_label, *button;
   GThread *exec_thread;
   thread_args_t targs;
	
   erase_rom_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_deletable (GTK_WINDOW(erase_rom_window), FALSE);
   gtk_window_set_modal(GTK_WINDOW(erase_rom_window), TRUE);
   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  
  info_label = gtk_label_new("Erasing ROM...");
  progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progress_bar), 0.1);
  button = gtk_button_new_with_label("Close");
  gtk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_box_pack_start(GTK_BOX(vbox), info_label, TRUE, TRUE, 0 );												
  gtk_box_pack_start(GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0 );
  gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0 );
  
  gtk_container_add (GTK_CONTAINER (erase_rom_window), vbox);
  
  gtk_widget_show_all(erase_rom_window);
												
  targs.stat = T_RUNNING;
  exec_thread = g_thread_new("erase_flash", &gbs_erase_flash, (void*) &targs);
  while (targs.stat != T_END)
  {
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progress_bar));
	gtk_main_iteration ();
  }
  if (targs.ret != STAT_OK)
  {
	gtk_label_set_text (GTK_LABEL(info_label), "Erase ROM Failed!");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 0);
  }
  else {
  	gtk_label_set_text (GTK_LABEL(info_label), "ROM Erased");
  	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 1);
  }
  
  gtk_widget_set_sensitive(button, TRUE);
  gtk_main();
  gtk_widget_destroy (erase_rom_window);
  g_thread_unref (exec_thread);

}

static void
erase_ram_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *erase_ram_window, *vbox, *progress_bar,
   			 *info_label, *button;
   GThread *exec_thread;
   thread_args_t targs;
	
   erase_ram_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_deletable (GTK_WINDOW(erase_ram_window), FALSE);
   gtk_window_set_modal(GTK_WINDOW(erase_ram_window), TRUE);
   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  
  info_label = gtk_label_new("Erasing RAM...");
  progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progress_bar), 0.1);
  button = gtk_button_new_with_label("Close");
  gtk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_box_pack_start(GTK_BOX(vbox), info_label, TRUE, TRUE, 0 );												
  gtk_box_pack_start(GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0 );
  gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0 );
  
  gtk_container_add (GTK_CONTAINER (erase_ram_window), vbox);
  
  gtk_widget_show_all(erase_ram_window);
												
  targs.stat = T_RUNNING;
  exec_thread = g_thread_new("erase_ram", &gbs_erase_ram, (void*) &targs);
  while (targs.stat != T_END)
  {
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progress_bar));
	gtk_main_iteration ();
  }
  if (targs.ret != STAT_OK)
  {
	gtk_label_set_text (GTK_LABEL(info_label), "Erase RAM Failed!");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 0);
  }
  else {
  	gtk_label_set_text (GTK_LABEL(info_label), "RAM Erased");
  	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 1);
  }
  
  gtk_widget_set_sensitive(button, TRUE);
  gtk_main();
  gtk_widget_destroy (erase_ram_window);
  g_thread_unref (exec_thread);

}


static void
read_rom_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *read_rom_window, *vbox, *progress_bar,
   			 *info_label, *button, *file_dialog;
   GtkFileFilter *filter;
   gchar* info_text;
   GThread *exec_thread;
   thread_args_t targs;
   uint8_t erc;
   rom_header_t header;
   
	
   read_rom_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_deletable (GTK_WINDOW(read_rom_window), FALSE);
   gtk_window_set_modal(GTK_WINDOW(read_rom_window), TRUE);
   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  
  info_label = gtk_label_new("Reading ROM...");
  progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progress_bar), 0.1);
  button = gtk_button_new_with_label("Close");
  gtk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_box_pack_start(GTK_BOX(vbox), info_label, TRUE, TRUE, 0 );												
  gtk_box_pack_start(GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0 );
  gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0 );
  
  gtk_container_add (GTK_CONTAINER (read_rom_window), vbox);
  
  gtk_widget_show_all(read_rom_window);
  
  // get header to get ROM size
  erc = gbs_read_header(&header);
  if (erc == STAT_OK && header.rom_bytes != 0)
  {
  
  // select ROM size
  targs.size = header.rom_bytes;
  info_text = g_strdup_printf("Reading %s ROM", header.rom_size);
  gtk_label_set_text (GTK_LABEL(info_label), info_text);
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "GameBoy ROMS");
  gtk_file_filter_add_pattern (filter, "*.gb");
  gtk_file_filter_add_pattern (filter, "*.sgb");
  gtk_file_filter_add_pattern (filter, "*.gbc");
  gtk_file_filter_add_pattern (filter, "*.GB");
  gtk_file_filter_add_pattern (filter, "*.SGB");
  gtk_file_filter_add_pattern (filter, "*.GBC");
  
  file_dialog = gtk_file_chooser_dialog_new ("Select ROM file to read to",
                                      NULL,
                                      GTK_FILE_CHOOSER_ACTION_SAVE,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                      NULL);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_dialog), filter);
  gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (file_dialog), FALSE);
  
  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
  {
    targs.file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog)); 
  }
  else
  {
  	gtk_widget_destroy (file_dialog);
  	gtk_widget_destroy (read_rom_window);
  	return;
  }

  gtk_widget_destroy (file_dialog);
  											
  targs.stat = T_RUNNING;
  exec_thread = g_thread_new("read_flash", &gbs_read_flash, (void*) &targs);
  while (targs.stat != T_END)
  {
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), targs.progress/100.0);
	gtk_main_iteration ();
  }
  if (targs.ret != STAT_OK)
  {
	gtk_label_set_text (GTK_LABEL(info_label), "Read ROM Failed!");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 0);
  }
  else {
  	gtk_label_set_text (GTK_LABEL(info_label), "ROM Read");
  	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 1);
  }
  
  }
  else
  {
  	gtk_label_set_text (GTK_LABEL(info_label), "Read ROM Failed!");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 0);
  }
  
  gtk_widget_set_sensitive(button, TRUE);
  gtk_main();
  gtk_widget_destroy (read_rom_window);
  g_thread_unref (exec_thread);

}


static void
write_rom_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *write_rom_window, *vbox, *progress_bar,
   			 *info_label, *button, *file_dialog;
   GtkFileFilter *filter;
   GThread *exec_thread;
   thread_args_t targs;
	
   write_rom_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_deletable (GTK_WINDOW(write_rom_window), FALSE);
   gtk_window_set_modal(GTK_WINDOW(write_rom_window), TRUE);
   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  
  info_label = gtk_label_new("Writing ROM...");
  progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progress_bar), 0.1);
  button = gtk_button_new_with_label("Close");
  gtk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_box_pack_start(GTK_BOX(vbox), info_label, TRUE, TRUE, 0 );												
  gtk_box_pack_start(GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0 );
  gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0 );
  
  gtk_container_add (GTK_CONTAINER (write_rom_window), vbox);
  
  gtk_widget_show_all(write_rom_window);
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "GameBoy ROMS");
  gtk_file_filter_add_pattern (filter, "*.gb");
  gtk_file_filter_add_pattern (filter, "*.sgb");
  gtk_file_filter_add_pattern (filter, "*.gbc");
  gtk_file_filter_add_pattern (filter, "*.GB");
  gtk_file_filter_add_pattern (filter, "*.SGB");
  gtk_file_filter_add_pattern (filter, "*.GBC");
  
  file_dialog = gtk_file_chooser_dialog_new ("Select ROM to write",
                                      NULL,
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                      NULL);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_dialog), filter);
  gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (file_dialog), FALSE);
  
  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
  {
    targs.file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog)); 
  }
  else
  {
  	gtk_widget_destroy (file_dialog);
  	gtk_widget_destroy (write_rom_window);
  	return;
  }

  gtk_widget_destroy (file_dialog);
  											
  targs.stat = T_RUNNING;
  exec_thread = g_thread_new("write_flash", &gbs_write_flash, (void*) &targs);
  while (targs.stat != T_END)
  {
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), targs.progress/100.0);
	gtk_main_iteration ();
  }
  if (targs.ret != STAT_OK)
  {
	gtk_label_set_text (GTK_LABEL(info_label), "Write ROM Failed!");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 0);
  }
  else {
  	gtk_label_set_text (GTK_LABEL(info_label), "ROM Written");
  	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 1);
  }
  
  gtk_widget_set_sensitive(button, TRUE);
  gtk_main();
  gtk_widget_destroy (write_rom_window);
  g_thread_unref (exec_thread);

}

static void
write_ram_cb (GSimpleAction *simple,
          GVariant      *parameter,
          gpointer       user_data)
{
   GtkWidget *write_ram_window, *vbox, *progress_bar,
   			 *info_label, *button, *file_dialog;
   GtkFileFilter *filter;
   GThread *exec_thread;
   thread_args_t targs;
	
   write_ram_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_deletable (GTK_WINDOW(write_ram_window), FALSE);
   gtk_window_set_modal(GTK_WINDOW(write_ram_window), TRUE);
   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  
  info_label = gtk_label_new("Writing RAM...");
  progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progress_bar), 0.1);
  button = gtk_button_new_with_label("Close");
  gtk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_box_pack_start(GTK_BOX(vbox), info_label, TRUE, TRUE, 0 );												
  gtk_box_pack_start(GTK_BOX(vbox), progress_bar, TRUE, TRUE, 0 );
  gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0 );
  
  gtk_container_add (GTK_CONTAINER (write_ram_window), vbox);
  
  gtk_widget_show_all(write_ram_window);
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "GameBoy SRAM dump");
  gtk_file_filter_add_pattern (filter, "*.sav");
  gtk_file_filter_add_pattern (filter, "*.SAV");
  
  file_dialog = gtk_file_chooser_dialog_new ("Select RAM dump to write",
                                      NULL,
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                      NULL);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_dialog), filter);
  gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (file_dialog), FALSE);
  
  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
  {
    targs.file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog)); 
  }
  else
  {
  	gtk_widget_destroy (file_dialog);
  	gtk_widget_destroy (write_ram_window);
  	return;
  }

  gtk_widget_destroy (file_dialog);
  											
  targs.stat = T_RUNNING;
  exec_thread = g_thread_new("write_ram", &gbs_write_ram, (void*) &targs);
  while (targs.stat != T_END)
  {
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), targs.progress/100.0);
	gtk_main_iteration ();
  }
  if (targs.ret != STAT_OK)
  {
	gtk_label_set_text (GTK_LABEL(info_label), "Write RAM Failed!");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 0);
  }
  else {
  	gtk_label_set_text (GTK_LABEL(info_label), "RAM Written");
  	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), 1);
  }
  
  gtk_widget_set_sensitive(button, TRUE);
  gtk_main();
  gtk_widget_destroy (write_ram_window);
  g_thread_unref (exec_thread);

}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *main_box;
  GtkWidget *info_box;
  GtkWidget *rom_box;
  GtkWidget *ram_box;
  GtkWidget *button_status;
  GtkWidget *button_header;
  GtkWidget *button_erase_rom;
  GtkWidget *button_read_rom;
  GtkWidget *button_write_rom;
  GtkWidget *button_erase_ram;
  GtkWidget *button_read_ram;
  GtkWidget *button_write_ram;
  GtkWidget *info_frame;
  GtkWidget *rom_frame;
  GtkWidget *ram_frame;
  GtkWidget *logo;
  GtkWidget *status_bar;
  GtkWidget *info_image;
  GtkWidget *erase_rom_image;
  GtkWidget *read_rom_image;
  GtkWidget *write_rom_image;
  GtkWidget *erase_ram_image;
  GtkWidget *read_ram_image;
  GtkWidget *write_ram_image;

  GSimpleAction *about_action;

  /* Create a window with a title and a default size */
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "GTK+ GBShopper");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);
  gtk_window_set_resizable(GTK_WINDOW (window), FALSE);
  
  GdkRGBA color_black;
  gdk_rgba_parse (&color_black, "black");
  
  GdkRGBA color_white;
  gdk_rgba_parse (&color_white, "white");
  
  gtk_widget_override_background_color (window, GTK_STATE_NORMAL, &color_black);

  /* Create widgets */
  main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  info_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  rom_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  ram_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

  info_frame = gtk_frame_new ("Cart Info");
  gtk_widget_override_background_color (info_frame, GTK_STATE_NORMAL, &color_black);
  gtk_widget_override_color (info_frame, GTK_STATE_NORMAL, &color_white);
  gtk_frame_set_shadow_type (GTK_FRAME(info_frame), GTK_SHADOW_NONE);
  
  rom_frame = gtk_frame_new ("ROM Operations");
  gtk_widget_override_background_color (rom_frame, GTK_STATE_NORMAL, &color_black);
  gtk_widget_override_color (rom_frame, GTK_STATE_NORMAL, &color_white);
  gtk_frame_set_shadow_type (GTK_FRAME(rom_frame), GTK_SHADOW_NONE);
  
  ram_frame = gtk_frame_new ("RAM Operations");
  gtk_widget_override_background_color (ram_frame, GTK_STATE_NORMAL, &color_black);
  gtk_widget_override_color (ram_frame, GTK_STATE_NORMAL, &color_white);
  gtk_frame_set_shadow_type (GTK_FRAME(ram_frame), GTK_SHADOW_NONE);

  logo = gtk_image_new_from_file("gbshooper.png");
  button_status = gtk_button_new_with_label ("Status");
  
  button_header = gtk_button_new();
  info_image = gtk_image_new_from_file("info-cart-48.png");
  gtk_button_set_image (GTK_BUTTON(button_header), info_image);
  gtk_widget_set_size_request(button_header, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_header),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_header, "Get cart information");

  button_erase_rom = gtk_button_new();
  erase_rom_image = gtk_image_new_from_file("erase-rom-48.png");
  gtk_button_set_image (GTK_BUTTON(button_erase_rom), erase_rom_image);
  gtk_widget_set_size_request(button_erase_rom, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_erase_rom),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_erase_rom, "Erase cart's ROM");
  
  button_read_rom = gtk_button_new();
  read_rom_image = gtk_image_new_from_file("read-rom-48.png");
  gtk_button_set_image (GTK_BUTTON(button_read_rom), read_rom_image);
  gtk_widget_set_size_request(button_read_rom, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_read_rom),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_read_rom, "Read cart's ROM to file");
  
  button_write_rom = gtk_button_new();
  write_rom_image = gtk_image_new_from_file("write-rom-48.png");
  gtk_button_set_image (GTK_BUTTON(button_write_rom), write_rom_image);
  gtk_widget_set_size_request(button_write_rom, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_write_rom),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_write_rom, "Write cart's ROM from file");

  button_erase_ram = gtk_button_new();
  erase_ram_image = gtk_image_new_from_file("erase-ram-48.png");
  gtk_button_set_image (GTK_BUTTON(button_erase_ram), erase_ram_image);
  gtk_widget_set_size_request(button_erase_ram, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_erase_ram),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_erase_ram, "Erase cart's RAM");
  
  button_read_ram = gtk_button_new();
  read_ram_image = gtk_image_new_from_file("read-ram-48.png");
  gtk_button_set_image (GTK_BUTTON(button_read_ram), read_ram_image);
  gtk_widget_set_size_request(button_read_ram, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_read_ram),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_read_ram, "Read cart's RAM to file");
  
  button_write_ram = gtk_button_new();
  write_ram_image = gtk_image_new_from_file("write-ram-48.png");
  gtk_button_set_image (GTK_BUTTON(button_write_ram), write_ram_image);
  gtk_widget_set_size_request(button_write_ram, 48, 48);
  gtk_button_set_relief(GTK_BUTTON(button_write_ram),GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button_write_ram, "Write cart's RAM from file");

  status_bar = gtk_statusbar_new();

  gtk_box_pack_start (GTK_BOX (main_box), info_frame, TRUE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (main_box), rom_frame, TRUE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (main_box), ram_frame, TRUE, TRUE, 10);
  gtk_box_pack_start (GTK_BOX (main_box), status_bar, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (info_frame), info_box);
  gtk_container_add (GTK_CONTAINER (rom_frame), rom_box);
  gtk_container_add (GTK_CONTAINER (ram_frame), ram_box);

  
  gtk_box_pack_start (GTK_BOX (info_box), button_header, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (rom_box), button_erase_rom, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rom_box), button_read_rom, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rom_box), button_write_rom, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (ram_box), button_erase_ram, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ram_box), button_read_ram, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (ram_box), button_write_ram, FALSE, FALSE, 0);
  

  gtk_container_add (GTK_CONTAINER (window), main_box);

  /* Create a new simple action, giving it a NULL parameter type. It will
   * always be NULL for actions invoked from a menu. (e.g clicking on an "ok"
   * or "cancel" button)
   */
  about_action = g_simple_action_new ("about", NULL);

  /* Connect the "activate" signal to the appropriate callback function.
   * It will indicate that the action was just activated.
   */
  g_signal_connect (about_action, "activate", G_CALLBACK (about_cb),
                    GTK_WINDOW (window));

  /* gets status from flashcart */
  g_signal_connect (button_status, "clicked", G_CALLBACK (status_cb), NULL);

  /* gets cart info from flashcart */
  g_signal_connect (button_header, "clicked", G_CALLBACK (header_cb), NULL);

  /* ROM erase */
  g_signal_connect (button_erase_rom, "clicked",
  					G_CALLBACK (erase_rom_cb), NULL);
  					
  /* ROM read */
  g_signal_connect (button_read_rom, "clicked",
  					G_CALLBACK (read_rom_cb), NULL);

  /* ROM write */
  g_signal_connect (button_write_rom, "clicked",
  					G_CALLBACK (write_rom_cb), NULL);
  					
  /* RAM erase */
  g_signal_connect (button_erase_ram, "clicked",
  					G_CALLBACK (erase_ram_cb), NULL);
  					
  /* RAM write */
  g_signal_connect (button_write_ram, "clicked",
  					G_CALLBACK (write_ram_cb), NULL);
  					

  /* Adds the about_action to the overall action map. An Action map is an
   * interface that contains a number of named GAction instances
   * (such as about_action)
   */
  g_action_map_add_action (G_ACTION_MAP (window), G_ACTION (about_action));

  gtk_widget_show_all (window);
}



/* Callback function for the response signal "activate" from the "quit" action
 * found in the function directly below.
 */
static void
quit_cb (GSimpleAction *simple,
         GVariant      *parameter,
         gpointer       user_data)
{
  GApplication *application = user_data;

  g_application_quit (application);
}



/* Startup function for the menu we are creating in this sample */
static void
startup (GApplication *app,
         gpointer      user_data)
{
  GMenu *menu;
  GSimpleAction *quit_action;

  /* Initialize the GMenu, and add a menu item with label "About" and action
   * "win.about". Also add another menu item with label "Quit" and action
   * "app.quit"
   */
  menu = g_menu_new ();
  g_menu_append (menu, "About", "win.about");
  g_menu_append (menu, "Quit", "app.quit");

  /* Create a new simple action for the application. (In this case it is the
   * "quit" action.
   */
  quit_action = g_simple_action_new ("quit", NULL);

  /* Ensure that the menu we have just created is set for the overall
   * application */
  gtk_application_set_app_menu (GTK_APPLICATION (app), G_MENU_MODEL (menu));

  g_signal_connect (quit_action,
                    "activate",
                    G_CALLBACK (quit_cb),
                    app);

  g_action_map_add_action (G_ACTION_MAP (app), G_ACTION (quit_action));

}



/* Startup function for the application */
int
main (int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("net.ladecadence.gbshooper",
  				G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  g_signal_connect (app, "startup", G_CALLBACK (startup), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
