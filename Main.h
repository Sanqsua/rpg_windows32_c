#pragma once
#define GAME_NAME "GAME_B"

#define GAME_RES_WIDTH 384

#define GAME_RES_HEIGHT 240

#define GAME_BPP 32

#define GAME_DRAWING_AREA_MEMORY_SIZE (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP/8)) //each frame is gonna occupy that much memory

#define CALCULATE_AVG_FPS_EVERY_X_FRAMES  100
#include <stdint.h>

#pragma warning(disable: 4820)
#pragma warning(disable: 5045)

typedef struct GAMEBITMAP
{
	
	BITMAPINFO BitmapInfo;
	void* Memory;
}GAMEBITMAP;

typedef struct PIXEL32
{
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;

}PIXEL32;

typedef struct GAMEPERFDATA
{
	uint64_t TotalFramesRendered;

	uint32_t FramesPerSecondAverage;

	uint32_t RawFramesPerSecond;
	
	MONITORINFO MonitorInfo;
	
	int32_t MonitorWidth;
	
	int32_t MonitorHeight;

}GAMEPERFDATA;

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);    // second message parameter
DWORD CreateMainGameWindow(void);
BOOL GameIsAlreadyRunning(void);

void ProcessPlayerInput(void);
void RenderFrameGraphics(void);
