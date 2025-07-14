/*
  demo.cpp  -  simple GTK Application

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/


#include "gtk-app.hpp"

#include <iostream>

uint
sayHello (Gtk::Window&)
{
  std::cout << "Hello Video" << std::endl;
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
