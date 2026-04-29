#include <Windows.h>
#include <string>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <fstream>
#include <comdef.h>
#include <iostream>
#include <float.h>
#include "imgui/imgui.h"
#include "../Cfg/fonts.hpp"
#include <ctime>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dwrite.lib")

ID2D1Factory* factory;
ID2D1HwndRenderTarget* target;
ID2D1SolidColorBrush* solid_brush;
IDWriteFactory* w_factory;
IDWriteTextFormat* w_format;
IDWriteTextLayout* w_layout;
HWND overlayWindow;
HINSTANCE appInstance;
HWND targetWindow;
HWND enumWindow = NULL;
time_t preTime = clock();
time_t showTime = clock();
int fps = 0;
extern bool StreamMode;
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif

bool o_Foreground = false;
bool o_DrawFPS = false;
bool o_VSync = false;
bool o_VSync90 = false;
std::wstring fontname = L"Courier";

// Link the static library (make sure that file is in the same directory as this file)
//#pragma comment(lib, "D2DOverlay.lib")

// Requires the targetted window to be active and the foreground window to draw.
#define D2DOV_REQUIRE_FOREGROUND	(1 << 0)

// Draws the FPS of the overlay in the top-right corner
#define D2DOV_DRAW_FPS				(1 << 1)

// Attempts to limit the frametimes so you don't render at 500fps
#define D2DOV_VSYNC					(1 << 2)
// Limits framerate to 90 FPS
#define D2DOV_VSYNC_90				(1 << 8)

// Sets the text font to Calibri
#define D2DOV_FONT_CALIBRI			(1 << 3)

// Sets the text font to Arial
#define D2DOV_FONT_ARIAL			(1 << 4)

// Sets the text font to Courier
#define D2DOV_FONT_COURIER			(1 << 5)

// Sets the text font to Gabriola
#define D2DOV_FONT_GABRIOLA			(1 << 6)

// Sets the text font to Impact
#define D2DOV_FONT_IMPACT			(1 << 7)

// The function you call to set up the above options.  Make sure its called before the DirectOverlaySetup function
void DirectOverlaySetOption(DWORD option);

// typedef for the callback function, where you'll do the drawing.
typedef void(*DirectOverlayCallback)(int width, int height);

// Initializes a the overlay window, and the thread to run it.  Input your callback function.
// Uses the first window in the current process to target.  If you're external, use the next function
void DirectOverlaySetup(DirectOverlayCallback callbackFunction);

// Used to specify the window manually, to be used with externals.
void DirectOverlaySetup(DirectOverlayCallback callbackFunction, HWND targetWindow);

// Define janela alternativa (ex: menu do cheat) para permitir desenho quando ela estiver em foco
void DirectOverlaySetAlternateForegroundWindow(HWND h);

// Ativa/desativa a verifica??o de foreground em tempo de execu??o (ESP s? desenha com emulador em foco)
void DirectOverlaySetRequireForeground(bool enable);

// Retorna true se ImGui/Binds/ESP devem ser vis?veis (emulador ou menu em foco, ou op??o desativada)
bool DirectOverlayIsContentVisible();

// Draws a line from (x1, y1) to (x2, y2), with a specified thickness.
// Specify the color, and optionally an alpha for the line.
void DrawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a = 1);

// Draws a rectangle on the screen.  Width and height are relative to the coordinates of the box.  
// Use the "filled" bool to make it a solid rectangle; ignore the thickness.
// To just draw the border around the rectangle, specify a thickness and pass "filled" as false.
void DrawBox(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled);

// Draws a circle.  As with the DrawBox, the "filled" bool will make it a solid circle, and thickness is only used when filled=false.
void DrawCircle(float x, float y, float radius, float thickness, float r, float g, float b, float a, bool filled);

// Allows you to draw an elipse.  Same as a circle, except you have two different radii, for width and height.
void DrawEllipse(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled);

// Draw a string on the screen.  Input is in the form of an std::string.
void DrawString(std::string str, float fontSize, float x, float y, float r, float g, float b, float a = 1);

DirectOverlayCallback drawLoopCallback = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Rect {
public:
	float x;
	float y;
	float width;
	float height;

	Rect() {
		this->x = 0;
		this->y = 0;
		this->width = 0;
		this->height = 0;
	}

	Rect(float x, float y, float width, float height) {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
	}

	bool operator==(const Rect& src) const {
		return (src.x == this->x && src.y == this->y && src.height == this->height &&
			src.width == this->width);
	}

	bool operator!=(const Rect& src) const {
		return (src.x != this->x && src.y != this->y && src.height != this->height &&
			src.width != this->width);
	}
};

void DrawString(std::string str, float fontSize, float x, float y, float r, float g, float b, float a)
{
	RECT re;
	GetClientRect(overlayWindow, &re);
	FLOAT dpix, dpiy;
	dpix = static_cast<float>(re.right - re.left);
	dpiy = static_cast<float>(re.bottom - re.top);
	HRESULT res = w_factory->CreateTextLayout(std::wstring(str.begin(), str.end()).c_str(), str.length() + 1, w_format, dpix, dpiy, &w_layout);
	if (SUCCEEDED(res))
	{
		DWRITE_TEXT_RANGE range = { 0, str.length() };
		w_layout->SetFontSize(fontSize, range);
		solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
		target->DrawTextLayout(D2D1::Point2F(x, y), w_layout, solid_brush);
		w_layout->Release();
		w_layout = NULL;
	}
}

void DrawName(const char* text, float fontSize, Vector2 pos, float r, float g, float b, float a = 1)
{
	RECT re;
	GetClientRect(overlayWindow, &re);
	FLOAT dpix, dpiy;
	dpix = static_cast<float>(re.right - re.left);
	dpiy = static_cast<float>(re.bottom - re.top);

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	wchar_t* wtext = new wchar_t[size_needed];
	MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, size_needed);

	IDWriteTextFormat* w_format;
	HRESULT res = w_factory->CreateTextFormat(
		AY_OBFUSCATE(L"Segoe UI Emoji"),
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		AY_OBFUSCATE(L"en-US"),
		&w_format
	);

	if (SUCCEEDED(res))
	{
		IDWriteTextLayout* w_layout;
		res = w_factory->CreateTextLayout(
			wtext,
			wcslen(wtext),
			w_format,
			dpix,
			dpiy,
			&w_layout
		);

		if (SUCCEEDED(res))
		{
			DWRITE_TEXT_METRICS textMetrics;
			w_layout->GetMetrics(&textMetrics);

			ID2D1SolidColorBrush* textBrush;
			target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &textBrush);

			target->DrawTextLayout(
				D2D1::Point2F(pos.X - (textMetrics.width / 2), pos.Y - textMetrics.height),
				w_layout,
				textBrush
			);

			textBrush->Release();

			w_layout->Release();
			w_format->Release();
		}

		delete[] wtext;
	}
}

void DrawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), solid_brush, thickness);
}

void DrawBox(const Rect& rect, float thickness, float r, float g, float b, float a = 1) {
	Vector3 v1 = Vector3(rect.x, rect.y);
	Vector3 v2 = Vector3(rect.x + rect.width, rect.y);
	Vector3 v3 = Vector3(rect.x + rect.width, rect.y + rect.height);
	Vector3 v4 = Vector3(rect.x, rect.y + rect.height);

	DrawLine(v1.X, v1.Y, v2.X, v2.Y, thickness, r, g, b, a);
	DrawLine(v2.X, v2.Y, v3.X, v3.Y, thickness, r, g, b, a);
	DrawLine(v3.X, v3.Y, v4.X, v4.Y, thickness, r, g, b, a);
	DrawLine(v4.X, v4.Y, v1.X, v1.Y, thickness, r, g, b, a);
}
void DrawCorneredBox(const Rect& rect, float thickness, float cornerSize, float r, float g, float b, float a = 1) {
	DrawLine(rect.x, rect.y, rect.x + cornerSize, rect.y, thickness, r, g, b, a);
	DrawLine(rect.x + rect.width - cornerSize, rect.y, rect.x + rect.width, rect.y, thickness, r, g, b, a);
	DrawLine(rect.x, rect.y + rect.height, rect.x + cornerSize, rect.y + rect.height, thickness, r, g, b, a);
	DrawLine(rect.x + rect.width - cornerSize, rect.y + rect.height, rect.x + rect.width, rect.y + rect.height, thickness, r, g, b, a);
	DrawLine(rect.x, rect.y, rect.x, rect.y + cornerSize, thickness, r, g, b, a);
	DrawLine(rect.x + rect.width, rect.y, rect.x + rect.width, rect.y + cornerSize, thickness, r, g, b, a);
	DrawLine(rect.x, rect.y + rect.height - cornerSize, rect.x, rect.y + rect.height, thickness, r, g, b, a);
	DrawLine(rect.x + rect.width, rect.y + rect.height - cornerSize, rect.x + rect.width, rect.y + rect.height, thickness, r, g, b, a);
}

void DrawFov(ID2D1HwndRenderTarget* target, float radius, float thickness, float r, float g, float b, float a, bool filled = false, float fillR = 0, float fillG = 0, float fillB = 0, float fillA = 0) {
	if (radius <= 0) return;

	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	float centerX = displaySize.x / 2.0f;
	float centerY = displaySize.y / 2.0f;

	target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	if (filled) {
		ID2D1SolidColorBrush* fillBrush = nullptr;

		// Se a cor for preta (0,0,0), ela sempre vai escurecer a tela.
		// Vamos garantir que o Alpha n?o seja exagerado e sugerir uma l?gica de brilho.
		float finalFillA = fillA;

		// Se voc? quiser que o preenchimento acompanhe a cor da linha mas seja bem sutil:
		// HRESULT hr = target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 0.1f), &fillBrush);

		HRESULT hr = target->CreateSolidColorBrush(D2D1::ColorF(fillR, fillG, fillB, finalFillA), &fillBrush);

		if (SUCCEEDED(hr) && fillBrush) {
			D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radius, radius);
			target->FillEllipse(ellipse, fillBrush);
			fillBrush->Release();
		}
	}

	ID2D1SolidColorBrush* outlineBrush = nullptr;
	HRESULT hr = target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &outlineBrush);
	if (SUCCEEDED(hr) && outlineBrush) {
		D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(centerX, centerY), radius, radius);
		target->DrawEllipse(ellipse, outlineBrush, thickness);
		outlineBrush->Release();
	}
}

void DrawDistance(const wchar_t* text, float fontSize, Vector2 pos, float r, float g, float b, float a = 1)
{
	RECT re;
	GetClientRect(overlayWindow, &re);
	FLOAT dpix, dpiy;
	dpix = static_cast<float>(re.right - re.left);
	dpiy = static_cast<float>(re.bottom - re.top);

	IDWriteTextFormat* w_format;
	HRESULT res = w_factory->CreateTextFormat(
		AY_OBFUSCATE(L"Arial"),
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		AY_OBFUSCATE(L"en-US"),
		&w_format
	);

	if (SUCCEEDED(res))
	{
		IDWriteTextLayout* w_layout;
		res = w_factory->CreateTextLayout(
			text,
			wcslen(text),
			w_format,
			dpix,
			dpiy,
			&w_layout
		);

		if (SUCCEEDED(res))
		{
			DWRITE_TEXT_METRICS textMetrics;
			w_layout->GetMetrics(&textMetrics);

			ID2D1SolidColorBrush* textBrush;
			target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &textBrush);

			target->DrawTextLayout(
				D2D1::Point2F(
					pos.X - (textMetrics.width / 2), // centraliza horizontalmente
					pos.Y // cresce pra baixo a partir daqui
				),
				w_layout,
				textBrush
			);

			textBrush->Release();

			w_layout->Release();
			w_format->Release();
		}
	}
}

void DrawRect(float x, float y, float width, float height, int color, float strokeWidth, bool filled)
{
	solid_brush->SetColor(D2D1::ColorF(color));
	target->DrawRectangle(D2D1::RectF(x, y, x + width, y + height), solid_brush, strokeWidth);
	if (filled) {
		target->FillRectangle(D2D1::RectF(x, y, x + width, y + height), solid_brush);
	}
}


void DrawVerticalHealthBar(float x, float y, float width, float height, float currentHealth, float maxHealth) {
	int healthColor = -16711936;
	float currentHealthHeight = (currentHealth * height) / maxHealth;

	float healthPercent = currentHealth / maxHealth;
	if (healthPercent <= 0.33f) {
		healthColor = -65536;
	}
	else if (healthPercent <= 0.66f) {
		healthColor = -256;
	}
	else {
		healthColor = -16711936;
	}

	DrawRect(x, y, width, height, -16777216, 1, false);
	DrawRect(x, y + height - currentHealthHeight, width, currentHealthHeight, healthColor, 1, true);
}

void DrawHorizontalHealthBar(float x, float y, float width, float height, float currentHealth, float maxHealth) {
	int healthColor = -16711936;
	float currentHealthWidth = (currentHealth * width) / maxHealth;

	float healthPercent = currentHealth / maxHealth;
	if (healthPercent <= 0.33f) {
		healthColor = -65536;
	}
	else if (healthPercent <= 0.66f) {
		healthColor = -256;
	}
	else {
		healthColor = -16711936;
	}
	DrawRect(x, y, width, height, -16777216, 1, false);
	DrawRect(x, y, currentHealthWidth, height, healthColor, 1, true);
}

void DrawRoundHealthBar(const Rect& rect, float maxHealth, float currentHealth) {
	int healthColor = -16711936;
	float currentHealthWidth = (currentHealth * rect.height) / maxHealth;
	float maxHealthWidth = (maxHealth * rect.height) / maxHealth;

	if (currentHealth <= (maxHealth * 1.0)) {
		healthColor = -16711936;
	}
	if (currentHealth <= (maxHealth * 0.66)) {
		healthColor = -256;
	}
	if (currentHealth <= (maxHealth * 0.33)) {
		healthColor = -65536;
	}

	DrawRect(rect.x - rect.width / 4, rect.y, rect.width / 10, maxHealthWidth, -16777216, 1, false);
	DrawRect(rect.x - rect.width / 4, rect.y + rect.height - currentHealthWidth, rect.width / 10, currentHealthWidth, healthColor, 1, true);
}

ImColor GetRGBColor(float speed = 1.0f)
{
	float time = ImGui::GetTime() * speed;

	float r = sinf(time * 2.0f) * 0.5f + 0.5f;
	float g = sinf(time * 2.0f + 2.094f) * 0.5f + 0.5f; // 120?
	float b = sinf(time * 2.0f + 4.188f) * 0.5f + 0.5f; // 240?

	return ImColor(r, g, b, 1.0f);
}

void DrawHealthBarAtPosition(const Rect& rect, float currentHealth, float maxHealth, int position) {
	float barWidth = 4;
	float barHeight = rect.height;
	float barX, barY;

	switch (position) {
	case 0: // Left
		barX = rect.x - rect.width / 4;
		barY = rect.y;
		DrawVerticalHealthBar(barX, barY, barWidth, barHeight, currentHealth, maxHealth);
		break;

	case 1: // Right
		barX = rect.x + rect.width + rect.width / 4 - barWidth;
		barY = rect.y;
		DrawVerticalHealthBar(barX, barY, barWidth, barHeight, currentHealth, maxHealth);
		break;

	case 2: // Top
		barX = rect.x;
		barY = rect.y - rect.height / 4 - 10;
		barWidth = rect.width;
		barHeight = 4;
		DrawHorizontalHealthBar(barX, barY, barWidth, barHeight, currentHealth, maxHealth);
		break;

	case 3: // Bottom
		barX = rect.x;
		barY = rect.y + rect.height + rect.height / 4;
		barWidth = rect.width;
		barHeight = 4;
		DrawHorizontalHealthBar(barX, barY, barWidth, barHeight, currentHealth, maxHealth);
		break;
	}
}

// Health bar with position and offsets (igual ao projeto de refer?ncia)
void DrawRoundHealthBarWithPosition(const Rect& rect, float maxHealth, float currentHealth, int position, float offsetY = 0.0f, float offsetX = 0.0f) {
	int healthColor = -16711936;
	float currentHealthWidth = (currentHealth * rect.height) / maxHealth;
	float maxHealthWidth = (maxHealth * rect.height) / maxHealth;

	if (currentHealth <= (maxHealth * 0.33)) {
		healthColor = -65536; // Vermelho
	}
	else if (currentHealth <= (maxHealth * 0.66)) {
		healthColor = -256; // Amarelo
	}
	else {
		healthColor = -16711936; // Verde
	}

	float barX = rect.x - rect.width / 4.0f;
	float barY = rect.y;
	float barWidth = rect.width / 10.0f;
	float barHeight = maxHealthWidth;

	switch (position) {
	case 0: // Left - barra vertical ? esquerda (padr?o)
		barX += offsetX;
		barY += offsetY;
		DrawRect(barX, barY, barWidth, maxHealthWidth, -16777216, 1, false);
		DrawRect(barX, barY + rect.height - currentHealthWidth, barWidth, currentHealthWidth, healthColor, 1, true);
		return;
	case 1: // Right - barra vertical ? direita
		barX = rect.x + rect.width + 2.0f + offsetX;
		barY = rect.y + offsetY;
		DrawRect(barX, barY, barWidth, maxHealthWidth, -16777216, 1, false);
		DrawRect(barX, barY + rect.height - currentHealthWidth, barWidth, currentHealthWidth, healthColor, 1, true);
		return;
	case 2: // Top - barra horizontal no topo
		{
			barWidth = rect.width + 5.0f;
			barHeight = 5.0f;
			barX = rect.x - 2.0f;
			barY = rect.y - barHeight - 2.0f + offsetY;
			float filledWidth = barWidth * (currentHealth / maxHealth);
			DrawRect(barX, barY, barWidth, barHeight, -16777216, 1, false);
			DrawRect(barX, barY, filledWidth, barHeight, healthColor, 1, true);
			return;
		}
	case 3: // Bottom - barra horizontal embaixo
	default:
		{
			barWidth = rect.width + 5.0f;
			barHeight = 5.0f;
			barX = rect.x - 2.0f;
			barY = rect.y + rect.height + 2.0f + offsetY;
			float filledWidth = barWidth * (currentHealth / maxHealth);
			DrawRect(barX, barY, barWidth, barHeight, -16777216, 1, false);
			DrawRect(barX, barY, filledWidth, barHeight, healthColor, 1, true);
			return;
		}
	}
}

void DrawCircle(float x, float y, float radius, float thickness, float r, float g, float b, float a, bool filled)
{
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	if (filled) target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), solid_brush);
	else target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), solid_brush, thickness);
}

void DrawEllipse(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled)
{
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	if (filled) target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), width, height), solid_brush);
	else target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), width, height), solid_brush, thickness);
}

// Implementa��es das fun��es do overlay inteligente
static HWND s_altForegroundWindow = nullptr;

// Verifica��o robusta: desenha se target OU janela alternativa est� em foco, ou se fg � filho/raiz do target
inline bool IsTargetOrAltInForeground() {
	HWND fg = GetForegroundWindow();
	if (!fg) return true;  // fallback: desenhar se n?o conseguir detectar
	if (fg == targetWindow) return true;
	if (s_altForegroundWindow && fg == s_altForegroundWindow) return true;  // menu em foco
	if (targetWindow && IsChild(targetWindow, fg)) return true;  // fg ? filho do emulador
	HWND fgRoot = GetAncestor(fg, GA_ROOT);
	HWND targetRoot = GetAncestor(targetWindow, GA_ROOT);
	if (fgRoot && targetRoot && fgRoot == targetRoot) return true;  // mesma janela raiz
	return false;
}

void d2oSetup(HWND tWindow) {
	targetWindow = tWindow;
	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = AY_OBFUSCATE("d2do");
	RegisterClass(&wc);
	overlayWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		wc.lpszClassName, AY_OBFUSCATE("Overlay"), WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, wc.hInstance, NULL);

	MARGINS mar = { -1 };
	DwmExtendFrameIntoClientArea(overlayWindow, &mar);
	D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &factory);
	factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)),
		D2D1::HwndRenderTargetProperties(overlayWindow, D2D1::SizeU(200, 200),
			D2D1_PRESENT_OPTIONS_IMMEDIATELY), &target);
	target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &solid_brush);
	target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&w_factory));
	w_factory->CreateTextFormat(fontname.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, AY_OBFUSCATE(L"en-us"), &w_format);
}

void mainLoop() {
	MSG message;
	message.message = WM_NULL;
	ShowWindow(overlayWindow, 1);
	UpdateWindow(overlayWindow);
	SetLayeredWindowAttributes(overlayWindow, RGB(0, 0, 0), 255, LWA_ALPHA);
	static bool lastStreamMode = false;
	if (StreamProof != lastStreamMode && overlayWindow != nullptr) {
		UINT affinity = StreamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
		SetWindowDisplayAffinity(overlayWindow, affinity);
		lastStreamMode = StreamProof;
	}

	if (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, overlayWindow, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		UpdateWindow(overlayWindow);
		WINDOWINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		GetWindowInfo(targetWindow, &info);
		D2D1_SIZE_U siz;
		siz.height = ((info.rcClient.bottom) - (info.rcClient.top));
		siz.width = ((info.rcClient.right) - (info.rcClient.left));

		if (!IsIconic(overlayWindow)) {
			SetWindowPos(overlayWindow, NULL, info.rcClient.left, info.rcClient.top, siz.width, siz.height, SWP_SHOWWINDOW);
			target->Resize(&siz);
		}
		target->BeginDraw();
		target->Clear(D2D1::ColorF(0, 0, 0, 0));
		if (drawLoopCallback != NULL) {
			if (o_Foreground) {
				if (IsTargetOrAltInForeground())
					goto toDraw;
				else goto noDraw;
			}

		toDraw:
			time_t postTime = clock();
			time_t frameTime = postTime - preTime;
			preTime = postTime;

			if (o_DrawFPS) {
				if (postTime - showTime > 100) {
					fps = 1000 / (float)frameTime;
					showTime = postTime;
				}
				DrawString(std::to_string(fps), 20, siz.width - 50, 0, 0, 1, 0);
			}

			if (o_VSync) {
				int pausetime = 17 - frameTime;  // 17ms = ~60 FPS
				if (pausetime > 0 && pausetime < 30) {
					Sleep(pausetime);
				}
			}

			if (o_VSync90) {
				int pausetime = 11 - frameTime;  // 11ms = ~90 FPS (1000ms / 90 = 11.11ms)
				if (pausetime > 0 && pausetime < 30) {
					Sleep(pausetime);
				}
			}

			drawLoopCallback(siz.width, siz.height);
		}
	noDraw:
		target->EndDraw();
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uiMessage)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uiMessage, wParam, lParam);
	}
	return 0;
}

void DirectOverlaySetAlternateForegroundWindow(HWND h) { s_altForegroundWindow = h; }

void DirectOverlaySetRequireForeground(bool enable) { o_Foreground = enable; }

bool DirectOverlayIsContentVisible() {
	if (!o_Foreground) return true;  // op??o desativada: sempre vis?vel
	return IsTargetOrAltInForeground();
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == GetCurrentProcessId())
	{
		enumWindow = hwnd;
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI OverlayThread(LPVOID lpParam)
{
	if (lpParam == NULL) {
		EnumWindows(EnumWindowsProc, NULL);
	}
	else {
		enumWindow = (HWND)lpParam;
	}
	d2oSetup(enumWindow);
	for (;;) {
		mainLoop();
	}
}

void DirectOverlaySetup(DirectOverlayCallback callback) {
	drawLoopCallback = callback;
	CreateThread(0, 0, OverlayThread, NULL, 0, NULL);
}

void DirectOverlaySetup(DirectOverlayCallback callback, HWND _targetWindow) {
	drawLoopCallback = callback;
	std::thread(OverlayThread, _targetWindow).detach();
	// CreateThread(0, 0, OverlayThread, _targetWindow, 0, NULL);
}

void DirectOverlaySetOption(DWORD option) {
	if (option & D2DOV_REQUIRE_FOREGROUND) o_Foreground = true;
	if (option & D2DOV_DRAW_FPS) o_DrawFPS = true;
	if (option & D2DOV_VSYNC) o_VSync = true;
	if (option & D2DOV_VSYNC_90) o_VSync90 = true;
	if (option & D2DOV_FONT_ARIAL) fontname = AY_OBFUSCATE(L"arial");
	if (option & D2DOV_FONT_COURIER) fontname = AY_OBFUSCATE(L"Courier");
	if (option & D2DOV_FONT_CALIBRI) fontname = AY_OBFUSCATE(L"Calibri");
	if (option & D2DOV_FONT_GABRIOLA) fontname = AY_OBFUSCATE(L"Gabriola");
	if (option & D2DOV_FONT_IMPACT) fontname = AY_OBFUSCATE(L"Impact");
}
