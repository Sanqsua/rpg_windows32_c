//includes
#pragma warning(push, 3)

#include <stdio.h>

#include <windows.h>

#include <Psapi.h>

#include <emmintrin.h>

#pragma warning(pop)

#include <stdint.h>

#include "Main.h"


//GlobalVariables
HWND gGameWindow;

BOOL gGameIsRunning;

GAMEBITMAP gBackBuffer;

GAMEPERFDATA gPerformanceData;

PLAYER gPlayer;

int __stdcall WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, PSTR CommandLine, INT CmdShow)
 {

    // Setup variables

    UNREFERENCED_PARAMETER(Instance);

    UNREFERENCED_PARAMETER(PreviousInstance);

    UNREFERENCED_PARAMETER(CommandLine);

    UNREFERENCED_PARAMETER(CmdShow);

    MSG Message = { 0 };

    int64_t FrameStart = 0;

    int64_t FrameEnd = 0;

    int64_t ElapsedMicroseconds;

    int64_t ElapsedMicrosecondsAccumulatorRaw = 0;

    int64_t ElapsedMicrosecondsAccumulatorCooked = 0;

    HMODULE NtDllModuleHandle;

    if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll"))== NULL)
    {
        MessageBoxA(NULL, "Couldn't load ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);
      
        goto Exit;
    }

    if ((NtQueryTimerResolution = (_NtQueryTimerResolution) GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution"))== NULL)
    {
        MessageBoxA(NULL, "Couldn't get address of NtQueryTimerResolution funtion in ntdll.dll","Error", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    NtQueryTimerResolution(&gPerformanceData.MinimumTimerResolution, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);
    
    GetSystemInfo(&gPerformanceData.SystemInfo);

    GetSystemTimeAsFileTime((FILETIME*)&gPerformanceData.CurrentSystemTime);

    if (GameIsAlreadyRunning() == TRUE)
    {
        MessageBoxA(NULL, "Another instance of this program is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        goto Exit;
    }

    QueryPerformanceFrequency((LARGE_INTEGER*)&gPerformanceData.PerfFrequency);

    gPerformanceData.DisplayDebugInfo = TRUE;

    gBackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitmapInfo.bmiHeader);

    gBackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;

    gBackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;

    gBackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;

    gBackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;

    gBackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;

    gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (gBackBuffer.Memory == NULL)
    {
        MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY_SIZE);



    // GAMELoop

    gPlayer.WorldPosX = 25;
    
    gPlayer.WorldPosY = 25;

    gGameIsRunning = TRUE;

    while (gGameIsRunning == TRUE)
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);

        while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE))
        {
            DispatchMessageA(&Message);
        }

        ProcessPlayerInput();

        RenderFrameGraphics();

        //CALCULATE TIMING
        QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

        ElapsedMicroseconds = FrameEnd - FrameStart;

        ElapsedMicroseconds *= 1000000;

        ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

        gPerformanceData.TotalFramesRendered++;

        ElapsedMicrosecondsAccumulatorRaw += ElapsedMicroseconds;

        while (ElapsedMicroseconds < TARGET_MICROSECONDS_PER_FRAME)
        {
           

            ElapsedMicroseconds = FrameEnd - FrameStart;

            ElapsedMicroseconds *= 1000000;

            ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

            QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

            if (ElapsedMicroseconds < ((int64_t )TARGET_MICROSECONDS_PER_FRAME -( (gPerformanceData.CurrentTimerResolution*0.1f))*5))
            {
                Sleep(1); // Could be anywhere from 1ms to a full system timer tick? (~15.625ms)
            }
        }

        ElapsedMicrosecondsAccumulatorCooked += ElapsedMicroseconds;


        //every 120 frames do debug stuff
        if ((gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES) == 0)
        {
            //CPU CALC -------------------------------------------------

            GetProcessTimes(GetCurrentProcess(),
                &gPerformanceData.ProcessCreationTime,
                &gPerformanceData.ProcessExitTime,
                (FILETIME*)&gPerformanceData.CurrentKernelCPUTime,
                (FILETIME*)&gPerformanceData.CurrentUserCPUTime
            );

            gPerformanceData.CPUPercent =( gPerformanceData.CurrentKernelCPUTime - gPerformanceData.PreviousKernelCPUTime) +\
                (gPerformanceData.CurrentUserCPUTime - gPerformanceData.PreviousUserCPUTime);

            gPerformanceData.CPUPercent /= gPerformanceData.CurrentSystemTime - gPerformanceData.PreviousSystemTime;

            gPerformanceData.CPUPercent /= gPerformanceData.SystemInfo.dwNumberOfProcessors;

            gPerformanceData.CPUPercent *= 100;

            //CPU CALC -------------------------------------------------

            GetProcessHandleCount(GetCurrentProcess(NULL), &gPerformanceData.HandleCount);

            K32GetProcessMemoryInfo(GetCurrentProcess(NULL),(PPROCESS_MEMORY_COUNTERS_EX*) &gPerformanceData.MemInfo, sizeof(gPerformanceData.MemInfo));
            
            gPerformanceData.RawFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

            gPerformanceData.CookedFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

            ElapsedMicrosecondsAccumulatorRaw = 0;

            ElapsedMicrosecondsAccumulatorCooked = 0;
      
            gPerformanceData.PreviousKernelCPUTime = gPerformanceData.CurrentKernelCPUTime;

            gPerformanceData.PreviousUserCPUTime = gPerformanceData.CurrentUserCPUTime;

            gPerformanceData.PreviousSystemTime = gPerformanceData.CurrentSystemTime;
        }
    }

Exit:

    return(0);
}

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_CLOSE:
    {
        gGameIsRunning = FALSE;

        PostQuitMessage(0);

        break;
    }
    default:
    {
        Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
    }
    }

    return(Result);
}

DWORD CreateMainGameWindow(void)
{
    
    DWORD Result = ERROR_SUCCESS;

    WNDCLASSEXA WindowClass = { 0 };
  
    WindowClass.cbSize = sizeof(WNDCLASSEXA);

    WindowClass.style = 0;

    WindowClass.lpfnWndProc = MainWindowProc;

    WindowClass.cbClsExtra = 0;

    WindowClass.cbWndExtra = 0;

    WindowClass.hInstance = GetModuleHandleA(NULL);

    WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);

    WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);

    WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);

    WindowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));

    WindowClass.lpszMenuName = NULL;

    WindowClass.lpszClassName = GAME_NAME "_WINDOWCLASS";


    if (RegisterClassExA(&WindowClass) == 0)
    {
        Result = GetLastError();

        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    gGameWindow = CreateWindowExA(0, WindowClass.lpszClassName, "Window Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandleA(NULL), NULL);

    if (gGameWindow == NULL)
    {
        Result = GetLastError();

        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);

    if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0)
    {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;
        goto Exit;
    } 
    //calc the Monitorwidth
     gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;

     gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW)== 0)
    {
        Result = GetLastError();
        goto Exit;
    }
    
    if (SetWindowPos(
        gGameWindow, 
        HWND_TOP, 
        gPerformanceData.MonitorInfo.rcMonitor.left, 
        gPerformanceData.MonitorInfo.rcMonitor.top, 
        gPerformanceData.MonitorWidth, 
        gPerformanceData.MonitorHeight, 
        SWP_FRAMECHANGED)) 
    {
        Result = GetLastError();
        goto Exit;
    }
Exit:

    return(Result);
}

BOOL GameIsAlreadyRunning(void)
{
    HANDLE Mutex = NULL;

    Mutex = CreateMutexA(NULL, FALSE, GAME_NAME "_GameMutex");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

void ProcessPlayerInput(void)
{
    int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    
    int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F1);
    
    int16_t LeftKeyIsDown = GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('A');

    int16_t RightKeyIsDown= GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('D');

    int16_t UpKeyIsDown = GetAsyncKeyState(VK_UP)| GetAsyncKeyState('W');

    int16_t DownKeyIsDown = GetAsyncKeyState(VK_DOWN) | GetAsyncKeyState('S');
    static int16_t DebugKeyWasDown;
    
    static int16_t LeftKeyWasDown;
    
    static int16_t RightKeyWasDown;
    
    static int16_t DownKeyWasDown;
    
    static int16_t UpKeyWasDown;
    
    if (EscapeKeyIsDown)
    {
        SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
    }
    if (DebugKeyIsDown && !DebugKeyWasDown)
    {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }

    if (LeftKeyIsDown)
    {
        if (gPlayer.WorldPosX > 0) 
        {
            gPlayer.WorldPosX -= 1;

        }
        
    }

    if (RightKeyIsDown)
    {
        if (gPlayer.WorldPosX < GAME_RES_WIDTH - 16) {
            gPlayer.WorldPosX += 1;
        }
    }

    if (DownKeyIsDown)
    {
        if (gPlayer.WorldPosY < GAME_RES_HEIGHT -16) {
            gPlayer.WorldPosY += 1;

        }
    }

    if (UpKeyIsDown)
    {
        if (gPlayer.WorldPosY > 0) {
            gPlayer.WorldPosY -= 1;

        }
    }
    DebugKeyWasDown = DebugKeyIsDown;
    LeftKeyWasDown = LeftKeyIsDown;
    RightKeyWasDown = RightKeyIsDown;
    UpKeyWasDown = UpKeyIsDown;
    DownKeyWasDown = DownKeyIsDown;
}


void RenderFrameGraphics(void)
{
#ifdef SIMD
    __m128i QuadPixel= { 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff };
    ClearScreen(&QuadPixel);
#else
    PIXEL32 Pixel = { 0 };
    Pixel.Blue = 0x7f;
    Pixel.Blue = 0x00;
    Pixel.Blue = 0x00;
    Pixel.Blue = 0xFF;
    ClearScreen2((PIXEL32*)&Pixel);
#endif

    int32_t ScreenX = gPlayer.WorldPosX;

    int32_t ScreenY = gPlayer.WorldPosY;

    int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - \
        (GAME_RES_WIDTH * ScreenY) + ScreenX;

    for (int32_t y = 0; y < 16; y++)
    {
        for (int32_t x = 0; x < 16; x++)
        {
            memset((PIXEL32*)gBackBuffer.Memory + (uintptr_t)StartingScreenPixel + x - ((uintptr_t)GAME_RES_WIDTH * y), 0xFF, sizeof(PIXEL32));
        }
    }

  
    HDC DeviceContext = GetDC(gGameWindow); //get a handle to device context
    //DeviceIndependent
    StretchDIBits(DeviceContext,
        0,
        0,
        gPerformanceData.MonitorWidth,
        gPerformanceData.MonitorHeight ,
        0, 
        0, 
        GAME_RES_WIDTH, 
        GAME_RES_HEIGHT, 
        gBackBuffer.Memory, 
        &gBackBuffer.BitmapInfo, 
        DIB_RGB_COLORS, 
        SRCCOPY);


    //Debuginfo


    if (gPerformanceData.DisplayDebugInfo == TRUE) {
 
        SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));
        
        char DebugTextBuffer[64] = { 0 };

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Raw:    %.01f", gPerformanceData.RawFPSAverage);

        TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "FPS Cooked: %.01f", gPerformanceData.CookedFPSAverage);

        TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Min. Timer Res: %.01f", gPerformanceData.MinimumTimerResolution / 10000.0f);

        TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Max. Timer Res: %.01f", gPerformanceData.MaximumTimerResolution / 10000.0f);

        TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Cur. Timer Res: %.01f", gPerformanceData.CurrentTimerResolution / 10000.0f);

        TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Handles: %lu", gPerformanceData.HandleCount);

        TextOutA(DeviceContext, 0, 65, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "Memory: %lu Kb", gPerformanceData.MemInfo.PrivateUsage/1024);

        TextOutA(DeviceContext, 0, 78, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, sizeof(DebugTextBuffer), "CPU: %.02f%%", gPerformanceData.CPUPercent);

        TextOutA(DeviceContext, 0, 91, DebugTextBuffer, (int)strlen(DebugTextBuffer));
    } 
    

    
    ReleaseDC(gGameWindow, DeviceContext);


}
#ifdef SIMD
__forceinline void ClearScreen(_In_ __m128i* Color)
{
    for (int i = 0; i < GAME_RES_WIDTH * GAME_RES_HEIGHT; i += 4)
    {
        _mm_store_si128((PIXEL32*)gBackBuffer.Memory + i,*Color);
    }
}
#else
__forceinline void ClearScreen2(_In_ PIXEL32* Pixel)
{
    for (int i = 0; i < GAME_RES_WIDTH * GAME_RES_HEIGHT; i++)
    {
        memcpy((PIXEL32*)gBackBuffer.Memory + i, Pixel,sizeof(PIXEL32));
    } 
}
#endif
