#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../main.h"
#include "../input.h"
#include "../config.h"
#include "../util.h"
#include "alsa.h"

#include <GL/glx.h>
#include <X11/Xatom.h>
#include <pthread.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>


#define EVENT_MASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask \
                    | KeyPressMask | KeyReleaseMask | StructureNotifyMask)

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

static const int visual_attribs[] = {
    GLX_X_RENDERABLE, True,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 0,
    GLX_STENCIL_SIZE, 0,
    GLX_DOUBLEBUFFER, True,
    None
};

static const int context_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    None
};

typedef struct {
    uint8_t id, v8;
    uint16_t v16;
    uint32_t value;
    void *data;
} msg_t;

static volatile bool done, msg_lock;
static volatile uint8_t msg_write;
static uint8_t msg_read;
static volatile msg_t msg[256];

static bool mouse_lock;

void quit(void)
{
    done = 1;
}

static void lock(volatile bool *_lock)
{
    while (__sync_lock_test_and_set(_lock, 1))
        while (*_lock);
}

static void unlock(volatile bool *_lock)
{
    __sync_lock_release(_lock);
}

void postmessage(uint8_t id, uint8_t v8, uint16_t v16, uint32_t value, void *data)
{
    volatile msg_t *m;

    lock(&msg_lock);
    m = &msg[msg_write++];
    m->id = id;
    m->v8 = v8;
    m->v16 = v16;
    m->value = value;
    m->data = data;
    unlock(&msg_lock);
}

static void messages(void)
{
    volatile msg_t *m;
    uint8_t w;

    lock(&msg_lock);
    w = msg_write;
    unlock(&msg_lock);

    while (msg_read != w) {
        m = &msg[msg_read++];
        do_msg(m->id, m->v8, m->v16, m->value, m->data);
    }
}

void thread(void (func)(void*), void *args)
{
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1 << 16);
    pthread_create(&thread, &attr, (void*(*)(void*))func, args);
    pthread_attr_destroy(&attr);
}

/* static void preinit(void *args)
{
    do_preinit();
    preinit_done = 1;
} */

static int sock;

int send2(const void *data, int len)
{
    /*static int i;

    if (++i == 10)
        return (i = 0);*/

    int res;

    res = send(sock, data, len, 0);
    if (res != len)
        printf("send() returned %i (%i)\n", res, len);

    return res;
}

static Display* xlib_init(const config_t *cfg, Window *p_win, GLXContext *p_ctx)
{
    Display *display;
    Window win;
    GLXContext ctx;
    GLXFBConfig *fbc, fb;
    XVisualInfo *vi;
    XSetWindowAttributes swa;
    glXCreateContextAttribsARBProc glXCreateContextAttribs;
    int (*glXSwapInterval) (int);
    int glx_major, glx_minor;
    int cw_flags;
    int i;

    display = XOpenDisplay(NULL);
    if (!display)
        return 0;

    if (!glXQueryVersion(display, &glx_major, &glx_minor))
        goto EXIT_CLOSE_DISPLAY;

    if (glx_major < 1 || (glx_major == 1 && glx_minor < 3)) {
        printf("Invalid GLX version (%i.%i)\n", glx_major, glx_minor);
        goto EXIT_CLOSE_DISPLAY;
    }

    if(!(fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &i))) {
        printf("glXChooseFBConfig failed\n");
        goto EXIT_CLOSE_DISPLAY;
    }

    printf("%u matching FB configs\n", i);

    fb = fbc[0]; /* 0 index is closest match */

    XFree(fbc);
    if (!fb)
        goto EXIT_CLOSE_DISPLAY;

    vi = glXGetVisualFromFBConfig(display, fb);
    printf("chosen visualid=0x%x\n", (int) vi->visualid);

    win = RootWindow(display, vi->screen);

    swa.colormap = XCreateColormap(display, win, vi->visual, AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = EVENT_MASK;
    cw_flags = (CWBorderPixel | CWColormap | CWEventMask);

    win = XCreateWindow(display, win, 0, 0, cfg->width, cfg->height, 0, vi->depth, InputOutput,
                        vi->visual, cw_flags, &swa);
    XFree(vi);
    XFreeColormap(display, swa.colormap);

    if (!win) {
        printf("XCreateWindow() failed\n");
        goto EXIT_CLOSE_DISPLAY;
    }

    XStoreName(display, win, NAME);
    XMapWindow(display, win);

    glXCreateContextAttribs = (void*)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    if (!glXCreateContextAttribs) {
        printf("glXCreateContextAttribsARB() not found\n");
        ctx = glXCreateNewContext(display, fb, GLX_RGBA_TYPE, 0, True);
    } else {
        ctx = glXCreateContextAttribs(display, fb, 0, True, context_attribs);
    }

    if(!ctx) {
        printf("context creation failed\n");
        goto EXIT_FREE_WINDOW;
    }

    if(!glXIsDirect(display, ctx)) {
        printf("Indirect GLX rendering context\n");
    }

    glXMakeCurrent(display, win, ctx);

    if((glXSwapInterval = (void*)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI"))) {
        printf("vsync enabled\n");
        glXSwapInterval(1);
    }

    *p_win = win;
    *p_ctx = ctx;
    return display;

EXIT_FREE_WINDOW:
    XDestroyWindow(display, win);
EXIT_CLOSE_DISPLAY:
    XCloseDisplay(display);
    return NULL;
}

static void keypress(XKeyEvent *ev)
{
    KeySym sym;

    sym = XLookupKeysym(ev, 0);
    if (sym == XK_F1) {
        if (!mouse_lock)
            XGrabPointer(ev->display, ev->window, True, 0, GrabModeAsync, GrabModeAsync,
                         ev->window, None, CurrentTime);
        else
            XUngrabPointer(ev->display, CurrentTime);
        mouse_lock = !mouse_lock;
        return;
    }
    //XLookupString()
    input_keydown(&input, sym, sym);
}

static void keyrelease(XKeyEvent *ev)
{
    KeySym sym;

    sym = XLookupKeysym(ev, 0);
    input_keyup(&input, sym);
}

static void buttonpress(XButtonEvent *ev)
{
    if (ev->button < Button1)
        return;

    if (ev->button <= Button3)
        input_mdown(&input, ev->x, ev->y, 0, ev->button - Button1);
    else if (ev->button <= Button5)
        input_mwheel(&input, (ev->button == Button4) ? 1.0 : -1.0);
}

static void buttonrelease(XButtonEvent *ev)
{
    if (ev->button >= Button1 && ev->button <= Button3)
        input_mup(&input, ev->x, ev->y, 0, ev->button - Button1);
}

static void motionnotify(XMotionEvent *ev)
{
    input_mmove(&input, ev->x, ev->y, 0);
}

static void configurenotify(XConfigureEvent *ev)
{
    do_resize(ev->width, ev->height);
}

static void xlib_events(Display *display)
{
    XEvent event;

    while (XPending(display)) {
        XNextEvent(display, &event);
        switch(event.type) {
        case KeyPress:
            keypress(&event.xkey);
            break;
        case KeyRelease:
            keyrelease(&event.xkey);
            break;
        case LeaveNotify:
            input_mleave(&input, 0);
            break;
        case MotionNotify:
            motionnotify(&event.xmotion);
            break;
        case ButtonPress:
            buttonpress(&event.xbutton);
            break;
        case ButtonRelease:
            buttonrelease(&event.xbutton);
            break;
        case ConfigureNotify:
            configurenotify(&event.xconfigure);
            break;
        }
    }
}

static uint64_t gettime(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
        return 0;

    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
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

int main(int argc, char* argv[])
{
    uint8_t buf[65536];
    int len;
    Display *display;
    Window win;
    GLXContext ctx;
    uint64_t t_now, t_last;
    struct addrinfo *root, *info;
    config_t cfg;

    if (argc != 3) {
        printf("usage: %s [IP] [PORT]\n", argv[0]);
        return 0;
    }

    loadconfig(&cfg);

    thread(alsa_thread, "default"); //"plughw:0,0"

    sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (sock < 0)
        goto EXIT;

    if (getaddrinfo(argv[1], argv[2], 0, &root)) {
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

    display = xlib_init(&cfg, &win, &ctx);
    if (!display)
        goto EXIT_CLOSE_SOCK;

    if (!do_init(&cfg))
        goto EXIT_CLEANUP_XLIB;

    t_last = gettime();
    do {
        while ((len = recv(sock, buf, sizeof(buf), 0)) >= 0)
            do_recv(buf, len);

        messages();

        t_now = gettime();
        do_frame((double)(t_now - t_last) / 1000000000.0);
        t_last = t_now;
        glXSwapBuffers(display, win);
        xlib_events(display);
    } while(!done);

    do_cleanup();

EXIT_CLEANUP_XLIB:
    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, ctx);
    XDestroyWindow(display, win);
    XCloseDisplay(display);
EXIT_CLOSE_SOCK:
    close(sock);
EXIT:
    while (!alsa_quit)
        alsa_init = 0;
    return 1;
}
