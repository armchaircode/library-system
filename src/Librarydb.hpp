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

typedef std::pair<std::vector<std::size_t>, std::vector<std::string>> BookStack;

class Librarydb{
    public:
        Librarydb(const std::string& dbfile) : db_path(dbfile) { init(); }

        BookStack getFavourites(std::string username);
        BookStack getBorrowed(std::string username);
        BookStack getAllBooks(); // returns book_id of all books
        Book getBook(std::size_t book_id);


        std::vector<std::string> searchBook(std::string val);
        SQLite::Statement searchUser(std::string val);

        void addUser(const User nuser, const std::string password);
        void removeUser(const std::string username);

        void addBook(const Book nbook);
        void removeBook(std::size_t book_id);

        void addFavourite(std::string username, std::size_t book_id);
        void removeFavourite(std::string username, std::size_t book_id);

        void borrow(std::string username, std::size_t book_id);
        void unborrow(std::string username, std::size_t book_id);

        std::optional<User> restoreSession(std::size_t session);
        void newSession(std::string username, std::size_t session);
        void clearSession(std::string username);

        std::optional<User> authenticate(const std::string username, const std::string password);

        bool usernameExists(const std::string& username);
        bool emailIsUsed(const std::string& email);

    private:
        void init();
        std::string db_path;
        std::unique_ptr<SQLite::Database> databs;
        void makeSchema();
};

inline std::unique_ptr<Librarydb> db;
