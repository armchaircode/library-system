#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "User.hpp"
#include "Book.hpp"

#include <memory>
#include <string>

class Librarydb{
    public:
        SQLite::Database& getdb() {
            return *databs;
        }
        void init();
        SQLite::Statement getfavourites(std::string& username);
        SQLite::Statement searchBook(std::string& val);
        SQLite::Statement searchUser(std::string& val);
        void addUser(User& nuser);
        void removeUser(std::string& username);
        void addBook(Book& nbook);
        void removeBook(std::string& isbn);
        void addFavourite(std::string& username, std::string& isbn);
        void borrow(std::string& username, std::string& isbn);
        void unborrow(std::string& username, std::string& isbn);
        std::string db_path;
    private:
        std::unique_ptr<SQLite::Database> databs;
        void populate();
};

inline std::unique_ptr<Librarydb> db;
