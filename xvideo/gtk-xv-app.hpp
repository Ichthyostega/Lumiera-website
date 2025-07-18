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

#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/application.h>
#include <glibmm/dispatcher.h>

#include <functional>
#include <memory>

using Glib::ustring;
using std::move;

using uint = unsigned int;


/**
 * Minimalistic GTK application, used as framework for a demo of X-Video output.
 * @tparam CTX custom data context to allocate while periodic processing is active
 * @remark The actual actions can be installed as functors / callbacks
 *       - onStart() installs a function to be invoked when the button is first clicked;
 *         this function must return a _context object,_ which represents "the process"
 *         and will be copied into heap storage; the further functors will receive context.
 *       - onFrame(CTX&) installs a function to be invoked periodically; the GtkXvApp::run() function
 *         takes a parameter do define the invocation frequency as _frames per second_
 *       - onClose(CTX&) will be invoked when the application is shut down
 * @note all processing happens in the GUI-thread; if a functor blocks, the application is deadlocked.
 */
template<class CTX>
class GtkXvApp
  : public Gtk::Application
  {
    class DemoWindow
      : public Gtk::Window
      {
      public:
        DemoWindow()
          {
            set_default_size (CTX::VIDEO_WIDTH, CTX::VIDEO_HEIGHT);
            set_resizable (false);
            add (button_);
            button_.show();
          }

        Gtk::Button button_{"click to start XVideo display..."};
      };

    DemoWindow demoWindow_;


  public:
    GtkXvApp (ustring appID)
      : Gtk::Application{appID}
      { }


    using StartTask = std::function<CTX(Gtk::Window&)>;
    using FrameTask = std::function<void(CTX&)>;
    using CloseTask = std::function<void(CTX&)>;

    GtkXvApp&
    onStart (StartTask task)
      {
        startTask_ = move(task);
        return *this;
      }

    GtkXvApp&
    onFrame (FrameTask task)
      {
        frameTask_ = move(task);
        return *this;
      }

    GtkXvApp&
    onClose (CloseTask task)
      {
        closeTask_ = move(task);
        return *this;
      }

    int
    run (uint framesPerSec)
      {
        uint timeout_ms = 1000 / framesPerSec;
        demoWindow_.button_.signal_clicked().connect([&]{ triggerProcessing(timeout_ms); });
        return Gtk::Application::run (demoWindow_);
      }                       // blocks while application is active


  private:
    std::unique_ptr<CTX> processor_;
    StartTask startTask_;
    FrameTask frameTask_;
    CloseTask closeTask_;

    void
    triggerProcessing (uint timeout_ms)
      {
        if (processor_) return;
        if (startTask_ and timeout_ms)
          {
            processor_ = std::make_unique<CTX> (startTask_(demoWindow_));
            demoWindow_.button_.set_sensitive(false); // disable the button
            demoWindow_.button_.set_label("active");

            if (frameTask_)
              Glib::signal_timeout().connect ([this]{ frameTask_(*processor_); return true; }
                                             ,timeout_ms
                                             );
            if (closeTask_)
              this->signal_shutdown().connect([this]{ closeTask_(*processor_); }
                                             );
          }
      }
  };

#endif /*GTK_APP_H*/
