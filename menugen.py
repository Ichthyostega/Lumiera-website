#!/usr/bin/python
# -*- coding: utf-8 -*-
# -*- python -*-
##
## menugen.py  -  generate naviagtion menu for the Lumiera.org website
##

#  Copyright (C)         Lumiera.org
#    2011,               Hermann Vosseler <Ichthyostega@web.de>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of
#  the License, or (at your option) any later version.
#####################################################################


import os
import re
import sys
import types
from os import path
from optparse import OptionParser
#from subprocess import Popen, PIPE


#------------CONFIGURATION-----------------------------
PROGVER       = 0.1
PROGNAME      = 'menugen'
TREE_ROOT     = 'root'
#------------CONFIGURATION-----------------------------




def addPredefined():
    ''' populate the menu with a set of basic nodes createing a backbone,
        which can then be extended by values extracted from individual pages.
    '''
    root = Node(TREE_ROOT, label='Lumiera')
    proj = root.linkChild('Project')
    doc  = root.linkChild('Documentation')
    
    doc.linkChild('Design')
    doc.linkChild('Technical')
    
    Node('Design').linkChild('GUI')
    Node('Workflow').linkParent('Design')
    Node('Roadmap').linkParent('Project')
    
    root.linkChild('Devs Vault').linkChild('Roadmap')
    
    tech = Node('Documentation/Technical')
    tech.linkChild('GUI')
    tech.linkChild('Proc')
    tech.linkChild('Backend')
    



# -----------parse-cmdline----------------------------
def parseAndDo():
    
    usage = "usage: %prog [options] [directory]"
    
    parser = OptionParser(usage=usage, version="%s %1.2f" % (PROGNAME,PROGVER)) 
    
    parser.add_option("-p", "--predefined",    action="store_true"
                     ,help="populate the menu with a built-in backbone structure")
    parser.add_option("-s", "--scan",          action="store_true"
                     ,help="scan recursively to discover pages. Starts in current directory, unless another starting point is explicitly stated")
    parser.add_option("-d", "--debug",         action="store_true"
                     ,help="diagnostic dump of internal datastructures")
    parser.add_option("-t", "--text",          action="store_true"
                     ,help="output textual representation of the menu")
    parser.add_option("-a", "--asciidoc",      action="store_true"
                     ,help="output menu as asciidoc bulleted list")
    
    (options, args) = parser.parse_args()
    
    if len(args) >= 1:
        startdir = args[0]
    else:
        startdir = '.'
    if len(args) > 1:
        __warn('additional arguments "%s" ignored.' % args[1:])
    if not (options.debug or options.text or options.asciidoc):
        __warn('no output generation option specified.')
    
    
    #------dispatch-action--------------
    if options.predefined:
        addPredefined()
    if options.scan:
        discoverPages(startdir)
    if options.debug:
        dumpTables()
    if options.asciidoc:
        generateAsciidocMenu()
    elif options.text:
        generateTextMenu()
    
    sys.exit(0)



##################### Datastructures #############################

class NodeIndex:
    def __init__(self):
        self.cache = {}
        self.all = []
    
    def find(self, key):
        node = self.cache.get(key)
        if not node:
            for entry in self.all:
                if entry.matches(key):
                    node = entry
                    self.cache[key] = node
        return node # may be None
    
    def add(self, key, node):
        self.cache[key] = node
        self.all.append(node)



class Node(object):
    ''' Menu building block: An entry within the menu tree or DAG
        Operations provided for building the structure and for adding atributes.
        The final menu generation step traverses the node structure and accesses
        the properties to generate the desired output
    '''
    index = NodeIndex()
    
    def __new__(type, id, **args):
        ''' Factory function for nodes: retrieve existing or create new '''
        assert id
        assert isinstance(id, basestring)
        node = Node.index.find(id)
        if node == None:
            node = object.__new__(type)
            Node.index.add(id,node)
        return node
    
    def __init__(self, id, **args):
        if not self._isInit():
            self.id = id
            self.label = id
            self.parents = []
            self.children = []
        self.__dict__.update(args)
    
    def _isInit(self): return 'id' in self.__dict__
    
    def __repr__(self):
        return 'Node(%s)' % self.id
    
    
    def linkChild (self, childId):
        child = Node(childId)
        if not child in self.children:
            self.children.append(child)
            child.parents.append(self)
        return child
    
    def linkParent (self, parentId):
        parent = Node(parentId)
        if not parent in self.parents:
            self.parents.append(parent)
            parent.children.append(self)
        return parent
    
    def matches (self, nodeKey):
        ''' decide if this node is equivalent to the given nodeKey.
            That is, either the key is our own ID, or it matches some
            postfix part of our location in the menu and ends with our ID.
            Typically this is used to retrieve an existing node by symbolic ID,
            using the Node('id') constructor notation
        '''
        return (self.id == nodeKey or 
                ('/' in nodeKey and self.menuPath().endswith(nodeKey)))
    
    def menuPath(self):
        ''' constructs the path within the menu, starting with this node as leaf '''
        if self.parents:
            return self.parents[0].menuPath() + '/' + self.id
        else:
            return self.id






##################### Parsing and Discovery ######################

def discoverPages (startdir):
    startdir = path.abspath(startdir)
    __warn("discoverPages(%s)" % startdir)




##################### Output Generation ##########################

def dumpTables():
    print '\nMenu Tree:\n'
    walkMenuTree (Dumper())
    print '(end)Menu Tree\n\n'


def generateTextMenu():
    walkMenuTree (TextFormatter())


def generateAsciidocMenu():
    __warn("generateAsciidocMenu()")


def walkMenuTree (tool, subTree = Node(TREE_ROOT)):
    ''' Tree visitation
    '''
    if not subTree.children:
        tool.treatLeaf (subTree)
    else:
        tool.treatPrefix (subTree)
        for child in subTree.children:
            walkMenuTree (tool, child)
        tool.treatPostfix (subTree)



class TextFormatter:
    
    def __init__(self):
        self.level = 0
    
    INDENT   ='    |'
    LEAF     =' +-'
    PRE_SUB  =' +- |'
    POST_SUB ='    |____________'
    
    def show(self, text):
        print self.level * self.INDENT + text
    
    def showNode(self, kind, node):
        self.show ('%s %s' % (kind, node.label))
    
    def treatLeaf(self, node):
        self.showNode(self.LEAF, node)
    
    def treatPrefix(self, node):
        self.showNode(self.PRE_SUB, node)
        self.level += 1
    
    def treatPostfix(self, node):
        self.level -=1
        self.show(self.POST_SUB)



class Dumper(TextFormatter):
    
    def __init__(self):
        TextFormatter.__init__(self)
    
    INDENT   ='\t'
    LEAF     ='Leaf'   
    PRE_SUB  ='Sub'
    POST_SUB ='----'
    
    
    def showNode(self, kind, node):
        self.show ('%s: "%s"' % (kind, node.id))
        if (node.label != node.id):
            self.show ('....: label='+str(node.label))








#
# --Messages-and-errors-------------------------------------
#
def __err(text):
    print "--ERROR-------------------------"
    print text
    sys.exit(255)

def __warn(text):
    print "--WARNING--   " + str(text)

def __exerr(text):
    ######DEBUG raise sys.exc_type,sys.exc_value
    __err(text + ": »%s« (%s)" % (sys.exc_type,sys.exc_value))


if __name__ == "__main__":
    parseAndDo()
