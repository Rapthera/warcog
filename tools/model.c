#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <math.h>

typedef struct {float x, y;} V2;
typedef struct {float x, y, z;} V3;
typedef struct {float x, y, z, w;} V4;
typedef struct {float m[4][4];} M44;

typedef struct {
	char name[80];
	int start, end;

	float	mUnk3;
	int		mUnk4;
	float	mUnk5;
	int		mUnk6;
	float	mUnk7;
	V3	mMins;
	V3	mMaxs;
} SEQ;

typedef struct {
	uint32_t unk1;
	char path[260];
	uint32_t unk2;
} TEX;

typedef struct {
	int	frame;
	V3 vec, in, out;
} KEYFRAME;

typedef struct {
	int	frame;
	V3 vec;
} KEYFRAME_LINEAR;

typedef struct {
	int	frame;
	V4 vec, in, out;
} KEYFRAME4;

typedef struct {
	int	frame;
	V4 vec;
} KEYFRAME4_LINEAR;

typedef struct {
    char name[80];
    int id, parent, nodetype;

    KEYFRAME *keyframe[3];
    uint32_t nkeyframe[3], linetype[3];
    uint32_t geosetid, geosetanimid;

    uint32_t child_id;
    bool used, taken;
} BONE;

typedef union {
    void *p;
    char *ch;
    int8_t *int8;
    uint8_t *uint8;
    int16_t *int16;
    uint16_t *uint16;
    int32_t *int32;
    uint32_t *uint32;
    float *f;
    double *d;
    V3 *v3;
    V4 *v4;
    SEQ *seq;
    TEX *tex;
    BONE *b;
} P;

typedef struct {
    int tex, nvert, nindex, ngroup, nmatrix;
    V3 *vert, *norm;
    V2 *uv;
    uint8_t *vertgroup;
    uint16_t *index;
    int *group, *matrix;
} CHUNK;

typedef struct {
    V3 v;
    int16_t tx, ty;
    int8_t nx, ny, nz;
    uint8_t ngroup;
    uint8_t group[4];
} VERT;

typedef struct {
    uint16_t vis, frames;
    int start, end;
} ANIM;

static uint16_t indice[16384];
static VERT vert[4096];
static BONE bone[128], cbone[128];
static int bone_newid[128];
static CHUNK chunk[64];
static ANIM anim[64];
static SEQ *seq;
static V3 *pivot;
static int npivot, nbone, nchunk, nseq, nanim;

#define tag(x) htonl(x)

#define set(x, y) \
    y = size; \
    assert(p.p + y * sizeof(*x) <= end); \
    x = p.p; \
    p.p += y * sizeof(*x);

P loadchunk(P p, int size, CHUNK *ck)
{
    void *end;
    int tag, i, j;

    end = p.p + size;

    for (i = 0; i < 8; i++) {
        assert(p.p < end);

        tag = *p.uint32++;
        size = *p.uint32++;
        switch (tag(tag)) {
        case 'VRTX':
            set(ck->vert, ck->nvert);
            break;
        case 'NRMS':
            assert(size == ck->nvert);
            set(ck->norm, ck->nvert);
            break;
        case 'PTYP':
            assert(size == 1);
            assert(*p.uint32++ == 4); /* polygon type (triangle) */
            break;
        case 'PCNT':
            assert(size == 1);
            p.uint32++; /* polygon count */
            break;
        case 'PVTX':
            assert(size % 3 == 0);
            set(ck->index, ck->nindex);
            break;
        case 'GNDX':
            assert(size == ck->nvert);
            set(ck->vertgroup, ck->nvert);
            break;
        case 'MTGC':
            set(ck->group, ck->ngroup);
            break;
        case 'MATS':
            set(ck->matrix, ck->nmatrix);
            break;
        default:
            assert(0);
        }
    }

    assert(p.p + 44 <= end);

    ck->tex = *p.uint32++;
    p.uint32++; //?
    p.uint32++; //?
    p.f++; //?
    p.v3++; //min bounds
    p.v3++; //max bounds

    size = *p.uint32++;
    for (i = 0; i < size; i++) {
        p.f++; p.v3++; p.v3++; //?
    }

    tag = *p.uint32++;
    assert(tag == tag('UVAS'));
    size = *p.uint32++;

    tag = *p.uint32++;
    assert(tag == tag('UVBS'));
    size = *p.uint32++;
    assert(size == ck->nvert);
    set(ck->uv, ck->nvert);

    assert(p.p == end);

    for (i = 0, j = 0; i < ck->ngroup; i++)
        j += ck->group[i];

    assert(j == ck->nmatrix);

    return p;
}

P loadbone(P p, int size)
{
    BONE *b;
    void *end;
    int tag, i, j;

    end = p.p + size;

    b = &bone[p.b->id];
    memcpy(b, p.p, 92);
    p.p += 92;

    if (b->id >= nbone)
        nbone = b->id + 1;

    printf("bone: %s %i %i %i\n", b->name, b->id, b->parent, b->nodetype);

    //assert(b->nodetype != 264);

    for (i = 0; i < 3 && end - p.p > 8; i++) {
        tag = *p.uint32++;
        j = 0;
        switch (tag(tag)) {
        case 'KGSC':
            j = 2;
        case 'KGTR':
            b->nkeyframe[j] = *p.uint32++;
			b->linetype[j] = *p.uint32++;
			p.uint32++; //?

            assert(b->linetype[j] <= 3); /* != 0 !!*/
            b->keyframe[j] = p.p;
            if (b->linetype[j] <= 1) {
                p.p += b->nkeyframe[j] * sizeof(KEYFRAME_LINEAR);
            } else {
                p.p += b->nkeyframe[j] * sizeof(KEYFRAME);
            }
            break;
        case 'KGRT':
            b->nkeyframe[1] = *p.uint32++;
			b->linetype[1] = *p.uint32++;
			p.uint32++; //?

			assert(b->linetype[1] == 2 || b->linetype[1] == 1);
			b->keyframe[1] = p.p;
			if (b->linetype[1] == 1) {
                p.p += b->nkeyframe[1] * sizeof(KEYFRAME4_LINEAR);
			} else {
			    p.p += b->nkeyframe[1] * sizeof(KEYFRAME4);
			}
            break;
        default:
            assert(0);
        }
    }

    if (p.p != end) {
        b->geosetid = *p.uint32++;
        b->geosetanimid = *p.uint32++;
    }

    assert(p.p == end);
    return p;
}

static void loadmodel(void *data, int len)
{
    P p = {.p = data};
    int tag, size, i, j, n;
    void *end, *block_end;

    assert(len >= 4);
    end = data + len;
    tag = *p.uint32++;
    assert(tag == tag('MDLX'));

    while (p.p < end) {
        tag = *p.uint32++;
        size = *p.uint32++;
        block_end = p.p + size;

        switch (tag(tag)) {
        case 'VERS':
            assert(size == 4);
            assert(*p.uint32++ == 800);
            break;
        case 'MODL':
            assert(size == 372);
            printf("name: %.80s\n", p.ch); p.ch += 80;
            printf("description: %.260s\n", p.ch); p.ch += 260;

            p.uint32++; //unknown
            p.v3++; //min bounds
            p.v3++; //max bounds
            p.uint32++; //blendtime - ?
            break;
        case 'SEQS':
            assert(size % sizeof(SEQ) == 0);
            seq = p.p;
            nseq = size / sizeof(SEQ);
            for (i = 0; p.p != block_end; i++, p.seq++) {
                printf("anim %u: %s %u %u\n", i, p.seq->name, p.seq->start, p.seq->end);
            }
            break;
        case 'TEXS':
            assert(size % sizeof(TEX) == 0);
            for (i = 0; p.p != block_end; i++, p.tex++) {
                printf("texture %u: %s %u %u\n", i, p.tex->path, p.tex->unk1, p.tex->unk2);
            }
            break;
        case 'MTLS':
            for (i = 0; p.p < block_end; i++) {
                size = *p.uint32++;
                p.uint32 += 2;
                tag = *p.uint32++;
                assert(tag == tag('LAYS'));
                n = *p.uint32++;

                for (j = 0; j != n; j++) {
                    size = p.uint32[0];
                    printf("material %u: layer %u=%u\n", i, j, p.uint32[3]);
                    p.p += size;
                }
            }
            break;
        case 'GEOS':
            for (i = 0; p.p < block_end; i++) {
                size = *p.uint32++;
                p = loadchunk(p, size - 4, &chunk[i]);
                printf("chunk %u: material=%u, nvert=%u, nindex=%u, ngroup=%u, nmatrix=%u\n", i,
                       chunk[i].tex, chunk[i].nvert, chunk[i].nindex, chunk[i].ngroup, chunk[i].nmatrix);
            }
            nchunk = i;
            break;
        case 'BONE':
            while (p.p < block_end) {
                size = (*p.uint32++);
                p = loadbone(p, size + 4);
            }
            break;
        case 'HELP':
            while (p.p < block_end) {
                size = (*p.uint32++);
                p = loadbone(p, size - 4);
            }
            break;
        case 'PIVT':
            assert(size % sizeof(V3) == 0);
            npivot = size / sizeof(V3);
            pivot = p.p;
            p.p += size;
            printf("pivot points: %u\n", npivot);
            break;
        default:
            printf("warning: ignored tag %c%c%c%c\n", p.ch[-8], p.ch[-7], p.ch[-6], p.ch[-5]);
            p.p += size;
            break;
        }

        assert(p.p == block_end);
    }

    assert(nbone <= npivot);

    assert(p.p == end);
}

/* marine
#define use_chunks 0b00100011
#define use_anims 0b0011111111
*/

/* zerg */
#define use_chunks 0b111011
#define use_anims 0b11111111

static void convert(const char *name, void *data, int len)
{
    VERT *v;
    CHUNK *ck;
    ANIM *a;
    BONE *b;
    int nvert, nindex;
    int i, j, k, id, g, m, frame;
    char path[256];
    FILE *mdl, *header;

    loadmodel(data, len);

    mdl = fopen(name, "wb");
    assert(mdl);

    sprintf(path, "%s.h", name);
    header = fopen(path, "w");
    assert(header);

    nvert = 0; nindex = 0;
    for (i = 0; i < nchunk; i++) {
        if (!(use_chunks & (1 << i)))
            continue;
        ck = &chunk[i];

        float tx_min = INFINITY, tx_max = -INFINITY, ty_min = INFINITY, ty_max = -INFINITY;

        for (j = 0; j < ck->nvert; j++) {
            v = &vert[nvert + j];

            v->v = ck->vert[j];

            tx_min = fmin(tx_min, ck->uv[j].x);
            tx_max = fmax(tx_max, ck->uv[j].x);

            ty_min = fmin(ty_min, ck->uv[j].y);
            ty_max = fmax(ty_max, ck->uv[j].y);

            if (ck->uv[j].x < 0)
                ck->uv[j].x = 0.0;

            if (ck->uv[j].x > 65535.0 / 65536.0)
                ck->uv[j].x = 65535.0 / 65536.0;

            if (ck->uv[j].y < 0)
                ck->uv[j].y = 0.0;

            if (ck->uv[j].y > 65535.0 / 65536.0)
                ck->uv[j].y = 65535.0 / 65536.0;

            //assert(ck->uv[j].x >= 0.0 && ck->uv[j].x <= 1.0);
            //assert(ck->uv[j].y >= 0.0 && ck->uv[j].y <= 1.0);
            v->tx = ck->uv[j].x * 65536.0 + 0.5;
            v->ty = ck->uv[j].y * 65536.0 + 0.5;

            assert(ck->norm[j].x >= -1.0 && ck->norm[j].x <= 1.0);
            assert(ck->norm[j].y >= -1.0 && ck->norm[j].y <= 1.0);;
            assert(ck->norm[j].z >= -1.0 && ck->norm[j].z <= 1.0);;

            v->nx = roundf(ck->norm[j].x * 127.0);
            v->ny = roundf(ck->norm[j].y * 127.0);
            v->nz = roundf(ck->norm[j].z * 127.0);

            id = ck->vertgroup[j];
            assert(id < ck->ngroup);
            g = ck->group[id];

            if (g > 4) {
                printf("warning ignored g > 4: %i\n", g);
                g = 4;
            }

            for (m = 0, k = 0; k < id; k++)
                m += ck->group[k];

            v->ngroup = g;
            for (k = 0; k < g; m++, k++) {
                id = ck->matrix[m];
                assert (id < nbone);

                bone[id].used = 1;
                v->group[k] = id;
            }
        }

        printf("%i %f %f %f %f\n", i, tx_min, tx_max, ty_min, ty_max);

        for (j = 0; j < ck->nindex; j++) {
            indice[nindex + j] = ck->index[j] + nvert;
        }

        nvert += ck->nvert;
        nindex += ck->nindex;
    }

    /* apply bone "used" to parents */
    for (i = 0; i < nbone; i++) {
        b = &bone[i];

        if (b->used)
            while (b->parent != -1 && (b = &bone[b->parent], !b->used))
                b->used = 1;
    }

    /* reorder bones, removing unused bones */
    for (i = 0, j = 0; i < nbone; i++) {
        b = &bone[i];

        if (!b->used) {
            printf("unused bone: %u\n", i);
            continue;
        }

        if (b->taken)
            continue;

        int chain[16];
        k = 0; id = i;
        do {
            chain[k++] = id;
        } while ((id = b->parent) != -1 && (b = &bone[b->parent], !b->taken));

        while (k--) {
            id = chain[k];
            bone[id].taken = 1;
            cbone[j] = bone[id];
            bone_newid[id] = j;
            j++;
        }
    }
    nbone = j;

    /* update parents  */
    for (i = 0; i < nbone; i++) {
        b = &cbone[i];
        if (b->parent != -1) {
            assert(bone[b->parent].id == cbone[bone_newid[b->parent]].id);
            b->parent = bone_newid[b->parent];
        }
        printf("bone: %16s \t%i \t%i \t%i\n", b->name, i, b->parent, b->nodetype);
    }

    /* update bone indices in vertices */
    for (i = 0; i < nvert; i++) {
        v = &vert[i];

        for (j = 0; j < v->ngroup; j++)
            v->group[j] = bone_newid[v->group[j]];
    }

    /* process anims */
    for (i = 0; i < nseq; i++) {
        /*if (!(use_anims & (1 << i)))
            continue;*/

        a = &anim[nanim++];

        a->start = seq[i].start;
        a->end = seq[i].end;

        a->vis = 0xFFFF;
        a->frames = a->end - a->start;
    }

    printf("final: nvert=%u nindex=%u nbone=%u nanim=%u\n", nvert, nindex, nbone, nanim);

    struct {
        uint8_t ngeo, teamcolor_end, nbone, nanim;
    } mdl_header = {1, 1, nbone, nanim};

    fwrite(&mdl_header, sizeof(mdl_header), 1, mdl);

    /* anim info */
    for (i = 0; i < nanim; i++)
        fwrite(&anim[i], 4, 1, mdl);

    /* geo index ranges */
    uint16_t index = nindex, texture = 0;
    fwrite(&index, 2, 1, mdl);
    fwrite(&texture, 2, 1, mdl);

    /* indices */
    fwrite(indice, 2, nindex, mdl);

    if (1) {
        uint16_t zero = 0;
        fwrite(&zero, 2, 1, mdl);
    }

    /* bone data */
    printf("warning: all interpolation becomes linear in target format\n");
    for (i = 0; i < nbone; i++) {
        b = &cbone[i];

        assert(b->nkeyframe[0] <= 255 && b->nkeyframe[1] <= 255);

        struct {
            uint8_t parent, n_tr, n_rt, linetype;
            V3 pivot;
        } bone_header = {b->parent,  0, 0, b->linetype[0], pivot[b->id]};

        uint8_t trans[nanim], quats[nanim];
        uint8_t data[65336];
        P p = {.p = data};

        for (j = 0, k = 0, id = 0, frame = 0; j < b->nkeyframe[0]; j++) {
            V3 *v;
            if (b->linetype[0] == 1) {
                KEYFRAME_LINEAR *k = (KEYFRAME_LINEAR*)b->keyframe[0];
                k += j;
                assert(k->frame > frame);
                frame = k->frame;
                v = &k->vec;
            } else if (b->linetype[0] == 2) {
                KEYFRAME *k = b->keyframe[0];
                k += j;
                assert(k->frame > frame);
                frame = k->frame;
                v = &k->vec;
            } else {
                assert(0);
            }

            while (frame > anim[id].end) {
                trans[id++] = k;
                if (id == nanim)
                    goto BREAK1;
            }

            if (frame >= anim[id].start) {
                *p.uint32++ = frame - anim[id].start;
                *p.v3++ = *v;
                k++;
            }
        }
        BREAK1:
        bone_header.n_tr = k;

        while (id < nanim) trans[id++] = k;

        for (j = 0, k = 0, id = 0, frame = 0; j < b->nkeyframe[1]; j++) {
            V4 *v;
            if (b->linetype[1] == 1) {
                KEYFRAME4_LINEAR *k = (KEYFRAME4_LINEAR*)b->keyframe[1];
                k += j;
                assert(k->frame > frame);
                frame = k->frame;
                v = &k->vec;
            } else if (b->linetype[1] == 2) {
                KEYFRAME4 *k = (KEYFRAME4*)b->keyframe[1];
                k += j;
                assert(k->frame > frame);
                frame = k->frame;
                v = &k->vec;
            } else {
                assert(0);
            }

            while (frame > anim[id].end) {
                quats[id++] = k;
                if (id == nanim)
                    goto BREAK2;
            }

            if (frame >= anim[id].start) {
                *p.uint32++ = frame - anim[id].start;
                *p.v4++ = *v;
                k++;
            }
        }
        BREAK2:
        bone_header.n_rt = k;

        while (id < nanim) quats[id++] = k;

        fwrite(&bone_header, sizeof(bone_header), 1, mdl);
        fwrite(trans, 1, sizeof(trans), mdl);
        fwrite(quats, 1, sizeof(quats), mdl);

        if (nanim & 1) {
            uint16_t zero = 0;
            fwrite(&zero, 2, 1, mdl);
        }

        fwrite(data, 1, p.uint8 - data, mdl);
    }

    /* vertex data */
    fwrite(vert, sizeof(VERT), nvert, mdl);

    fclose(mdl);
    fclose(header);
}

static void* file_raw(const char *path, int *ret_len)
{
    FILE *file;
    int len;
    void *data;

    file = fopen(path, "rb");
    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(len);
    if (data)
        fread(data, 1, len, file);

    fclose(file);

    *ret_len = len;
    return data;
}

int main(int argc, const char *argv[])
{
    void *data;
    int len, i;

    for (i = 2; i != argc; i++) {
        data = file_raw(argv[i], &len);
        if (!data)
            continue;

        convert(argv[1], data, len);
        free(data);
    }

    return 0;
}
