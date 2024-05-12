#include "Librarydb.hpp"
#include "SQLiteCpp/Exception.h"
#include "SQLiteCpp/Transaction.h"
#include "User.hpp"

#include <memory>
#include <optional>
#include <vector>
#include <iostream>
#include <exception>

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
    trxn.commit();
    }
    catch(SQLite::Exception& e) {
        std::cerr << "[ERROR: Failed to create database schema. Sqlite errored out with <"<<e.what()<<">\n";
        trxn.rollback();
        throw std::exception();
    }
}

User Librarydb::restoreSession(std::string session){
    // TODO: query database for session information
    return User{};
}

SQLite::Statement Librarydb::getFavourites(std::string username) {
    std::string query = "";
    //query += username;
    //TODO: query favourites
    return SQLite::Statement{*databs, query};
}
SQLite::Statement Librarydb::getBorrowed(std::string username) {
    std::string query = "";
    //TODO: query borrowed
    return SQLite::Statement{*databs, query};
}

SQLite::Statement Librarydb::searchBook(std::string val) {
    // TODO: prepare query
    std::string query = "";
    return SQLite::Statement{*databs, query};
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
    // TODO: remove the book
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
    SQLite::Statement stmnt(*databs, "SELECT [book_id] FROM [books]");
    while (stmnt.executeStep())
        books.push_back(stmnt.getColumn(0).getString());
    return std::move(books);
}

Book Librarydb::getBook(std::string book_id) {
    Book bk{};
    SQLite::Statement stmnt(*databs, "SELECT * FROM [books] WHERE isbn = ?");
    stmnt.bind(1, book_id);
    stmnt.executeStep();
    //TODO: Copy book details from columns to Book object
    return std::move(bk);
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
