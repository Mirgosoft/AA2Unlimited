--@INFO Shows game subtitles

local _M = {}
local opts = {
	{ "fontFam", "Arial", "Font family: %s" },
	{ "fontSize", 24, "Font size, px: %i[1,]" },
	{ "lineHeight", 120, "Line height, percents: %i[100,300]{Percent of Font size param (Not work if params `Separate color for male` and `Outline quality` are disabled)}" },
	{ "duration", 5, "Show duration, sec: %i[1,]" },
	{ "maxLines", 4, "Maximum number of lines: %i[1,]"},
	
	{ "textColFemale", "255 155 255", "Text color (main) RGB: %c"},
	{ "diffColForM", 1, "Separate color for male: %b"},
	{ "textColMale", "155 244 244", "Text color (for male) RGB: %c"},
	
	{ "outlineQuality", 2, "Outline quality: %l|Only text (Off)|With Shadow (Med)|With Outline (High)|{Higher values can slightly affect performance}" },
	{ "outlineSpread", 2, "Text outline spread, px: %i[1,10]"},
	{ "outlineColor", "0 0 0", "Outline color RGB: %c"},
	{ "outlineColorA", 255, "Outline Alpha: %i[0,255]"},
	
	{ "textAlign", 0, "Text Alignment: %l|Left|Center|{(if `Center`, param `Position X` not working)}"},
	{ "areaPosX", 1.0, "Subs Position X, percents: %r[0,95,0.1]{Percent of Game window Width (not works, if param `Alignment` set to `Center`)}"},
	{ "areaPosY", 5.0, "Subs Position Y, percents: %r[0,95,0.1]{Percent of Game window Height}"},
}

local subtitles = {}

local function reload_subtitles()
	local dialogue
	local count = 0
	local subtitles_path = aau_path("configs", "subtitles.txt")
	local file = io.open(subtitles_path, "r")
	if not file then return end
	for line in file:lines() do
		file, dialogue = line:match("%s*([%w_.]+).+\"(.*)\"%s*$")
		if file and dialogue then
			subtitles[file] = dialogue
			count = count + 1
		end
	end
	log.info("Loaded %d subtitles", count)
end

function on.load_audio(fname)
	local sub = subtitles[fname]
	if sub then AddSubtitles(sub, fname) end
end

function on.launch()
	InitSubtitlesParams(opts.fontFam, opts.fontSize, opts.lineHeight, opts.duration, opts.maxLines,
		opts.textColFemale, opts.diffColForM, opts.textColMale, 
		opts.outlineQuality, opts.outlineSpread, opts.outlineColor, opts.outlineColorA,
		opts.textAlign, math.ceil(opts.areaPosX * 100), math.ceil(opts.areaPosY * 100))
end

function _M:load()
	mod_load_config(self, opts)
	reload_subtitles()
	if opts.areaPosX > 95 or opts.areaPosY > 95 then
		opts.areaPosX = 1.0;
		opts.areaPosY = 5.0;
	end
end

function _M:unload()
	subtitles = {}
end

function _M:config()
	mod_edit_config(self, opts, "Subtitles options")
end

return _M

