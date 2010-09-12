var m = [];
var opened = [];

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
