#include "Librarydb.hpp"
#include "SQLiteCpp/Exception.h"
#include "SQLiteCpp/Statement.h"
#include "SQLiteCpp/Transaction.h"
#include "User.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <iostream>
#include <exception>


// forward declarations
std::string extractBookData(const SQLite::Statement& stmnt);

void Librarydb::init() {
    if(db_path.empty()) {
        return;
    }
    databs = std::make_unique<SQLite::Database>(db_path, SQLite::OPEN_READWRITE);
    if(not databs->tableExists("users"))
        makeSchema();
}

void Librarydb::makeSchema(){
    // Create users table
    SQLite::Transaction trxn(*databs);
    try
    {
    databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [users]
                 (
                    [username] VARCHAR(20) PRIMARY KEY NOT NULL,
                    [email] VARCHAR(50) NOT NULL UNIQUE,
                    [password] VARCHAR(50) NOT NULL,
                    [type] VARCHAR(7) NOT NULL DEFAULT 'Regular',
                    CHECK ([type] IN ('Admin', 'Regular'))
                 )
                 )#");
    // Default admin account
    databs->exec(R"#(
                 INSERT INTO [users] (email, username, password, type)
                    VALUES ('root@library.me', 'Root', '1234567890', 'Admin')
                 )#");
    // Create books table
    databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [books]
                 (
                    [book_id] INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                    [title] VARCHAR(100) NOT NULL,
                    [author] VARCHAR(50) NOT NULL,
                    [publisher] VARCHAR(50) NOT NULL,
                    [pub_year] INTEGER(4) NOT NULL,
                    [description] VARCHAR(200),
                    [edtion] INTEGER(2),
                    [rating] NUMERIC(2,1)
                 )
                 )#");
    // Create favourites relationship table
    databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [favourites]
                 (
                    [username] VARCHAR(50) NOT NULL,
                    [book_id] INTEGER NOT NULL,
                    CONSTRAINT [pk_favourites] PRIMARY KEY (username, book_id),
                    FOREIGN KEY ([username]) REFERENCES [users] (username)
                        ON DELETE CASCADE ON UPDATE NO ACTION,
                    FOREIGN KEY (book_id) REFERENCES [books] (book_id)
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");
    // Crate borrowed relationship table
     databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [borrows]
                 (
                    [username] VARCHAR(50) NOT NULL,
                    [book_id] INTEGER NOT NULL,
                    CONSTRAINT [pk_favourites] PRIMARY KEY (username, book_id),
                    FOREIGN KEY (username) REFERENCES [users] (username)
                        ON DELETE CASCADE ON UPDATE NO ACTION,
                    FOREIGN KEY (book_id) REFERENCES [books] (book_id)
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");

    databs->exec(R"#(
                CREATE TABLE IF NOT EXISTS [sessions] (
                    [username] VARCHAR(50) NOT NULL PRIMARY KEY,
                    [session] INTEGER UNIQUE,
                    FOREIGN KEY (username) REFERENCES [users] (username)
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");
    trxn.commit();
    }
    catch(SQLite::Exception& e) {
        std::cerr << "[ERROR: Failed to create database schema. Sqlite errored out with <"<<e.what()<<">\n";
        trxn.rollback();
        throw std::exception();
    }
}

std::optional<User> Librarydb::restoreSession(std::size_t session){
    auto query = R"#(
        SELECT [users].[username], [email], [type]
            FROM [users] JOIN [sessions]
                ON users.username = sessions.username
            WHERE sessions.session = ?
    )#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, static_cast<int64_t>(session));
    if(stmnt.executeStep()) {
        User usr;
        usr.username = stmnt.getColumn(0).getString();
        usr.email = stmnt.getColumn(1).getString();
        usr.type = (stmnt.getColumn(2).getString() == "Regular" ? UserClass::NORMAL : UserClass::ADMIN);
        return usr;
    }
    else {
        return {};
    }
}

void Librarydb::newSession(std::string username, std::size_t session) {
    clearSession(username);
    auto query = R"#(
        INSERT INTO [sessions] (username, session)
        VALUES (?, ?)
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.bind(2, static_cast<int64_t>(session));
    stmnt.exec();
}

void Librarydb::clearSession(std::string username) {
    auto query = R"#(
        DELETE FROM [sessions]
        WHERE username = ?
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.exec();
}

std::vector<std::string> Librarydb::getFavourites(std::string username) {
    std::string query = R"#(
        SELECT [book_id], [title], [author],
            FROM [favourites] JOIN [books]
                ON favourites.book_id = books.book_id
            WHERE favourites.username = ?)#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, username);

    std::vector<std::string> result;
    while (stmnt.executeStep()) {
        result.push_back(std::move(extractBookData(stmnt)));
    }
    return std::move(result);
}

std::string extractBookData(const SQLite::Statement& stmnt) {
    std::string entry;
    entry += "Title: " + stmnt.getColumn(1).getString();
    entry += "Author: " + stmnt.getColumn(2).getString();
    return std::move(entry);
}

std::vector<std::string> Librarydb::getBorrowed(std::string username) {
    std::string query = R"#(
        SELECT [book_id], [title], [author],
            FROM [borrows] JOIN [books]
                ON borrows.book_id = books.book_id
            WHERE borrows.username = ?)#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, username);

    std::vector<std::string> result;
    while (stmnt.executeStep()) {
        result.push_back(std::move(extractBookData(stmnt)));
    }
    return std::move(result);
}

std::vector<std::string> Librarydb::searchBook(std::string val) {
    std::string query = R"#(
        SELECT [book_id], [title], [author]
            FROM 
    )#";
    std::vector<std::string> result;
    return std::move(result);
}

SQLite::Statement Librarydb::searchUser(std::string val) {
    // TODO: prepare query
    std::string query = "";
    //query += isbn;
    return SQLite::Statement{*databs, query};
}

void Librarydb::addUser(User nuser, std::string password){
    std::string query = R"#(
        INSERT INTO [users]
            (email, username, password)
        VALUES (?, ?, ?)
    )#";
    auto stmnt = SQLite::Statement(*databs, query);
    stmnt.bind(1, nuser.email);
    stmnt.bind(2, nuser.username);
    stmnt.bind(3, password);
    stmnt.exec();
}

void Librarydb::removeUser(std::string username){
    SQLite::Statement stmnt(*databs, "DELETE FROM [Users] WHERE username = ?");
    stmnt.bind(1, username);
    stmnt.exec();
}

void Librarydb::addBook(Book nuser){
    // TODO: query is not complete, cuz I'm not sure If I should add more book details or not;
    auto query = R"#(
        INSERT INTO [Books] ()
        VALUES ()
    )#";
    SQLite::Statement stmnt(*databs, query);
    // TODO: add bindings from nuser object
    stmnt.exec();
}

void Librarydb::removeBook(std::string book_id) {
    auto query = R"#(
        DELETE FROM [books]
            WHERE book_id = ?
    )#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, book_id);
    stmnt.exec();
}

void Librarydb::addFavourite(std::string username, std::string book_id) {
    auto query = R"#(
        INSERT INTO [favourites] (username, book_id)
        VALUES ( ?, ?);
    )#";

    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.bind(2, book_id);
    stmnt.exec();
}

void Librarydb::borrow(std::string username, std::string book_id) {
    auto query = R"#(
        INSERT INTO [borrows] (username, book_id)
        VALUES ( ?, ?);
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.bind(2, book_id);
    stmnt.exec();
}

void Librarydb::unborrow(std::string username, std::string book_id) {
    SQLite::Statement stmnt(*databs, "DELETE FROM [borrows] WHERE username = ? AND book_id = ?");
    stmnt.bind(1, username);
    stmnt.bind(2, book_id);
    stmnt.exec();
}

std::vector<std::string> Librarydb::getAllBooks() {
    std::vector<std::string> books;
    SQLite::Statement stmnt(*databs, "SELECT [title] FROM [books]");
    while (stmnt.executeStep())
        books.push_back(stmnt.getColumn(0).getString());
    return std::move(books);
}

std::string Librarydb::getBookDetail(std::string book_id) {
    auto query = R"#(
        SELECT *
            FROM [books]
                WHERE book_id = ?
    )#";

    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, book_id);

    std::string result;
    if (stmnt.executeStep()) {
        result += extractBookData(stmnt);
    }

    return std::move(result);
}

std::optional<User> Librarydb::authenticate(const std::string username, const std::string password) {
    SQLite::Statement stmnt{*databs, "SELECT [email], [username], [type] FROM [Users] WHERE username = ? AND password = ?"};
    stmnt.bind(1, username);
    stmnt.bind(2, password);
    if (stmnt.executeStep()) {
        // correct credentials. Extract user data from columns
        return {
            User{stmnt.getColumn(0).getString(), stmnt.getColumn(1).getString(),
            stmnt.getColumn(2).getString() == "Regular" ? UserClass::NORMAL : UserClass::ADMIN}
        };
    }
    else {
        // incorrect credentials
        return {};
    }
}

bool Librarydb::usernameExists(const std::string& username) {
    SQLite::Statement stmnt(*databs, "SELECT [email] FROM [users] WHERE username = ?");
    stmnt.bind(1, username);
    return stmnt.executeStep();
}

bool Librarydb::emailIsUsed(const std::string& email) {
    SQLite::Statement stmnt(*databs, "SELECT [username] FROM [users] WHERE email = ?");
    stmnt.bind(1, email);
    return stmnt.executeStep();
}
