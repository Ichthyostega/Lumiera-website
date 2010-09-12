#!/usr/bin/env luajit-2.0.0-beta5

require "lfs"

-- Config

interesting_file=".*%.txt$"
index_file = "index"  -- TODO match index or parent dir name

meta_pattern = "^//(MENU.*): (.*)$"

index_defaults = {
   MENU = {"on"},
--   MENUTITLE =,
--   MENUSORT = "%name%"
}

nonindex_defaults = {
   MENU = {"off"},
--   MENUTITLE = "%asciidoctitle%",
--   MENUSORT = "%name%"
}


-- State Storage
menu_state = {entries = {}}

-- Metas are:
--  MENU: on|off
--  MENUTITLE: Presentation Title text
--  MENUSORT: sorting word generatory
--  MENUCHILDOF: make this child of menu entry, can appear multiple times
--


function recdump_table(table, prefix)
   for name, entry in pairs(table) do
      if type(entry) == "table" and name ~= "parent" then
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
            local name = attr.mode == "directory" and file or file:match("(.*)"..interesting_file)

            if name then

               if attr.mode == "directory" then
                  -- recurse into subdirs
                  state.entries[name] = {
                     fullname = fullname,
                     name = name,
                     type = attr.mode,
                     parent = state,
                     entries = {}
                  }
                  discover_dir(fullname, state.entries[name])
               elseif fullname:match(interesting_file) ~= nil then
                  state.entries[name] = {
                     fullname = fullname,
                     name = name,
                     type = attr.mode,
                     parent = state
                  }
               end
            end
         end
      end
   end
end



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


function parse_single_file(entry)
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
         if not entry[key] then entry[key] = {} end
         table.insert(entry[key], value)
      end
   end
end


function set_defaults(entry)
   local defaults
   if entry.name == index_file then -- TODO interpolation and regex match
      defaults = index_defaults
   else
      defaults = nonindex_defaults
   end

   for key,value in pairs(defaults) do
      if not entry[key] then
         entry[key] = value
      end
   end
end


function up_index(entry)
   -- TODO
end

-- collect directory structure, subdirs and interesting files
discover_dir(".", menu_state)

-- parse file contents for metadata
process_table(menu_state, parse_single_file)

-- fill in defaults where they are missing
process_table(menu_state, set_defaults)

-- menus for index pages apply to the 'parent' dir entry
process_table(menu_state, up_index)


recdump_table(menu_state, "")


