function defvar(var, value)
    -- if type(value) == "string" then
    --     defines { var .. '="' .. value .. '"' }
    -- else
        defines { var .. '=' .. value }
    -- end
end

function fullname(str)
    project().fullname = str;
end

function shortname(str)
    project().shortname = str;
end

function author(str)
    project().author = str;
end

function version(str)
    project().version = str;
end

function channel(str)
    project().channel = str;
end

function icon(str)
    if project().icon == nil then
        project().icon = ""
    end
    for k,v in ipairs(configuration().keywords) do
        if os.is(v) then
            project().icon = str;
            return
        end
    end
end

function if_valid(value, if_false)
  if value ~= nil then return value else return if_false end
end

function  res_win32(info)
    local template = [[
#define IDI_ICON1 101
#include "windows.h"
IDI_ICON1       ICON            "{icon}"
LANGUAGE        LANG_ENGLISH,   SUBLANG_ENGLISH_US
VS_VERSION_INFO VERSIONINFO
FILEVERSION     {cversion}
PRODUCTVERSION  {cversion}
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0x0L
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_APP
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "000904b0"
        BEGIN
            VALUE "FileVersion",        "{version}"
            VALUE "ProductVersion",     "{version}"
            VALUE "InternalName",       "{name}.exe"
            VALUE "OriginalFilename",   "{name}.exe"
            VALUE "ProductName",        "{fullname}"
            VALUE "FileDescription",    "{fullname} {version} {channel}"
            VALUE "LegalCopyright",     "Copyright (C) {year} {author}"
            VALUE "CompanyName",        "{author}"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x9, 1200
    END
END
]]
    local path_sln = path.getrelative(".", solution().location)
    local path_icon = path.translate(path.getrelative(path_sln, path.join(info.pathname, info.icon)))
    local path_rc   = path.join(path_sln, info.filename..".rc")
    local cver = info.version:gsub("%.", ",")
    path_icon = path_icon:gsub("\\", "\\\\")

    template = template:gsub("{name}",      info.name)
    template = template:gsub("{fullname}",  info.fullname)
    template = template:gsub("{shortname}", info.shortname)
    template = template:gsub("{author}",    info.author)
    template = template:gsub("{version}",   info.version)
    template = template:gsub("{channel}",   info.channel)
    template = template:gsub("{cversion}",  cver)
    template = template:gsub("{icon}",      path_icon)
    template = template:gsub("{year}",      os.date("%Y"))

    local f = assert(io.open(path_rc, "w"))
    f:write(template)
    f:close()
    print("Generating "..path_rc)
    configuration {}
        files { path_rc }
end

function resourcefile(filename)
    if os.is("windows") then
        local info = {}
        info.pathname = ''
        info.filename = filename
        info.name = project().name
        info.fullname = if_valid(project().fullname, name)
        info.shortname = if_valid(project().shortname, name)
        info.author = if_valid(project().author, "")
        info.version = if_valid(project().version, "")
        info.channel = if_valid(project().channel, "")
        info.icon = project().icon
        if icon ~= nil then
            res_win32(info)
        end
        defvar("BUILD_FULLNAME",  info.fullname);
        defvar("BUILD_SHORTNAME", info.shortname);
        defvar("BUILD_AUTHOR",    info.author);
        defvar("BUILD_VERSION",   info.version);
        defvar("BUILD_CHANNEL",   info.channel);
    end
end
