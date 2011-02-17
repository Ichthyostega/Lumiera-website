# -*- coding: utf-8 -*-
# -*- python -*-
##
## menuformat.py  -  template and output generation for the Lumiera.org navigation menu
##

#  Copyright (C)         Lumiera.org
#    2011,               Hermann Vosseler <Ichthyostega@web.de>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of
#  the License, or (at your option) any later version.
#####################################################################


#import os
#import re
#import sys
#import types
#from os import path
#from optparse import OptionParser
#from subprocess import Popen, PIPE
import string
import menugen

#------------CONFIGURATION-----------------------------
#------------CONFIGURATION-----------------------------


menuTemplate = '''
HEAD
<script>
    $menuScript
</script>

<body>
    $menuBody
</body>
'''

def generateHTML(buildingBlocks):
    ''' instantiate the menu template,
        thereby expanding the placeholders
        based on the passed in variable mappings
    '''
    engine = string.Template(menuTemplate)
    return engine.substitute(buildingBlocks)





class HtmlGenerator(menugen.Formatter):
    
    INDENT   ='    '
    LEAF     ='<li><a href="%s" target="_top" >%s</li>'   
    PRE_SUB  ='<li><a href="%s" target="_top" class="nav">%s</li><ul>'
    POST_SUB ='</ul>'
    
    
    def showNode(self, template, node):
        url = 'TODO/'+node.id
        label = node.label
        self.show (self.format (template % (url,label)))



class ScriptGenerator(menugen.Formatter):
    
    INDENT   ='    '
    LEAF     ='menuTable.addNode (id="%s", url="%s")'   
    PRE_SUB  ='menuTable.addSubMenu (id="%s", url="%s")'
    POST_SUB ='//(end)'
    
    
    def showNode(self, template, node):
        url = 'TODO/'+node.id
        self.show (self.format (template % (node.id, url)))


