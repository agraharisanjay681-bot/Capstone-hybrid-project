#pragma once

#include <string>
#include <vector>

extern "C" {
#include "inventory.h"
}

enum class SortBy { Id, Name };

class InventoryManager {
public:
    /* CRUD wrappers around the C backend */
    bool addItem(const Item &item);
    bool getItem(int id, Item &out) const;
    bool updateItem(int id, const Item &updated);
    bool deleteItem(int id);

    /* Returns a sorted vector of all active items */
    std::vector<Item> listItems(SortBy sortBy = SortBy::Id) const;

    /* Display helpers */
    void printItem(const Item &it) const;
    void printAll(SortBy sortBy = SortBy::Id) const;

    /* Interactive menu actions (prompt, validate, call CRUD) */
    void menuAdd();
    void menuView();
    void menuUpdate();
    void menuDelete();
    void menuList();
};
