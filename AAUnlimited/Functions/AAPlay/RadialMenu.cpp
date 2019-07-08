#include "StdAfx.h"

#include <iostream>
#include <map>

typedef void(*FnPtr)(); // Only void Functions and without arguments

namespace RadialMenu {

	bool enabled = false;
	bool initializedBefore = false;
	int toggleType = 0;
	int cancelTime = 500;
	std::wstring defaultDesc { L"Move cursor to select action" };
	std::wstring canceledBtnText{ L"Canceled" };
	DWORD lastRightClickTime = 0;
	int startCursorX = 0;
	int startCursorY = 0;
	float currentVerScale = 1;

	RDM_MENU menu;						// Most info about menu
	std::map<std::string, FnPtr> cppFuncMap;
	RDM_BUTTON menuButtonsArr[2][8];	// 2 menues (general and H) with 8 buttons maximum
	int countButtons[2] = { 0, 0 };		// Added buttons for each menu
	float buttonDegreesRange[2] = { 1, 1 }; // Sector cost in degrees
	

	// Create Map of c++ functions, which will be using on select RadMenu buttons
	void CreateMapFuncCPP(){
		cppFuncMap[std::string("Controls_F1")] = Controls::EnterMenuF1;
		cppFuncMap[std::string("Controls_ESC")] = Controls::ExitGameESC;
		cppFuncMap[std::string("Controls_Space")] = Controls::PressKeySpace;
		cppFuncMap[std::string("Controls_F9")] = Controls::screenshotF9;
		cppFuncMap[std::string("Controls_F11")] = Controls::screenshotF11;
	}

	void CallFunc(std::string func_map_key) {
		if (cppFuncMap.count(func_map_key) > 0) // If CPP func exist - call it
			cppFuncMap[func_map_key]();
		else
			LUA_EVENT_NORET(func_map_key);
		// In *.lua file need create event: function on.`your_lua_event_name`() ... end
	}

	void ShowMenu() {
		if (menu.showed || menu.showCancelMsg) return;
		if (menu.nowHscene)
			menu.showedType = 1;
		else
			menu.showedType = 0;
		// Remember Cursor position
		startCursorX = DrawD3D::cursor.x;
		startCursorY = DrawD3D::cursor.y;

		//LOGPRIO(Logger::Priority::INFO) << "RadMenu ShowMenu() OK\r\n";

		menu.showed = true;
	}

	void ApplyChoise(bool cancel, bool cancel_immediately = false) {
		if (!menu.showed) return;

		menu.showed = false;
		if (cancel == true || menu.selectedBtnNode == -1) // Cancel choise
		{
			if (!cancel_immediately) // Indicating of cancel
			{
				menu.showCancelMsg = true;
				menu.cancelMsgStartTime = GetTickCount();
			}
			//LOGPRIO(Logger::Priority::INFO) << "RadMenu ApplyChoise(cancel)\r\n";
		}
		else if (cancel == false) // Apply choise
		{
			CallFunc(menuButtonsArr[menu.showedType][menu.selectedBtnNode].funcName);
			//LOGPRIO(Logger::Priority::INFO) << "RadMenu ApplyChoise(apply)\r\n";
		}
		menu.selectedBtnNode = -1;
	}


	void CreateHUD(bool force_create) {
		if (!enabled && !force_create) return;

		int outCircleHeight = round(780 * currentVerScale);
		int inCircleHeight = round(320 * currentVerScale);

		// Main parts of menu
		menu.inCircleD3dKey = DrawD3D::CreateCircleFilled(
			menu.posX, menu.posY, true, inCircleHeight, D3DCOLOR_ARGB(255, 33, 33, 33), menu.inCircleD3dKey);
		menu.outCircleD3dKey = DrawD3D::CreateCircleFilled(
			menu.posX, menu.posY, true, outCircleHeight, D3DCOLOR_ARGB(188, 11, 11, 11), menu.outCircleD3dKey);
		menu.cancelMsgBackgrD3dKey = DrawD3D::CreateBoxFilled(
			menu.posX, menu.posY, true, round(75 * currentVerScale), 3, D3DCOLOR_ARGB(244, 233, 22, 22), menu.cancelMsgBackgrD3dKey);
		menu.cancelMsgTextD3dKey = DrawD3D::CreateFontHUD(menu.posX, menu.posY, true, DT_CENTER, round(240 * currentVerScale),
			round(44 * currentVerScale * menu.fontSizeMultiplier), FW_BOLD, false, menu.fontFamily.c_str(),
			canceledBtnText.c_str(), D3DCOLOR_ARGB(255, 244, 244, 244), menu.cancelMsgTextD3dKey);
		menu.defaultDescD3dKey = DrawD3D::CreateFontHUD(menu.posX, menu.posY, true, DT_CENTER, round(250 * currentVerScale),
			round(32 * currentVerScale * menu.fontSizeMultiplier), FW_BOLD, false, menu.fontFamily.c_str(),
			defaultDesc.c_str(), D3DCOLOR_ARGB(255, 188, 188, 188), menu.defaultDescD3dKey);

		// Buttons (General and H-scene)
		int place_radius = round((outCircleHeight + inCircleHeight) / 4.0);
		int pos_x = 0; 
		int pos_y = 0;
		float current_angle = 0;
		for (int type_i = 0; type_i < 2; type_i++)
		{
			buttonDegreesRange[type_i] = 360.00 / countButtons[type_i];
			for (int button_i = 0; button_i < countButtons[type_i]; button_i++) {
				// Find the right place for button
				current_angle = (90 + (buttonDegreesRange[type_i] * button_i)) * DrawD3D::D3DX_PI / 180;
				pos_x = round(menu.posX - place_radius * cos(current_angle));
				pos_y = round(menu.posY - place_radius * sin(current_angle));
				// Button Active Background
				menuButtonsArr[type_i][button_i].backgrActiveD3dKey = DrawD3D::CreateBoxFilled(pos_x, pos_y, true, 
					round(80 * currentVerScale), 3, D3DCOLOR_ARGB(255, 188, 22, 166), menuButtonsArr[type_i][button_i].backgrActiveD3dKey);
				// Button Title
				menuButtonsArr[type_i][button_i].titleD3dKey = DrawD3D::CreateFontHUD(pos_x, pos_y, true, DT_CENTER, round(200 * currentVerScale),
					round(32 * currentVerScale * menu.fontSizeMultiplier), FW_BOLD, false, menu.fontFamily.c_str(),
					menuButtonsArr[type_i][button_i].titleIngame.c_str(), D3DCOLOR_ARGB(255, 244, 244, 244), menuButtonsArr[type_i][button_i].titleD3dKey);
				// Button Description
				menuButtonsArr[type_i][button_i].descD3dKey = DrawD3D::CreateFontHUD(menu.posX, menu.posY, true, DT_CENTER, round(270 * currentVerScale),
					round(32 * currentVerScale * menu.fontSizeMultiplier), FW_BOLD, false, menu.fontFamily.c_str(),
					menuButtonsArr[type_i][button_i].shortDesc.c_str(), D3DCOLOR_ARGB(255, 233, 233, 233), menuButtonsArr[type_i][button_i].descD3dKey);
			}
		}
	}

	void Render() {
		if (!enabled) return;

		if (menu.showCancelMsg) { // If need to show cancel message
			if (GetTickCount() - menu.cancelMsgStartTime > cancelTime)
				menu.showCancelMsg = false;
			DrawD3D::renderShapeHUD(menu.cancelMsgBackgrD3dKey);
			DrawD3D::renderFontHUD(menu.cancelMsgTextD3dKey);
		}

		if (!menu.showed) return;
		// Main parts of menu
		DrawD3D::renderShapeHUD(menu.outCircleD3dKey);
		DrawD3D::renderShapeHUD(menu.inCircleD3dKey);
		// Buttons
		// Active Backround and Short Description first, if present
		if (sqrt(pow((startCursorX - DrawD3D::cursor.x), 2)
			+ pow((startCursorY - DrawD3D::cursor.y), 2)) > menu.deadzone) // if not in deadzone for select
		{
			// Get angle in degrees
			int angle = 180 - round(atan2(DrawD3D::cursor.x - startCursorX, DrawD3D::cursor.y - startCursorY)
				* 180.0 / DrawD3D::D3DX_PI);

			int selected_node = round(angle / buttonDegreesRange[menu.showedType]);
			if (selected_node >= countButtons[menu.showedType])
				selected_node = 0;
			menu.selectedBtnNode = selected_node;
			
			DrawD3D::renderShapeHUD(menuButtonsArr[menu.showedType][selected_node].backgrActiveD3dKey);
			DrawD3D::renderFontHUD(menuButtonsArr[menu.showedType][selected_node].descD3dKey);
		}
		else { // If currently nothing selected
			menu.selectedBtnNode = -1;
			DrawD3D::renderFontHUD(menu.defaultDescD3dKey);
		}
		// Buttons Texts
		for (int button_i = 0; button_i < countButtons[menu.showedType]; button_i++)
			DrawD3D::renderFontHUD(menuButtonsArr[menu.showedType][button_i].titleD3dKey);
	}

	// ScriptLua functions
	void InitRadialMenuParams(const char *font_family, int mini_version, int font_size, int deadzone, int cancel_time, 
		int toggle_type, const char * default_desc, const char* canceled_button_text) {

		if (!General::IsAAPlay) return;
		enabled = false; // Hide menu, until Init/reInit success

		menu.fontFamily = General::utf8.from_bytes(font_family);
		menu.fontSizeMultiplier = font_size / 100.000;
		menu.deadzone = deadzone;
		toggleType = toggle_type;
		cancelTime = cancel_time;
		defaultDesc = General::utf8.from_bytes(default_desc);
		canceledBtnText = General::utf8.from_bytes(canceled_button_text);

		menu.posX = 960; // Default menu position (for 1920x1080 resolution template)
		menu.posY = 540;
		currentVerScale = 1;
		if (mini_version == 1) { // Minified ver. make smaller and move to the right
			currentVerScale = 0.5;
			menu.posX = 1670;
		}

		// If D3D HUD already created - create HUD for Menu also.
		if (DrawD3D::fontCreated)
			CreateHUD(true);

		if (!initializedBefore) {
			// Create Map of c++ functions, which will be using on click RadMenu buttons
			CreateMapFuncCPP();

			// Handle user input clicks and keys
			GameTick::RegisterMsgFilter(GameTick::MsgFilterFunc([](MSG *m) {
				if (toggleType == 0) {
					if (m->message == WM_RBUTTONUP)
					{
						if (menu.showed)
							ApplyChoise(true); // Cancel choise
						else {
							if (GetTickCount() - lastRightClickTime < 400)
								ShowMenu();
						}
						lastRightClickTime = GetTickCount();
					}
					else if (m->message == WM_LBUTTONUP)
						ApplyChoise(false);
				}
				else if (toggleType == 1) {
					if (m->message == WM_XBUTTONDOWN && GET_XBUTTON_WPARAM(m->wParam) == XBUTTON1)
						ShowMenu();
					else if (m->message == WM_XBUTTONUP && GET_XBUTTON_WPARAM(m->wParam) == XBUTTON1)
						ApplyChoise(false);
					else if (m->message == WM_RBUTTONUP || m->message == WM_LBUTTONUP)
						ApplyChoise(true); // Cancel choise
				}
				else if (toggleType == 2)
				{
					if (m->message == WM_XBUTTONDOWN && GET_XBUTTON_WPARAM(m->wParam) == XBUTTON2)
						ShowMenu();
					else if (m->message == WM_XBUTTONUP && GET_XBUTTON_WPARAM(m->wParam) == XBUTTON2)
						ApplyChoise(false);
					else if (m->message == WM_RBUTTONUP || m->message == WM_LBUTTONUP)
						ApplyChoise(true); // Cancel choise
				}
				return false;
			}));
		}

		initializedBefore = true;
		enabled = true;
	}

	void AddButton(int buttons_arr_node, const char * func_name,
		const char * title_ingame, const char * short_desc)
	{
		if (!General::IsAAPlay) return;

		if (buttons_arr_node == -1) { // Clear previous buttons data before Init/reInit
			enabled = false; // Hide menu, until Init/reInit success
			countButtons[0] = 0;
			countButtons[1] = 0;
			return;
		}

		menuButtonsArr[buttons_arr_node][countButtons[buttons_arr_node]].funcName = func_name;
		menuButtonsArr[buttons_arr_node][countButtons[buttons_arr_node]].titleIngame = General::utf8.from_bytes(title_ingame);
		menuButtonsArr[buttons_arr_node][countButtons[buttons_arr_node]].shortDesc = General::utf8.from_bytes(short_desc);
		countButtons[buttons_arr_node]++;
	}

	void SetHstatus(bool current_status) {
		menu.nowHscene = current_status;
		if (menu.showed) {
			if ((menu.showedType == 0 && current_status == true) ||
				(menu.showedType == 1 && current_status == false))
				ApplyChoise(true, true); // Cancel choise & close menu (immediately)
		}
	}
}