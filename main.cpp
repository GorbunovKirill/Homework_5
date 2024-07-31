#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <stdexcept>


struct ClientData {
    int id;
    std::string firstName;
    std::string lastName;
    std::string email;
};


class ClientManager {
public:

    ClientManager(const std::string& connStr) : connection(connStr) {
        if (connection.is_open()) {
            std::cout << "Connected to database: " << connection.dbname() << std::endl;
        }
        else {
            throw std::runtime_error("Failed to connect to database");
        }
    }

    void initDbStructure() {
        pqxx::work txn(connection);
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

    int addClient(const std::string& firstName, const std::string& lastName, const std::string& email) {
        pqxx::work txn(connection);
        pqxx::result res = txn.exec_params("INSERT INTO clients (first_name, last_name, email) VALUES ($1, $2, $3) RETURNING id;", firstName, lastName, email);
        int clientId = res[0][0].as<int>();
        txn.commit();
        return clientId;
    }

    void addPhoneNumber(int clientId, const std::string& phoneNumber) {
        pqxx::work txn(connection);
        txn.exec_params("INSERT INTO phones (client_id, phone_number) VALUES ($1, $2);", clientId, phoneNumber);
        txn.commit();
    }


    void updateClient(int clientId, const std::string& firstName, const std::string& lastName, const std::string& email) {
        pqxx::work txn(connection);
        txn.exec_params("UPDATE clients SET first_name = $1, last_name = $2, email = $3 WHERE id = $4;", firstName, lastName, email, clientId);
        txn.commit();
    }


    void removeClient(int clientId) {
        pqxx::work txn(connection);
        txn.exec_params("DELETE FROM clients WHERE id = $1;", clientId);
        txn.commit();
    }


    void findClient(const std::string& searchValue) {
        pqxx::work txn(connection);
        pqxx::result r = txn.exec_params(
            "SELECT c.id, c.first_name, c.last_name, c.email, p.phone_number "
            "FROM clients c "
            "LEFT JOIN phones p ON c.id = p.client_id "
            "WHERE c.first_name ILIKE $1 OR c.last_name ILIKE $1 OR c.email ILIKE $1 OR p.phone_number ILIKE $1 "
            "ORDER BY c.id;",
            "%" + searchValue + "%"
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


    void removePhoneNumber(int clientId, int phoneIndex) {
        pqxx::work txn(connection);
        pqxx::result r = txn.exec_params(
            "SELECT id FROM phones WHERE client_id = $1 ORDER BY id LIMIT 1 OFFSET $2;",
            clientId, phoneIndex - 1
        );

        if (r.empty()) {
            throw std::runtime_error("Phone index is invalid.");
        }

        int phoneId = r[0][0].as<int>();
        txn.exec_params("DELETE FROM phones WHERE id = $1;", phoneId);
        txn.commit();
    }

private:
    pqxx::connection connection;
};


int main() {
    try {
        std::string connStr =
            "dbname=homework5 "
            "user=postgres "
            "password=Suslik17 "
            "hostaddr=127.0.0.1 "
            "port=5432 ";

        ClientManager manager(connStr);
        manager.initDbStructure();

        int choice;
        while (true) {
            std::cout << "1. Add New Client" << std::endl;
            std::cout << "2. Add Phone Number" << std::endl;
            std::cout << "3. Update Client" << std::endl;
            std::cout << "4. Remove Phone Number" << std::endl;
            std::cout << "5. Remove Client" << std::endl;
            std::cout << "6. Find Client" << std::endl;
            std::cout << "7. Exit" << std::endl;
            std::cout << "Enter your choice: ";
            std::cin >> choice;

            switch (choice) {
            case 1: {
                std::string firstName, lastName, email;
                std::cout << "Enter first name: ";
                std::cin >> firstName;
                std::cout << "Enter last name: ";
                std::cin >> lastName;
                std::cout << "Enter email: ";
                std::cin >> email;
                int clientId = manager.addClient(firstName, lastName, email);
                std::cout << "Client added with ID: " << clientId << std::endl;
                break;
            }
            case 2: {
                int clientId;
                std::string phoneNumber;
                std::cout << "Enter client ID: ";
                std::cin >> clientId;
                std::cout << "Enter phone number: ";
                std::cin >> phoneNumber;
                manager.addPhoneNumber(clientId, phoneNumber);
                std::cout << "Phone number added." << std::endl;
                break;
            }
            case 3: {
                int clientId;
                std::string firstName, lastName, email;
                std::cout << "Enter client ID to update: ";
                std::cin >> clientId;
                std::cout << "Enter new first name: ";
                std::cin >> firstName;
                std::cout << "Enter new last name: ";
                std::cin >> lastName;
                std::cout << "Enter new email: ";
                std::cin >> email;
                manager.updateClient(clientId, firstName, lastName, email);
                std::cout << "Client updated." << std::endl;
                break;
            }
            case 4: {
                int clientId, phoneIndex;
                std::cout << "Enter client ID: ";
                std::cin >> clientId;
                std::cout << "Enter phone index to delete: ";
                std::cin >> phoneIndex;
                manager.removePhoneNumber(clientId, phoneIndex);
                std::cout << "Phone number removed." << std::endl;
                break;
            }
            case 5: {
                int clientId;
                std::cout << "Enter client ID to delete: ";
                std::cin >> clientId;
                manager.removeClient(clientId);
                std::cout << "Client removed." << std::endl;
                break;
            }
            case 6: {
                std::string searchValue;
                std::cout << "Enter search value: ";
                std::cin >> searchValue;
                manager.findClient(searchValue);
                break;
            }
            case 7:
                return 0;
            default:
                std::cerr << "Invalid choice. Please try again." << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

