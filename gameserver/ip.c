#include "ip.h"
#include <stdlib.h>

ipentry_t* ip_find(iptable_t *table, uint32_t ip)
{
    uint16_t low, high, i;
    ipentry_t *entry, *root;

    low = ip & 0xFFFF;
    high = ip >> 16;
    entry = root = table->entry[low];
    if (entry) {
        do {
            if(entry->ip == high)
                return entry;
        } while ((entry++)->next);

        i = entry - root;
        entry = realloc(root, (i + 1) * sizeof(*entry));
        if (!entry)
            return 0;

        table->entry[low] = entry;
        entry += i;

        (entry - 1)->next = 1;
    } else {
        entry = table->entry[low] = malloc(sizeof(*entry));
    }

    entry->ip = ip;
    entry->value = 0;
    entry->next = 0;
    entry->key = 12345; /* TODO: random number */
    return entry;
}

void ip_clear(iptable_t *table)
{
    ipentry_t **p;

    for (p = table->entry; p != (void*)table->entry + sizeof(table->entry); p++) {
        free(*p);
        *p = 0;
    }
}
