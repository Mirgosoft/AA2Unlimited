local fileutils = require "poser.fileutils"
local signals = require "poser.signals"
local lists = require "poser.lists"
local toggles = require "poser.toggles"
local charamgr = require "poser.charamgr"

local function unhidemeshes(frame)
	local count = frame.m_nChildren
	for i = 0, count - 1, 1 do
		unhidemeshes(frame:m_children(i))
	end
	--frame.m_renderFlag = 0
	frame.m_meshFlagHide = 0
end

-- ####
-- Legs
-- ####

local legsdata = { -- legsdata[gender-id][legs-wear-title 1..n] == {socks-id 0..5 , shoes-id 0..4}
	{	-- male
		["S00_null_kutusita"] = {1, 0}, ["S00_null_kawa"] = {1, 2}, ["S00_null_uwabaki"] = {1, 1}, ["S00_null_shoes"] = {1, 3}, 
		["S00_null_AK"] = {0, 2}, ["S00_null_AU"] = {0, 1}, ["S00_null_AS"] = {0, 3}, ["S00_null_asi"] = {0, 0}
	},
	{	-- female
		["A00_null_KO"] = {1, 0}, ["A00_null_KH"] = {2, 0}, ["A00_null_KC"] = {4, 0}, ["A00_null_KR"] = {3, 0}, ["A00_null_KP"] = {5, 0},
		["A00_null_KOU"] = {1, 1}, ["A00_null_KOK"] = {1, 2}, ["A00_null_KOS"] = {1, 3}, 
		["A00_null_KHU"] = {2, 1}, ["A00_null_KHK"] = {2, 2}, ["A00_null_KHS"] = {2, 3}, 
		["A00_null_KCU"] = {4, 1}, ["A00_null_KCK"] = {4, 2}, ["A00_null_KCS"] = {4, 3}, 
		["A00_null_KRU"] = {3, 1}, ["A00_null_KRK"] = {3, 2}, ["A00_null_KRS"] = {3, 3}, 
		["A00_null_KPU"] = {5, 1}, ["A00_null_KPK"] = {5, 2}, ["A00_null_KPS"] = {5, 3},  ["A00_null_KPP"] = {5, 4},
		["A00_null_asi"] = {0, 0}, ["A00_null_AU"] = {0, 1}, ["A00_null_AK"] = {0, 2}, ["A00_null_AS"] = {0, 3}, 
		{0, 0}, {0, 0}, {0, 0}, {0, 0}
	}
}

local legslist = iup.list {
	expand = "yes",
	visiblelines = 8,
}

local populatelegs = function()
	local char = charamgr.currentcharacter()
	if not char then
		return
	end
	legslist[1] = nil
	if not char.struct.m_xxLegs then
		return
	end
	local legsbaseframe = char.struct.m_xxLegs:FindBone("A00_null_kutu") or char.struct.m_xxLegs:FindBone("S00_null_kutu")
	if not legsbaseframe then
		return
	end
	local legcount = legsbaseframe.m_nChildren
	for i = 0, legcount - 1, 1 do
		legslist[i + 1] = legsbaseframe:m_children(i).m_name
	end
end
charamgr.currentcharacterchanged.connect(populatelegs)

local selectlegs = function(index, legstitle)
	local char = charamgr.currentcharacter()
	if not char then
		return
	end
	if not char.struct.m_xxLegs then
		return
	end
	if GetIsClothesScreen() then -- if AAPlay Clothing Editor
		if not legsdata[char.struct.m_charData.m_gender+1][legstitle] then return end
		char.struct.m_charData:m_clothes(char.struct.m_currClothes).socks = legsdata[char.struct.m_charData.m_gender+1][legstitle][1]
		if peek_walk(GameBase + 0x3761D0, 0x40, 0xC08) ~= 9 then
			char.struct.m_charData:m_clothes(char.struct.m_currClothes).indoorShoes = legsdata[char.struct.m_charData.m_gender+1][legstitle][2]
		else
			char.struct.m_charData:m_clothes(char.struct.m_currClothes).outdoorShoes = legsdata[char.struct.m_charData.m_gender+1][legstitle][2]
		end
		-- Update the current clothing visual
		local character = charamgr.current
		if character then
			character:update(1)
		end
	end
	local legsbaseframe = char.struct.m_xxLegs:FindBone("A00_null_kutu") or char.struct.m_xxLegs:FindBone("S00_null_kutu")
	local legcount = legsbaseframe.m_nChildren
	for i = 1, legcount, 1 do
		local frame = legsbaseframe:m_children(i - 1)
		if i == index then
			unhidemeshes(frame)
			frame.m_renderFlag = 0
		else
			frame.m_renderFlag = 2
		end
	end
end

legslist.action = function(self, text, item, state)
	if state == 1 then
		selectlegs(item, text)
	end
end


-- #####
-- Skirt
-- #####

local skirtlist = iup.list {
	expand = "yes",
	visiblelines = 8,
}

local populateskirt = function()
	local char = charamgr.currentcharacter()
	if not char then
		return
	end
	skirtlist[1] = nil
	-- males misuse the m_xxSkirt member for something other than a xx file
	if char.struct.m_boobs == 0 or not char.struct.m_xxSkirt then
		return
	end
	local skirtbaseframe = char.struct.m_xxSkirt:FindBone("A00_null_sukato")
	if not skirtbaseframe then
		return
	end
	local legcount = skirtbaseframe.m_nChildren
	for i = 0, legcount - 1, 1 do
		skirtlist[i + 1] = skirtbaseframe:m_children(i).m_name
	end
end
charamgr.currentcharacterchanged.connect(populateskirt)

local selectskirt = function(index)
	local char = charamgr.currentcharacter()
	if not char then
		return
	end
	if not char.struct.m_xxSkirt then
		return
	end
	local skirtbaseframe = char.struct.m_xxSkirt:FindBone("A00_null_sukato")
	local legcount = skirtbaseframe.m_nChildren
	for i = 1, legcount, 1 do
		if i == index then
			local frame = skirtbaseframe:m_children(i - 1)
			unhidemeshes(frame)
			frame.m_renderFlag = 0
		else
			skirtbaseframe:m_children(i - 1).m_renderFlag = 2
		end
	end
end

skirtlist.action = function(self, text, item, state)
	if state == 1 then
		selectskirt(item)
	end
end


-- #######
-- Strapon
-- #######

local straplist = iup.list {
	expand = "yes",
	visiblelines = 8,
}

local populatestrap = function()
	local char = charamgr.currentcharacter()
	if not char then
		return
	end
	straplist[1] = nil
	local strapbaseframe = char.struct.m_xxBody:FindBone("A00_null_dan") or char.struct.m_xxBody:FindBone("S00_null_dan")
	local last = 0
	if strapbaseframe then
		local strapcount = strapbaseframe.m_nChildren
		for i = 0, strapcount - 1, 1 do
			straplist[i + 1] = strapbaseframe:m_children(i).m_name
			last = i + 1
		end
	end
	straplist[last + 1] = "None"
end
charamgr.currentcharacterchanged.connect(populatestrap)

local selectstrap = function(index)
	local char = charamgr.currentcharacter()
	if not char then
		return
	end
	local strapbaseframe = char.struct.m_xxBody:FindBone("A00_null_dan") or char.struct.m_xxBody:FindBone("S00_null_dan")
	local strapcount = strapbaseframe.m_nChildren
	for i = 1, strapcount, 1 do
		if i == index then
			local frame = strapbaseframe:m_children(i - 1)
			unhidemeshes(frame)
			frame.m_renderFlag = 0
		else
			strapbaseframe:m_children(i - 1).m_renderFlag = 2
		end
	end
end

straplist.action = function(self, text, item, state)
	if state == 1 then
		selectstrap(item)
	end
end


-- ########
-- Clothing
-- ########

local clothslot = toggles.slotbuttons("Slot", { "1", "2", "3", "4" }, function(type)
	local character = charamgr.current
	if character then
		character.clothtype = type
	end
end)

local clothstate = toggles.slotbuttons("State", { "0", "1", "2", "3", "4" }, function(state)
	local character = charamgr.current
	if character then
		character:update(state)
	end
end)

local loadstyle = exe_type == "play" and toggles.slotbuttons("Style", { "1", "2", "3", "4", ">" }, function(state)
	local character = charamgr.current
	if character then
		local characterdata = GetCharInstData(character.struct.m_seat)
		local stylecount = characterdata:GetStyleCount()
		if stylecount > 1 then
			local style
			if state == 4 then
				style = (characterdata:GetCurrentStyle() + 1) % stylecount
			else
				style = state
			end
			characterdata:SetCurrentStyle(style)
			character:reload(character.struct.m_clothState, character.struct.m_xxSkeleton.m_poseNumber)
		end
	end
end)

local loadclothbutton = iup.button { title = "Load Cloth", action = function(self)
	local character = charamgr.current
	if character then
		local file = fileutils.getfiledialog(play_path("data\\save\\cloth\\*.cloth"))
		local size
		if file then file, size = fileutils.readfile(file) end
		if file and size == 92 then character:loadcloth(file) end
	end
end}

local decalstrength = 3
local decals = exe_type == "play" and (iup.frame {
	title = "Skin",
	iup.vbox {
		toggles.slotbuttons("Strength", { "0", "1", "2", "3" }, function(strength)
			decalstrength = strength
		end),
		toggles.slotbuttons("Decals", { "0", "1", "2", "3", "4" }, function(bodypart)
			local character = charamgr.current
			if character then
				local characterdata = GetCharInstData(character.struct.m_seat)
				characterdata:ApplyDecal(bodypart, decalstrength)
log.spam("applydecal %d %d", bodypart, decalstrength)
			end
		end),
	}
})


local _M = iup.hbox {
	iup.vbox {
		iup.frame {
			title = "Clothing",
			iup.vbox {
				clothslot,
				clothstate,
				loadstyle,
				iup.hbox {
					loadclothbutton,
				},
			},
		},
		decals,
	},
	iup.frame {
		title = "Leg Wear",
		legslist,
	},
	iup.frame {
		title = "Skirt",
		skirtlist,
	},
	iup.frame {
		title = "Strap-on",
		straplist,
	},
}

return _M
