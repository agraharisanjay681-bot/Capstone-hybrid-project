/**
 * ============================================================================
 *  HYBRID INVENTORY MANAGER
 *  A single-file C++ application with embedded C backend logic
 *  Architecture: C structs + binary file I/O (backend)
 *               C++ classes + STL (frontend/UI)
 * ============================================================================
 *
 *  BUILD:
 *      g++ -std=c++17 -Wall -Wextra -o inventory_manager hybrid_inventory_manager.cpp
 *
 *  RUN:
 *      ./inventory_manager
 *
 *  DATA FILE:
 *      inventory.dat  (auto-created on first run, persists across restarts)
 * ============================================================================
 */

// ============================================================================
//  SECTION 1 — INCLUDES & PLATFORM DETECTION
// ============================================================================

#include <cstdio>       // fopen, fread, fwrite, fseek, fclose, printf
#include <cstring>      // strncpy, strlen, memset
#include <cstdlib>      // atoi, atof
#include <cmath>        // fabs
#include <cctype>       // isspace
#include <iostream>     // std::cin, std::cout, std::cerr
#include <string>       // std::string
#include <vector>       // std::vector
#include <algorithm>    // std::sort
#include <limits>       // std::numeric_limits
#include <iomanip>      // std::setw, std::left, std::fixed, std::setprecision
#include <stdexcept>    // std::runtime_error
#include <sstream>      // std::ostringstream

#ifdef _WIN32
    #include <windows.h>  // for ANSI enable on Windows
#endif

// ============================================================================
//  SECTION 2 — ANSI COLOUR CODES (graceful fallback if not supported)
// ============================================================================

namespace Color {
    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* WHITE   = "\033[37m";
    const char* BG_BLUE = "\033[44m";

    /** Call once at startup to enable ANSI on Windows 10+. */
    void enable() {
#ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#endif
    }
}

// ============================================================================
//  SECTION 3 — C BACKEND (structs + binary file API)
//              All functions use plain C file I/O; no C++ features here.
// ============================================================================

// ---- 3.1  Data structure ------------------------------------------------

#define ITEM_NAME_LEN   40
#define DATA_FILE       "inventory.dat"
#define MAX_ITEMS_HARD  100000   /* hard upper bound for safety */

/**
 * Core data record stored verbatim to the binary file.
 * sizeof(Item) == fixed layout; never add virtual functions here.
 */
typedef struct {
    int   id;                    /**< Unique positive integer key         */
    char  name[ITEM_NAME_LEN];   /**< Item name (null-terminated)         */
    int   quantity;              /**< Stock count, must be >= 0           */
    float price;                 /**< Unit price, must be >= 0.0          */
    int   is_deleted;            /**< Soft-delete flag: 1=deleted, 0=live */
} Item;

// ---- 3.2  Internal file helpers -----------------------------------------

/** Open the data file for reading + writing (binary). Creates it if absent. */
static FILE* open_data_file(const char* mode) {
    return fopen(DATA_FILE, mode);
}

/**
 * Count total records (including deleted) in the file.
 * Returns -1 on I/O error.
 */
static long count_records(FILE* fp) {
    if (fseek(fp, 0L, SEEK_END) != 0) return -1L;
    long size = ftell(fp);
    if (size < 0) return -1L;
    return size / (long)sizeof(Item);
}

// ---- 3.3  Public C backend API -----------------------------------------

/**
 * add_item — append a new item to the data file.
 *
 * Validates:
 *   - id > 0
 *   - name non-empty
 *   - quantity >= 0
 *   - price >= 0
 *   - no duplicate live id
 *
 * @return 1 on success, 0 on failure.
 */
int add_item(const Item* item) {
    if (!item)                       return 0;
    if (item->id <= 0)               return 0;
    if (item->name[0] == '\0')       return 0;
    if (item->quantity < 0)          return 0;
    if (item->price < 0.0f)          return 0;

    /* Try to open existing file for r+b; create if missing */
    FILE* fp = open_data_file("r+b");
    if (!fp) {
        fp = open_data_file("wb");   /* first run — create */
        if (!fp) return 0;
        /* file is empty, jump straight to write */
        fclose(fp);
        fp = open_data_file("r+b");
        if (!fp) return 0;
    }

    /* Scan for duplicate id */
    long n = count_records(fp);
    if (n < 0) { fclose(fp); return 0; }

    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return 0; }
    for (long i = 0; i < n; i++) {
        Item rec;
        if (fread(&rec, sizeof(Item), 1, fp) != 1) { fclose(fp); return 0; }
        if (!rec.is_deleted && rec.id == item->id) {
            fclose(fp);
            return 0;   /* duplicate */
        }
    }

    /* Append at end */
    if (fseek(fp, 0L, SEEK_END) != 0) { fclose(fp); return 0; }
    int ok = (fwrite(item, sizeof(Item), 1, fp) == 1) ? 1 : 0;
    fclose(fp);
    return ok;
}

/**
 * get_item — load a single live item by id into *out.
 *
 * @return 1 if found and live, 0 otherwise.
 */
int get_item(int id, Item* out) {
    if (id <= 0 || !out) return 0;

    FILE* fp = open_data_file("rb");
    if (!fp) return 0;

    long n = count_records(fp);
    if (n < 0) { fclose(fp); return 0; }
    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return 0; }

    int found = 0;
    for (long i = 0; i < n; i++) {
        Item rec;
        if (fread(&rec, sizeof(Item), 1, fp) != 1) break;
        if (!rec.is_deleted && rec.id == id) {
            *out = rec;
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

/**
 * update_item — overwrite a live record in-place using fseek.
 *
 * @return 1 on success, 0 if not found or validation fails.
 */
int update_item(int id, const Item* updated) {
    if (id <= 0 || !updated)             return 0;
    if (updated->name[0] == '\0')        return 0;
    if (updated->quantity < 0)           return 0;
    if (updated->price < 0.0f)           return 0;

    FILE* fp = open_data_file("r+b");
    if (!fp) return 0;

    long n = count_records(fp);
    if (n < 0) { fclose(fp); return 0; }
    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return 0; }

    int found = 0;
    for (long i = 0; i < n; i++) {
        long pos = (long)(i * sizeof(Item));
        Item rec;
        if (fread(&rec, sizeof(Item), 1, fp) != 1) break;
        if (!rec.is_deleted && rec.id == id) {
            /* Seek back to this record and overwrite */
            if (fseek(fp, pos, SEEK_SET) != 0) break;
            Item to_write = *updated;
            to_write.id = id;              /* preserve key */
            to_write.is_deleted = 0;
            if (fwrite(&to_write, sizeof(Item), 1, fp) == 1) found = 1;
            break;
        }
    }
    fflush(fp);
    fclose(fp);
    return found;
}

/**
 * delete_item — soft-delete: sets is_deleted=1 in-place.
 *
 * @return 1 on success, 0 if not found.
 */
int delete_item(int id) {
    if (id <= 0) return 0;

    FILE* fp = open_data_file("r+b");
    if (!fp) return 0;

    long n = count_records(fp);
    if (n < 0) { fclose(fp); return 0; }
    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return 0; }

    int found = 0;
    for (long i = 0; i < n; i++) {
        long pos = (long)(i * sizeof(Item));
        Item rec;
        if (fread(&rec, sizeof(Item), 1, fp) != 1) break;
        if (!rec.is_deleted && rec.id == id) {
            rec.is_deleted = 1;
            if (fseek(fp, pos, SEEK_SET) == 0) {
                if (fwrite(&rec, sizeof(Item), 1, fp) == 1) found = 1;
            }
            break;
        }
    }
    fflush(fp);
    fclose(fp);
    return found;
}

/**
 * list_items — fill buffer[] with up to max_items live records.
 *
 * @return number of live items copied (<= max_items), or -1 on I/O error.
 */
int list_items(Item* buffer, int max_items) {
    if (!buffer || max_items <= 0) return 0;

    FILE* fp = open_data_file("rb");
    if (!fp) return 0;   /* no file yet → 0 items, not an error */

    long n = count_records(fp);
    if (n < 0) { fclose(fp); return -1; }
    if (fseek(fp, 0L, SEEK_SET) != 0) { fclose(fp); return -1; }

    int count = 0;
    for (long i = 0; i < n && count < max_items; i++) {
        Item rec;
        if (fread(&rec, sizeof(Item), 1, fp) != 1) break;
        if (!rec.is_deleted) {
            buffer[count++] = rec;
        }
    }
    fclose(fp);
    return count;
}

// ============================================================================
//  SECTION 4 — C++ FRONTEND (InventoryManager class)
// ============================================================================

// ---- 4.1  Utility helpers -----------------------------------------------

/** Strip leading/trailing whitespace from a std::string (in-place). */
static void trim(std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) { s.clear(); return; }
    size_t end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end - start + 1);
}

/** Print a horizontal rule of 'width' dashes with optional colour. */
static void hline(int width, const char* col = Color::DIM) {
    std::cout << col;
    for (int i = 0; i < width; ++i) std::cout << "\xe2\x94\x80";  /* UTF-8 ─ */
    std::cout << Color::RESET << '\n';
}

/** Clear pending bad bits + leftover chars from std::cin. */
static void flush_cin() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// ---- 4.2  InventoryManager ----------------------------------------------

/**
 * InventoryManager — C++ class that owns all user interaction.
 *
 * It calls the C backend API for persistence and uses STL containers
 * (std::vector, std::sort) for in-memory operations.
 */
class InventoryManager {
public:
    InventoryManager()  = default;
    ~InventoryManager() = default;

    /** Entry point: show menu in a loop until user chooses Exit. */
    void run();

private:
    // ---- Menu actions ----
    void action_add();
    void action_view();
    void action_update();
    void action_delete();
    void action_list();

    // ---- Input helpers ----
    int         read_int(const std::string& prompt,
                         int lo = std::numeric_limits<int>::min(),
                         int hi = std::numeric_limits<int>::max());
    float       read_float(const std::string& prompt, float lo = 0.0f);
    std::string read_string(const std::string& prompt, int max_len = ITEM_NAME_LEN - 1);

    // ---- Display helpers ----
    void print_banner()  const;
    void print_menu()    const;
    void print_item_header() const;
    void print_item_row(const Item& it) const;
    void print_item_footer() const;
    void print_success(const std::string& msg) const;
    void print_error(const std::string& msg)   const;
    void print_info(const std::string& msg)    const;

    static const int TABLE_WIDTH = 72;
};

// ---- 4.3  run() ---------------------------------------------------------

void InventoryManager::run() {
    print_banner();

    while (true) {
        print_menu();

        int choice = read_int("  Enter choice", 1, 6);

        switch (choice) {
            case 1: action_add();    break;
            case 2: action_view();   break;
            case 3: action_update(); break;
            case 4: action_delete(); break;
            case 5: action_list();   break;
            case 6:
                std::cout << '\n'
                          << Color::CYAN << Color::BOLD
                          << "  Goodbye! Data saved to '" DATA_FILE "'.\n"
                          << Color::RESET << '\n';
                return;
            default:
                print_error("Invalid choice. Please select 1–6.");
        }

        std::cout << '\n';
        std::cout << Color::DIM << "  Press Enter to continue..." << Color::RESET;
        flush_cin();
    }
}

// ---- 4.4  action_add() --------------------------------------------------

void InventoryManager::action_add() {
    std::cout << '\n' << Color::BOLD << Color::CYAN
              << "  ── ADD NEW ITEM ──" << Color::RESET << '\n';

    Item it;
    memset(&it, 0, sizeof(Item));
    it.is_deleted = 0;

    it.id = read_int("  Item ID (positive integer)", 1);

    std::string name = read_string("  Item name", ITEM_NAME_LEN - 1);
    strncpy(it.name, name.c_str(), ITEM_NAME_LEN - 1);
    it.name[ITEM_NAME_LEN - 1] = '\0';

    it.quantity = read_int("  Quantity (>= 0)", 0);
    it.price    = read_float("  Price (>= 0.00)", 0.0f);

    int result = add_item(&it);
    if (result == 1) {
        print_success("Item added successfully!");
        print_item_header();
        print_item_row(it);
        print_item_footer();
    } else {
        print_error("Failed to add item. "
                    "Check: duplicate ID, empty name, or invalid values.");
    }
}

// ---- 4.5  action_view() -------------------------------------------------

void InventoryManager::action_view() {
    std::cout << '\n' << Color::BOLD << Color::CYAN
              << "  ── VIEW ITEM ──" << Color::RESET << '\n';

    int id = read_int("  Enter item ID", 1);

    Item it;
    memset(&it, 0, sizeof(Item));

    if (get_item(id, &it)) {
        print_item_header();
        print_item_row(it);
        print_item_footer();
    } else {
        print_error("Item not found or has been deleted.");
    }
}

// ---- 4.6  action_update() -----------------------------------------------

void InventoryManager::action_update() {
    std::cout << '\n' << Color::BOLD << Color::CYAN
              << "  ── UPDATE ITEM ──" << Color::RESET << '\n';

    int id = read_int("  Enter ID of item to update", 1);

    /* Check item exists first */
    Item existing;
    memset(&existing, 0, sizeof(Item));
    if (!get_item(id, &existing)) {
        print_error("Item not found or has been deleted.");
        return;
    }

    std::cout << Color::DIM << "  Current record:\n" << Color::RESET;
    print_item_header();
    print_item_row(existing);
    print_item_footer();

    std::cout << Color::DIM << "  Leave a field blank to keep current value.\n"
              << Color::RESET;

    /* Name */
    std::string name_prompt = std::string("  New name [") + existing.name + "]";
    std::string new_name = read_string(name_prompt, ITEM_NAME_LEN - 1);
    if (new_name.empty()) new_name = existing.name;

    /* Quantity */
    std::cout << "  New quantity [" << existing.quantity << "]: ";
    std::string qty_str;
    std::getline(std::cin, qty_str);
    trim(qty_str);
    int new_qty = qty_str.empty() ? existing.quantity : std::stoi(qty_str);

    /* Price */
    std::cout << "  New price [" << std::fixed << std::setprecision(2)
              << existing.price << "]: ";
    std::string price_str;
    std::getline(std::cin, price_str);
    trim(price_str);
    float new_price = price_str.empty() ? existing.price
                                        : static_cast<float>(std::stod(price_str));

    /* Validate */
    if (new_name.empty() || new_qty < 0 || new_price < 0.0f) {
        print_error("Invalid values supplied. Update aborted.");
        return;
    }

    Item updated = existing;
    strncpy(updated.name, new_name.c_str(), ITEM_NAME_LEN - 1);
    updated.name[ITEM_NAME_LEN - 1] = '\0';
    updated.quantity = new_qty;
    updated.price    = new_price;

    if (update_item(id, &updated)) {
        print_success("Item updated successfully!");
        print_item_header();
        print_item_row(updated);
        print_item_footer();
    } else {
        print_error("Failed to update item.");
    }
}

// ---- 4.7  action_delete() -----------------------------------------------

void InventoryManager::action_delete() {
    std::cout << '\n' << Color::BOLD << Color::CYAN
              << "  ── DELETE ITEM ──" << Color::RESET << '\n';

    int id = read_int("  Enter ID of item to delete", 1);

    Item it;
    memset(&it, 0, sizeof(Item));
    if (!get_item(id, &it)) {
        print_error("Item not found or already deleted.");
        return;
    }

    std::cout << Color::YELLOW << "  You are about to delete:\n" << Color::RESET;
    print_item_header();
    print_item_row(it);
    print_item_footer();

    std::cout << Color::YELLOW << "  Confirm deletion? [y/N]: " << Color::RESET;
    std::string ans;
    std::getline(std::cin, ans);
    trim(ans);

    if (ans == "y" || ans == "Y") {
        if (delete_item(id)) {
            print_success("Item soft-deleted (record hidden, file preserved).");
        } else {
            print_error("Deletion failed.");
        }
    } else {
        print_info("Deletion cancelled.");
    }
}

// ---- 4.8  action_list() -------------------------------------------------

void InventoryManager::action_list() {
    std::cout << '\n' << Color::BOLD << Color::CYAN
              << "  ── ALL ITEMS ──" << Color::RESET << '\n';

    /* Allocate buffer via std::vector for safety */
    std::vector<Item> buf(MAX_ITEMS_HARD);
    int n = list_items(buf.data(), MAX_ITEMS_HARD);

    if (n < 0) {
        print_error("I/O error reading inventory file.");
        return;
    }
    if (n == 0) {
        print_info("No items in inventory.");
        return;
    }

    buf.resize(static_cast<size_t>(n));

    /* Ask sort preference */
    std::cout << "  Sort by: [1] ID (default)  [2] Name  [3] Price  [4] Quantity\n"
              << "  Choice: ";
    std::string sort_str;
    std::getline(std::cin, sort_str);
    trim(sort_str);

    int sort_choice = sort_str.empty() ? 1 : std::stoi(sort_str);

    switch (sort_choice) {
        case 2:
            std::sort(buf.begin(), buf.end(), [](const Item& a, const Item& b) {
                return std::string(a.name) < std::string(b.name);
            });
            break;
        case 3:
            std::sort(buf.begin(), buf.end(), [](const Item& a, const Item& b) {
                return a.price < b.price;
            });
            break;
        case 4:
            std::sort(buf.begin(), buf.end(), [](const Item& a, const Item& b) {
                return a.quantity < b.quantity;
            });
            break;
        default:   /* 1 or anything else → sort by ID */
            std::sort(buf.begin(), buf.end(), [](const Item& a, const Item& b) {
                return a.id < b.id;
            });
    }

    std::cout << '\n';
    print_item_header();
    for (const Item& it : buf) print_item_row(it);
    print_item_footer();

    std::cout << Color::DIM << "  Total live items: "
              << Color::BOLD << n << Color::RESET << '\n';
}

// ---- 4.9  Input helpers -------------------------------------------------

int InventoryManager::read_int(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::cout << Color::WHITE << prompt;
        if (lo != std::numeric_limits<int>::min() ||
            hi != std::numeric_limits<int>::max()) {
            std::cout << " (" << lo << "–" << hi << ")";
        }
        std::cout << ": " << Color::RESET;

        std::string line;
        if (!std::getline(std::cin, line)) {
            // EOF or stream error
            return lo;
        }
        trim(line);
        if (line.empty()) continue;

        try {
            size_t pos = 0;
            int v = std::stoi(line, &pos);
            if (pos != line.size()) throw std::invalid_argument("trailing chars");
            if (v < lo || v > hi) {
                print_error("Value out of range [" + std::to_string(lo) +
                            ", " + std::to_string(hi) + "]. Try again.");
                continue;
            }
            return v;
        } catch (...) {
            print_error("Invalid integer. Try again.");
        }
    }
}

float InventoryManager::read_float(const std::string& prompt, float lo) {
    while (true) {
        std::cout << Color::WHITE << prompt << " (>= "
                  << std::fixed << std::setprecision(2) << lo
                  << "): " << Color::RESET;

        std::string line;
        if (!std::getline(std::cin, line)) return lo;
        trim(line);
        if (line.empty()) continue;

        try {
            size_t pos = 0;
            float v = static_cast<float>(std::stod(line, &pos));
            if (pos != line.size()) throw std::invalid_argument("trailing chars");
            if (v < lo) {
                print_error("Value must be >= " + std::to_string(lo) + ".");
                continue;
            }
            return v;
        } catch (...) {
            print_error("Invalid number. Try again.");
        }
    }
}

std::string InventoryManager::read_string(const std::string& prompt, int max_len) {
    while (true) {
        std::cout << Color::WHITE << prompt << ": " << Color::RESET;
        std::string line;
        if (!std::getline(std::cin, line)) return "";
        trim(line);
        if (line.empty()) return "";   /* caller decides if empty is OK */
        if ((int)line.size() > max_len) {
            print_error("Input too long (max " + std::to_string(max_len) +
                        " chars). Truncating to fit.");
            line = line.substr(0, static_cast<size_t>(max_len));
        }
        return line;
    }
}

// ---- 4.10  Display helpers ----------------------------------------------

void InventoryManager::print_banner() const {
    std::cout << '\n';
    std::cout << Color::CYAN << Color::BOLD;
    std::cout << "  ╔══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║          H Y B R I D   I N V E N T O R Y   M A N A G E R           ║\n";
    std::cout << "  ║           C Backend  ×  C++ Frontend  ×  Binary Persistence         ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════════╝\n";
    std::cout << Color::RESET;
    std::cout << Color::DIM << "  Data file: " << DATA_FILE << '\n' << Color::RESET;
    std::cout << '\n';
}

void InventoryManager::print_menu() const {
    hline(TABLE_WIDTH);
    std::cout << Color::BOLD << Color::WHITE
              << "  MENU\n" << Color::RESET;
    hline(TABLE_WIDTH);
    std::cout << "  " << Color::CYAN << "[1]" << Color::RESET << " Add Item\n";
    std::cout << "  " << Color::CYAN << "[2]" << Color::RESET << " View Item by ID\n";
    std::cout << "  " << Color::CYAN << "[3]" << Color::RESET << " Update Item\n";
    std::cout << "  " << Color::CYAN << "[4]" << Color::RESET << " Delete Item\n";
    std::cout << "  " << Color::CYAN << "[5]" << Color::RESET << " List All Items\n";
    std::cout << "  " << Color::RED   << "[6]" << Color::RESET << " Exit\n";
    hline(TABLE_WIDTH);
}

void InventoryManager::print_item_header() const {
    std::cout << '\n';
    hline(TABLE_WIDTH, Color::BLUE);
    std::cout << Color::BOLD << Color::BLUE
              << std::left
              << std::setw(6)  << "  ID"
              << std::setw(22) << "  Name"
              << std::setw(12) << "  Qty"
              << std::setw(14) << "  Price"
              << "  Status\n"
              << Color::RESET;
    hline(TABLE_WIDTH, Color::BLUE);
}

void InventoryManager::print_item_row(const Item& it) const {
    const char* status_col = it.is_deleted ? Color::RED : Color::GREEN;
    const char* status_txt = it.is_deleted ? "Deleted" : "Active";

    std::cout << std::left
              << Color::WHITE
              << std::setw(6)  << ("  " + std::to_string(it.id))
              << std::setw(22) << ("  " + std::string(it.name))
              << std::setw(12) << ("  " + std::to_string(it.quantity))
              << "  " << Color::YELLOW << std::fixed << std::setprecision(2)
              << std::setw(12) << it.price
              << status_col << "  " << status_txt
              << Color::RESET << '\n';
}

void InventoryManager::print_item_footer() const {
    hline(TABLE_WIDTH, Color::BLUE);
    std::cout << '\n';
}

void InventoryManager::print_success(const std::string& msg) const {
    std::cout << '\n' << Color::GREEN << Color::BOLD
              << "  ✓ " << msg << Color::RESET << '\n';
}

void InventoryManager::print_error(const std::string& msg) const {
    std::cout << '\n' << Color::RED << Color::BOLD
              << "  ✗ " << msg << Color::RESET << '\n';
}

void InventoryManager::print_info(const std::string& msg) const {
    std::cout << '\n' << Color::YELLOW
              << "  ℹ " << msg << Color::RESET << '\n';
}

// ============================================================================
//  SECTION 5 — main()
// ============================================================================

int main() {
    Color::enable();   /* enable ANSI on Windows 10+ */

    try {
        InventoryManager mgr;
        mgr.run();
    } catch (const std::exception& ex) {
        std::cerr << Color::RED << "\n  FATAL: " << ex.what()
                  << Color::RESET << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << Color::RED << "\n  FATAL: Unknown exception."
                  << Color::RESET << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// ============================================================================
//  END OF FILE
// ============================================================================
