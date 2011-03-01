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
import string
from os import path
from optparse import OptionParser
from itertools import ifilter,chain

import menuformat   # defines specific HTML to generate


#------------CONFIGURATION-----------------------------
PROGVER      = 0.1
PROGNAME     = 'menugen'
TREE_ROOT    = 'root'
INDEX_NAME   = 'index'
SRC_FILE_EXT = '.txt'
WEBPAGE_EXT  = '.html'
menuSpec_RE  = re.compile(r'^//\s*MENU\s*:\s*', re.IGNORECASE)
#------------CONFIGURATION-----------------------------




def addPredefined():
    ''' populate the menu with a set of basic nodes createing a backbone,
        which can then be extended by values extracted from individual pages.
    '''
    root = Node(TREE_ROOT, label='Lumiera.org')
    proj = root.linkChild('project')
    doc  = root.linkChild('documentation')
    
    root.linkChild('download')
    root.linkChild('contribute')
    root.linkChild('search')
    
    vault = root.linkChild(Node('devs-vault', label="Dev's Vault"))
    
    root.enabled(False)   # suppress adding any further children
    
    # explicitly recurse into the following subdirectories
    root.discover(includes='project documentation download contribute search devs-vault'.split())
    
    proj.linkChild ('screenshots')
    proj.linkChild ('faq')
    proj.linkChild ('news')
    proj.linkChild ('press')
    proj.linkChild ('donate')
    proj.linkChild ('roadmap')
    proj.linkChild ('background')
    proj.linkChild ('contact')
    proj.linkChild ('credits')
    
    vault.linkChild('roadmap')
    vault.linkChild('devs')
    
    doc.linkChild('user')
    doc.linkChild('media')
    doc.linkChild('design')
    doc.linkChild('technical')
    
    # make the 'media' subdir appear below root/documentation
    doc.discover(includes=[TREE_ROOT+'/media'])
    doc.discover(includes='user design technical'.split())
    
    # define external links
    proj.link('http://issues.lumiera.org/roadmap', label="Roadmap (Trac)")
    vault.link('http://www.lumiera.org/gitweb',    label="Gitweb")




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
    startdir = path.abspath(startdir)
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



##################### Parsing and Discovery ######################

def discoverPages (startdir):
    if not isDir(startdir):
        __err('unable to scan/discover contents: '+
              '"%s" is not an accessable directory' % startdir)
    
    discoverLocation (TREE_ROOT)


def discoverLocation(loc, parent=None):
    file = findSource (loc)
    node = scanSource (loc, file, parent)
    for child in discoverChildren(node, loc):
        discoverLocation (child, parent=node)



isDir  = lambda p: p and path.isdir(p)  and os.access(p,os.R_OK)
isFile = lambda p: p and path.isfile(p) and os.access(p,os.R_OK)

nameID = lambda p: p and path.splitext(path.basename(p))[0]


########################
### Discovery strategies

def expandRoot (loc):
    ''' expand a relative path
        starting with the TREE_ROOT token.
        @return: absolute path based on the global startdir
    '''
    global startdir
    if (loc and loc.startswith(TREE_ROOT)):
        return path.join(startdir, stripPrefix(loc,TREE_ROOT))
    else:
        return loc


def relativeRoot (loc):
    global startdir
    if not loc: return TREE_ROOT
    if loc.startswith(TREE_ROOT): return loc
    if not path.isabs(loc):
        loc = path.join(startdir, loc)
    
    loc = path.normpath(loc)
    if not loc.startswith(startdir):
        __err('unable to establish page URL: '
             +'Location %s is outside webroot %s' %(loc, startdir))
    loc = stripPrefix(loc, startdir)
    return path.join (TREE_ROOT, loc)


def stripPrefix (loc,prefix):
    if loc and loc.startswith(prefix):
        loc = loc[len(prefix):]
        if loc.startswith('/'):
            loc = loc[1:]
    return loc


def navigateRelative(location, subPath):
    if path.isabs(subPath) or subPath.startswith(TREE_ROOT):
        return expandRoot(subPath)    # use subPath as absolute path
    else:
        fullpath = expandRoot(location)
        if not isDir(fullpath):
            location = path.dirname(location)
        return path.join(location, subPath)


def findSource (loc):
    ''' strategy to find a relevant source file
        corresponding to the given path location
    '''
    return buildFilename (loc, SRC_FILE_EXT)


def buildFilename (loc, FILE_EXT):
    ''' strategy to build acceptable file names,
        especially handling directory index files
        @param loc: filename or location to search 
        @param FILE_EXT: extension to require on files 
        @return: an existing file or index file
    '''
    loc = expandRoot(loc)
    if isDir(loc):
        IndexFile = INDEX_NAME+FILE_EXT
        filePath = path.join(loc,IndexFile)  # try name/index.EXT
        if isFile(filePath):
            return filePath
        IndexFile = nameID(loc)+FILE_EXT
        filePath = path.join(loc,IndexFile)  # try name/name.EXT
        if isFile(filePath):
            return filePath
    (base,ext) = path.splitext(loc)
    filePath = base+FILE_EXT                 # try name.EXT
    if isFile(filePath):
        return filePath
    else:
        return None

    
def discoverChildren(node, currentLocation):
    ''' get possible child locations to scan.
        Decision where to search is delegated to the Node
    '''
    if not node: return []
    
    candidates = node.childDiscovery(currentLocation)
    uniqueChildren = set (candidates)
    return uniqueChildren;


def discoverChildrenRecursively(location):
    ''' strategy how to proceed from a given location
        to find possible child menu entries. Should
        return None to stop recursive descent '''
    target = expandRoot(location)
    if isDir(target):
        currentBase = nameID(location)
        for entry in os.listdir(target):
            eID = nameID(entry)
            if not eID          : continue      # skip hidden files
            if INDEX_NAME  ==eID: continue      # skip name/index.txt
            if currentBase ==eID: continue      # skip name/name.txt
            yield path.join(location,eID)
    elif isFile(target):
        yield path.join(path.dirname(target), nameID(target))


class DiscoveryRedirect:
    ''' functor object to be used for discovering children.
        like the simple #discoverChildrenRecursively strategy,
        it it will be attached to a node to be used when discovering
        possible sub-menu entries. But here this discovery can be
        parametised with relative paths to include and names to filter
    '''
    def __init__(self, includes=[], excludes=[], srcdirs=[]):
        filter = lambda loc: not nameID(loc) in excludes
        self.excludesFilter = filter
        self.includes = includes
        self.srcdirs = srcdirs
    
    def __call__(self, location):
        ''' when invoked to discover a start location,
            apply our includes and excludes relative to that
            location and yield an iterator for possible children
            - each of our srcdirs is appended in turn to the
              given location to form a new place to explore
            - the includes are immediately treated as chilrden
            - the bare name of all results found there
              are checked against our excludes
        '''
        if not (self.srcdirs or self.includes):
            self.srcdirs = ['.']
        prependLocation = lambda relPath: navigateRelative(location, relPath)
        buildSubIter = lambda loc: ifilter(self.excludesFilter, [loc])
        buildDirIter = lambda loc: ifilter(self.excludesFilter,
                                           discoverChildrenRecursively(loc))
        toRetrieve = map(prependLocation, self.srcdirs )
        toInclude = map(prependLocation, self.includes )
        sources = map(buildDirIter, toRetrieve) + map(buildSubIter, toInclude)
        return chain(*sources)
    
    
    def chain(self, includes=[], excludes=[], srcdirs=[]):
        prevFilter = self.excludesFilter
        newfilter = lambda loc: not nameID(loc) in excludes and prevFilter(loc)
        self.excludesFilter = newfilter
        self.includes += includes
        self.srcdirs  += srcdirs







def scanSource (location, srcFile, parentNode):
    ''' scan the Asciidoc source text
        for Menu specs embedded in comments.
        Translate these into Node invocations
        @note various Placement subclasses perform
              the actual parsing and manipulations. 
    '''
    if not (srcFile and location): return None
    if not parentNode:
        nodeID = TREE_ROOT     # first Node encountered is /root by definition
    else:
        nodeID = nameID(location)
        nodeID = path.join(parentNode.menuPath(), nodeID)
    node = Node(nodeID)
    
    assert isFile(srcFile)
    srcTxt = file(srcFile)
    title = findTitle(srcTxt)
    if title and node.hasNoLabel():
        node.label = title
    for spec in extractMenuSpecs(srcTxt):
        ok = Placement.maybeParse (node, spec)
        if not ok:
            __warn('Source "%s":\t ignoring MENU: %s' % (nodeID, spec))
    
    # establish webpage path
    webpage = buildFilename(location, WEBPAGE_EXT)
    if not webpage:
        __warn('unable to resolve(%s): %s' % (WEBPAGE_EXT,location))
        webpage = srcFile
    
    # attach to menu if enabled
    node.establishPosition (parentNode, webpage)
    node.preprocess()
    return node


def findTitle(srcFile):
    assert not srcFile.closed
    srcFile.seek(0)
    title = None
    for line in srcFile:
        if not title:
            title = line.strip()      # search first nonempty line
        elif line.startswith('=' * (len(title)-1)):
            return title              # immediately followed by '====....'
        else:
            return None               # everything else is invalid


def extractMenuSpecs(srcFile):
    assert not srcFile.closed
    srcFile.seek(0)
    for line in srcFile:
        match = menuSpec_RE.match (line)
        if (match):
            yield line[match.end():].strip()






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
        if not id:
            return None # neutral
        if isinstance(id, Node):
            return id   # idempotent
        
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
            self.label = titleFormatted(self.id)
            self.parents = []
            self.children = []
            self.placements = []
            self._active = True
            self._relativePath = None
            self._childDiscoveryStrategy = discoverChildrenRecursively
        self.__dict__.update(args)
    
    def _isInit(self):
        return 'id' in self.__dict__
    
    def __str__(self):
        return 'Node(%s)' % self.id
    
    def enabled(self, yes=True):
        self._active = yes
    
    def hasNoLabel(self):
        return self.label == titleFormatted(self.id)
    
    
    def __iter__(self):
        ''' this is the main access interface:
            after the menu tree has been populated,
            the output generation will walk the tree
            by visiting each node and recursing down.
            @note invoking this method for the first time
                  will execute any stored placement and
                  processing constraints and specifications,
                  thus bringing the menu into final shape
        '''
        self._applyPlacementSpecs()
        return self.children.__iter__()
    
    def hasChildren(self): 
        self._applyPlacementSpecs()
        return 0 < len(self.children)
    
    def preprocess(self):
        ''' used by directory discovery / file parsing '''
        for placementSpec in self.placements:
            placementSpec.preprocess (self)
    
    def _applyPlacementSpecs(self):
        for placementSpec in self.placements:
            placementSpec.execute (self)
        self.placements = []
    
    
    def __getattr__(self, methodID):
        ''' enable DSL-style use of Node instances.
            When invoking an unknown method, we'll try
            all currently registered Placement spec handlers.
            The first one able to handle that method will create
            a Placement/Postprocessing entry, which will be stored
            to be applied later, before fetching the children.
        '''
        return Placement.maybeInvoke (self, methodID)
    
    
    
    def linkChild (self, childId):
        if not self._active: return None
        child = Node(childId)
        if child and not child in self.children:
            self.children.append(child)
            child.linkParent(self)
        return child
    
    def linkParent (self, parentId):
        if not self._active: return None
        parent = Node(parentId)
        if parent and not parent in self.parents:
            self.parents.append(parent)
            parent.linkChild(self)
        return parent
    
    def detach(self):
        ''' detach node from menu tree '''
        for parent in self.parents:
            parent.children.remove(self)
        self.parents = []
        self.children = []
        self._active = False
    
    
    def establishPosition (self,parent,webpage):
        ''' define the primary coordinates of this menu entry.
            @param parent: primary parent, defining the menuPath 
            @param webpage: used to form the in-tree URL
        '''
        parent = self.linkParent(parent)
        if parent:
            self.parents.remove (parent)
            self.parents.insert(0, parent)
        if webpage:
            self._relativePath = relativeRoot(webpage)
    
    
    def matches (self, nodeKey):
        ''' decide if this node is equivalent to the given nodeKey.
            That is, either the key is our own ID, or it matches some
            postfix part of our location in the menu and ends with our ID.
            Typically this is used to retrieve an existing node by symbolic ID,
            using the Node('id') constructor notation
        '''
        if not nodeKey: return False
        if self.id == nodeKey: return True
        mPath = self.menuPath()
        return ('/' in nodeKey
               and (   mPath.endswith(nodeKey))
                    or nodeKey.endswith(mPath))
    
    
    def childDiscovery(self, location):
        ''' @param location: current starting point to find children 
            @return iterator yielding possible child nodes to discover,
        '''
        return self._childDiscoveryStrategy (location)
    
    
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
        return self.url or normaliseLocalURL(self._relativePath or self.menuPath())
    
    
    def getParentUrl(self):
        if self.parents:
            return self.parents[0].getUrl()
        else:
            return ''




### Helpers for ID / URL handling

def normaliseComponentId(id):
    return nameID(id)
        

def normaliseLocalURL(url):
    if url.startswith(TREE_ROOT):
        url = url[4:]
    if not url.startswith('/'):
        url = '/'+url
    return url


def titleFormatted(nameID):
    nameID = nameID.strip()
    if nameID.islower():
        nameID = nameID.capitalize()
    else:
        # break 'CamelCase' words apart
        nameID = camelCase_RE.sub(r'\1 \2', nameID)
    return nameID

camelCase_RE = re.compile(r'([a-z])([A-Z])')




##################### Attachment Control #########################

class Placement(object):
    ''' baseclass for specifications
        to control if and how some entries are
        to be included into the generated menu tree.
        Concrete Placement subclasses are (post)processing Instructions
        They are either picked up by parsing a textual spec in Asciidoc page,
        or by invoking a suitable method on an existing Node instance, e.g.
        in the #addPredefined() function (internal DSL style).
        Each Node instance may collect a list of individual Placement specs.
        Typical examples being to sort the children, place an entry at a
        specific point, or disable recursion or menu generation alltogether.
    '''
    handlers = [] # List of all usable kinds of Placement specs (Subclasses)
    
    def preprocess(self,node): pass
    def execute(self, node):                   __err("abstract") # make this placement spec effective on the given node 
    def acceptVerb(self, methodID, *arg,**kw): __err("abstract") # try to accept a method invocation to yield a placement
    def acceptDSL(self, specificationTextLine):__err("abstract") # try to accept a textual spec from a file to be parsed
    
    @staticmethod
    def maybeParse (targetNode, specification):
        ''' try to find a suitable Placement subclass (handler),
            which is able to accept the given DSL text line
        '''
        for handler in Placement.handlers:
            try:
                placement = apply(handler).acceptDSL(specification)
                if placement:
                    targetNode.placements.append(placement)
                    return targetNode
            except:
                print_warning("»%s« (%s)" % (sys.exc_type,sys.exc_value))
        return None
    
    @staticmethod
    def maybeInvoke (targetNode, methodID):
        ''' @return functor to build the first suitable Placement subclass (handler),
            which is able to process the given method call with the concrete arguments
        '''
        def tryVerb (*arg,**kw):
            for handler in Placement.handlers:
                try:
                    placement = apply(handler).acceptVerb(methodID, *arg,**kw)
                    if placement:
                        targetNode.placements.append(placement)
                        return targetNode
                except: pass
            print_warning('DSL-method "%s" not applicable for %s' % (methodID,targetNode))
            return None
        
        return tryVerb




class PlaceChildAfter(Placement):
    ''' concrete child placement specification,
        denoting that a given child has to be placed at a
        specific point in the list of the child nodes (sub menu entries)
        of the menu entry currently in question. The position is given
        by mentioning another child, after which to place the entry.
        Alternatively, this placement spec may also be used to put
        a child node at the start of the list
    '''
    
    def __init__(self):
        self.refPoint = None
        self.childToPlace = None
    
    def __repr__(self):
        return '|child %s after %s|' % (self.childToPlace,self.refPoint)
    
    def execute(self, node):
        ''' This placement expresses an child ordering constraint
            Do what needs to be done to the children of the given node,
            in order to fulfill this constraint.
            @note no ref point -> prepend child
            @note ref point not found -> append child
        '''
        assert node
        assert self.childToPlace
        node.linkChild(self.childToPlace)
        node.children.remove (self.childToPlace)
        if not self.refPoint:
            insertPoint = 0
        elif self.refPoint and self.refPoint in node.children:
            insertPoint = 1 + node.children.index (self.refPoint)
        else:
            insertPoint = len(node.children)
        node.children.insert (insertPoint, self.childToPlace)
    
    
    def acceptVerb(self, methodID, child, refPoint=None):
        ''' when invoked as DSL method with the given parameters,
            try to configure this child placement constraint such
            as to reflect the given placement wish
            @return this constraint or None, if the DSL method name
                    or the concrete parameters are not suitable
        '''
        if 'putChildAfter' == methodID and child:
            self.childToPlace = Node(child)
            self.refPoint = Node(refPoint)
            return self
        if ('putChildFirst' == methodID or
            'prependChild' == methodID ) and child:
            self.childToPlace = Node(child)
            self.refPoint = None
            return self
        if ('putChildLast' == methodID or
            'appendChild' == methodID ) and child:
            self.childToPlace = Node(child)
            self.refPoint = 'atEnd'
            return self
        else:
            return None
    
    
    def acceptDSL(self, specificationTextLine):
        ''' try to parse the spec into a child ordering constraint.
            @return this constraint, suitably configured, or None
        '''
        match = childAfter_RE.search (specificationTextLine)
        if (match):
            self.childToPlace = Node (match.group(3))
            self.refPoint     = Node (match.group(4))
            assert self.childToPlace
            return self
        match = childPrepend_RE.search (specificationTextLine)
        if (match):
            self.childToPlace = Node (match.group(1))
            self.refPoint     = None
            assert self.childToPlace
            return self
        match = childAppend_RE.search (specificationTextLine)
        if (match):
            self.childToPlace = Node (match.group(3))
            self.refPoint     = 'atEnd'
            assert self.childToPlace
            return self
        else:
            return None


# DSL Parsing...
quote_ = r'[\'\"]?'
s__    = r'\s*' 
nodeID_= s__+quote_ + r'(\w[\w\/\-\.]*)' + quote_+s__

attach_child_after_ = r'((attach|put)\s+)?child'+nodeID_+r'after'+nodeID_
prepend_child_      = r'prepend(?:\s+child)?'+nodeID_
append_child_       = r'((append|attach)\s+)?child\s+'+nodeID_

childAfter_RE   = re.compile (attach_child_after_, re.IGNORECASE)
childAppend_RE  = re.compile (append_child_,       re.IGNORECASE)
childPrepend_RE = re.compile (prepend_child_,      re.IGNORECASE)






class AttachExternalLink(Placement):
    
    def __init__(self):
        self.subID = None
        self.label = None
        self.url = None
    
    def __repr__(self):
        return '|attach "%s" -->%s|' % (self.subID,self.url)
    
    def execute(self, node):
        assert node
        assert self.url
        if not self.subID:
            self.subID = nameID(self.url)
        if not self.label:
            self.label = titleFormatted(self.subID)
        
        nodeID = path.join(node.id, self.subID)
        newNode = Node(nodeID, label=self.label, url=self.url)
        node.linkChild(newNode)
    
    
    def acceptVerb(self, methodID, url, id=None, label=None):
        if methodID in ['link', 'attachLink']:
            self.url = url
            self.subID = id
            self.label = label
            return self
        else:
            return None
    
    
    def acceptDSL(self, specificationTextLine):
        match = externalLink_RE.search (specificationTextLine)
        if (match):
            self.subID = match.group(1)
            self.url   = match.group(2)
            self.label = match.group(3)
            return self
        else:
            return None



nodeSpec_       = r'(?:(?:child|node)'+nodeID_+')?'               # optional nodeID in group(1)
urlSpec_        = r'([^\s\[]+)'                                   # mandatory url/path in group(2)
labelSpec_      = r'\[([^\]]*)\]'                                 # label in [] as group(3) 
externalLink_   = nodeSpec_+'link:'+urlSpec_+labelSpec_+r'\s*$'   # id-url-label + only whitespace to line end

externalLink_RE = re.compile (externalLink_, re.IGNORECASE)






class DefineLabel(Placement):
    
    def __init__(self):
        self.label = None
        
    def __repr__(self):
        return '|pageLabel "%s"|' % self.label
    
    def preprocess(self,node):
        assert node
        if self.label: node.label = self.label
        
    def execute(self, node): pass
    def acceptVerb(self, _): return None
    
    def acceptDSL(self, specificationTextLine):
        match = labelSpec_RE.search (specificationTextLine)
        if (match):
            self.label = match.group(1).strip()
            return self
        else:
            return None


labelSpec_   = r'(?:label|title)\s+(.+)'
labelSpec_RE = re.compile (labelSpec_, re.IGNORECASE)






class SortChildren(Placement):
    
    def __init__(self):
        self.ascending = True
    
    def __repr__(self):
        return '|sort children|'
    
    def execute(self, node):
        ''' when applied, sort the child nodes alphabetically
        '''
        assert node
        node.children.sort(key = lambda child: child.label.lower(), reverse = not self.ascending)
    
    
    def acceptVerb(self, methodID, reverse=False):
        if 'sortChildren' == methodID:
            self.ascending = not reverse
            return self
        else:
            return None
    
    
    def acceptDSL(self, specificationTextLine):
        match = sortChildren_RE.search (specificationTextLine)
        if (match):
            direction = match.group(1) or ''
            direction = direction.lower()
            if direction.startswith('desc') or direction.startswith('reverse'):
                self.ascending = False
            return self
        else:
            return None


sortChildren_   = r'sort(?:\s+children)?(?:\s+(ascending|asc|descending|desc|reverse|reversed))?'
sortChildren_RE = re.compile (sortChildren_, re.IGNORECASE)





class EnableEntry(Placement):
    
    def __init__(self):
        self.on = None
        self.detach = False
    
    def __repr__(self):
        return '|enable=%s detach=%s|' % (self.on,self.detach)
    
    def preprocess(self, node):
        assert node
        node.enabled(self.on)
        if self.detach: node.detach()
    
    def execute(self, node):
        assert node
        assert None != self.on
        if self.detach: node.detach()
    
    
    def acceptVerb(self, methodID):
        if 'detach' == methodID:
            self.on = False
            self.detach = True
            return self
        # note: class Node has an 'enabled' method, which can be invoked directly.
        #       But this method only toggles activity, but doesn't detach
        else:
            return None
    
    
    def acceptDSL(self, specificationTextLine):
        match = activateNode_RE.search (specificationTextLine)
        if (match):
            if match.group(1):
                self.on = True
                self.detach = False
            else:
                self.on = False
            return self
        match = detachNode_RE.search (specificationTextLine)
        if (match):
            self.on = False
            self.detach = True
        else:
            return None


activateNode_   = r'(on|active|activate)|(off|disable|deactivate)'
detachNode_     = r'detach'

activateNode_RE = re.compile (activateNode_, re.IGNORECASE)
detachNode_RE   = re.compile (detachNode_,   re.IGNORECASE)





class RedirectDiscovery(Placement):
    ''' this placement spec instructs the node
        to serach for children at specific places ('includes')
        or to exclude some of the discovered children
    '''
    
    def __init__(self):
        self.includes = []
        self.excludes = []
        self.srcdirs  = []
    
    def __repr__(self):
        return '|discover %s (+%s) without %s|' % (self.includes,self.srcdirs,self.excludes)
    
    def preprocess(self, node):
        strategy = node._childDiscoveryStrategy
        if isinstance(strategy, DiscoveryRedirect):
            strategy.chain(self.includes, self.excludes, self.srcdirs)
        else:
            strategy = DiscoveryRedirect(self.includes, self.excludes, self.srcdirs)
        node._childDiscoveryStrategy = strategy 
    
    def execute(self, node): pass
    
    
    def acceptVerb(self, methodID, includes=[], excludes=[], srcdirs=[]):
        if 'discover' == methodID:
            self.includes=includes
            self.excludes=excludes
            self.srcdirs =srcdirs
            return self
        else:
            return None
    
    
    def acceptDSL(self, specificationTextLine):
        match = pullSrcDirSpec_RE.search (specificationTextLine)
        if (match):
            tokens = match.group(1)
            tokens = listSplitter_RE.split(tokens)
            self.srcdirs += tokens
        else:
            for match in discoverySpec_RE.finditer (specificationTextLine):
                tokens = match.group(2)
                tokens = listSplitter_RE.split(tokens)
                if 'include' == match.group(1):
                    self.includes += tokens
                else:
                    self.excludes += tokens
        
        if self.includes or self.excludes or self.srcdirs:
            return self  # successfully parsed
        else:
            return None  # fail, maybe try other Placement spec


pathToken_       = r'[\w\./]+'
listDelim_       = r'\s*,\s*'
tokenList_       = '('+pathToken_+'(?:'+listDelim_+pathToken_+')*)'
discoverySpec_   = r'(include|exclude)\s+'+tokenList_
pullSrcDirSpec_  = r'include\s*dirs?\s+'+tokenList_

discoverySpec_RE = re.compile (discoverySpec_, re.IGNORECASE)
pullSrcDirSpec_RE= re.compile (pullSrcDirSpec_, re.IGNORECASE)
listSplitter_RE  = re.compile (listDelim_)





### Define all usable Placement kinds:
Placement.handlers += [AttachExternalLink
                      ,RedirectDiscovery
                      ,PlaceChildAfter
                      ,SortChildren
                      ,DefineLabel
                      ,EnableEntry
                      ]



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
    if not subTree.hasChildren():
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
    
def __exerr(text):
    __err(text + ": »%s« (%s)" % (sys.exc_type,sys.exc_value))

def __warn(text):
    print >> sys.stderr, "--WARNING--   " + str(text)

print_warning = __warn



if __name__ == "__main__":
    parseAndDo()
