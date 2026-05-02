/*
 * main.cpp  –  entry point; drives the console menu.
 */

#include "InventoryManager.hpp"

#include <iostream>
#include <limits>

static void printMenu()
{
    std::cout << "\n"
              << "==============================\n"
              << "   Inventory Manager\n"
              << "==============================\n"
              << "  1  Add item\n"
              << "  2  View item\n"
              << "  3  Update item\n"
              << "  4  Delete item\n"
              << "  5  List all\n"
              << "  6  Exit\n"
              << "------------------------------\n"
              << "  Choice: ";
}

int main()
{
    InventoryManager mgr;

    while (true) {
        printMenu();

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [!] Please enter a number 1-6.\n";
            continue;
        }

        switch (choice) {
        case 1: mgr.menuAdd();    break;
        case 2: mgr.menuView();   break;
        case 3: mgr.menuUpdate(); break;
        case 4: mgr.menuDelete(); break;
        case 5: mgr.menuList();   break;
        case 6:
            std::cout << "  Bye!\n";
            return 0;
        default:
            std::cout << "  [!] Invalid choice. Enter 1-6.\n";
        }
    }
}
