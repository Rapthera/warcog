#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <process.h>

#include <ks.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#include "../main.h"
#include "../input.h"
#include "../config.h"
#include "../util.h"
#include "../audio.h"

#include <assert.h>

const GUID IID_IMMDeviceEnumerator={0xa95664d2,0x9614,0x4f35,{0xa7,0x46,0xde,0x8d,0xb6,0x36,0x17,0xe6}};
const CLSID CLSID_MMDeviceEnumerator={0xbcde0395,0xe52f,0x467c,{0x8e,0x3d,0xc4,0x57,0x92,0x91,0x69,46}};
const GUID IID_IAudioClient = {0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2}};
const GUID IID_IAudioRenderClient={0xf294acfc,0x3146,0x4483,{0xa7,0xbf,0xad,0xdc,0xa7,0xc2,0x60,0xe2}};
const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {0x3, 0, 0x10, {0x80,0, 0,0xaa,0,0x38,0x9b,0x71}};

#include "wgl.h"

static const int ctx_attribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0
};

static BOOL (WINAPI*wglSwapIntervalEXT)(int interval);

static int sock;
static HWND hwnd;

static volatile bool _redraw, done;
static bool clipcursor, init_done;

typedef union {
    struct {
        uint8_t id, v8;
        uint16_t v16;
        uint32_t value;
    };
    uint64_t v;
} msg_t;

static TRACKMOUSEEVENT tme = {
    sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0
};

static bool tracking;

_Static_assert(sizeof(WPARAM) >= 8, "wparam size");
void postmessage(uint8_t id, uint8_t v8, uint16_t v16, uint32_t value, void *data)
{
    msg_t m = {{id, v8, v16, value}};
    PostMessage(hwnd, WM_USER, m.v, (LPARAM)data);
}

int send2(const void *data, int len)
{
    int res;

    res = send(sock, data, len, 0);
    if (res != len)
        printf("send() returned %i (%i)\n", res, len);

    return res;
}

void thread(void (func)(void*), void *args)
{
    if (_beginthread(func, 0, args) < 0)
        printf("_beginthread() failed\n");
}

void quit(void)
{
    done = 1;
}

static bool linkgl(void)
{
#include "wgl_link.h"
	wglSwapIntervalEXT = (void*)wglGetProcAddress("wglSwapIntervalEXT");

    return 1;
}

static void loadconfig(config_t *cfg)
{
    char *cfg_str;
    bool res;

    cfg_str = file_text("config.txt");
    if (!cfg_str)
        printf("failed to load config file, using defaults\n");

    res = cfg_init(cfg, cfg_str);
    free(cfg_str);
    if (!res) {
        printf("invalid config file, using defaults\n");
        cfg_init(cfg, NULL);
    }

    printf("cfg.name %s\n", cfg->name);
    printf("cfg.dlrate %u\n", cfg->dlrate);
}

static void update_clip(HWND hwnd)
{
    RECT r;
    POINT p = {0};

    GetClientRect(hwnd, &r);
    ClientToScreen(hwnd, &p);

    r.left += p.x;
    r.right += p.x;
    r.top += p.y;
    r.bottom += p.y;
    ClipCursor(&r);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x, y, w, h;
    msg_t m;

    x = GET_X_LPARAM(lparam);
    y = GET_Y_LPARAM(lparam);
    w = LOWORD(lparam);
    h = HIWORD(lparam);

    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (init_done)
            do_resize(w, h);
        break;
    case WM_MOVE:
        break;
    case WM_EXITSIZEMOVE:
        break;
    case WM_MOUSELEAVE:
        tracking = 0;
        break;
    case WM_MOUSEMOVE:
        if (clipcursor)
            update_clip(hwnd); //TODO: shouldnt be here

        if (!tracking) {
            TrackMouseEvent(&tme);
            tracking = 1;
        }
        input_mmove(&input, x, y, 0);
        break;
    case WM_LBUTTONDOWN:
        input_mdown(&input, x, y, 0, 0);
        break;
    case WM_LBUTTONUP:
        input_mup(&input, x, y, 0, 0);
        break;
    case WM_RBUTTONDOWN:
        input_mdown(&input, x, y, 0, 2);
        break;
    case WM_RBUTTONUP:
        input_mup(&input, x, y, 0, 2);
        break;
    case WM_MBUTTONDOWN:
        input_mdown(&input, x, y, 0, 1);
        break;
    case WM_MBUTTONUP:
        input_mup(&input, x, y, 0, 1);
        break;
    case WM_MOUSEWHEEL:
        input_mwheel(&input, (double) (int16_t)HIWORD(wparam) / 120.0);
        break;
    case WM_KEYDOWN:
        if (wparam == VK_F1) {
            if (!clipcursor)
                update_clip(hwnd);
            else
                ClipCursor(0);
            clipcursor = !clipcursor;
            break;
        }
        input_keydown(&input, wparam, wparam);
        break;
    case WM_KEYUP:
        input_keyup(&input, wparam);
        break;
    case WM_USER:
        m.v = wparam;
        do_msg(m.id, m.v8, m.v16, m.value, (void*)lparam);
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    return 0;
}


#define release(x) (x)->lpVtbl->Release(x)
#define check(x) if (FAILED(x)) goto failure

void audio_thread(void *args)
{
    HRESULT hr;
    HANDLE event = 0;
    WAVEFORMATEX *fmt;
    IMMDeviceEnumerator *penum;
    IMMDevice *pdev;
    IAudioClient *acl = 0;
    IAudioRenderClient *rcl = 0;
    uint32_t padding, buf_size;
    REFERENCE_TIME defp, minp;
    //sample_t buf[audio_buffersize];
    uint8_t *data;

    CoInitialize(0);

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                          (void**)&penum);
    check(hr);

    hr = penum->lpVtbl->GetDefaultAudioEndpoint(penum, eRender, eConsole, &pdev);
    release(penum);

    check(hr);

    hr = pdev->lpVtbl->Activate(pdev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&acl);
    release(pdev);
    check(hr);

    hr = acl->lpVtbl->GetMixFormat(acl, &fmt);
    check(hr);

    if (fmt->wFormatTag != WAVE_FORMAT_EXTENSIBLE || fmt->nChannels != 2 ||
        fmt->nSamplesPerSec != 44100 ||
        memcmp(&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, &((WAVEFORMATEXTENSIBLE*)fmt)->SubFormat, sizeof(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))) {
        printf("warning: unsupported output format: %u %u %lu %u\n", fmt->wFormatTag, fmt->nChannels, fmt->nSamplesPerSec, fmt->wBitsPerSample);
        goto failure;
    }

    hr = acl->lpVtbl->Initialize(acl, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                 500000 /* 50ms */, 0, fmt, 0);
    check(hr);

    CoTaskMemFree(fmt);

    event = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(event);

    hr = acl->lpVtbl->SetEventHandle(acl, event);
    check(hr);

    hr = acl->lpVtbl->GetBufferSize(acl, &buf_size);
    check(hr);

    hr = acl->lpVtbl->GetDevicePeriod(acl, &defp, &minp);
    check(hr);

    hr = acl->lpVtbl->GetCurrentPadding(acl, &padding);
    check(hr);

    hr = acl->lpVtbl->GetService(acl, &IID_IAudioRenderClient, (void**)&rcl);
    check(hr);

    hr = acl->lpVtbl->Start(acl);
    check(hr);

    printf("%u %u\n", buf_size, (int)defp);

    do {
        hr = acl->lpVtbl->GetCurrentPadding(acl, &padding);
        check(hr);

        if (buf_size - padding >= audio_buffersize) {
            hr = rcl->lpVtbl->GetBuffer(rcl, audio_buffersize, &data);
            check(hr);

            audio_getsamples(&audio, (sample_t*)data, audio_buffersize);

            hr = rcl->lpVtbl->ReleaseBuffer(rcl, audio_buffersize, 0);
            check(hr);
        }
    } while (WaitForSingleObject(event, 2000) == WAIT_OBJECT_0);

    printf("audio thread exit\n");

failure:
    if (rcl) release(rcl);
    if (acl) release(acl);
    if (event) CloseHandle(event);

    return;
}


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd, int nCmdShow)
{
    WSADATA wsadata;
    WNDCLASS wc;
    RECT window;
    int w, h;
    u_long mode;
    MSG msg;
    int pxformat;
    HDC hdc;
    HGLRC hrc, hrc_tmp;
    PIXELFORMATDESCRIPTOR pfd;
    LARGE_INTEGER freq, now, then;
    struct addrinfo *root, *info;
    const char *version;
    config_t cfg;

    int len;
    uint8_t buf[65536];

    if (__argc != 3) {
        printf("usage: %s [ADDR] [PORT]\n", __argv[0]);
        return 0;
    }

    if (!QueryPerformanceFrequency(&freq))
        return 0;

    if (WSAStartup(MAKEWORD(2,2), &wsadata) != NO_ERROR)
        return 0;

    loadconfig(&cfg);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        goto EXIT_WSACLEANUP;

    mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode))
        goto EXIT_CLOSE_SOCK;

    if (getaddrinfo(__argv[1], __argv[2], 0, &root)) {
        printf("getaddrinfo() failed\n");
        goto EXIT_CLOSE_SOCK;
    }

    info = root;
    do {
        if (info->ai_socktype && info->ai_socktype != SOCK_DGRAM)
            continue;

        if (connect(sock, info->ai_addr, info->ai_addrlen))
            break;
    } while ((info = info->ai_next));
    freeaddrinfo(root);

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = NAME;

    if (!RegisterClass(&wc))
        goto EXIT_CLOSE_SOCK;

    memset(&window, 0, sizeof(window));
    if (!AdjustWindowRect(&window, WS_OVERLAPPEDWINDOW, 0))
        goto EXIT_UNREGISTER;

    w = cfg.width + window.right - window.left;
    h = cfg.height + window.bottom - window.top;

    //minSize.x = MIN_WIDTH + window.right - window.left;
    //minSize.y = MIN_HEIGHT + window.bottom - window.top;

    hwnd = CreateWindow(NAME, NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, w, h, 0, 0, instance, 0);
    if (!hwnd)
        goto EXIT_UNREGISTER;

    tme.hwndTrack = hwnd;

    /*if (WSAAsyncSelect(sock, hwnd, WM_SOCKET, FD_READ))
        goto EXIT_DESTROY_WINDOW; */

    hdc = GetDC(hwnd);
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DEPTH_DONTCARE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

    pxformat = ChoosePixelFormat(hdc, &pfd);
    if (!SetPixelFormat(hdc, pxformat, &pfd))
        goto EXIT_DESTROY_WINDOW;

    hrc_tmp = wglCreateContext(hdc);
    if (!hrc_tmp)
        goto EXIT_DESTROY_WINDOW;

    if (!wglMakeCurrent(hdc, hrc_tmp)) {
        wglDeleteContext(hrc_tmp);
        goto EXIT_DESTROY_WINDOW;
    }

    if (!linkgl()) {
        wglMakeCurrent(hdc, 0);
        wglDeleteContext(hrc_tmp);
        goto EXIT_DESTROY_WINDOW;
    }

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs =
    (void*) wglGetProcAddress("wglCreateContextAttribsARB");
    if (!wglCreateContextAttribs)
        printf("....\n");

    hrc = wglCreateContextAttribs(hdc, 0, ctx_attribs);
    wglMakeCurrent(hdc, 0);
    wglDeleteContext(hrc_tmp);
    if (!hrc)
        goto EXIT_DESTROY_WINDOW;

    if (!wglMakeCurrent(hdc, hrc))
        goto EXIT_DELETE_CONTEXT;

    version = (const char*)glGetString(GL_VERSION);
    printf("GL version: %s\n", version);

    wglSwapIntervalEXT(1);

    if (!do_init(&cfg))
        goto EXIT_DELETE_CONTEXT;

    init_done = 1;

    thread(audio_thread, 0);

    QueryPerformanceCounter(&then);
    while (!done) {
        while ((len = recv(sock, (char*)buf, sizeof(buf), 0)) >= 0)
            do_recv(buf, len);

        QueryPerformanceCounter(&now);
        do_frame((double)(now.QuadPart - then.QuadPart) / freq.QuadPart);
        then = now;
        SwapBuffers(hdc);

        while (PeekMessage(&msg, 0, 0, 0, 1)) {
            if (msg.message == WM_QUIT) {
                done = 1;
                break;
            }

            //TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

EXIT_DELETE_CONTEXT:
    wglMakeCurrent(hdc, 0);
    wglDeleteContext(hrc);
EXIT_DESTROY_WINDOW:
    DestroyWindow(hwnd);
EXIT_UNREGISTER:
    UnregisterClass(NAME, instance);
EXIT_CLOSE_SOCK:
    closesocket(sock);
EXIT_WSACLEANUP:
    WSACleanup();
    return 0;
}
