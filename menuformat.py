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


import string
import menugen



menuTemplate = '''<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
    "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">

<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="expires" content="0">
<title>Navigation</title>
<link rel="stylesheet" href="/xhtml11.css" type="text/css" />
<link rel="stylesheet" href="/page.css" type="text/css" />
<link rel="stylesheet" href="/menu.css" type="text/css" />
<script type="text/javascript" src="/js/menu.js"></script>
</head>

<body>
<ul id='menu'>
$menuBody
</ul>

<script type="text/javascript">
// Generated Script allowing to mark menu entries as selected
$menuScript
</script>

</body>
</html>
'''


def generateHTML(buildingBlocks):
    ''' instantiate the menu template,
        thereby expanding the placeholders
        based on the passed in variable mappings
    '''
    engine = string.Template(menuTemplate)
    return engine.substitute(buildingBlocks)


def expandButtonHTML(id):
    return '<span class="expand_button" onclick="menuTable.toggle(\'%s\')">+</span>' % id



class HtmlGenerator(menugen.Formatter):
    
    INDENT   ='    '
    LEAF     ='<li id="$ID"><a href="$URL" target="_top" >$LABEL</a></li>'
    PRE_SUB  ='<li class="submenu"><a href="$URL" target="_top" >$LABEL</a>  $EXPANDBUTTON <ul id="$ID">'
    POST_SUB ='</ul></li>'
    
    
    def showNode(self, template, node):
        nodeID = node.menuPath()
        self.show (self.format (template,
                                 ID=nodeID,
                                 URL=node.getUrl(),
                                 LABEL=node.label,
                                 EXPANDBUTTON=expandButtonHTML(nodeID)))



class ScriptGenerator(menugen.Formatter):
    
    INDENT   ='    '
    LEAF     ="menuTable.addNode ('$ID', '$URL', '$PARENT')"
    PRE_SUB  ="menuTable.addNode ('$ID', '$URL', '$PARENT', isSubmenu=true)"
    POST_SUB ='//(end $ID)'
    
    
    def showNode(self, template, node):
        self.show (self.format (template,
                                 ID=node.menuPath(),
                                 URL=node.getUrl(),
                                 PARENT=node.getParentUrl()))


