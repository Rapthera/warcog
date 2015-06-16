#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

const char *unit_func[] = {
    "onframe",
    "onspawn",
    "ondeath",
    "canmove",
    "visible",
    0
};

const char *unit_ret[] = {
    "void",
    "void",
    "void",
    "bool",
    "bool",
    0
};

const char *unit_args[] = {
    "entity_t *x",
    "entity_t *x",
    "entity_t *x",
    "entity_t *x",
    "entity_t *x, entity_t *y",
    0
};

const char *unit_argn[] = {
    "x", "x", "x", "x", "x,y", 0
};

const char *ability_func[] = {
    "onimpact",
    "cancast",
    "onstateexpire",
    0
};

const char *ability_ret[] = {
    "void", "bool", "void", 0
};

const char *ability_args[] = {
    "entity_t *y, ability_t *x, target_t z, uint8_t w",
    "entity_t *y, ability_t *x",
    "entity_t *y, ability_t *x, state_t *z",
    0
};

const char *ability_argn[] = {
    "y,x,z,w", "y,x", "y,x,z", 0
};

const char *state_func[] = {
    "onexpire", 0
};

const char *state_ret[] = {
    "void", 0
};

const char *state_args[] = {
    "entity_t *y, state_t *x", 0
};

const char *state_argn[] = {
    "y,x", 0
};


typedef struct {
    bool private;
    char name[128], parent[127];
} data_t;

FILE *file, *header;
data_t data[256];
int ndata;

const char **func, **ret, **args, **argn;
const char *mname, *prefix;

void caller(const char *name)
{
    int i;

    fprintf(file, "caller_%s\n{\n    switch(x->def) {\n", name);

    for (i = 0; i < ndata; i++) {
        if (data[i].private)
            continue;

        fprintf(file, "    case %s%s:\n        return call_%s(%s);\n",
                prefix, data[i].name, name, data[i].name);
    }


    fprintf(file, "    default: __builtin_unreachable();\n    }\n}\n\n");
}

void defines(void)
{
    int i;
    const char *p, *r, *a;

    fprintf(file, "/* Automatically generated file - do not modify directly */\n");
    fprintf(file, "#include \"main.h\"\n\n");

    i = 0;
    while (func[i]) {
        p = func[i];
        r = ret[i];
        a = args[i];

        fprintf(file, "#define __%s(name, ...) name##_%s(__VA_ARGS__)\n", p, p);
        fprintf(file, "#define _%s(name, ...) __%s(name, __VA_ARGS__)\n", p, p);
        fprintf(file, "#define %s(...) _%s(name, __VA_ARGS__)\n", p, p);

        fprintf(file, "#define %s__ %s %s(%s) {return super(%s);}\n", p, r, p, a, argn[i]);

        fprintf(file, "#define caller_%s %s %s_%s(%s)\n", p, r, mname, p, a);
        fprintf(file, "#define call_%s(name) name##_%s(%s)\n", p, p, argn[i]);

        i++;
    }

    fprintf(header, "/* Automatically generated file - do not modify directly */\n\n");
}

void callers(void)
{
    const char **pp, *p;

    pp = func;
    while ((p = *pp++)) {
        caller(p);
    }
}

void start(void)
{
    const char **pp, *p;

    pp = func;
    while ((p = *pp++)) {
        fprintf(file, "#define %s_\n", p);
    }
}

void end(void)
{
    const char **pp, *p;

    pp = func;
    while ((p = *pp++)) {
        fprintf(file, "#ifdef %s_\n", p);
        fprintf(file, "#undef %s_\n", p);
        fprintf(file, "#define function %s\n", p);
        fprintf(file, "%s__\n", p);
        fprintf(file, "#undef function\n");
        fprintf(file, "#endif\n");
    }
}

void writef(FILE *file, char *name, char *a, char *b, char *c)
{
    fprintf(header, "%.*s%s_%.*s;\n", (int)(b - a), a, name, (int)(c - 1 - b), b);
}

void units(char *name, char *p)
{
    data_t *un;
    char *parent;

    un = &data[ndata++];
    printf("data: %s\n", name);

    strcpy(un->name, name);
    fprintf(file, "#define name %s\n", name);

    parent = p;
    if (*p == '{') {
        un->parent[0] = 0;
        un->private = 1;
        //strcpy(un->parent, name);
    } else {
        while (*p != '\n') p++; *p++ = 0;
        strcpy(un->parent, parent);
        un->private = 0;
    }

    fprintf(file, "#define parent %s\n", un->parent);
    start();

    int depth = 0, pdepth = 0;
    char *name_start, *name_end, *line_start;
    bool first = 0;
    while (*p) {
        switch (*p) {
        case '{':
            if (!first) {
                fprintf(file, "#define %s_info", un->name);
                if (un->parent[0])
                    fprintf(file, " %s_info", un->parent);
                while (*(++p) != '}' || depth) { // !*p
                    if (*p == '{')
                        depth++;
                    if (*p == '}')
                        depth--;
                    if (*p == '\n')
                        fputc('\\', file);
                    fputc(*p, file);
                }
                first = 1;
                break;
            }

            depth++;
            if (depth > 1)
                goto DEFAULT;

            writef(header, un->name, line_start, name_start, p);

            fprintf(file, "#undef %.*s_\n", (int)(name_end - name_start), name_start);
            fprintf(file, "#define function %.*s\n{", (int)(name_end - name_start), name_start);
            break;
        case '}':
            depth--;
            if (depth)
                goto DEFAULT;

            fprintf(file, "#undef function\n}");
            break;
        case '(':
            pdepth += 2;
            name_end = p;
        case ')':
            pdepth--;
        default:
        DEFAULT:
            fputc(*p, file);
            break;
        }

        if (*p == ' ' && isalpha(p[1]) && !pdepth)
            name_start = p + 1;

        if (*p++ == '\n' && isalpha(*p))
            line_start = p;
    }

    end();
    fprintf(file, "#undef name\n");
    fprintf(file, "#undef parent\n\n");
}

char* file_text(const char *path)
{
    FILE *file;
    char *data;
    int len;

    file = fopen(path, "rb");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(len + 1);
    if (data) {
        fread(data, 1, len, file);
        data[len] = 0;
    }

    fclose(file);
    return data;
}

void dofolder(const char *arg, const char *folder, void (*func)(char*, char*))
{
    char path[256], *p;
    int path_len;
    DIR *dir;
    struct dirent *ent;

    path_len = sprintf(path, "%s/%s/", arg, folder);
    dir = opendir(path);
    if (!dir)
        return;

    while ((ent = readdir(dir))) {
        p = strrchr(ent->d_name, '.');
        if (!p || strcmp(p, ".c"))
            continue;

        strcpy(path + path_len, ent->d_name);
        *p = 0;
        p = file_text(path);
        if (!p)
            continue;

        func(ent->d_name, p);
        free(p);
    }
    closedir(dir);
}

int main(int argc, const char *argv[])
{
    char path[256];
    int i, j;

    if (argc != 2)
        return 0;

    func = unit_func;
    ret = unit_ret;
    args = unit_args;
    argn = unit_argn;
    mname = "entity";
    prefix = "";

    sprintf(path, "%s/units.c", argv[1]);
    file = fopen(path, "wb");
    if (!file)
        return 0;

    sprintf(path, "%s/units.h", argv[1]);
    header = fopen(path, "wb");
    if (!header)
        return 0;

    defines();

    dofolder(argv[1], "units", units);

    fprintf(file, "const entitydef_t entitydef[] = {\n");
    fprintf(header, "\nenum {\n");
    for (j = 0, i = 0; i < ndata; i++) {
        if (data[i].private)
            continue;

        fprintf(file, "    {%s_info},\n", data[i].name);
        fprintf(header, "    %s%s,\n", prefix, data[i].name);
        j++;

    }
    fprintf(file, "};\n\n");
    fprintf(header, "};\n");

    fprintf(header, "\n#define nentitydef %u\n", j);

    callers();
    fclose(file);
    fclose(header);

    ndata = 0;
    func = ability_func;
    ret = ability_ret;
    args = ability_args;
    argn = ability_argn;
    mname = "ability";
    prefix = "a_";

    sprintf(path, "%s/abilities.c", argv[1]);
    file = fopen(path, "wb");
    if (!file)
        return 0;

    sprintf(path, "%s/abilities.h", argv[1]);
    header = fopen(path, "wb");
    if (!header)
        return 0;

    defines();

    dofolder(argv[1], "abilities", units);

    fprintf(file, "const abilitydef_t abilitydef[] = {\n");
    fprintf(header, "\nenum {\n");
    for (j = 0,i = 0; i < ndata; i++) {
        if (data[i].private)
            continue;

        fprintf(file, "    {%s_info},\n", data[i].name);
        fprintf(header, "    %s%s,\n", prefix, data[i].name);
        j++;

    }
    fprintf(file, "};\n\n");
    fprintf(header, "};\n");

    fprintf(header, "\n#define nabilitydef %u\n", j);

    callers();
    fclose(file);
    fclose(header);

    ndata = 0;
    func = state_func;
    ret = state_ret;
    args = state_args;
    argn = state_argn;
    mname = "state";
    prefix = "s_";

    sprintf(path, "%s/states.c", argv[1]);
    file = fopen(path, "wb");
    if (!file)
        return 0;

    sprintf(path, "%s/states.h", argv[1]);
    header = fopen(path, "wb");
    if (!header)
        return 0;

    defines();

    dofolder(argv[1], "states", units);

    fprintf(file, "const statedef_t statedef[] = {\n");
    fprintf(header, "\nenum {\n");
    for (j = 0,i = 0; i < ndata; i++) {
        if (data[i].private)
            continue;

        fprintf(file, "    {%s_info},\n", data[i].name);
        fprintf(header, "    %s%s,\n", prefix, data[i].name);
        j++;

    }
    fprintf(file, "};\n\n");
    fprintf(header, "};\n");

    fprintf(header, "\n#define nstatedef %u\n", j);

    callers();
    fclose(file);
    fclose(header);


    /* text */
    char *data, *p, *name, *text;
    int k;

    sprintf(path, "%s/text.c", argv[1]);
    file = fopen(path, "wb");
    if (!file)
        return 0;

    sprintf(path, "%s/text.h", argv[1]);
    header = fopen(path, "wb");
    if (!header)
        return 0;

    sprintf(path, "%s/text.txt", argv[1]);
    data = p = file_text(path);
    if (!p)
        return 0;

    fprintf(file, "/* Automatically generated file - do not modify directly */\n");
    fprintf(file, "const char text[] =\n");

    fprintf(header, "/* Automatically generated file - do not modify directly */\n\n");

    k = 0;
    while (*p) {
        while (*p == '\n') p++;

        name = p;
        while (*p != '\n') p++;
        *p++ = 0;

        text = p;
        while (*p != '\n') p++;
        *p++ = 0;
        j = p - text;
        fprintf(file, "\"%s\\0\"\n", text);
        fprintf(header, "#define %s %u\n", name, k);
        k += j;
    }

    fprintf(header, "#define text_len %u\n", k);
    fprintf(file, ";\n");

    free(data);
    fclose(file);
    fclose(header);

    return 0;
}
