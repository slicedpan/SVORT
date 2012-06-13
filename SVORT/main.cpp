#include "Engine.h"
#include "WindowSettings.h"

int main(int argc, char** argv)
{
	WindowSettings w;
	w.Width = 1000;
	w.Height = 600;
	w.Title = "SVO Ray Tracer";
	Engine engine(w);
	engine.Run();
	return 0;
}