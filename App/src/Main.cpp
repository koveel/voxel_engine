#include "pch.h"

#include "App.h"

using namespace Engine;

void testbed_start(App&);
void testbed_update(App&);
void testbed_stop(App&);
void testbed_event(Event&);

int main()
{
	App* app = new App("Engine", 1280, 800);
	app->Hooks = {
		testbed_start, testbed_update, testbed_stop, testbed_event,
	};
	app->run();

	delete app;
}