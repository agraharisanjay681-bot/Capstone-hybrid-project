#ifndef INVENTORY_H
#define INVENTORY_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NAME_LEN 40
#define INVENTORY_FILE "inventory.dat"

typedef struct {
    int   id;
    char  name[MAX_NAME_LEN];
    int   quantity;
    float price;
    int   is_deleted; /* 0 = active, 1 = soft-deleted */
} Item;

/*
 * add_item    – appends a new record; rejects duplicate IDs.
 * get_item    – finds an active record by ID and copies it into *out.
 * update_item – overwrites the record for the given ID.
 * delete_item – sets is_deleted = 1 for the given ID.
 * list_items  – fills buffer[] with up to max_items active records;
 *               returns the count actually copied.
 *
 * add / get / update / delete return 1 on success, 0 on failure.
 */
int add_item(const Item *item);
int get_item(int id, Item *out);
int update_item(int id, const Item *updated);
int delete_item(int id);
int list_items(Item *buffer, int max_items);

#ifdef __cplusplus
}
#endif

#endif /* INVENTORY_H */
