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

#include "commons.hpp"

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/application.h>
#include <glibmm/dispatcher.h>

#include <functional>
#include <memory>

using Glib::ustring;
using std::move;



/**
 * Minimalistic GTK application, used as framework for video output demo.
 * @tparam CTX custom data context to allocate while periodic processing is active
 * @remark The actual actions can be installed as functors / callbacks
 *       - onStart() installs a function to be invoked when the button is first clicked;
 *         this function must return a _context object,_ which represents "the process"
 *         and will be copied into heap storage; the further functors will receive context.
 *       - onFrame(CTX&) installs a function to be invoked periodically; the GtkApp::run() function
 *         takes a parameter do define the invocation frequency as _frames per second_
 *       - onClose(CTX&) will be invoked when the application is shut down
 * @note all processing happens in the GUI-thread; if a functor blocks, the application is deadlocked.
 */
template<class CTX>
class GtkApp
  : public Gtk::Application
  {
    class DemoWindow
      : public Gtk::Window
      {
      public:
        DemoWindow()
          {
            add (button_);
            button_.show();
          }

        Gtk::Button button_{"do It"};
      };

    DemoWindow demoWindow_;


  public:
    GtkApp (ustring appID)
      : Gtk::Application{appID}
      { }


    using StartTask = std::function<CTX(Gtk::Window&, FrameRate)>;
    using FrameTask = std::function<void(CTX&)>;
    using CloseTask = std::function<void(CTX&)>;

    GtkApp&
    onStart (StartTask task)
      {
        startTask_ = move(task);
        return *this;
      }

    GtkApp&
    onFrame (FrameTask task)
      {
        frameTask_ = move(task);
        return *this;
      }

    GtkApp&
    onClose (CloseTask task)
      {
        closeTask_ = move(task);
        return *this;
      }

    int
    run (FrameRate fps)
      {
        demoWindow_.button_.signal_clicked().connect([&]{ triggerProcessing(fps); });
        return Gtk::Application::run (demoWindow_);
      }                       // blocks while application is active


  private:
    std::unique_ptr<CTX> processor_;
    StartTask startTask_;
    FrameTask frameTask_;
    CloseTask closeTask_;

    void
    triggerProcessing (FrameRate fps)
      {
        if (processor_) return;

        uint timeout_ms = 1000 / fps;
        if (startTask_ and timeout_ms)
          {
            processor_ = std::make_unique<CTX> (startTask_(demoWindow_, fps));
            demoWindow_.button_.set_sensitive(false); // disable the button
            demoWindow_.button_.set_label("active");

            auto frameHook = [this]{
                                     if (frameTask_)
                                       frameTask_(*processor_);
                                     return bool(frameTask_); // false ≙ stop tick
                                   };
            auto closeHook = [this]{
                                     frameTask_ = FrameTask{};// disable tick
                                     if (closeTask_)
                                       closeTask_(*processor_);
                                   };

            Glib::signal_timeout().connect (frameHook, timeout_ms );
            this->signal_shutdown().connect (closeHook);
          }
      }
  };

#endif /*GTK_APP_H*/
