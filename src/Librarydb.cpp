#include "Librarydb.hpp"
#include "SQLiteCpp/Exception.h"
#include "SQLiteCpp/Statement.h"
#include "SQLiteCpp/Transaction.h"
#include "User.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>


// forward declarations
std::string extractBookData(const SQLite::Statement& stmnt);

void Librarydb::init() {
    if(db_path.empty()) {
        throw std::invalid_argument{"empty database filename"};
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
                    VALUES ('root@library.me', 'root', '1234567890', 'Admin')
                 )#");
    // Create books table
    databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [books]
                 (
                    [book_id] INTEGER NOT NULL PRIMARY KEY,
                    [title] VARCHAR(100) NOT NULL,
                    [author] VARCHAR(50) NOT NULL,
                    [quantity] INTEGER NOT NULL,
                    [publisher] VARCHAR(50),
                    [pub_year] INTEGER,
                    [description] VARCHAR(200),
                    [edtion] INTEGER,
                    [rating] NUMERIC(2,1),
                    [file] BLOB
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
        trxn.rollback();
        throw;
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

BookStack Librarydb::getFavourites(std::string username) {
    std::string query = R"#(
        SELECT [books].[book_id], [title], [author]
            FROM [favourites] JOIN [books]
                ON favourites.book_id = books.book_id
            WHERE favourites.username = ?)#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, username);

    BookStack result;
    while (stmnt.executeStep()) {
        result.first.push_back(stmnt.getColumn(0).getInt64());
        result.second.push_back(stmnt.getColumn(1).getString() + "_" + stmnt.getColumn(2).getString());
    }
    return std::move(result);
}

std::string extractBookData(const SQLite::Statement& stmnt) {
    std::string entry;
    entry += "Title: " + stmnt.getColumn(1).getString();
    entry += "Author: " + stmnt.getColumn(2).getString();
    return std::move(entry);
}

BookStack Librarydb::getBorrowed(std::string username) {
    std::string query = R"#(
        SELECT [books].[book_id], [title], [author]
            FROM [borrows] JOIN [books]
                ON borrows.book_id = books.book_id
            WHERE borrows.username = ?
    )#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, username);

    BookStack result;
    while (stmnt.executeStep()) {
        result.first.push_back(stmnt.getColumn(0).getInt64());
        result.second.push_back(stmnt.getColumn(1).getString() + "_" + stmnt.getColumn(2).getString());
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

void Librarydb::addBook(const Book nuser){
    auto query = R"#(
        INSERT INTO [Books] (
                    [book_id], [title], [author], [quantity], [publisher],
                    [pub_year], [description], [edtion], [rating]
        )
        VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, static_cast<std::int64_t>(nuser.book_id));
    stmnt.bind(2, nuser.title);
    stmnt.bind(3, nuser.author);
    stmnt.bind(4, nuser.quantity);
    stmnt.bind(5, nuser.publisher);
    stmnt.bind(6, nuser.pub_year);
    stmnt.bind(7, nuser.description);
    stmnt.bind(8, nuser.edtion);
    stmnt.bind(9, nuser.rating);
    stmnt.exec();
}

void Librarydb::removeBook(std::size_t book_id) {
    auto query = R"#(
        DELETE FROM [books]
            WHERE book_id = ?
    )#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, static_cast<std::int64_t>(book_id));
    stmnt.exec();
}

void Librarydb::addFavourite(std::string username, std::size_t book_id) {
    auto query = R"#(
        INSERT INTO [favourites] (username, book_id)
        VALUES ( ?, ?);
    )#";

    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.bind(2, static_cast<std::int64_t>(book_id));
    stmnt.exec();
}

void Librarydb::removeFavourite(std::string username, std::size_t book_id) {
    auto query = R"#(
        DELETE FROM [favourites]
        WHERE username = ? AND book_id = ?
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.bind(2, static_cast<std::int64_t>(book_id));
    stmnt.exec();
}

void Librarydb::borrow(std::string username, std::size_t book_id) {
    auto query = R"#(
        INSERT INTO [borrows] (username, book_id)
        VALUES ( ?, ?);
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.bind(2, static_cast<int64_t>(book_id));
    stmnt.exec();
}

void Librarydb::unborrow(std::string username, std::size_t book_id) {
    SQLite::Statement stmnt(*databs, "DELETE FROM [borrows] WHERE username = ? AND book_id = ?");
    stmnt.bind(1, username);
    stmnt.bind(2, static_cast<std::int64_t>(book_id));
    stmnt.exec();
}

BookStack Librarydb::getAllBooks() {
    BookStack books;
    SQLite::Statement stmnt(*databs, "SELECT [book_id], [title], [author] FROM [books]");
    while (stmnt.executeStep()) {
        books.first.push_back(stmnt.getColumn(0).getInt64());
        books.second.push_back(stmnt.getColumn(1).getString() + "_" + stmnt.getColumn(2).getString());
    }
    return std::move(books);
}

Book Librarydb::getBook(std::size_t book_id) {
    auto query = R"#(
        SELECT [book_id], [title], [author], [quantity], [publisher],
                    [pub_year], [description], [edtion], [rating]
            FROM [books]
                WHERE book_id = ?
    )#";

    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, static_cast<std::int64_t>(book_id));

    if (stmnt.executeStep()) {
        Book bok;
        bok.book_id = stmnt.getColumn(0).getInt64();
        bok.title = stmnt.getColumn(1).getString();
        bok.author = stmnt.getColumn(2).getString();
        bok.quantity = stmnt.getColumn(3).getInt();
        bok.publisher = stmnt.getColumn(4).getString();
        bok.pub_year = stmnt.getColumn(5).getInt();
        bok.description = stmnt.getColumn(6).getString();
        bok.edtion = stmnt.getColumn(7).getInt();
        bok.rating = stmnt.getColumn(8).getDouble();

        return std::move(bok);
    }

    return {};
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
