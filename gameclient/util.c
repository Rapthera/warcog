#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "xz/xz.h"

char* file_text(const char *path)
{
    FILE *file;
    char *data;
    int len;

    file = fopen(path, "r");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(len + 1);
    if (!data)
        goto EXIT_CLOSE;

    if (fread(data, 1, len, file) != len)
        goto EXIT_FREE;

    data[len] = 0;
    fclose(file);
    return data;

EXIT_FREE:
    free(data);
EXIT_CLOSE:
    fclose(file);
    return 0;
}

void* file_raw(const char *path)
{
    FILE *file;
    int len;
    void *data;

    file = fopen(path, "rb");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = malloc(len);
    if (data)
        if (fread(data, 1, len, file) != len)
            free(data), data = 0;

    fclose(file);
    return data;
}

uint32_t inflate(void *dest, void *src, uint32_t dest_size, uint32_t src_len)
{
    //int res;
    struct xz_dec *dec;
    struct xz_buf buf = {
        .in = src,
        .in_pos = 0,
        .in_size = src_len,

        .out = dest,
        .out_pos = 0,
        .out_size = dest_size,
    };

    xz_crc32_init();

    dec = xz_dec_init(XZ_SINGLE, 0);
    if (!dec)
        return 0;

    xz_dec_run(dec, &buf); //res =
    xz_dec_end(dec);

    /* out_pos is only set on success */
    return buf.out_pos;
}
