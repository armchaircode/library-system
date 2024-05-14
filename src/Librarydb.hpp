#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "User.hpp"
#include "Book.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Librarydb{
    public:
        SQLite::Database& getdb() {
            return *databs;
        }
        void init();
        std::vector<std::string> getFavourites(std::string username);
        std::vector<std::string> getBorrowed(std::string username);
        std::vector<std::string> searchBook(std::string val);
        SQLite::Statement searchUser(std::string val);
        void addUser(User nuser, std::string password);
        void removeUser(std::string username);
        void addBook(Book nbook);
        void removeBook(std::string book_id);
        void addFavourite(std::string username, std::string book_id);
        void borrow(std::string username, std::string book_id);
        void unborrow(std::string username, std::string book_id);

        std::optional<User> restoreSession(std::size_t session);
        void newSession(std::string username, std::size_t session);
        void clearSession(std::string username);

        std::vector<std::string> getAllBooks(); // returns isbns of all books
        std::string getBookDetail(std::string book_id);

        std::optional<User> authenticate(const std::string username, const std::string password);

        bool usernameExists(const std::string& username);
        bool emailIsUsed(const std::string& email);

        std::string db_path;
    private:
        std::unique_ptr<SQLite::Database> databs;
        void makeSchema();
};

inline std::unique_ptr<Librarydb> db;
