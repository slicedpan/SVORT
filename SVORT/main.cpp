#include "Engine.h"
#include "WindowSettings.h"

int main(int argc, char** argv)
{
	WindowSettings w;
	w.Width = 1200;
	w.Height = 600;
	//w.Fullscreen = true;
	w.Title = "SVO Ray Tracer";
	Engine engine(w);
	engine.Run();
	return 0;
}