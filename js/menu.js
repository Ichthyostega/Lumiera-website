var m = [];
var opened = [];

function klickmi() {
  url = document.forms['trostForm'].testUrl.value
  
	d = document;
	f = d.frames ? d.frames['inavi'] : d.getElementById('inavi');
	mf = f.contentWindow 
	
	if (mf){
		mf.menuTable.select(url)
	}
}

function testMe(text) {
	alert(text)
	mu = document.getElementById('menu')
	if (mu) {
		mu.style.display = 'none'
	}
}

function getMenuRoot()
  {
    return document.getElementById('menu')
  }

NOP = function() { }

function addCSSClass (elm, classID)
  {
    clzz = elm.className
//  alert("addCSSClass: clzz="+clzz+'|| indexOf='+clzz.indexOf(classID))
    if (-1 == clzz.indexOf(classID))
      {
//      alert("inne")
        clzz += ' '+classID
        elm.className = clzz
      }
//  alert("addCSSClass: clzz="+clzz)
  }

function removeCSSClass (elm, classID)
  {
    clzz = elm.className
//  alert("removeCSS clzz="+clzz+"||")
    if (clzz && -1 < clzz.indexOf (classID))
      {
        clzz = clzz.replace(classID,'')
//      alert("drinne clzz="+clzz)
        elm.className = clzz
      }
  }

function expand (elm)
  {
//  alert("expand("+elm.id+")")
    elm.style.display = 'block'
  }

function collapse (elm)
  {
//  alert("collapse("+elm.id+")")
    elm.style.display = 'none'
  }


function MenuNode(id, parent, isSubmenu)
  {
    this.elm = document.getElementById(id)
    this.isSubmenu = isSubmenu
    this.parent = parent
    
    this.markActive = function()
      {
        this.expand()
        addCSSClass (this.elm, 'current')
      }
    
    this.unmark = function()
      {
        removeCSSClass (this.elm, 'current')
        this.collapse()
      }
    
    this.expand = function()
      {
        if (this.parent)
            this.parent.expand()
        expand(this.elm)
      }
    
    this.collapse = function()
      {
        if (this.isSubmenu)
          collapse(this.elm)
        if (this.parent)
          this.parent.collapse()
      }
    
    if (!this.elm)
      {
        this.markActive = NOP
        this.unmark     = NOP
        this.expand     = NOP
        this.collapse   = NOP
      }
  }


var menuTable = {}

menuTable.index = { }
menuTable.current = [ ]

menuTable.addNode = function(id,url,parent, isSubmenu) 
  {
    parentEntry = this.index[parent]
    node = new MenuNode(id,parentEntry, isSubmenu)
    this.index[url] = node
    this.current.push(node) // new nodes are marked as "current",
  }                        //  causing them to be collapsed on next selection

menuTable.select = function(url)
  {
//  alert("select "+url)
    element = this.index[url]
    if (element)
      {
        while (0 < this.current.length) {
          old = this.current.pop()
          old.unmark()
        }
        this.current.push (element)
        element.markActive()
      }
  }

/*
function hide_submenus() {
  m = document.getElementById("menu").children;

  a = [];
  b = getCookie("opened");
  if (b)
    a = eval(b);

  for (var i=0; i < m.length; i++) {

    if (m[i].lastElementChild.nodeName == 'UL') {
      if (findIndex(a, i) !== false)
      {
        opened.push(m[i]);
        m[i].lastElementChild.style.display = "block";
      }
      else
        m[i].lastElementChild.style.display = "none";

      m[i].className = 'openable';
      span = document.createElement("span");
      span.innerHTML = "+";
      span.onclick = open_submenu;
      m[i].insertBefore(span, m[i].children[1]);
    }
  }
}

function open_submenu(a){
  if(this.nextElementSibling.style.display == "block")
  {
    remove(opened, this.parentNode);
    this.nextElementSibling.style.display = "none";
  }
  else
  {
    opened.push(this.parentNode);
    this.nextElementSibling.style.display = "block";
  }

  store_cookie();
}

function store_cookie() {
  a = [];
  for (var i=0; i < opened.length; i++)
    a.push(findIndex(m, opened[i]));

  str = "[" + a.join(",") + "]";

  document.cookie="opened=" + str;
}

remove = function(t, value) {
  for (var i=0; i < t.length; i++)
    if (t[i] == value)
      t.splice(i,1);
};

findIndex = function(t, value) {
  for (var i=0; i < t.length; i++)
    if (t[i] == value)
      return i;

   return false;
};

function getCookie(c_name) {

  if (document.cookie.length > 0)
  {
    c_start=document.cookie.indexOf(c_name + "=");
    if (c_start != -1)
    {
      c_start=c_start + c_name.length+1;
      c_end=document.cookie.indexOf(";", c_start);
      if (c_end == -1) c_end=document.cookie.length;
      return unescape(document.cookie.substring(c_start, c_end));
    }
  }
  return "";
}

window.onload = hide_submenus;
*/