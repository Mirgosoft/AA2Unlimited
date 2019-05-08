#include "StdAfx.h"
#include <d3d9.h>
#include <vector>
#include "DrawD3D.h"

#pragma comment (lib, "d3d9.lib")

struct vertex
{
	FLOAT x, y, z, rhw;
	DWORD color;
};

struct sScreen
{
	float Width;
	float Height;
	float x_center;
	float y_center;
} Screen;

#define MAX_FONTS 6
#define D3DX_PI (3.14159265358979323846)


namespace DrawD3D {
	bool initialized = false;
	bool fontCreated = false;
	IDirect3DDevice9* pDevice;
	LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;
	LPDIRECT3DINDEXBUFFER9 g_pIB = NULL;
	int FontNr = 0;
	double scaleCoefficient = 0;
	int trueGameMarginY = 0;
	//const wchar_t *hudFontFamily = ((std::wstring)(L"Times New Roman")).c_str();
	std::wstring buff_text;
	const std::wstring box_code = L"\u2586";
	const std::wstring circle_code_Left = L"\u25D6";
	const std::wstring circle_code_Right = L"\u25D7";
	float box1000MarginX = 0;	// Position and sizes of shapes for fontsize == 1000
	float box1000MarginY = 246;
	float box1000Width = 664;
	float box1000Height = 750.000;
	float circle1000MarginLeftSideX = 55;
	float circle1000MarginRightSideX = 250;
	float circle1000MarginY = 305;
	float circle1000Radius = 195.000;// (Width)
	float circle1000Height = 390;
	// 500 - Maximum shapes in HUD
	const int max_shapes = 500;
	int key_next = 0;
	RECT HUDarrayRect[max_shapes];
	IUnknown *HUDarrayFont[max_shapes];
	const wchar_t *HUDarrayText[max_shapes];
	D3DCOLOR HUDarrayColor[max_shapes];

	IUnknown *fontFPS;
	RECT rectFPS = { 0,0,256,64 };
	IUnknown *fontTEST;

	void *(WINAPI *DrawText)(IUnknown* Font, void*, LPCTSTR text, int, LPRECT rect, DWORD dt_params, D3DCOLOR color);
	void *(WINAPI *D3DXCreateFont)(
		IDirect3DDevice9 *pDevice,
		INT               Height,
		UINT              Width,
		UINT              Weight,
		UINT              MipLevels,
		BOOL              Italic,
		DWORD             CharSet,
		DWORD             OutputPrecision,
		DWORD             Quality,
		DWORD             PitchAndFamily,
		LPCTSTR           pFacename,
		IUnknown        **ppFont
		);

	void Reset()
	{
		D3DVIEWPORT9 screen;
		pDevice->GetViewport(&screen);

		Screen.Width = screen.Width;
		Screen.Height = screen.Height;
		Screen.x_center = Screen.Width / 2;
		Screen.y_center = Screen.Height / 2;
	}

	void InitDraw(IDirect3DDevice9 *pDev) {
		pDevice = pDev;
		// fuck you microsoft for the d3dx9 SDK stupidity, no way im installing that shit
		HMODULE hm = GetModuleHandleA("d3dx9_42");
		D3DXCreateFont = decltype(D3DXCreateFont)(GetProcAddress(hm, "D3DXCreateFontW"));

		// Fill HUD RECT arrays with default values
		if (!initialized) {
			std::fill_n(HUDarrayRect, max_shapes, RECT{ 0, 0, 0, 0 });
			const wchar_t *empty_val = ((std::wstring)(L"")).c_str();
			std::fill_n(HUDarrayText, max_shapes, empty_val);
		}
		initialized = true;
		LOGPRIONC(Logger::Priority::INFO) "SVP test_3_\r\n";
	}


	/* If you need a font for hud, you can make his size (Height) scalable, 
	thats depends on current game resolution. 
	(Keep in mind, all hud sizes need to create and set in CreateFontD3d() 
	for 1920x1080 resolution)
	When you set 'autoScale' argument to 'true' in CreateFontD3d(),
	this function automaticaly scaled the size from 1920x1080 template to
	current user's resolution */
	void CreateFontD3d(INT Height, UINT Width, UINT Weight,
		UINT MipLevels, BOOL Italic, DWORD CharSet, DWORD OutputPrecision, DWORD Quality,
		DWORD PitchAndFamily, LPCTSTR pFacename, IUnknown **ppFont, bool autoScale, const char *error_msg) 
	{
		if (autoScale)
			Height = round(Height * scaleCoefficient);
		*ppFont = 0;
		D3DXCreateFont(pDevice, Height, Width, Weight, MipLevels, Italic, CharSet, OutputPrecision,
			Quality, PitchAndFamily, pFacename, *&ppFont);
		if (ppFont)
			DrawText = decltype(DrawText)(((void***)*ppFont)[0][15]);
		else
			LOGPRIONC(Logger::Priority::WARN) error_msg << "\r\n";
	}

	int CreateBoxFilled(float x, float y, 
		float height, int count_boxes_X, // boxes 705x535 (for font size == 1000)
		D3DCOLOR color, int key_node)
	{
		int key_In = key_node;
		if (key_In == -1) { // If need to find new place for shape in HUD data memory
			if (key_next >= max_shapes)
			{
				LOGPRIONC(Logger::Priority::WARN) "Out of memory for creating BoxFilled HUD shape\r\n";
				return -1;
			}
			key_node = key_next;
			key_next++;
		}
		// Scaling request params to current user's resolution + Margin, if resolution not 16:9
		x = x * scaleCoefficient;
		y = y * scaleCoefficient + trueGameMarginY;
		height = height * scaleCoefficient;

		double fontRatio = height / box1000Height;
		int fontHeight = round(1000 * fontRatio); // Total Font Height
		
		HUDarrayFont[key_node] = 0;
		D3DXCreateFont(pDevice, fontHeight, 0, FW_REGULAR, 1, false, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, General::utf8.from_bytes("Arial").c_str(), &HUDarrayFont[key_node]);
		if (HUDarrayFont[key_node])
			DrawText = decltype(DrawText)(((void***)HUDarrayFont[key_node])[0][15]);
		else
		{
			LOGPRIONC(Logger::Priority::WARN) "Error Creating BoxFilled HUD shape\r\n";
			if (key_In == -1)
				key_next--;
			return -1;
		}

		HUDarrayRect[key_node].left = x;							// Total rect for result font
		HUDarrayRect[key_node].top = y - round(box1000MarginY * fontRatio);
		HUDarrayRect[key_node].right = x + round(box1000Width * fontRatio * (count_boxes_X + 1));//(+ 1 * box for reserve)
		HUDarrayRect[key_node].bottom = y + fontHeight;

		buff_text.clear();
		for (int i = 0; i < count_boxes_X; i++)
			buff_text += box_code;
		HUDarrayText[key_node] = buff_text.c_str();					// Total text

		HUDarrayColor[key_node] = color;

		return key_node;
	}

	int CreateHalfCircleFilled(bool leftSide, double x, double y,
		double height, DWORD color, int key_node)
	{
		int key_In = key_node;
		if (key_In == -1) { // If need to find new place for shape in HUD data memory
			if (key_next >= max_shapes)
			{
				LOGPRIONC(Logger::Priority::WARN) "Out of memory for creating HalfCircleFilled HUD shape\r\n";
				return -1;
			}
			key_node = key_next;
			key_next++;
		}
		// Scaling request params to current user's resolution
		x = x * scaleCoefficient;
		y = y * scaleCoefficient + trueGameMarginY;
		height = height * scaleCoefficient;


		double fontRatio = height / circle1000Height;

		float circle1000marginX = leftSide ? circle1000MarginLeftSideX : circle1000MarginRightSideX;
		

		int fontHeight = round(1000 * fontRatio); // Total Font Height
		LOGPRIONC(Logger::Priority::INFO) "HUD_fontRatio_" << fontRatio << "_\r\n";
		LOGPRIONC(Logger::Priority::INFO) "HUD_fontHeight_" << fontHeight << "_\r\n";
		
		HUDarrayFont[key_node] = 0;
		D3DXCreateFont(pDevice, fontHeight, 0, FW_REGULAR, 1, false, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, General::utf8.from_bytes("Arial").c_str(), &HUDarrayFont[key_node]);
		if (HUDarrayFont[key_node])
			DrawText = decltype(DrawText)(((void***)HUDarrayFont[key_node])[0][15]);
		else
		{
			LOGPRIONC(Logger::Priority::WARN) "Error Creating HalfCircleFilled HUD shape\r\n";
			if(key_In == -1)
				key_next--;
			return -1;
		}

		HUDarrayRect[key_node].left = x - round(circle1000marginX * fontRatio);	// Total rect for result font
		HUDarrayRect[key_node].top = y - round(circle1000MarginY * fontRatio);
		HUDarrayRect[key_node].right = x + round(circle1000Radius * fontRatio * 2);//(+ 1 shape for reserve)
		HUDarrayRect[key_node].bottom = y + fontHeight;
		LOGPRIONC(Logger::Priority::INFO) "HUD left _" << HUDarrayRect[key_node].left << "_\r\n";
		LOGPRIONC(Logger::Priority::INFO) "HUD top _" << HUDarrayRect[key_node].top << "_\r\n";
		LOGPRIONC(Logger::Priority::INFO) "HUD right _" << HUDarrayRect[key_node].right << "_\r\n";
		LOGPRIONC(Logger::Priority::INFO) "HUD bottom _" << HUDarrayRect[key_node].bottom << "_\r\n";

		HUDarrayText[key_node] = leftSide ? 				// Total text
			circle_code_Left.c_str() :
			circle_code_Right.c_str();

		HUDarrayColor[key_node] = color;

		return key_node;
	}

	void CreateCircleFilled() {

	}

	// ************************************ Font Creation **************************************
	void MakeFonts(double scale_coefficient, int true_game_margin_Y)
	{
		scaleCoefficient = scale_coefficient;
		trueGameMarginY = true_game_margin_Y;

		LOGPRIONC(Logger::Priority::INFO) "SVP test_4_\r\n";
		LOGPRIONC(Logger::Priority::INFO) "SVP test_5_\r\n";
		if (fontCreated) // If Second time make a fonts - overvrite shapes also (using last D3d device).
			key_next = 0;

		// FPS font
		CreateFontD3d(24, 0, FW_ULTRABOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, General::utf8.from_bytes("Arial").c_str(),
			&fontFPS, false, "FPS Font creation failed");

		// Subs Font
		CreateFontD3d(Subtitles::fontSize, 0, FW_ULTRABOLD, 1, false, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, General::utf8.from_bytes(Subtitles::fontFamily).c_str(),
			&Subtitles::Font, false, "Subs Font creation failed");

		// Notifications Font
		CreateFontD3d(Notifications::fontSize, 0, FW_ULTRABOLD, 1, false, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, General::utf8.from_bytes(Notifications::fontFamily).c_str(),
			&Notifications::Font, false, "Notifications Font creation failed");

		// Other fonts
		// ...



		/* For Shapes _TEST
		// TEST font
		CreateFontD3d(1000, 0, FW_REGULAR, 1, false, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, General::utf8.from_bytes("Arial").c_str(),
		&fontTEST, false, "TEST Font creation failed");//*/
		
		CreateBoxFilled(50, 250, 600, 1, D3DCOLOR_RGBA(33, 255, 255, 188), -1);
		CreateHalfCircleFilled(true, 900, 100, 800, D3DCOLOR_RGBA(33, 255, 255, 188), -1);
		CreateHalfCircleFilled(false, 1400, 200, 800, D3DCOLOR_RGBA(33, 255, 255, 188), -1);//*/

		fontCreated = true;
	}

	// ************************************ Render **************************************
	void Render() {
		



		// Other fonts
		// ...


		/* For Shapes _TEST
		RECT rectFullscreen5 = { 10, 10, 1200, 1200 };
		DrawText(fontTEST, 0, L"\u2586\u2586",
			-1, &rectFullscreen5, DT_NOCLIP, D3DCOLOR_ARGB(166, 255, 22, 22));
		// (all sign: u25D6 u25D7 u2586 | u2588 | u268B u2585  u26AB)
		/*RECT rectFullscreen6 = { 10, 10, 500, 500 }; 
		DrawText(fontTEST, 0, L"\u2586\u2588",
		-1, &rectFullscreen6, DT_NOCLIP, D3DCOLOR_ARGB(166, 255, 22, 22));
		RECT rectFullscreen7 = { 10, 300, 500, 500 };
		DrawText(fontTEST, 0, L"\u25D6\u2588",
		-1, &rectFullscreen7, DT_NOCLIP, D3DCOLOR_ARGB(166, 255, 22, 22));
		RECT rectFullscreen8 = { 10, 600, 500, 500 };
		DrawText(fontTEST, 0, L"\u25D7\u2588",
		-1, &rectFullscreen8, DT_NOCLIP, D3DCOLOR_ARGB(166, 255, 22, 22));//*/
		
		for (int i = 0;i < 3;i++)
			DrawText(HUDarrayFont[i], 0, HUDarrayText[i],
				-1, &HUDarrayRect[i], DT_NOCLIP, HUDarrayColor[i]);//*/

	}

	/*void CDraw::Line(float x1, float y1, float x2, float y2, float width, bool antialias, DWORD color)
	{
	ID3DXLine *m_Line;

	D3DXCreateLine(pDevice, &m_Line);
	D3DXVECTOR2 line[] = { D3DXVECTOR2(x1, y1), D3DXVECTOR2(x2, y2) };
	m_Line->SetWidth(width);
	if (antialias) m_Line->SetAntialias(1);
	m_Line->Begin();
	m_Line->Draw(line, 2, color);
	m_Line->End();
	m_Line->Release();
	}*/

	// Currently not working (need to find a way for correct attaching to game Device)
	/*
	void Circle(float x, float y, float radius, int rotate, int type, bool smoothing, int resolution, DWORD color)
	{
		std::vector<vertex> circle(resolution + 2);
		float angle = rotate * D3DX_PI / 180;
		float pi;

		if (type == full) pi = D3DX_PI;        // Full circle
		if (type == half) pi = D3DX_PI / 2;      // 1/2 circle
		if (type == quarter) pi = D3DX_PI / 4;   // 1/4 circle

		for (int i = 0; i < resolution + 2; i++)
		{
			circle[i].x = (float)(x - radius * cos(i*(2 * pi / resolution)));
			circle[i].y = (float)(y - radius * sin(i*(2 * pi / resolution)));
			circle[i].z = 0;
			circle[i].rhw = 1;
			circle[i].color = color;
		}

		// Rotate matrix
		int _res = resolution + 2;
		for (int i = 0; i < _res; i++)
		{
			circle[i].x = x + cos(angle)*(circle[i].x - x) - sin(angle)*(circle[i].y - y);
			circle[i].y = y + sin(angle)*(circle[i].x - x) + cos(angle)*(circle[i].y - y);
		}

		pDevice->CreateVertexBuffer((resolution + 2) * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL);

		VOID* pVertices;
		g_pVB->Lock(0, (resolution + 2) * sizeof(vertex), (void**)&pVertices, 0);
		memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(vertex));
		g_pVB->Unlock();


		pDevice->SetTexture(0, NULL);
		pDevice->SetPixelShader(NULL);
		if (smoothing)
		{
			pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
			pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
		}
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		pDevice->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
		pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

		pDevice->DrawPrimitive(D3DPT_LINESTRIP, 0, resolution);
		if (g_pVB != NULL) g_pVB->Release();
	}

	void CircleFilled(LPDIRECT3DDEVICE9 pDev, float x, float y, float rad, float rotate, int type, int resolution, DWORD color)
	{
		std::vector<vertex> circle(resolution + 2);
		float angle = rotate * D3DX_PI / 180;
		float pi;

		if (type == full) pi = D3DX_PI;        // Full circle
		if (type == half) pi = D3DX_PI / 2;      // 1/2 circle
		if (type == quarter) pi = D3DX_PI / 4;   // 1/4 circle

		circle[0].x = x;
		circle[0].y = y;
		circle[0].z = 0;
		circle[0].rhw = 1;
		circle[0].color = color;

		for (int i = 1; i < resolution + 2; i++)
		{
			circle[i].x = (float)(x - rad * cos(pi*((i - 1) / (resolution / 2.0f))));
			circle[i].y = (float)(y - rad * sin(pi*((i - 1) / (resolution / 2.0f))));
			circle[i].z = 0;
			circle[i].rhw = 1;
			circle[i].color = color;
		}

		// Rotate matrix
		int _res = resolution + 2;
		for (int i = 0; i < _res; i++)
		{
			circle[i].x = x + cos(angle)*(circle[i].x - x) - sin(angle)*(circle[i].y - y);
			circle[i].y = y + sin(angle)*(circle[i].x - x) + cos(angle)*(circle[i].y - y);
		}

		pDev->CreateVertexBuffer((resolution + 2) * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL);

		VOID* pVertices;
		g_pVB->Lock(0, (resolution + 2) * sizeof(vertex), (void**)&pVertices, 0);
		memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(vertex));
		g_pVB->Unlock();

		pDev->SetTexture(0, NULL);
		pDev->SetPixelShader(NULL);
		pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		pDev->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
		pDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		pDev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);
		if (g_pVB != NULL) g_pVB->Release();
	}

	void Box(float x, float y, float w, float h, float linewidth, DWORD color)
	{
		if (linewidth == 0 || linewidth == 1)
		{
			BoxFilled(x, y, w, 1, color);             // Top
			BoxFilled(x, y + h - 1, w, 1, color);         // Bottom
			BoxFilled(x, y + 1, 1, h - 2 * 1, color);       // Left
			BoxFilled(x + w - 1, y + 1, 1, h - 2 * 1, color);   // Right
		}
		else
		{
			BoxFilled(x, y, w, linewidth, color);                                     // Top
			BoxFilled(x, y + h - linewidth, w, linewidth, color);                         // Bottom
			BoxFilled(x, y + linewidth, linewidth, h - 2 * linewidth, color);               // Left
			BoxFilled(x + w - linewidth, y + linewidth, linewidth, h - 2 * linewidth, color);   // Right
		}
	}

	void BoxBordered(float x, float y, float w, float h, float border_width, DWORD color, DWORD color_border)
	{
		BoxFilled(x, y, w, h, color);
		Box(x - border_width, y - border_width, w + 2 * border_width, h + border_width, border_width, color_border);
	}

	void BoxFilled(float x, float y, float w, float h, DWORD color)
	{
		vertex V[4];

		V[0].color = V[1].color = V[2].color = V[3].color = color;

		V[0].z = V[1].z = V[2].z = V[3].z = 0;
		V[0].rhw = V[1].rhw = V[2].rhw = V[3].rhw = 0;

		V[0].x = x;
		V[0].y = y;
		V[1].x = x + w;
		V[1].y = y;
		V[2].x = x + w;
		V[2].y = y + h;
		V[3].x = x;
		V[3].y = y + h;

		unsigned short indexes[] = { 0, 1, 3, 1, 2, 3 };

		pDevice->CreateVertexBuffer(4 * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB, NULL);
		pDevice->CreateIndexBuffer(2 * sizeof(short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &g_pIB, NULL);

		VOID* pVertices;
		g_pVB->Lock(0, sizeof(V), (void**)&pVertices, 0);
		memcpy(pVertices, V, sizeof(V));
		g_pVB->Unlock();

		VOID* pIndex;
		g_pIB->Lock(0, sizeof(indexes), (void**)&pIndex, 0);
		memcpy(pIndex, indexes, sizeof(indexes));
		g_pIB->Unlock();

		pDevice->SetTexture(0, NULL);
		pDevice->SetPixelShader(NULL);
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		pDevice->SetStreamSource(0, g_pVB, 0, sizeof(vertex));
		pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		pDevice->SetIndices(g_pIB);

		pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);

		g_pVB->Release();
		g_pIB->Release();
	}*/
	
	/*void CDraw::BoxRounded(float x, float y, float w, float h, float radius, bool smoothing, DWORD color, DWORD bcolor)
	{
	BoxFilled(x + radius, y + radius, w - 2 * radius - 1, h - 2 * radius - 1, color);   // Center rect.
	BoxFilled(x + radius, y + 1, w - 2 * radius - 1, radius - 1, color);            // Top rect.
	BoxFilled(x + radius, y + h - radius - 1, w - 2 * radius - 1, radius, color);     // Bottom rect.
	BoxFilled(x + 1, y + radius, radius - 1, h - 2 * radius - 1, color);            // Left rect.
	BoxFilled(x + w - radius - 1, y + radius, radius, h - 2 * radius - 1, color);     // Right rect.

	// Smoothing method
	if (smoothing)
	{
	CircleFilled(x + radius, y + radius, radius - 1, 0, quarter, 16, color);             // Top-left corner
	CircleFilled(x + w - radius - 1, y + radius, radius - 1, 90, quarter, 16, color);        // Top-right corner
	CircleFilled(x + w - radius - 1, y + h - radius - 1, radius - 1, 180, quarter, 16, color);   // Bottom-right corner
	CircleFilled(x + radius, y + h - radius - 1, radius - 1, 270, quarter, 16, color);       // Bottom-left corner

	Circle(x + radius + 1, y + radius + 1, radius, 0, quarter, true, 16, bcolor);          // Top-left corner
	Circle(x + w - radius - 2, y + radius + 1, radius, 90, quarter, true, 16, bcolor);       // Top-right corner
	Circle(x + w - radius - 2, y + h - radius - 2, radius, 180, quarter, true, 16, bcolor);    // Bottom-right corner
	Circle(x + radius + 1, y + h - radius - 2, radius, 270, quarter, true, 16, bcolor);      // Bottom-left corner

	Line(x + radius, y + 1, x + w - radius - 1, y + 1, 1, false, bcolor);       // Top line
	Line(x + radius, y + h - 2, x + w - radius - 1, y + h - 2, 1, false, bcolor);   // Bottom line
	Line(x + 1, y + radius, x + 1, y + h - radius - 1, 1, false, bcolor);       // Left line
	Line(x + w - 2, y + radius, x + w - 2, y + h - radius - 1, 1, false, bcolor);   // Right line
	}
	else
	{
	CircleFilled(x + radius, y + radius, radius, 0, quarter, 16, color);             // Top-left corner
	CircleFilled(x + w - radius - 1, y + radius, radius, 90, quarter, 16, color);        // Top-right corner
	CircleFilled(x + w - radius - 1, y + h - radius - 1, radius, 180, quarter, 16, color);   // Bottom-right corner
	CircleFilled(x + radius, y + h - radius - 1, radius, 270, quarter, 16, color);       // Bottom-left corner
	}
	}

	void CDraw::Text(char *text, float x, float y, int orientation, int font, bool bordered, DWORD color, DWORD bcolor)
	{
	RECT rect;

	switch (orientation)
	{
	case lefted:
	if (bordered)
	{
	SetRect(&rect, x - 1, y, x - 1, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, bcolor);
	SetRect(&rect, x + 1, y, x + 1, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, bcolor);
	SetRect(&rect, x, y - 1, x, y - 1);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, bcolor);
	SetRect(&rect, x, y + 1, x, y + 1);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, bcolor);
	}
	SetRect(&rect, x, y, x, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, color);
	break;
	case centered:
	if (bordered)
	{
	SetRect(&rect, x - 1, y, x - 1, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, bcolor);
	SetRect(&rect, x + 1, y, x + 1, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, bcolor);
	SetRect(&rect, x, y - 1, x, y - 1);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, bcolor);
	SetRect(&rect, x, y + 1, x, y + 1);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, bcolor);
	}
	SetRect(&rect, x, y, x, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, color);
	break;
	case righted:
	if (bordered)
	{
	SetRect(&rect, x - 1, y, x - 1, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_RIGHT | DT_NOCLIP, bcolor);
	SetRect(&rect, x + 1, y, x + 1, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_RIGHT | DT_NOCLIP, bcolor);
	SetRect(&rect, x, y - 1, x, y - 1);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_RIGHT | DT_NOCLIP, bcolor);
	SetRect(&rect, x, y + 1, x, y + 1);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_RIGHT | DT_NOCLIP, bcolor);
	}
	SetRect(&rect, x, y, x, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_RIGHT | DT_NOCLIP, color);
	break;
	}
	}

	void CDraw::Message(char *text, float x, float y, int font, int orientation)
	{
	RECT rect = { x, y, x, y };

	switch (orientation)
	{
	case lefted:
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CALCRECT | DT_LEFT, BLACK(255));

	BoxRounded(x - 5, rect.top - 5, rect.right - x + 10, rect.bottom - rect.top + 10, 5, true, DARKGRAY(150), SKYBLUE(255));

	SetRect(&rect, x, y, x, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, ORANGE(255));
	break;
	case centered:
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CALCRECT | DT_CENTER, BLACK(255));

	BoxRounded(rect.left - 5, rect.top - 5, rect.right - rect.left + 10, rect.bottom - rect.top + 10, 5, true, DARKGRAY(150), SKYBLUE(255));

	SetRect(&rect, x, y, x, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, ORANGE(255));
	break;
	case righted:
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_CALCRECT | DT_RIGHT, BLACK(255));

	BoxRounded(rect.left - 5, rect.top - 5, rect.right - rect.left + 10, rect.bottom - rect.top + 10, 5, true, DARKGRAY(150), SKYBLUE(255));

	SetRect(&rect, x, y, x, y);
	pFont[font]->DrawTextA(NULL, text, -1, &rect, DT_RIGHT | DT_NOCLIP, ORANGE(255));
	break;
	}
	}

	void CDraw::Sprite(LPDIRECT3DTEXTURE9 tex, float x, float y, float resolution, float scale, float rotation)
	{
	float screen_pos_x = x;
	float screen_pos_y = y;

	// Texture being used is 64x64:
	D3DXVECTOR2 spriteCentre = D3DXVECTOR2(resolution / 2, resolution / 2);

	// Screen position of the sprite
	D3DXVECTOR2 trans = D3DXVECTOR2(screen_pos_x, screen_pos_y);

	// Build our matrix to rotate, scale and position our sprite
	D3DXMATRIX mat;

	D3DXVECTOR2 scaling(scale, scale);

	// out, scaling centre, scaling rotation, scaling, rotation centre, rotation, translation
	D3DXMatrixTransformation2D(&mat, NULL, 0.0, &scaling, &spriteCentre, rotation, &trans);

	//pDevice->SetRenderState(D3DRS_ZENABLE, false);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	pDevice->SetPixelShader(NULL);
	sSprite->Begin(NULL);
	sSprite->SetTransform(&mat); // Tell the sprite about the matrix
	sSprite->Draw(tex, NULL, NULL, NULL, 0xFFFFFFFF);
	sSprite->End();
	}

	bool CDraw::Font()
	{
	for (int i = 0; i<FontNr; i++)
	if (pFont[i]) return false;
	return true;
	}

	void CDraw::AddFont(char* Caption, float size, bool bold, bool italic)
	{
	D3DXCreateFont(pDevice, size, 0, (bold) ? FW_BOLD : FW_NORMAL, 1, (italic) ? 1 : 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, Caption, &pFont[++FontNr]);
	}

	void CDraw::FontReset()
	{
	for (int i = 0; i < FontNr; i++) pFont[i]->OnLostDevice();
	for (int i = 0; i < FontNr; i++) pFont[i]->OnResetDevice();
	}

	void CDraw::OnLostDevice()
	{
	for (int i = 0; i < FontNr; i++) pFont[i]->OnLostDevice();
	}*/
}
