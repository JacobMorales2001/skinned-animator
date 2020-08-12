#include "GraphicsApplication.hpp"




//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow, MRenderer::GraphicsApplication& g_gApp);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
//void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    MRenderer::GraphicsApplication g_gApp(800, 600);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_gApp.m_aspectRatio = static_cast<float>(g_gApp.m_windowWidth) / static_cast<float>(g_gApp.m_windowHeight);
    if (FAILED(InitWindow(hInstance, nCmdShow, g_gApp)))
        return 0;

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_gApp.Update();
            g_gApp.Render();
        }
    }

    g_gApp.CleanupDevice();

    return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow, MRenderer::GraphicsApplication& g_gApp)
{
    //TODO: Allow Resizing in the creation of the window

    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, NULL);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"GraphicsIIClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, NULL);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    g_gApp.m_instance = hInstance;
    RECT rc = { 0, 0, g_gApp.m_windowWidth, g_gApp.m_windowHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_gApp.m_window = CreateWindow(L"GraphicsIIClass", L"Direct3D 12 Graphics Project",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
        nullptr);
    if (!g_gApp.m_window)
        return E_FAIL;

    if (FAILED(g_gApp.InitializeDevice()))
    {
        g_gApp.CleanupDevice();
        return 0;
    }

    ShowWindow(g_gApp.m_window, nCmdShow);

    return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        //TODO: Handle resizing
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}