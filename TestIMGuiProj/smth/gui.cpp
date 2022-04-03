#include <codecvt>
#include <string>
#include <iostream>

#include "gui.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

void setDigButtons();
void paintCenterText(std::string text);
void paintFullWidthInputText(char* label, char* buf, size_t buf_size);
void fillStrWithNulls(char* str);

int defFontSize = 20;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;

	// Два нижних блока нужны, чтобы проект собирался с юникодовской кодировкой

	std::string s = "class001";
	std::wstring stemp = std::wstring(s.begin(), s.end());
	LPCWSTR sw = stemp.c_str();

	std::string windowNameStr = windowName;
	std::wstring windowNameWStr = std::wstring(windowNameStr.begin(), windowNameStr.end());
	LPCWSTR windowNameLPCWSTR = windowNameWStr.c_str();

	windowClass.lpszClassName = sw;
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		sw,
		windowNameLPCWSTR,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", defFontSize, NULL, io.Fonts->GetGlyphRangesCyrillic());
	// Последние два аргумента в функции выше необходимы для работы русского языка
	// Два первых агрумента отвечают за шрифт и его размер

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}
char mainInputFieldBuf[100] = "";					/* Почему это глобальная переменная? 
													Потому что прорисовка покадровая
													(попробуйте ее засунуть в начало метода Render()
													и сразу поймете) */

ImVec2 defCalcDigBtnSize(defFontSize + 10, defFontSize + 10);
char firstOperand[100] = "";
char secOperand[100] = "";
bool isFirstOperand = true;
bool isOperatorAlrExist = false;
bool isComAlrExist = false;
char op = '\0';
double calcResult = 0.0;
unsigned int firstOperatorLen = 0;
unsigned int secOperatorLen = 0;

void gui::Render() noexcept							// Здесь происходит ПОКАДРОВАЯ отрисовка
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		(char*)u8"Калькулятор",						/* Тут название окна ImGui, НЕ виндового окна 
													(название виндового окна задавать в main.cpp) */
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	// Писать код сюда
	paintCenterText((char*)u8"Привет, мир!");		/* Пример, как надо использовать русский язык
													   в ImGui, т.е. перед строкой ставим u8 и 
													   ОБЯЗАТЕЛЬНО приводим к char* */
	paintFullWidthInputText(
		(char*)"##input1",							// "##" означает невидимость подписи InputText
		mainInputFieldBuf, 
		IM_ARRAYSIZE(mainInputFieldBuf));	

	setDigButtons(); 

	ImGui::End();
}

// Вспомогательные функции

void setDigButtons()
{
	if (ImGui::Button("1", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "1");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "1");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "1");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();								// Расположение следующего элемента на той же линии,
													// Можете убрать и посмотреть, что будет
	if (ImGui::Button("2", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "2");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "2");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "2");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("3", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "3");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "3");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "3");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("+", defCalcDigBtnSize))
	{
		if (!isOperatorAlrExist)
		{
			mainInputFieldBuf[0] = '\0';
			op = '+';
			isOperatorAlrExist = true;
			isFirstOperand = false;
			isComAlrExist = false;
		}
	}

	if (ImGui::Button("4", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "4");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "4");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "4");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("5", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "5");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "5");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "5");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("6", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "6");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "6");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "6");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("-", defCalcDigBtnSize))
	{
		if (!isOperatorAlrExist)
		{
			mainInputFieldBuf[0] = '\0';
			op = '-';
			isOperatorAlrExist = true;
			isFirstOperand = false;
			isComAlrExist = false;
		}
	}

	if (ImGui::Button("7", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "7");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "7");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "7");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("8", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "8");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "8");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "8");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("9", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "9");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "9");
			firstOperatorLen++;
		}
		else
		{
			strcat_s(secOperand, "9");
			secOperatorLen++;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("*", defCalcDigBtnSize))
	{
		if (!isOperatorAlrExist)
		{
			mainInputFieldBuf[0] = '\0';
			op = '*';
			isOperatorAlrExist = true;
			isFirstOperand = false;
			isComAlrExist = false;
		}
	}

	if (ImGui::Button(".", defCalcDigBtnSize))
	{
		if (!isComAlrExist)
		{
			strcat_s(mainInputFieldBuf, ".");
			isComAlrExist = true;
			if (isFirstOperand)
			{
				strcat_s(firstOperand, ".");
				firstOperatorLen++;
			}
			else
			{
				strcat_s(secOperand, ".");
				secOperatorLen++;
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("0", defCalcDigBtnSize))
	{
		strcat_s(mainInputFieldBuf, "0");
		if (isFirstOperand)
		{
			strcat_s(firstOperand, "0");
		}
		else
		{
			strcat_s(secOperand, "0");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("=", defCalcDigBtnSize))
	{
		isOperatorAlrExist = false;
		isFirstOperand = true;
		isComAlrExist = false;

		std::string firstOperandStr(firstOperand);
		std::string secOperandStr(secOperand);
		double firstOperandDouble = std::stod(firstOperandStr);
		double secOperandDouble = std::stod(secOperandStr);

		switch (op)
		{
		case '+':
			calcResult = firstOperandDouble + secOperandDouble;
			break;
		case '-':
			calcResult = firstOperandDouble - secOperandDouble;
			break;
		case '*':
			calcResult = firstOperandDouble * secOperandDouble;
			break;
		case '/':
			calcResult = firstOperandDouble / secOperandDouble;
			break;
		}

		std::string resStr = std::to_string(calcResult);
		const char* cResStr = resStr.c_str();
		strcpy_s(mainInputFieldBuf, cResStr);

		fillStrWithNulls(firstOperand);
		fillStrWithNulls(secOperand);
		//firstOperand[0] = '\0';
		strcpy_s(firstOperand, cResStr);
		secOperand[0] = '\0';
		firstOperatorLen = 0;
		secOperatorLen = 0;
		isFirstOperand = true;
		isOperatorAlrExist = false;
		isComAlrExist = true;
		op = '\0';
		calcResult = 0.0;
	}
	ImGui::SameLine();
	if (ImGui::Button("/", defCalcDigBtnSize))
	{
		if (!isOperatorAlrExist)
		{
			mainInputFieldBuf[0] = '\0';
			op = '/';
			isOperatorAlrExist = true;
			isFirstOperand = false;
			isComAlrExist = false;
		}
	}
}

void fillStrWithNulls(char* str)
{
	for (int i = 0; i < strlen(str); ++i)
	{
		str[i] = '0';
	}
}

void paintCenterText(std::string text)				// Если задать в параметрах char* вместо string,
													// то не работает, надо потом переделать
{
	const char* c_text = text.c_str();				/* Важная строчка — здесь идет перевод из string
													в const char *, т.е. в "C-строку" */
	float width = ImGui::GetWindowWidth();
	ImVec2 calc = ImGui::CalcTextSize(c_text);
	ImGui::SetCursorPosX(width / 2 - calc.x / 2);
	ImGui::Text(c_text);
}

void paintFullWidthInputText(char* label, char* buf, size_t buf_size)
{
	float width = ImGui::GetWindowWidth();
	ImGui::PushItemWidth(width);
	ImGui::InputText(label, buf, buf_size);
	ImGui::PopItemWidth();
}
