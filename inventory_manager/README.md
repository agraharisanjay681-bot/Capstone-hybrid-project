# Inventory Manager

A console-based inventory management system that demonstrates **C / C++ interoperability**:

| Layer | Language | Responsibility |
|---|---|---|
| Storage backend | C (C11) | Binary file I/O with `fread`/`fwrite`/`fseek` |
| Application layer | C++ (C++17) | Menu, input validation, STL containers |

---

## File Structure

```
inventory_manager/
├── include/
│   ├── inventory.h          ← C struct + C-linkage API (extern "C")
│   └── InventoryManager.hpp ← C++ class wrapping the C backend
├── src/
│   ├── inventory.c          ← C implementation (binary file storage)
│   ├── InventoryManager.cpp ← C++ class + STL (vector, sort)
│   └── main.cpp             ← Entry point / menu loop
├── Makefile                 ← Primary build system
├── CMakeLists.txt           ← Alternative (CMake) build
└── README.md
```

Data is persisted to **`inventory.dat`** (created automatically in the working directory).

---

## Build & Run

### Option A – Make (recommended)

```bash
# Build
make

# Run
./inventory_manager

# Clean (removes build artefacts and inventory.dat)
make clean
```

### Option B – CMake

```bash
mkdir build && cd build
cmake ..
make
./inventory_manager
```

### Requirements

| Tool | Minimum version |
|---|---|
| GCC / Clang | GCC 7+ or Clang 6+ |
| GNU Make | 3.81+ |
| CMake (optional) | 3.14+ |

---

## Menu Options

```
1  Add item      – enter ID, name, qty, price; duplicates rejected
2  View item     – look up one active item by ID
3  Update item   – overwrite name / qty / price of an existing record
4  Delete item   – soft-delete (record stays in file, hidden from views)
5  List all      – show all active items, sorted by ID or name
6  Exit
```

---

## Item Data Format (C struct)

```c
typedef struct {
    int   id;            // unique positive integer
    char  name[40];      // non-empty, max 39 chars
    int   quantity;      // >= 0
    float price;         // >= 0.0
    int   is_deleted;    // 0 = active, 1 = soft-deleted
} Item;
```

---

## Design Notes

### C Backend (`inventory.c`)

* Records are appended sequentially; each occupies `sizeof(Item)` bytes.
* `fseek(fp, n * sizeof(Item), SEEK_SET)` jumps to the nth record for in-place updates and soft deletes.
* No external libraries; only `<stdio.h>` and `<string.h>`.

### C++ Layer (`InventoryManager.cpp`)

* **`std::vector<Item>`** – collects active records returned by `list_items()`.
* **`std::sort`** – sorts the vector by ID or name before printing (lambda comparators).
* All user input is validated in a loop before the C functions are called.

---

## Test Cases

The following scenarios were verified manually (add 3 items → exit → restart → verify):

- **Persistence after restart**
  Add items 1 (`Widget`, qty 10, $1.99), 2 (`Gadget`, qty 5, $9.99), 3 (`Doohickey`, qty 20, $0.49).
  Exit. Restart. *List all* shows all three items with correct values. ✅

- **Update persists after restart**
  Update item 2: change name to `Gadget Pro`, price to $14.99.
  Exit. Restart. *View item 2* shows the updated values. ✅

- **Soft delete hides record**
  Delete item 1. *List all* shows only items 2 and 3.
  *View item 1* returns "not found". ✅

- **Duplicate ID rejected**
  Attempt to add a new item with ID 2 (already exists).
  System prints an error; item count stays the same. ✅

- **Invalid input re-prompts**
  Enter `-5` for ID → error, re-asks.
  Enter `-1` for quantity → error, re-asks.
  Enter `-0.5` for price → error, re-asks.
  Enter empty string for name → error, re-asks.
  All inputs eventually accepted after valid values are provided. ✅
