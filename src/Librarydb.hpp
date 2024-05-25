#pragma once

#include "User.hpp"
#include "Book.hpp"

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"

#include <cstddef> // size_t
#include <memory> // unique_ptr
#include <string> //string

class Librarydb{
    public:
        Librarydb(const std::string& dbfile) : db_path(dbfile) { init(); }

        BookStack getFavourites(const std::string username);
        BookStack getBorrowed(const std::string username);
        BookStack getAllBooks(); // returns an array of Books
        BookPtr getBook(const std::size_t book_id);

        Users getAllUsers(); // returns an array of User

        void addUser(const UserPtr& nuser, const std::string password);
        void removeUser(const std::string username);

        void addBook(const BookPtr& book);
        void removeBook(std::size_t book_id);

        void addFavourite(const std::string username, const std::size_t book_id);
        void removeFavourite(const std::string username, const std::size_t book_id);

        void borrow(const std::string username, const std::size_t book_id);
        void unborrow(const std::string username, const std::size_t book_id);

        UserPtr restoreSession(std::size_t session);
        void newSession(const std::string username, std::size_t session);
        void clearSession(const std::string username);

        UserPtr authenticate(const std::string username, const std::string password);

        bool usernameExists(const std::string& username);
        bool emailIsUsed(const std::string& email);
        void changePassword(const std::string& username, const std::string& password);

        void makeAdmin(const std::string& username);
        void demoteAdmin(const std::string& username);

    private:
        void init();
        std::string db_path;
        std::unique_ptr<SQLite::Database> databs;
        void makeSchema();
        BookPtr extractBookInfo(const SQLite::Statement& stmnt);
        UserPtr extractUserInfo(const SQLite::Statement& stmnt);
};

inline std::unique_ptr<Librarydb> db;
