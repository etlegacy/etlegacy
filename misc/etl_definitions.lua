---@meta legacy

---@class et
et = {}

function et.RegisterModname(modname) end

function et.FindSelf() end

function et.FindMod(vmnumber) end

function et.IPCSend(vmnumber, message) end

function et.G_Print(message) end

function et.G_LogPrint(message) end

function et.ConcatArgs(start) end

function et.trap_Argc() end

function et.trap_Argv(index) end

function et.trap_Cvar_Get(name) end

function et.trap_Cvar_Set(name, value) end

function et.trap_GetConfigstring(cs_index) end

function et.trap_SetConfigstring(cs_index, string) end

function et.trap_SendConsoleCommand(exec_when, text) end

function et.trap_SendServerCommand(clientnum, text) end

function et.trap_DropClient(clientnum, reason, ban_time) end

function et.ClientNumberFromString(string) end

function et.G_Say(clientnum, mode, text) end

function et.MutePlayer(clientnum, duration, reason) end

function et.UnmutePlayer(clientnum) end

function et.trap_GetUserinfo(clientnum) end

function et.trap_SetUserinfo(clientnum, string) end

function et.ClientUserinfoChanged(clientnum) end

function et.Info_RemoveKey(infostring, key) end

function et.Info_SetValueForKey(infostring, key, value) end

function et.Info_ValueForKey(infostring, key) end

function et.Q_CleanStr(string) end

function et.trap_FS_FOpenFile(filename, mode) end

function et.trap_FS_Read(file, count) end

function et.trap_FS_Write(buffer, count, file) end

function et.trap_FS_Rename(oldname, newname) end

function et.trap_FS_FCloseFile(file) end

function et.trap_FS_GetFileList(path, extension) end

function et.G_SoundIndex(soundfile) end

function et.G_ModelIndex(modelname) end

function et.G_globalSound(soundindex) end

function et.G_Sound(entnum, soundindex) end

function et.G_ClientSound(clientnum, soundindex) end

function et.trap_Milliseconds() end

function et.isBitSet(value, bit) end

function et.G_Damage(target, inflictor, attacker, damage, dflags, mod) end

function et.G_AddSkillPoints(clientnum, points) end

function et.G_LoseSkillPoints(clientnum, points) end

function et.G_XP_Set(clientnum, xp) end

function et.G_ResetXP(clientnum) end

function et.AddWeaponToPlayer(clientnum, weapon) end

function et.RemoveWeaponFromPlayer(clientnum, weapon) end

function et.GetCurrentWeapon(clientnum) end

function et.G_CreateEntity() end

function et.G_DeleteEntity(entity) end

function et.G_TempEntity(origin, event) end

function et.G_FreeEntity(entity) end

function et.G_EntitiesFree() end

function et.G_SetEntState(entity, state) end

function et.trap_LinkEntity(entity) end

function et.trap_UnlinkEntity(entity) end

function et.G_GetSpawnVar(entity, key) end

function et.G_SetSpawnVar(entity, key, value) end

---Get entity field value
---@param entitynum number
---@param fieldname string
---@param array_index? number field array index
---@return string|integer|table
function et.gentity_get(entitynum, fieldname, array_index) end

---Set entity field value
---@param entitynum number
---@param fieldname string
---@param array_index? number field array index
---@param value string|integer|table
function et.gentity_set(entitynum, fieldname, array_index, value) end

function et.G_AddEvent(entity, event, eventParm) end

function et.G_ShaderRemap(old_shader, new_shader) end

function et.G_ResetRemappedShaders() end

function et.G_ShaderRemapFlush() end

---Set global fog parameters
---@param params string
function et.G_SetGlobalFog(params) end

function et.trap_Trace(trace, start, mins, maxs, endpos, entitynum, mask) end


--- Callbacks
--- LuaLS not super clear on how the callbacks should be defined for the parameters to be documented correctly
--- https://github.com/LuaLS/lua-language-server/issues/477

---Called when the game is initialized
---@param leveltime number
---@param randomSeed number
---@param restart boolean
---@type fun(leveltime: number, randomSeed: number, restart: boolean)
et_InitGame = nil

---Called when the game is shut down
et_RunFrame = nil
---@param leveltime number Current level time
function et_RunFrame(leveltime) end
