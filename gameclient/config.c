#include "config.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 768
#define DEFAULT_NAME "Newbie"

static char* startswith(char *p, const char *str)
{
    char ch;

    while ((ch = *str++) && ch == *p) p++;
    return (!ch) ? p : NULL;
}

static bool skip(char **p, const char *str)
{
    char *end;

    return ((end = startswith(*p, str)) && (*p = end));
}

static bool getint(char **p, int *value)
{
    char *end;

    *value = strtol(*p, &end, 10);
    return (*value != 0 && *end == '\n' && (*p = end + 1, 1));
}

static bool getstr(char **p, char *str, int size)
{
    char *s;

    s = *p;
    while (*s != '\n') {
        if (!*s)
            return 0;

        if (!--size)
            return 0;

        *str++ = *s++;
    }

    *str = 0;
    *p = s + 1;
    return 1;
}

static void cfg_defaults(config_t *cfg)
{
    cfg->width = DEFAULT_WIDTH;
    cfg->height = DEFAULT_HEIGHT;
    cfg->windowed = 1;

    cfg->dlrate = 10;
    strcpy(cfg->name, DEFAULT_NAME);
}

bool cfg_init(config_t *cfg, char *cfg_str)
{
    char *p;
    int i;

    cfg_defaults(cfg);

    if (!cfg_str)
        return 1;

    p = cfg_str;
    do {
        if (skip(&p, "width ")) {
            if (!getint(&p, &cfg->width))
                return 0;
        } else if (skip(&p, "height ")) {
            if (!getint(&p, &cfg->height))
                return 0;
        } else if (skip(&p, "dlrate ")) {
            if (!getint(&p, &i))
                return 0;
            cfg->dlrate = i > 10 ? 10 : i;
        } else if (skip(&p, "name ")) {
            if (!getstr(&p, cfg->name, sizeof(cfg->name)))
                return 0;
        } else if (skip(&p, "windowed\n")) {
            cfg->windowed = 1;
        } else if (skip(&p, "fullscreen\n")) {
            cfg->windowed = 0;
        } else break;
    } while (1);

    return 1;
}
