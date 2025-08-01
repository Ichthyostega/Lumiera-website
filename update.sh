#!/usr/bin/env bash

THEPATH="/home/benn/devl/cpp/projects/NetBeans/lumiera/froscon/my/examples/"
SUBDIRS="egl glx sdl1 xvideo"


main()
{
        echo "Run from the examples directroy"
        echo "The version of gcc on my machine has a problem with static_assert()"
        echo "This script copies a few edited files in a parall dir, my, to froscon"

        
        [ ! -d "$THEPATH" ] && echo "ERROR: path does not exist" $THEPATH && exit 1
        
        for  dir in ${SUBDIRS}  ; do
                [ ! -d $THEPATH$dir ] && echo "ERROR, no subdir $THEPATH$dir" && exit 1

                cp $THEPATH$dir/commons.hpp ./$dir
        done
        echo "Updated"



}


main $@
