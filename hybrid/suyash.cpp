#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
using namespace std;

#define FILE_NAME "inventory.dat"

// ================= STRUCT =================
struct Item {
    int id;
    char name[40];
    int quantity;
    float price;
};

// ================= LOAD FROM FILE =================
vector<Item> loadItems() {
    vector<Item> items;
    ifstream file(FILE_NAME, ios::binary);

    Item it;
    while (file.read((char*)&it, sizeof(it))) {
        items.push_back(it);
    }

    file.close();
    return items;
}

// ================= SAVE TO FILE =================
void saveItems(vector<Item>& items) {
    ofstream file(FILE_NAME, ios::binary);

    for (auto& it : items) {
        file.write((char*)&it, sizeof(it));
    }

    file.close();
}

// ================= ADD ITEM =================
void addItem() {
    vector<Item> items = loadItems();

    Item it;
    cout << "Enter ID: ";
    cin >> it.id;

    // check duplicate
    for (auto& x : items) {
        if (x.id == it.id) {
            cout << "ID already exists!\n";
            return;
        }
    }

    cout << "Enter Name: ";
    cin.ignore();
    cin.getline(it.name, 40);

    cout << "Enter Quantity: ";
    cin >> it.quantity;

    cout << "Enter Price: ";
    cin >> it.price;

    items.push_back(it);
    saveItems(items);

    cout << "Item added successfully!\n";
}

// ================= VIEW ITEM =================
void viewItem() {
    vector<Item> items = loadItems();

    int id;
    cout << "Enter ID: ";
    cin >> id;

    for (auto& it : items) {
        if (it.id == id) {
            cout << "\nFound:\n";
            cout << "ID: " << it.id << endl;
            cout << "Name: " << it.name << endl;
            cout << "Qty: " << it.quantity << endl;
            cout << "Price: " << it.price << endl;
            return;
        }
    }

    cout << "Item not found!\n";
}

// ================= LIST =================
void listItems() {
    vector<Item> items = loadItems();

    if (items.empty()) {
        cout << "No items found!\n";
        return;
    }

    cout << "\nAll Items:\n";
    for (auto& it : items) {
        cout << "ID: " << it.id
             << " | Name: " << it.name
             << " | Qty: " << it.quantity
             << " | Price: " << it.price << endl;
    }
}

// ================= DELETE =================
void deleteItem() {
    vector<Item> items = loadItems();

    int id;
    cout << "Enter ID to delete: ";
    cin >> id;

    bool found = false;

    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->id == id) {
            items.erase(it);
            found = true;
            break;
        }
    }

    saveItems(items);

    if (found)
        cout << "Item deleted!\n";
    else
        cout << "Item not found!\n";
}

// ================= MENU =================
int main() {
    int choice;

    while (true) {
        cout << "\n==== Inventory Manager (Vector) ====\n";
        cout << "1. Add Item\n";
        cout << "2. View Item\n";
        cout << "3. List All\n";
        cout << "4. Delete Item\n";
        cout << "5. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
            case 1: addItem(); break;
            case 2: viewItem(); break;
            case 3: listItems(); break;
            case 4: deleteItem(); break;
            case 5: return 0;
            default: cout << "Invalid choice!\n";
        }
    }
}