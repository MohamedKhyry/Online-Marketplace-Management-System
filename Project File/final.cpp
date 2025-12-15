/*
 * PROJECT: Online Marketplace Management System
 * DESCRIPTION: A C++ console application for buying and selling products.
 * Demonstrates the practical use of Data Structures:
 * - Vectors (Database Storage)
 * - Stacks (Shopping Cart - LIFO)
 * - Queues (Checkout Process - FIFO)
 * - Priority Queues (Rating System - Max Heap)
 *
 * FEATURES:
 * - Seller/Customer Login & Registration
 * - Product Inventory Management
 * - Shopping Cart with Undo functionality
 * - Order Processing & Receipt Generation
 * - Data Persistence via Text Files
 */

#include <iostream>
#include <vector>     // DATA STRUCTURE: Vector (Dynamic Arrays)
#include <string>
#include <algorithm>
#include <iomanip>    // For tables and output formatting
#include <fstream>    // For File I/O (Persistence)
#include <sstream>    // For string manipulation
#include <stack>      // DATA STRUCTURE: Stack (LIFO for Cart)
#include <queue>      // DATA STRUCTURE: Queue (FIFO for Checkout) & Priority Queue (Sorting)
#include <ctime>      // For Date/Time on receipt
#include <cstdlib>    // For system("cls") or system("clear")
#include <limits>     // For robust input clearing

using namespace std;

// ==========================================
// 1. DATA STRUCTURES & CLASSES
// ==========================================

// Represents a product in the marketplace
class Product {
public:
    int id;
    string name;
    double price;
    string category;
    int quantity;   // Current stock level
    int sellerId;   // Links product to a specific seller

    double ratingSum;
    int ratingCount;

    // Default Constructor
    Product() {}

    // Parameterized Constructor
    Product(int pid, string pname, double pprice, string pcat, int pqty, int sid, double rSum = 0, int rCount = 0) {
        id = pid;
        name = pname;
        price = pprice;
        category = pcat;
        quantity = pqty;
        sellerId = sid;
        ratingSum = rSum;
        ratingCount = rCount;
    }

    // Logic: Calculate average rating (Safe division)
    double getAverageRating() const {
        if (ratingCount == 0) return 0.0;
        return ratingSum / ratingCount;
    }

    // Logic: Update rating stats
    void addRating(double rate) {
        ratingSum += rate;
        ratingCount++;
    }

    // OPERATOR OVERLOADING
    // Required for Priority Queue comparisons.
    // Logic: Higher Average Rating = Higher Priority.
    bool operator<(const Product& other) const {
        return getAverageRating() < other.getAverageRating();
    }
};

// Represents a Seller user
class Seller {
public:
    int id;
    string name;
    string email;

    Seller(int sid, string sname, string semail) {
        id = sid;
        name = sname;
        email = semail;
    }
};

// Helper struct for items stored inside the cart
struct CartItem {
    Product product;
    int buyQty;
};

// Represents a Customer user
class Customer {
public:
    int id;
    string name;
    string address;
    string phone;
    string email;

    // DATA STRUCTURE: STACK
    // Reason: Allows the "Undo" feature. The last item added is the first to be removed (LIFO).
    //
    stack<CartItem> cartStack;

    Customer(int cid, string cname, string caddr, string cphone, string cemail) {
        id = cid;
        name = cname;
        address = caddr;
        phone = cphone;
        email = cemail;
    }
};

// ==========================================
// 2. SYSTEM MANAGER (MAIN CONTROLLER)
// ==========================================

class Marketplace {
private:
    // Main Databases
    // DATA STRUCTURE: VECTOR
    // Reason: Efficient random access (indexing) and dynamic resizing.
    vector<Seller> sellers;
    vector<Customer> customers;
    vector<Product> products;

    // ID Trackers (Auto-increment logic)
    int productCounter = 1;
    int sellerCounter = 1;
    int customerCounter = 1;

    // Session State (Stores index of currently logged-in user)
    int currentSellerIdx = -1;
    int currentCustomerIdx = -1;

public:
    Marketplace() {
        loadData(); // Load data from files on startup
    }

    ~Marketplace() {
        saveData(); // Save data to files on exit
    }

    // --- UTILITY: UI & INPUT HANDLING ---

    // Clears the console screen for a clean UI
    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    // Pauses execution to let user read messages
    void pause() {
        cout << "\nPress Enter to continue...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
    }

    // Robust Input: Prevents infinite loops if user enters text instead of numbers
    int getIntInput() {
        int choice;
        while (!(cin >> choice)) {
            cout << "Invalid input. Please enter a number: ";
            cin.clear(); // Clear error flag
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard bad input
        }
        return choice;
    }

    // --- FILE I/O OPERATIONS ---

    // Writes all runtime data to text files
    void saveData() {
        // 1. Save Sellers
        ofstream sFile("sellers.txt");
        for (const auto& s : sellers) sFile << s.id << "|" << s.name << "|" << s.email << endl;

        // 2. Save Customers
        ofstream cFile("customers.txt");
        for (const auto& c : customers) cFile << c.id << "|" << c.name << "|" << c.address << "|" << c.phone << "|" << c.email << endl;

        // 3. Save Products
        ofstream pFile("products.txt");
        for (const auto& p : products) {
            pFile << p.id << "|" << p.name << "|" << p.price << "|" << p.category << "|"
                  << p.quantity << "|" << p.sellerId << "|" << p.ratingSum << "|" << p.ratingCount << endl;
        }

        // 4. Save Carts (Persisting the Stack)
        // Format: CustomerID|ProductID|Quantity
        ofstream cartFile("carts.txt");
        for (const auto& c : customers) {
            if (!c.cartStack.empty()) {
                // Copy stack to temporary stack to read without destroying original
                stack<CartItem> tempStack = c.cartStack;
                while(!tempStack.empty()) {
                    CartItem item = tempStack.top();
                    tempStack.pop();
                    cartFile << c.id << "|" << item.product.id << "|" << item.buyQty << endl;
                }
            }
        }
    }

    // Reads data from text files into Vectors
    void loadData() {
        string line;

        // 1. Load Sellers
        ifstream sFile("sellers.txt");
        if(sFile.is_open()) {
            while (getline(sFile, line)) {
                stringstream ss(line); string segment; vector<string> data;
                while(getline(ss, segment, '|')) data.push_back(segment);
                if(data.size() >= 3) {
                    int id = stoi(data[0]);
                    sellers.push_back(Seller(id, data[1], data[2]));
                    if (id >= sellerCounter) sellerCounter = id + 1;
                }
            }
        }

        // 2. Load Customers
        ifstream cFile("customers.txt");
        if(cFile.is_open()) {
            while (getline(cFile, line)) {
                stringstream ss(line); string segment; vector<string> data;
                while(getline(ss, segment, '|')) data.push_back(segment);
                if(data.size() >= 5) {
                    int id = stoi(data[0]);
                    customers.push_back(Customer(id, data[1], data[2], data[3], data[4]));
                    if (id >= customerCounter) customerCounter = id + 1;
                }
            }
        }

        // 3. Load Products
        ifstream pFile("products.txt");
        if(pFile.is_open()) {
            while (getline(pFile, line)) {
                stringstream ss(line); string segment; vector<string> data;
                while(getline(ss, segment, '|')) data.push_back(segment);
                if(data.size() >= 8) {
                    int id = stoi(data[0]);
                    products.push_back(Product(id, data[1], stod(data[2]), data[3], stoi(data[4]), stoi(data[5]), stod(data[6]), stoi(data[7])));
                    if (id >= productCounter) productCounter = id + 1;
                }
            }
        }

        // 4. Load Carts
        // Reconstructs the customer stacks
        ifstream cartFile("carts.txt");
        if (cartFile.is_open()) {
            while (getline(cartFile, line)) {
                stringstream ss(line); string segment; vector<string> data;
                while(getline(ss, segment, '|')) data.push_back(segment);

                if(data.size() >= 3) {
                    int cid = stoi(data[0]);
                    int pid = stoi(data[1]);
                    int qty = stoi(data[2]);

                    // Locate Customer
                    Customer* targetCust = nullptr;
                    for (auto &c : customers) {
                        if (c.id == cid) {
                            targetCust = &c;
                            break;
                        }
                    }

                    // Locate Product
                    Product targetProd;
                    bool prodFound = false;
                    for (auto &p : products) {
                        if (p.id == pid) {
                            targetProd = p;
                            prodFound = true;
                            break;
                        }
                    }

                    // Push to stack if both exist
                    if (targetCust != nullptr && prodFound) {
                        CartItem item;
                        item.product = targetProd;
                        item.buyQty = qty;
                        targetCust->cartStack.push(item);
                    }
                }
            }
        }
    }

    // --- HELPER: Display ---
    void printHeader(string title) {
        cout << "\n========================================\n";
        cout << "   " << title << "\n";
        cout << "========================================\n";
    }

    // ==========================================
    // 3. SELLER MODULE
    // ==========================================

    void registerSeller() {
        clearScreen();
        string name, email;
        printHeader("Seller Registration");
        cout << "Enter Name: "; cin.ignore(); getline(cin, name);
        cout << "Enter Email: "; cin >> email;

        sellers.push_back(Seller(sellerCounter++, name, email));
        cout << "\n[SUCCESS] Welcome, " << name << "! You have been registered.\n";
        saveData();
        pause();
    }

    bool loginSeller() {
        clearScreen();
        string email;
        printHeader("Seller Login");
        cout << "Enter Email: "; cin >> email;
        // Search Logic: Linear Search through Vector
        for (int i = 0; i < sellers.size(); i++) {
            if (sellers[i].email == email) {
                currentSellerIdx = i;
                cout << "\n[SUCCESS] Welcome back, " << sellers[i].name << "!\n";
                pause();
                return true;
            }
        }
        cout << "\n[ERROR] Email not found.\n";
        pause();
        return false;
    }

    void sellerMenu() {
        int choice;
        do {
            clearScreen();
            printHeader("SELLER DASHBOARD");
            cout << "Logged in as: " << sellers[currentSellerIdx].name << endl;
            cout << "----------------------------------------\n";
            cout << "1. Add New Product\n";
            cout << "2. Logout\n";
            cout << "----------------------------------------\n";
            cout << "Enter Choice: ";
            choice = getIntInput();

            if (choice == 1) {
                string name, cat; double price; int qty;
                cout << "\nEnter Product Name: "; cin.ignore(); getline(cin, name);
                cout << "Enter Category: "; getline(cin, cat);
                cout << "Enter Price: $"; cin >> price;
                cout << "Enter Quantity: "; cin >> qty;

                products.push_back(Product(productCounter++, name, price, cat, qty, sellers[currentSellerIdx].id));
                cout << "\n[SUCCESS] Product '" << name << "' added successfully!\n";
                saveData();
                pause();
            }
        } while (choice != 2);
        currentSellerIdx = -1;
    }

    // ==========================================
    // 4. CUSTOMER MODULE
    // ==========================================

    void registerCustomer() {
        clearScreen();
        string name, email, addr, phone;
        printHeader("Customer Registration");
        cout << "Enter Name: "; cin.ignore(); getline(cin, name);
        cout << "Enter Email: "; cin >> email;
        cout << "Enter Address: "; cin.ignore(); getline(cin, addr);
        cout << "Enter Phone: "; cin >> phone;

        customers.push_back(Customer(customerCounter++, name, addr, phone, email));
        cout << "\n[SUCCESS] Welcome, " << name << "! Registration complete.\n";
        saveData();
        pause();
    }

    bool loginCustomer() {
        clearScreen();
        string email;
        printHeader("Customer Login");
        cout << "Enter Email: "; cin >> email;
        // Search Logic: Linear Search
        for (int i = 0; i < customers.size(); i++) {
            if (customers[i].email == email) {
                currentCustomerIdx = i;
                cout << "\n[SUCCESS] Welcome back, " << customers[i].name << "!\n";
                pause();
                return true;
            }
        }
        cout << "\n[ERROR] Email not found.\n";
        pause();
        return false;
    }

    // Utility: Table Formatting
    void displayProductTable(const vector<Product>& pList) {
        cout << "\n";
        cout << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Category" << setw(10) << "Price" << setw(10) << "Stock" << setw(10) << "Rating" << endl;
        cout << "--------------------------------------------------------------------------------\n";
        for (const auto& p : pList) {
            cout << left << setw(5) << p.id << setw(20) << p.name << setw(15) << p.category
                 << "$" << setw(9) << p.price << setw(10) << p.quantity << setw(10) << setprecision(2) << p.getAverageRating() << endl;
        }
        cout << "--------------------------------------------------------------------------------\n";
    }

    // FEATURE: TOP RATED PRODUCTS
    // DATA STRUCTURE: PRIORITY QUEUE (Max Heap)
    // Reason: Automatically orders products by rating (Operator < Overload) without manually sorting.
    void showTopRatedProducts() {
        clearScreen();
        priority_queue<Product> pq;
        for (const auto& p : products) pq.push(p);

        cout << "\n--- Recommended Products (By Rating) ---\n";
        vector<Product> temp;
        // Extract top elements from PQ
        while (!pq.empty()) {
            temp.push_back(pq.top());
            pq.pop();
        }
        displayProductTable(temp);
        pause();
    }

    // FEATURE: VIEW CART
    // Logic: Stack is LIFO, so we pop to a temp stack to display items, then (optionally) restore.
    void viewCart() {
        clearScreen();
        Customer* c = &customers[currentCustomerIdx];
        if (c->cartStack.empty()) {
            cout << "\n[INFO] Your Cart is Empty.\n";
            pause();
            return;
        }

        cout << "\n--- Your Shopping Cart ---\n";
        stack<CartItem> tempStack = c->cartStack; // Copy stack
        double currentTotal = 0;

        while(!tempStack.empty()) {
            CartItem item = tempStack.top();
            tempStack.pop();
            double itemTotal = item.product.price * item.buyQty;
            currentTotal += itemTotal;
            cout << "* " << item.product.name << " (Qty: " << item.buyQty << ") - $" << itemTotal << endl;
        }
        cout << "--------------------------\n";
        cout << "Total Estimate: $" << currentTotal << endl;
        pause();
    }

    // CUSTOMER DASHBOARD
    void customerMenu() {
        int choice;
        Customer* c = &customers[currentCustomerIdx];

        do {
            clearScreen();
            printHeader("CUSTOMER DASHBOARD");
            cout << "Logged in as: " << c->name << endl;
            cout << "----------------------------------------\n";
            cout << "1. Browse All Products (By Rating)\n";
            cout << "2. Filter by Category\n";
            cout << "3. Search by Name\n"; // --- NEW OPERATION ---
            cout << "4. Add Product to Cart\n";
            cout << "5. View Cart\n";
            cout << "6. Undo Last Item (Remove from Cart)\n";
            cout << "7. Checkout\n";
            cout << "8. Logout\n";
            cout << "----------------------------------------\n";
            cout << "Enter Choice: ";
            choice = getIntInput();

            if (choice == 1) showTopRatedProducts();
            else if (choice == 2) {
                // Filter Logic
                clearScreen();
                string cat;
                cout << "Enter Category Name: "; cin.ignore(); getline(cin, cat);
                vector<Product> filtered;
                for(auto &p : products) {
                    if(p.category == cat) filtered.push_back(p);
                }
                if(filtered.empty()) cout << "\n[INFO] No products found in this category.\n";
                else displayProductTable(filtered);
                pause();
            }
            // --- NEW OPERATION IMPLEMENTATION ---
            else if (choice == 3) {
                clearScreen();
                string searchName;
                cout << "Enter Product Name (Partial or Full): "; cin.ignore(); getline(cin, searchName);
                vector<Product> filtered;
                for(auto &p : products) {
                    // Check if searchName is inside the product name
                    if(p.name.find(searchName) != string::npos) {
                        filtered.push_back(p);
                    }
                }
                if(filtered.empty()) cout << "\n[INFO] No products found matching '" << searchName << "'.\n";
                else displayProductTable(filtered);
                pause();
            }
            // ------------------------------------
            else if (choice == 4) {
                // Add to Cart Logic
                int pid, qty;
                cout << "Enter Product ID: "; cin >> pid;
                cout << "Enter Quantity: "; cin >> qty;

                bool found = false;
                for(auto &p : products) {
                    if(p.id == pid) {
                        found = true;
                        if (qty > p.quantity) {
                            cout << "\n[ERROR] Insufficient Stock! Only " << p.quantity << " available.\n";
                        } else {
                            CartItem item; item.product = p; item.buyQty = qty;
                            c->cartStack.push(item); // Push to Stack
                            cout << "\n[SUCCESS] Added " << qty << " x " << p.name << " to cart.\n";
                            saveData(); // Auto-save
                        }
                        break;
                    }
                }
                if(!found) cout << "\n[ERROR] Product ID not found.\n";
                pause();
            }
            else if (choice == 5) {
                viewCart();
            }
            else if (choice == 6) {
                // Undo Logic: Pop from Stack
                if(!c->cartStack.empty()) {
                    cout << "\n[REMOVED] " << c->cartStack.top().product.name << " removed from cart.\n";
                    c->cartStack.pop();
                    saveData();
                } else {
                    cout << "\n[INFO] Cart is already empty.\n";
                }
                pause();
            }
            else if (choice == 7) {
                processCheckout();
            }

        } while (choice != 8);

        currentCustomerIdx = -1;
    }

    // FEATURE: CHECKOUT
    // DATA STRUCTURE: QUEUE
    // Reason: Simulates a checkout line (First In, First Out) processing of items.
    void processCheckout() {
        clearScreen();
        Customer* c = &customers[currentCustomerIdx];
        if (c->cartStack.empty()) {
            cout << "\n[INFO] Cart is empty. Add items before checking out.\n";
            pause();
            return;
        }

        // Transfer items from Cart (Stack) to Checkout Line (Queue)
        queue<CartItem> checkoutQueue;
        while (!c->cartStack.empty()) {
            checkoutQueue.push(c->cartStack.top());
            c->cartStack.pop();
        }

        double total = 0;
        printHeader("OFFICIAL RECEIPT");

        // Timestamp
        time_t now = time(0);
        char* dt = ctime(&now);
        cout << "Date: " << dt;
        cout << "----------------------------------------\n";

        // Process Queue
        while (!checkoutQueue.empty()) {
            CartItem item = checkoutQueue.front();
            checkoutQueue.pop();

            bool stockAvailable = false;
            // Update live stock in Product Vector
            for(auto &p : products) {
                if(p.id == item.product.id) {
                    if (p.quantity >= item.buyQty) {
                        p.quantity -= item.buyQty; // Deduct Stock
                        total += (item.product.price * item.buyQty);
                        cout << left << setw(20) << item.product.name << " x " << item.buyQty << " = $" << (item.product.price * item.buyQty) << endl;
                        stockAvailable = true;

                        // RATING LOGIC (Fixed Range 1-5)
                        cout << "   -> Rate " << p.name << " (1-5): ";
                        int r;
                        while (true) {
                            if (cin >> r && r >= 1 && r <= 5) {
                                p.addRating(r);
                                break;
                            } else {
                                cout << "      [Invalid] Please enter 1-5: ";
                                cin.clear();
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            }
                        }

                    } else {
                        cout << "[ERROR] Could not process " << item.product.name << ". Stock insufficient.\n";
                    }
                    break;
                }
            }
        }
        cout << "----------------------------------------\n";
        cout << "TOTAL PAID: $" << total << endl;
        cout << "----------------------------------------\n";
        cout << "Thank you for your purchase!\n";
        saveData(); // Commit changes to files
        pause();
    }

    // MAIN LOOP
    void run() {
        int mainChoice;
        while (true) {
            clearScreen();
            printHeader("ONLINE MARKETPLACE SYSTEM");
            cout << "1. Seller Menu\n";
            cout << "2. Customer Menu\n";
            cout << "3. Exit\n";
            cout << "----------------------------------------\n";
            cout << "Enter Choice: ";
            mainChoice = getIntInput();

            if (mainChoice == 1) {
                clearScreen();
                int c;
                cout << "\n1. Register New Seller\n2. Login\nChoice: ";
                c = getIntInput();
                if (c == 1) registerSeller();
                else if(loginSeller()) sellerMenu();
            }
            else if (mainChoice == 2) {
                clearScreen();
                int c;
                cout << "\n1. Register New Customer\n2. Login\nChoice: ";
                c = getIntInput();
                if (c == 1) registerCustomer();
                else if(loginCustomer()) customerMenu();
            }
            else break;
        }
    }
};

// ==========================================
// 5. MAIN EXECUTION
// ==========================================
int main() {
    Marketplace system;
    system.run();
    return 0;
}