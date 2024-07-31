#include <iostream>
#include <pqxx/pqxx>
#include <Windows.h>
#pragma execution_character_set( "utf-8" )
#include <string>
#include <vector>

class ClientManager {
private:
    pqxx::connection* conn;

public:
    ClientManager(const std::string& connStr) {
        try {
            conn = new pqxx::connection(connStr);
            if (conn->is_open()) {
                std::cout << "Connected to database: " << conn->dbname() << std::endl;
            }
            else {
                std::cerr << "Failed to connect to database" << std::endl;
                exit(1);
            }


            createTables();
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    ~ClientManager() {
        delete conn;
    }

    void createTables() {
        try {
            pqxx::work txn(*conn);
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS clients (
                    id SERIAL PRIMARY KEY,
                    first_name TEXT NOT NULL,
                    last_name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
            )");

            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS phones (
                    id SERIAL PRIMARY KEY,
                    client_id INTEGER REFERENCES clients(id) ON DELETE CASCADE,
                    phone_number TEXT NOT NULL
                );
            )");
            txn.commit();
        }
        catch (const std::exception& e) {
            std::cerr << "Error creating tables: " << e.what() << std::endl;
        }
    }

    void addClient() {
        std::string firstName, lastName, email;
        std::cout << "Enter first name: ";
        std::cin >> firstName;
        std::cout << "Enter last name: ";
        std::cin >> lastName;
        std::cout << "Enter email: ";
        std::cin >> email;

        try {
            pqxx::work txn(*conn);
            pqxx::result res = txn.exec_params("INSERT INTO clients (first_name, last_name, email) VALUES ($1, $2, $3) RETURNING id;", firstName, lastName, email);
            int clientId = res[0][0].as<int>();

            txn.commit();
            std::cout << "Client added with ID: " << clientId << std::endl;


            char choice;
            std::cout << "Do you want to add phone numbers for this client? (y/n): ";
            std::cin >> choice;

            if (choice == 'y' || choice == 'Y') {
                addPhonesForClient(clientId);
            }

        }
        catch (const std::exception& e) {
            std::cerr << "Error adding client: " << e.what() << std::endl;
        }
    }

    void addPhonesForClient(int clientId) {
        char addMore;
        do {
            std::string phoneNumber;
            std::cout << "Enter phone number: ";
            std::cin >> phoneNumber;

            try {
                pqxx::work txn(*conn);
                txn.exec_params("INSERT INTO phones (client_id, phone_number) VALUES ($1, $2);", clientId, phoneNumber);
                txn.commit();
                std::cout << "Phone number added." << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Error adding phone number: " << e.what() << std::endl;
            }

            std::cout << "Do you want to add another phone number? (y/n): ";
            std::cin >> addMore;
        } while (addMore == 'y' || addMore == 'Y');
    }

    void addPhone() {
        int clientId;
        std::string phoneNumber;
        std::cout << "Enter client ID: ";
        std::cin >> clientId;
        std::cout << "Enter phone number: ";
        std::cin >> phoneNumber;

        try {
            pqxx::work txn(*conn);

            pqxx::result checkClient = txn.exec_params("SELECT id FROM clients WHERE id = $1;", clientId);
            if (checkClient.empty()) {
                std::cerr << "Client ID " << clientId << " does not exist." << std::endl;
                return;
            }


            txn.exec_params("INSERT INTO phones (client_id, phone_number) VALUES ($1, $2);", clientId, phoneNumber);
            txn.commit();
            std::cout << "Phone number added." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error adding phone number: " << e.what() << std::endl;
        }
    }

    void updateClient() {
        int clientId;
        std::cout << "Enter client ID to update: ";
        std::cin >> clientId;

        try {
            pqxx::work txn(*conn);
            pqxx::result r = txn.exec_params("SELECT first_name, last_name, email FROM clients WHERE id = $1;", clientId);

            if (r.empty()) {
                std::cerr << "Client ID " << clientId << " does not exist." << std::endl;
                return;
            }

            std::string firstName = r[0][0].as<std::string>();
            std::string lastName = r[0][1].as<std::string>();
            std::string email = r[0][2].as<std::string>();

            std::cout << "Current data: " << std::endl;
            std::cout << "First Name: " << firstName << std::endl;
            std::cout << "Last Name: " << lastName << std::endl;
            std::cout << "Email: " << email << std::endl;

            char choice;
            std::cout << "Do you want to update first name? (y/n): ";
            std::cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                std::cout << "Enter new first name: ";
                std::cin >> firstName;
                txn.exec_params("UPDATE clients SET first_name = $1 WHERE id = $2;", firstName, clientId);
            }

            std::cout << "Do you want to update last name? (y/n): ";
            std::cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                std::cout << "Enter new last name: ";
                std::cin >> lastName;
                txn.exec_params("UPDATE clients SET last_name = $1 WHERE id = $2;", lastName, clientId);
            }

            std::cout << "Do you want to update email? (y/n): ";
            std::cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                std::cout << "Enter new email: ";
                std::cin >> email;
                txn.exec_params("UPDATE clients SET email = $1 WHERE id = $2;", email, clientId);
            }

            txn.commit();
            std::cout << "Client updated." << std::endl;


            std::cout << "Do you want to update phone numbers? (y/n): ";
            std::cin >> choice;

            if (choice == 'y' || choice == 'Y') {
                pqxx::work txn(*conn);
                pqxx::result phones = txn.exec_params("SELECT id, phone_number FROM phones WHERE client_id = $1;", clientId);

                if (phones.empty()) {
                    std::cout << "No phone numbers found for this client." << std::endl;
                    char addPhone;
                    std::cout << "Do you want to add a phone number? (y/n): ";
                    std::cin >> addPhone;
                    if (addPhone == 'y' || addPhone == 'Y') {
                        std::cout << "Please add phone number using menu" << std::endl;
                        std::cout << std::endl;
                    }
                }
                else {
                    updatePhonesForClient(clientId);
                }
            }

        }
        catch (const std::exception& e) {
            std::cerr << "Error updating client: " << e.what() << std::endl;
        }
    }

    void updatePhonesForClient(int clientId) {
        try {
            pqxx::work txn(*conn);
            pqxx::result r = txn.exec_params("SELECT id, phone_number FROM phones WHERE client_id = $1 ORDER BY id;", clientId);

            if (r.empty()) {
                std::cout << "No phones found for this client." << std::endl;


                char addMore;
                std::cout << "Do you want to add a phone number? (y/n): ";
                std::cin >> addMore;

                if (addMore == 'y' || addMore == 'Y') {
                    addPhonesForClient(clientId);
                }
            }
            else {
                std::cout << "Current phone numbers:" << std::endl;
                for (int i = 0; i < r.size(); ++i) {
                    std::cout << i + 1 << ". " << r[i][1].as<std::string>() << std::endl;
                }

                char choice;
                std::cout << "Do you want to update a phone number? (y/n): ";
                std::cin >> choice;

                if (choice == 'y' || choice == 'Y') {
                    int phoneIndex;
                    std::cout << "Enter phone index to update: ";
                    std::cin >> phoneIndex;

                    if (phoneIndex < 1 || phoneIndex > r.size()) {
                        std::cerr << "Invalid phone index." << std::endl;
                        return;
                    }

                    std::string newPhoneNumber;
                    std::cout << "Enter new phone number: ";
                    std::cin >> newPhoneNumber;

                    int phoneId = r[phoneIndex - 1][0].as<int>();
                    txn.exec_params("UPDATE phones SET phone_number = $1 WHERE id = $2;", newPhoneNumber, phoneId);
                    txn.commit();
                    std::cout << "Phone number updated." << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error updating phones: " << e.what() << std::endl;
        }
    }

    void deletePhone() {
        int clientId, phoneIndex;
        std::cout << "Enter client ID: ";
        std::cin >> clientId;

        try {
            pqxx::work txn(*conn);
            pqxx::result r = txn.exec_params("SELECT id, phone_number FROM phones WHERE client_id = $1 ORDER BY id;", clientId);

            if (r.empty()) {
                std::cerr << "No phones found for this client." << std::endl;
                return;
            }

            std::cout << "Current phone numbers:" << std::endl;
            for (int i = 0; i < r.size(); ++i) {
                std::cout << i + 1 << ". " << r[i][1].as<std::string>() << std::endl;
            }

            std::cout << "Enter phone index to delete: ";
            std::cin >> phoneIndex;

            if (phoneIndex < 1 || phoneIndex > r.size()) {
                std::cerr << "Invalid phone index." << std::endl;
                return;
            }

            int phoneId = r[phoneIndex - 1][0].as<int>();
            txn.exec_params("DELETE FROM phones WHERE id = $1;", phoneId);
            txn.commit();
            std::cout << "Phone number deleted." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error deleting phone number: " << e.what() << std::endl;
        }
    }

    void deleteClient() {
        int clientId;
        std::cout << "Enter client ID to delete: ";
        std::cin >> clientId;

        try {
            pqxx::work txn(*conn);
            pqxx::result r = txn.exec_params("SELECT id FROM clients WHERE id = $1;", clientId);

            if (r.empty()) {
                std::cerr << "Client ID " << clientId << " does not exist." << std::endl;
                return;
            }

            txn.exec_params("DELETE FROM clients WHERE id = $1;", clientId);
            txn.commit();
            std::cout << "Client deleted." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error deleting client: " << e.what() << std::endl;
        }
    }

    void findClient() {
        std::string searchTerm;
        std::cout << "Enter search term (name, surname, email, or phone): ";
        std::cin >> searchTerm;

        try {
            pqxx::work txn(*conn);
            pqxx::result r = txn.exec_params(
                "SELECT c.id, c.first_name, c.last_name, c.email, p.phone_number "
                "FROM clients c "
                "LEFT JOIN phones p ON c.id = p.client_id "
                "WHERE c.first_name ILIKE $1 OR c.last_name ILIKE $1 OR c.email ILIKE $1 OR p.phone_number ILIKE $1 "
                "ORDER BY c.id;",
                "%" + searchTerm + "%"
            );

            if (r.empty()) {
                std::cout << "No clients found." << std::endl;
            }
            else {
                std::cout << "Found clients:" << std::endl;
                for (const auto& row : r) {
                    std::cout << "ID: " << row[0].as<int>() << ", "
                        << "Name: " << row[1].as<std::string>() << " " << row[2].as<std::string>() << ", "
                        << "Email: " << row[3].as<std::string>() << ", "
                        << "Phone: " << (row[4].is_null() ? "N/A" : row[4].as<std::string>()) << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error finding client: " << e.what() << std::endl;
        }
    }

    void menu() {
        int choice;
        while (true) {
            std::cout << "1. Add New Client" << std::endl;
            std::cout << "2. Add Phone Number" << std::endl;
            std::cout << "3. Update Client" << std::endl;
            std::cout << "4. Delete Phone Number" << std::endl;
            std::cout << "5. Delete Client" << std::endl;
            std::cout << "6. Find Client" << std::endl;
            std::cout << "7. Exit" << std::endl;
            std::cout << "Enter your choice: ";
            std::cin >> choice;

            switch (choice) {
            case 1:
                addClient();
                break;
            case 2:
                addPhone();
                break;
            case 3:
                updateClient();
                break;
            case 4:
                deletePhone();
                break;
            case 5:
                deleteClient();
                break;
            case 6:
                findClient();
                break;
            case 7:
                return;
            default:
                std::cout << "Invalid choice, please try again." << std::endl;
            }
        }
    }
};


int main() {
    std::string connStr = "dbname=homework5 user=postgres password=Suslik17 hostaddr=127.0.0.1 port=5432";
    ClientManager manager(connStr);
    manager.menu();
    return 0;
}