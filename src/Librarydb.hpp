#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "User.hpp"
#include "Book.hpp"

#include <memory>
#include <string>
#include <vector>

class Librarydb{
    public:
        SQLite::Database& getdb() {
            return *databs;
        }
        void init();
        SQLite::Statement getFavourites(std::string username);
        SQLite::Statement getBorrowed(std::string username);
        SQLite::Statement searchBook(std::string val);
        SQLite::Statement searchUser(std::string val);
        void addUser(User nuser, std::string password);
        void removeUser(std::string username);
        void addBook(Book nbook);
        void removeBook(std::string isbn);
        void addFavourite(std::string username, std::string isbn);
        void borrow(std::string username, std::string isbn);
        void unborrow(std::string username, std::string isbn);
        User restoreSession(std::string session);

        std::vector<std::string> getAllBooks(); // returns isbns of all books
        Book getBook(std::string isbn);

        std::string db_path;
    private:
        std::unique_ptr<SQLite::Database> databs;
        void populate();
};

inline std::unique_ptr<Librarydb> db;
