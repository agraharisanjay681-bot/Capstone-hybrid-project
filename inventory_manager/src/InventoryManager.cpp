/*
 * InventoryManager.cpp  –  C++ wrapper around the C storage backend.
 *
 * Uses std::vector and std::sort (two required STL elements).
 */

#include "InventoryManager.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

extern "C" {
#include "inventory.h"
}

/* ------------------------------------------------------------------ */
/* InventoryManager implementation                                      */
/* ------------------------------------------------------------------ */

bool InventoryManager::addItem(const Item &item)
{
    return add_item(&item) == 1;
}

bool InventoryManager::getItem(int id, Item &out) const
{
    return get_item(id, &out) == 1;
}

bool InventoryManager::updateItem(int id, const Item &updated)
{
    return update_item(id, &updated) == 1;
}

bool InventoryManager::deleteItem(int id)
{
    return delete_item(id) == 1;
}

/*
 * listItems  –  fetches all active records into a vector,
 *               sorts them by the chosen key, and returns the vector.
 */
std::vector<Item> InventoryManager::listItems(SortBy sortBy) const
{
    /* Use a generously-sized local buffer; adjust if you need more. */
    static const int MAX_BUF = 10000;
    Item raw[MAX_BUF];

    int count = list_items(raw, MAX_BUF);

    std::vector<Item> items(raw, raw + count);   /* STL element 1 */

    /* STL element 2 – sort by requested key */
    if (sortBy == SortBy::Name) {
        std::sort(items.begin(), items.end(),    /* STL element 2 */
                  [](const Item &a, const Item &b) {
                      return std::string(a.name) < std::string(b.name);
                  });
    } else {
        std::sort(items.begin(), items.end(),
                  [](const Item &a, const Item &b) {
                      return a.id < b.id;
                  });
    }

    return items;
}

/* ------------------------------------------------------------------ */
/* Pretty-print helpers                                                 */
/* ------------------------------------------------------------------ */

static void printHeader()
{
    std::cout << "\n"
              << std::left
              << std::setw(6)  << "ID"
              << std::setw(22) << "Name"
              << std::setw(10) << "Qty"
              << std::setw(10) << "Price"
              << "\n"
              << std::string(48, '-') << "\n";
}

static void printRow(const Item &it)
{
    std::cout << std::left
              << std::setw(6)  << it.id
              << std::setw(22) << it.name
              << std::setw(10) << it.quantity
              << std::fixed << std::setprecision(2)
              << std::setw(10) << it.price
              << "\n";
}

void InventoryManager::printItem(const Item &it) const
{
    printHeader();
    printRow(it);
    std::cout << "\n";
}

void InventoryManager::printAll(SortBy sortBy) const
{
    auto items = listItems(sortBy);
    if (items.empty()) {
        std::cout << "  (no items in inventory)\n";
        return;
    }
    printHeader();
    for (const auto &it : items)
        printRow(it);
    std::cout << "\n";
}

/* ------------------------------------------------------------------ */
/* Input helpers                                                        */
/* ------------------------------------------------------------------ */

/*
 * readInt  –  keeps asking until the user enters an integer that
 *             satisfies the given predicate.
 */
static int readInt(const std::string &prompt,
                   bool (*pred)(int),
                   const std::string &errMsg)
{
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && pred(val))
            return val;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [!] " << errMsg << "\n";
    }
}

static float readFloat(const std::string &prompt,
                       bool (*pred)(float),
                       const std::string &errMsg)
{
    float val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && pred(val))
            return val;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [!] " << errMsg << "\n";
    }
}

static std::string readName(const std::string &prompt)
{
    std::string name;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, name);
        if (!name.empty() && name.size() < MAX_NAME_LEN)
            return name;
        std::cout << "  [!] Name must be 1–" << (MAX_NAME_LEN - 1) << " characters.\n";
    }
}

/* ------------------------------------------------------------------ */
/* Menu actions                                                         */
/* ------------------------------------------------------------------ */

void InventoryManager::menuAdd()
{
    Item it{};

    it.id = readInt("  Enter ID (positive integer): ",
                    [](int v) { return v > 0; },
                    "ID must be a positive integer.");

    std::string n = readName("  Enter name: ");
    std::strncpy(it.name, n.c_str(), MAX_NAME_LEN - 1);

    it.quantity = readInt("  Enter quantity (>= 0): ",
                          [](int v) { return v >= 0; },
                          "Quantity must be 0 or more.");

    it.price = readFloat("  Enter price (>= 0): ",
                         [](float v) { return v >= 0.0f; },
                         "Price must be 0 or more.");

    it.is_deleted = 0;

    if (addItem(it))
        std::cout << "  [+] Item " << it.id << " added.\n";
    else
        std::cout << "  [!] Failed – ID " << it.id << " already exists or is invalid.\n";
}

void InventoryManager::menuView()
{
    int id = readInt("  Enter ID to view: ",
                     [](int v) { return v > 0; },
                     "ID must be a positive integer.");
    Item it{};
    if (getItem(id, it))
        printItem(it);
    else
        std::cout << "  [!] Item " << id << " not found or has been deleted.\n";
}

void InventoryManager::menuUpdate()
{
    int id = readInt("  Enter ID to update: ",
                     [](int v) { return v > 0; },
                     "ID must be a positive integer.");

    Item existing{};
    if (!getItem(id, existing)) {
        std::cout << "  [!] Item " << id << " not found or has been deleted.\n";
        return;
    }

    std::cout << "  Current: name=\"" << existing.name
              << "\"  qty=" << existing.quantity
              << "  price=" << existing.price << "\n";

    Item updated = existing;

    std::string n = readName("  New name (Enter to keep \"" +
                              std::string(existing.name) + "\"): ");
    if (n.empty())
        n = existing.name;
    std::strncpy(updated.name, n.c_str(), MAX_NAME_LEN - 1);

    updated.quantity = readInt("  New quantity (>= 0): ",
                               [](int v) { return v >= 0; },
                               "Quantity must be 0 or more.");

    updated.price = readFloat("  New price (>= 0): ",
                              [](float v) { return v >= 0.0f; },
                              "Price must be 0 or more.");

    if (updateItem(id, updated))
        std::cout << "  [~] Item " << id << " updated.\n";
    else
        std::cout << "  [!] Update failed.\n";
}

void InventoryManager::menuDelete()
{
    int id = readInt("  Enter ID to delete: ",
                     [](int v) { return v > 0; },
                     "ID must be a positive integer.");

    std::cout << "  Are you sure you want to delete item " << id << "? (y/n): ";
    char c;
    std::cin >> c;
    if (c != 'y' && c != 'Y') {
        std::cout << "  Cancelled.\n";
        return;
    }

    if (deleteItem(id))
        std::cout << "  [-] Item " << id << " deleted.\n";
    else
        std::cout << "  [!] Item " << id << " not found or already deleted.\n";
}

void InventoryManager::menuList()
{
    std::cout << "  Sort by: [1] ID  [2] Name  (default = ID): ";
    int ch = 0;
    if (!(std::cin >> ch)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    SortBy sb = (ch == 2) ? SortBy::Name : SortBy::Id;
    printAll(sb);
}
