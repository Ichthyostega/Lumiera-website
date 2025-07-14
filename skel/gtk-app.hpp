/*
  gtk-app.cpp  -  simple GTK Application

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/

#ifndef GTK_APP_H
#define GTK_APP_H

#include <gtkmm/application.h>
#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <functional>

using Glib::ustring;

using Task = std::function<void(Gtk::Window&)>;

/**
 * Minimalistic GTK application, used as framework for video output demo.
 * The #run() function accepts a »Task« that will be performed in the GUI thread,
 * when clicking the Button.
 */
class GtkApp
  : public Gtk::Application
  {
    class DemoWindow
      : public Gtk::Window
      {
        Gtk::Button button_{"doIt"};
        
      public:
        DemoWindow()
          {
            add (button_);
            button_.show();
          }
        
        void
        connectTask (Task action)
          {
            button_.signal_clicked().connect(
              sigc::track_obj ([action,this]{ action(*this); }
                              ,*this
                              ));
          }
      };
    
    DemoWindow demoWindow_;
    
  public:
    GtkApp (ustring appID)
      : Gtk::Application{appID}
      { }
    
    int
    run (Task action)
      {
        demoWindow_.connectTask (action);
        return Gtk::Application::run (demoWindow_);
      }
  };


#endif /*GTK_APP_H*/

