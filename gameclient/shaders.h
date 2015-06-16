enum {
    s_post,
    s_2d,
    s_map,
    s_mapc,
    s_model,
    s_particle,
    s_water,
    s_strip,
    s_gs,
};
#define num_shader 9

typedef struct {
    const char *vert, *geom, *frag;
} shader_text;

extern const shader_text shader[];
