#include "App.h"
#include <memory>
#include "Cmd.h"
#include "Font.h"
#include "Screen.h"
#include "Tray.h"
#include "WinMax.h"
#include "WinPin.h"

namespace {
	std::shared_ptr<App> app;
}
App::~App()
{
}
App::App()
{
}
void App::Init(HINSTANCE instance, std::wstring&& cmd)
{
	app = std::shared_ptr<App>{new App()};
	app->instance = instance;
	Cmd::Init(cmd);
	Tray::Init();
	Font::Init();
	Screen::Init();
	WinMax::Init();
}

App* App::Get()
{
	return app.get();
}

int App::GetExitCode()
{
	return 0;
}

