#!/usr/bin/env lua

require "lfs"

-- CONFIG SECTION

-- called with a filename
-- return the 'name' of a interesting file or nil
function file_filter(name)
   return name:match("(.*)%.txt$")
end

-- called with a 'entry' table of the metadata for a file
-- returns non nil when this file is identified as a index for the current dir
function index_file(entry)
   return entry.name:match("index") or string.lower(entry.parent.name or "") == string.lower(entry.name)
end


-- pattern to identify in-file metadata, that is  //MENUkey: value
meta_pattern = "^//(MENU.*): (.*)$"

-- Metas are:
--  MENU: on|off
--  MENUTITLE: Presentation Title text
--  MENUSORT: sorting order definition, see below
--  MENUCHILDOF: make this child of menu entry, can appear multiple times
--



-- define defaults
defaults = {
   -- MENU is on for index files and off for normal files
   MENU =
      function (entry)
         if entry.type == "index" then
            return {"on"}
         elseif entry.type == "file" then
            return {"off"}
         end
      end,
   -- title is the asciidoc title if present or the capitalized filename
   MENUTITLE =
      function (entry)
         local h = entry.secondline:match("(=+)")
         if h and math.abs(entry.headline:len() - h:len()) <= 4 then
            return {entry.headline}
         end
         return {entry.name:sub(1,1):upper()..entry.name:sub(2)}
      end,
   -- sorting key is the filename, except for index entries where it is "........" (do we allow multiple index files per dir?)
   MENUSORT =
      function (entry)
         if entry.type == "index" then
            return {"........."}
         end
         return {entry.name:lower()}
      end,
   -- by default each entry is the child of its parent
   MENUCHILDOF =
      function (entry)
         -- TODO index pages are childof parent.parent
         return {(entry.parent.fullname or ""):match("[^/]*/(.*)")}
      end
}


-- END OF CONFIG SECTION


-- State Storage
menu_state = {entries = {}}

function recdump_table(table, prefix)
   for name, entry in pairs(table) do
      if type(entry) == "table" and name ~= "parent" and name ~= "index" then
         if next(entry) ~= nil then
            print (prefix..name..":")
            recdump_table(entry, prefix.." ")
         end
      else
         print (prefix..name.." = "..tostring(entry))
      end
   end
end


function discover_dir(dir, state)
   for file in lfs.dir(dir) do
      if file ~= "." and file ~= ".." then

         local fullname = dir.."/"..file
         local attr = lfs.symlinkattributes(fullname)

         if attr then
            local name
            if attr.mode == "directory"  then
               name = file
            else
               name = file_filter(file)
            end

            if name then

               if attr.mode == "directory" then
                  -- recurse into subdirs
                  state.entries[name] = {
                     fullname = fullname,
                     name = name,
                     type = "directory",
                     parent = state,
                     meta = {},
                     entries = {}
                  }
                  discover_dir(fullname, state.entries[name])
               elseif file_filter(fullname) then
                  state.entries[name] = {
                     fullname = fullname,
                     name = name,
                     parent = state,
                     meta = {},
                     entries = {}
                  }
                  state.entries[name].type = index_file(state.entries[name]) and "index" or "file"
               end
            end
         end
      end
   end
end


-- collect directory structure, subdirs and interesting files
discover_dir(".", menu_state)

function process_table(table, apply)
   for name, entry in pairs(table.entries) do
      if entry.type == "directory" then
         -- TODO with type apply (table, name, entry)
         process_table(entry, apply)
      else
         assert (type (entry) == "table")
         apply (entry)
      end
   end
end


-- parse file contents for metadata
process_table(menu_state,
              function (entry)
                 local header, header2
                 for line in io.lines(entry.fullname) do
                    if not header then
                       header = line
                       entry.headline = line
                    elseif not header2 then
                       -- the second line must be '=====..' for a valid asciidoc file header
                       header2 = line
                       entry.secondline = line
                    end

                    local key, value = line:match(meta_pattern)
                    if key then
                       if not entry.meta[key] then entry.meta[key] = {} end
                       table.insert(entry.meta[key], value)
                    end
                 end
              end
           )


-- menus for index pages apply to the 'parent' dir entry
process_table(menu_state,
              function (entry)
                 if entry.type == "index" then
                    entry.parent.meta = entry.meta
--                    entry.meta = {}
                    entry.parent.index = entry
                 end
              end
           )

-- fill in defaults where they are missing
process_table(menu_state,
              function (entry)
                 for key,value in pairs(defaults) do
                    if not entry.meta[key] then
                       if type(value) == "function" then
                          entry.meta[key] = value (entry)
                       else
                          entry.meta[key] = value
                       end
                    end
                 end
              end
           )

-- prepare menu building
reverse_index = {entries={}}
process_table(menu_state,
              function (entry)
                 local n = file_filter(entry.fullname):lower()
                 local t = {}

                 for s in n:gmatch("/+([^/]*)") do
                    table.insert(t,s)
                 end

                 local r = reverse_index
                 for i = #t,1,-1 do
                    if not r.entries[t[i] ] then
                       r.entries[t[i] ] = {entries={}}
                    end
                    r=r.entries[t[i] ]
                 end

                 r.ref = entry
              end
           )


-- create menu structure
the_menu = {entries = {}}
process_table(menu_state,
              function (entry)
                 if entry.meta.MENU[1] == "on" then
                    print ("menu for", entry.fullname, entry.meta.MENUTITLE[1], entry.meta.MENUCHILDOF[1])
                 end
              end
           )


--[[

-- sort it




--
-- documents have MENUCHILDOF: where/to/place/this
--
-- this is anchored from the back
--
--
--
--
--
-- reverse index in die entries einbaun
--
--
--
--
--
--
--
--
--
--
--
--




-- pretty print in 'MENUSORT' order

--]]


-- recdump_table(reverse_index, "")
recdump_table(menu_state, "state: ")
recdump_table(the_menu, "menu: ")


