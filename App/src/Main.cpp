#include "pch.h"

#include "App.h"

using namespace Engine;

int main()
{
	App* app = new App("Engine", 1280, 800);
	app->run();

	delete app;
}