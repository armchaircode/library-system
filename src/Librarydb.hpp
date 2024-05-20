#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "User.hpp"
#include "Book.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

class Librarydb{
    public:
        Librarydb(const std::string& dbfile) : db_path(dbfile) { init(); }

        BookStack getFavourites(std::string username);
        BookStack getBorrowed(std::string username);
        BookStack getAllBooks(); // returns an array of Books
        Book getBook(std::size_t book_id);

        Users getAllUsers(); // returns an array of User

        void addUser(const User nuser, const std::string password);
        void removeUser(const std::string username);

        void addBook(const Book& book);
        void removeBook(std::size_t book_id);

        void addFavourite(std::string username, std::size_t book_id);
        void removeFavourite(std::string username, std::size_t book_id);
        void unfavouriteAll(std::string username);

        void borrow(std::string username, std::size_t book_id);
        void unborrow(std::string username, std::size_t book_id);
        void unborrowAll(std::string username);

        std::optional<User> restoreSession(std::size_t session);
        void newSession(std::string username, std::size_t session);
        void clearSession(std::string username);

        std::optional<User> authenticate(const std::string username, const std::string password);

        bool usernameExists(const std::string& username);
        bool emailIsUsed(const std::string& email);
        void changePassword(std::string& username, std::string& password);

    private:
        void init();
        std::string db_path;
        std::unique_ptr<SQLite::Database> databs;
        void makeSchema();
        std::optional<Book> extractBookInfo(const SQLite::Statement& stmnt);
        std::optional<User> extractUserInfo(const SQLite::Statement& stmnt);
};

inline std::unique_ptr<Librarydb> db;
