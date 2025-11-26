#include <iostream>
#include <vector>
#include <string>
using namespace std;

struct Booking {
    int id;
    string name;
    string date;
    string route;
    string status;
};

class BookingSystem {
    vector<Booking> bookings;
    int nextId = 1;

public:
    void checkAvailability() {
        cout << "\nAvailable dates and routes:\n";
        cout << "2024-01-15: Delhi-Mumbai, Mumbai-Chennai\n";
        cout << "2024-01-16: Delhi-Kolkata, Chennai-Mumbai\n";
    }

    void makeBooking() {
        Booking b;
        b.id = nextId++;
        
        cout << "\nEnter passenger name: ";
        cin.ignore();
        getline(cin, b.name);
        
        cout << "Enter date (YYYY-MM-DD): ";
        cin >> b.date;
        
        cout << "Enter route: ";
        cin >> b.route;
        
        b.status = "Confirmed";
        bookings.push_back(b);
        
        cout << "Booking confirmed! ID: " << b.id << endl;
    }

    void cancelBooking() {
        int id;
        cout << "\nEnter booking ID to cancel: ";
        cin >> id;
        
        for (auto& b : bookings) {
            if (b.id == id) {
                b.status = "Cancelled";
                cout << "Booking cancelled!\n";
                return;
            }
        }
        cout << "Booking not found!\n";
    }

    void showChart() {
        cout << "\nAll Bookings:\n";
        cout << "ID\tName\tRoute\tStatus\n";
        for (auto& b : bookings) {
            cout << b.id << "\t" << b.name << "\t" << b.route << "\t" << b.status << endl;
        }
    }
};

int main() {
    BookingSystem system;
    int choice;
    
    while(true) {
        cout << "\n1. Check Availability\n2. Make Booking\n3. Cancel Booking\n4. Show Chart\n5. Exit\nChoice: ";
        cin >> choice;
        
        switch(choice) {
            case 1: system.checkAvailability(); break;
            case 2: system.makeBooking(); break;
            case 3: system.cancelBooking(); break;
            case 4: system.showChart(); break;
            case 5: return 0;
            default: cout << "Invalid choice!\n";
        }
    }
}