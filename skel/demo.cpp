/*
  demo.cpp  -  simple GTK Application

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU GPL version 2+ See the LICENSE file for details.

* ************************************************************************/


#include "commons.hpp"
#include "gtk-app.hpp"

#include <iostream>

uint
sayHello (Gtk::Window&, FrameRate fps)
{
  std::cout << "Hello Video ("<<fps<<" fps)" << std::endl;
  return 0;
}

void
markTick (uint& id)
{
  std::cout << "tick ... " << id++ << std::endl;
}

void
sayBye (uint& id)
{
  std::cout << "STOP " << id << " frames are enough!!!" << std::endl;
}


int
main (int, const char*[])
{
    return GtkApp<uint>{"demo.gtk"}
            .onStart (sayHello)
            .onFrame (markTick)
            .onClose (sayBye)
            .run(4);
}
