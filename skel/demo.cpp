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

int
main (int, const char*[])
{
    return GtkApp{"demo.gtk"}.run(
        [](auto& window)
          {
            std::cout << "Hello Video" << std::endl;
          });
}

