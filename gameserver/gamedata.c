#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <lzma.h>

#include "gamedata.h"
#include "protocol.h"
#include "game.h"

#define MAX_SIZE (1024 * 1024 * 4)

/* missing some error checking */

static bool loadmap(void)
{
    FILE *file;
    int res;
    bool eof;

    file = fopen(map_path "map", "rb");
    if (!file)
        return 0;

    res = fread(&map, 1, sizeof(map), file);
    eof = (feof(file) != 0);
    fclose(file);

    if (res != sizeof(map) || eof) {
        printf("map only %u of %u expected\n", res, (int)sizeof(map));
        return 0;
    }

    return 1;
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
        len = fread(data, 1, len, file);

    fclose(file);

    *ret_len = len;
    return data;
}

bool gamedata_init(void)
{
    void *p, *data;
    int size, parts, len;

    if (!loadmap())
        return 0;

    p = malloc(MAX_SIZE);
    if (!p)
        return 0;

    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_easy_encoder(&strm, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC32);
    if (ret != LZMA_OK) {
        free(p);
        return 0;
    }

    strm.next_out = p;
    strm.avail_out = MAX_SIZE;

    strm.next_in = (const void*)&mapdef;
    strm.avail_in = sizeof(mapdef);
    ret = lzma_code(&strm, LZMA_RUN);
    assert(ret == LZMA_OK);

    strm.next_in = (const void*)entitydef;
    strm.avail_in = mapdef.ndef * sizeof(entitydef_t);
    ret = lzma_code(&strm, LZMA_RUN);
    assert(ret == LZMA_OK);

    if (mapdef.nadef) {
        strm.next_in = (const void*)abilitydef;
        strm.avail_in = mapdef.nadef * sizeof(abilitydef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.nsdef) {
        strm.next_in = (const void*)statedef;
        strm.avail_in = mapdef.nsdef * sizeof(statedef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.neffect) {
        strm.next_in = (const void*)effectdef;
        strm.avail_in = mapdef.neffect * sizeof(effectdef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.nparticle) {
        strm.next_in = (const void*)particledef;
        strm.avail_in = mapdef.nparticle * sizeof(particledef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.ngs) {
        strm.next_in = (const void*)groundspritedef;
        strm.avail_in = mapdef.ngs * sizeof(groundspritedef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.nattachment) {
        strm.next_in = (const void*)attachmentdef;
        strm.avail_in = mapdef.nattachment * sizeof(attachmentdef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.nmdleffect) {
        strm.next_in = (const void*)mdleffectdef;
        strm.avail_in = mapdef.nmdleffect * sizeof(mdleffectdef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.nstrip) {
        strm.next_in = (const void*)stripdef;
        strm.avail_in = mapdef.nstrip * sizeof(stripdef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    if (mapdef.nsoundeff) {
        strm.next_in = (const void*)sounddef;
        strm.avail_in = mapdef.nsoundeff * sizeof(sounddef_t);
        ret = lzma_code(&strm, LZMA_RUN);
        assert(ret == LZMA_OK);
    }

    strm.next_in = (const void*)text;
    strm.avail_in = mapdef.textlen;
    ret = lzma_code(&strm, LZMA_RUN);
    assert(ret == LZMA_OK);

    strm.next_in = (const void*)&map;
    strm.avail_in = sizeof(map);
    ret = lzma_code(&strm, LZMA_RUN);
    assert(ret == LZMA_OK);

    data = file_raw(map_path "texture", &len);
    assert(data && len == 1024 * 1024 * 4 + 256 * 256 * 4 * (1 + mapdef.ntex));

    strm.next_in = data;
    strm.avail_in = len;
    ret = lzma_code(&strm, LZMA_RUN);
    assert(ret == LZMA_OK);
    free(data);

    data = file_raw(map_path "model", &len);
    assert(data);

    strm.next_in = data;
    strm.avail_in = len;
    ret = lzma_code(&strm, LZMA_RUN);
    assert(ret == LZMA_OK);
    free(data);

    data = file_raw(map_path "sound", &len);
    assert(data);

    strm.next_in = data;
    strm.avail_in = len;
    ret = lzma_code(&strm, LZMA_FINISH);
    assert(ret == LZMA_STREAM_END);
    free(data);


    size = strm.total_out;
    parts = (size - 1) / partsize + 1;

    p = realloc(p, parts * partsize);

    gamedata.data = p;
    gamedata.num_parts = parts;
    gamedata.size = size;
    gamedata.inflated = strm.total_in;

    printf("map size: %u->%u (%u)\n", gamedata.inflated, size, parts);
    return 1;
}
