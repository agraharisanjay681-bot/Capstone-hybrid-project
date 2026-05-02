/*
 * inventory.c  –  C backend: binary-file storage layer.
 *
 * Layout:  the file is a flat array of Item records.
 * Record n starts at offset  n * sizeof(Item).
 * Deletes are soft: is_deleted is set to 1 in place.
 */

#include "inventory.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

/*
 * open_file  –  opens INVENTORY_FILE in the requested mode.
 * Returns NULL on failure (caller checks).
 */
static FILE *open_file(const char *mode)
{
    return fopen(INVENTORY_FILE, mode);
}

/*
 * find_record  –  scans the open file for a record with the given id.
 * On success writes the record into *out and sets *offset to the byte
 * position where it starts.  Returns 1 on success, 0 if not found.
 */
static int find_record(FILE *fp, int id, Item *out, long *offset)
{
    Item tmp;
    long pos = 0;

    rewind(fp);
    while (fread(&tmp, sizeof(Item), 1, fp) == 1) {
        if (tmp.id == id) {
            if (out)    *out    = tmp;
            if (offset) *offset = pos;
            return 1;
        }
        pos += (long)sizeof(Item);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int add_item(const Item *item)
{
    FILE *fp;

    if (!item || item->id <= 0)
        return 0;

    /* Open for reading to check for duplicate IDs. */
    fp = open_file("rb");
    if (fp) {
        if (find_record(fp, item->id, NULL, NULL)) {
            fclose(fp);
            return 0; /* duplicate */
        }
        fclose(fp);
    }

    /* Append the new record. */
    fp = open_file("ab");
    if (!fp)
        return 0;

    fwrite(item, sizeof(Item), 1, fp);
    fclose(fp);
    return 1;
}

int get_item(int id, Item *out)
{
    FILE *fp;
    Item  tmp;
    int   found;

    if (!out || id <= 0)
        return 0;

    fp = open_file("rb");
    if (!fp)
        return 0;

    found = find_record(fp, id, &tmp, NULL);
    fclose(fp);

    if (found && !tmp.is_deleted) {
        *out = tmp;
        return 1;
    }
    return 0;
}

int update_item(int id, const Item *updated)
{
    FILE *fp;
    long  offset;
    int   found;

    if (!updated || id <= 0)
        return 0;

    /* Open for read+write without truncation. */
    fp = open_file("r+b");
    if (!fp)
        return 0;

    found = find_record(fp, id, NULL, &offset);
    if (!found) {
        fclose(fp);
        return 0;
    }

    fseek(fp, offset, SEEK_SET);
    fwrite(updated, sizeof(Item), 1, fp);
    fclose(fp);
    return 1;
}

int delete_item(int id)
{
    FILE *fp;
    Item  tmp;
    long  offset;

    if (id <= 0)
        return 0;

    fp = open_file("r+b");
    if (!fp)
        return 0;

    if (!find_record(fp, id, &tmp, &offset)) {
        fclose(fp);
        return 0;
    }

    if (tmp.is_deleted) {
        fclose(fp);
        return 0; /* already deleted */
    }

    tmp.is_deleted = 1;
    fseek(fp, offset, SEEK_SET);
    fwrite(&tmp, sizeof(Item), 1, fp);
    fclose(fp);
    return 1;
}

int list_items(Item *buffer, int max_items)
{
    FILE *fp;
    Item  tmp;
    int   count = 0;

    if (!buffer || max_items <= 0)
        return 0;

    fp = open_file("rb");
    if (!fp)
        return 0;

    while (count < max_items && fread(&tmp, sizeof(Item), 1, fp) == 1) {
        if (!tmp.is_deleted)
            buffer[count++] = tmp;
    }
    fclose(fp);
    return count;
}
