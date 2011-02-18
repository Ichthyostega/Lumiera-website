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
import string
import menuformat
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
    proj = root.linkChild('project')
    doc  = root.linkChild('documentation')
    
    doc.linkChild('design')
    doc.linkChild('technical')
    
    Node('design').linkChild('gui')
    Node('workflow').linkParent('design')
    Node('Roadmap').linkParent('project')
    
    root.linkChild('devs-vault').linkChild('Roadmap')
    Node('devs-vault').linkChild('devs')
    
    tech = Node('documentation/technical')
    tech.linkChild('technical/gui')
    tech.linkChild('technical/proc')
    tech.linkChild('technical/backend')
    



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
    parser.add_option("-w", "--webpage",       action="store_true"
                     ,help="generate active menu webpage (HTML)")
    
    (options, args) = parser.parse_args()
    
    global startdir
    if len(args) >= 1:
        startdir = args[0]
    else:
        startdir = '.'
    if len(args) > 1:
        __warn('additional arguments "%s" ignored.' % args[1:])
    if not (options.debug or options.text or options.webpage):
        __warn('no output generation option specified.')
    
    
    #------dispatch-action--------------
    if options.predefined:
        addPredefined()
    if options.scan:
        discoverPages(startdir)
    if options.debug:
        dumpTables()
    if options.webpage:
        generateHtmlMenu()
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
            self.id = normaliseComponentId(id)
            self.url = None
            self.label = self.id
            self.parents = []
            self.children = []
        self.__dict__.update(args)
    
    def _isInit(self): return 'id' in self.__dict__
    
    def __repr__(self): return 'Node(%s)' % self.id
    
    def __iter__(self): return self.children.__iter__() 
    
    
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
    
    
    def getUrl(self):
        ''' generate an URL suitable for accessing this entry.
            Pages in-tree are given as site-absolute URL, while
            external URLs are passed literally
        '''
        return self.url or normaliseLocalURL(self.menuPath())
    
    
    def getParentUrl(self):
        if self.parents:
            return self.parents[0].getUrl()
        else:
            return ''




### Helpers

def normaliseComponentId(id):
    p = id.rfind('/')
    if 0 <= p :
        id = id[p+1:]
    return id
        

def normaliseLocalURL(url):
    if url.startswith('root'):
        url = url[4:]
    if url.startswith('/'):
        url = url[1:]
    root = path.abspath(startdir)
    file = path.join(root, url)
    if path.isdir(file):
        file = path.join(file,'index.html')
        if path.isfile(file):
            url += "/index.html"
    if not path.exists(file):
        if path.isfile(file+'.html'):
            url += '.html'
        else:
            __warn('not found: '+file)
            
    if not url.startswith('/'):
        url = '/'+url
    return url
    



##################### Parsing and Discovery ######################

def discoverPages (startdir):
    startdir = path.abspath(startdir)
    __warn("discoverPages(%s)" % startdir)




##################### Output Generation ##########################

def dumpTables():
    print '\nMenu Tree:\n'
    print walkMenuTree (Dumper())
    print '(end)Menu Tree\n\n'


def generateTextMenu():
    print walkMenuTree (TextFormatter())


def generateHtmlMenu():
    from menuformat import HtmlGenerator, ScriptGenerator, generateHTML
    
    buildingBlocks = {'menuBody'  : walkMenuTree(HtmlGenerator())
                     ,'menuScript': walkMenuTree(ScriptGenerator())
                     }
    print generateHTML(buildingBlocks)


def walkMenuTree (tool, subTree = Node(TREE_ROOT)):
    ''' Tree visitation
    '''
    if not subTree.children:
        tool.treatLeaf (subTree)
    else:
        tool.treatPrefix (subTree)
        for child in subTree:
            walkMenuTree (tool, child)
        tool.treatPostfix (subTree)
    
    return tool



class Formatter:
    
    def __init__(self):
        self.level = 0
        self.output = []
        self.formatters = {}
    
    def __str__(self):
        return '\n'.join (self.output)
    
    # Subclasse have to define:
    # INDENT, LEAF, PRE_SUB, POST_SUB and the showNode() method
    
    def format(self, template, **vars):
        engine = self.formatters.get(template)
        if not engine:
            engine = string.Template(template)
            self.formatters[template] = engine
        renderedText = engine.substitute(vars)
        return self.level * self.INDENT + renderedText
    
    def show(self, formattedData):
        self.output.append(formattedData)
    
    def treatLeaf(self, node):
        self.showNode(self.LEAF, node)
    
    def treatPrefix(self, node):
        self.showNode(self.PRE_SUB, node)
        self.level += 1
    
    def treatPostfix(self, node):
        self.level -=1
        self.show (self.format(self.POST_SUB, ID=node.id))



class TextFormatter(Formatter):
    
    INDENT   ='    |'
    LEAF     =' +-'
    PRE_SUB  =' +- |'
    POST_SUB ='    |____________'
    
    def showNode(self, template, node):
        self.show (self.format (template+' '+node.label))



class Dumper(Formatter):
    
    INDENT   ='\t'
    LEAF     ='Leaf'   
    PRE_SUB  ='Sub_'
    POST_SUB ='--(end)$ID----------\n'
    
    
    def showNode(self, kind, node):
        self.show (self.format ('%s: "%s"' % (kind, node.id)))
        self.show (self.format ('....: url='+node.getUrl()))
        if (node.label != node.id):
            self.show (self.format ('....: label='+node.label))











#
# --Messages-and-errors-------------------------------------
#
def __err(text):
    print "--ERROR-------------------------"
    print >> sys.stderr, text
    sys.exit(255)

def __warn(text):
    print >> sys.stderr, "--WARNING--   " + str(text)

def __exerr(text):
    ######DEBUG raise sys.exc_type,sys.exc_value
    __err(text + ": »%s« (%s)" % (sys.exc_type,sys.exc_value))


if __name__ == "__main__":
    parseAndDo()
