#include "Engine.h"
#include "WindowSettings.h"

int main(int argc, char** argv)
{
	WindowSettings w;
	w.Width = 1024;
	w.Height = 768;
	//w.Fullscreen = true;
	w.Title = "SVO Ray Tracer";
	Engine engine(w);
	engine.Run();
	return 0;
}