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
#include <string>
#include <utility>
#include <vector>


// forward declarations

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
                    VALUES ('root@library.me', 'root', 'root', 'Admin')
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
                    [edition] INTEGER,
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
        return extractUserInfo(stmnt);
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
        SELECT [books].[book_id], [title], [author], [quantity], [publisher],
                [pub_year], [description], [edition], [rating]
            FROM [favourites] JOIN [books]
                ON favourites.book_id = books.book_id
            WHERE favourites.username = ?)#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, username);

    BookStack books;
    while (stmnt.executeStep()) {
        books.push_back(extractBookInfo(stmnt).value());
    }
    return std::move(books);
}

BookStack Librarydb::getBorrowed(std::string username) {
    std::string query = R"#(
        SELECT [books].[book_id], [title], [author], [quantity], [publisher],
                [pub_year], [description], [edition], [rating]
            FROM [borrows] JOIN [books]
                ON borrows.book_id = books.book_id
            WHERE borrows.username = ?
    )#";
    SQLite::Statement stmnt{*databs, query};
    stmnt.bind(1, username);

    BookStack books;
    while (stmnt.executeStep()) {
        books.push_back(extractBookInfo(stmnt).value());
    }
    return std::move(books);
}

Users Librarydb::getAllUsers() {
    std::vector<User> users;
    auto query = R"#(
        SELECT [username], [email], [type]
            FROM [users]
        WHERE NOT [username] = 'root'
    )#";
    SQLite::Statement stmnt{*databs, query};
    while(stmnt.executeStep()) {
        users.push_back(extractUserInfo(stmnt).value());
    }

    return std::move(users);
}


std::optional<User> Librarydb::extractUserInfo(const SQLite::Statement& stmnt) {
    if (not stmnt.hasRow())
        return {};

    User usr;
    usr.username = stmnt.getColumn(0).getString();
    usr.email = stmnt.getColumn(1).getString();
    usr.type = (stmnt.getColumn(2).getString() == "Regular" ? UserClass::NORMAL : UserClass::ADMIN);

    return usr;
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
    unborrowAll(username);
    unfavouriteAll(username);
    SQLite::Statement stmnt(*databs, "DELETE FROM [Users] WHERE username = ?");
    stmnt.bind(1, username);
    stmnt.exec();
}

void Librarydb::addBook(const Book& book){
    auto query = R"#(
        INSERT INTO [Books] (
                    [book_id], [title], [author], [quantity], [publisher],
                    [pub_year], [description], [edition], [rating]
        )
        VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, static_cast<std::int64_t>(book.book_id));
    stmnt.bind(2, book.title);
    stmnt.bind(3, book.author);
    stmnt.bind(4, book.quantity);
    book.publisher.empty() ? stmnt.bind(5) : stmnt.bind(5, book.publisher);
    book.pub_year < 0 ? stmnt.bind(6) : stmnt.bind(6, book.pub_year);
    book.description.empty() ? stmnt.bind(7) : stmnt.bind(7, book.description);
    book.edition < 0 ? stmnt.bind(8) : stmnt.bind(8, book.edition);
    stmnt.bind(9, 0.0);
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

    query = R"#(
        DELETE FROM [borrows]
            WHERE book_id = ?
    )#";
    stmnt = SQLite::Statement{*databs, query};
    stmnt.bind(1, static_cast<std::int64_t>(book_id));
    stmnt.exec();

    query = R"#(
        DELETE FROM [favourites]
            WHERE book_id = ?
    )#";
    stmnt = SQLite::Statement{*databs, query};
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
void Librarydb::unfavouriteAll(std::string username){
    SQLite::Statement stmnt(*databs, "DELETE FROM [favourites] WHERE username = ?");
    stmnt.bind(1, username);
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

    // Reduce available number by one
    query = R"#(
        UPDATE [books] SET quantity = quantity - 1
            WHERE book_id = ?
    )#";
    stmnt = SQLite::Statement(*databs, query);
    stmnt.bind(1, static_cast<int64_t>(book_id));
    stmnt.exec();
}

void Librarydb::unborrow(std::string username, std::size_t book_id) {
    SQLite::Statement stmnt(*databs, "DELETE FROM [borrows] WHERE username = ? AND book_id = ?");
    stmnt.bind(1, username);
    stmnt.bind(2, static_cast<std::int64_t>(book_id));
    stmnt.exec();
}

void Librarydb::unborrowAll(std::string username) {
    auto query = R"#(
        UPDATE [books]
            SET quantity = quantity + 1
        WHERE [book_id] IN (
            SELECT [book_id] FROM [borrows]
                WHERE username = ?
        )
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, username);
    stmnt.exec();

    query = R"#(
        DELETE FROM [borrows]
            WHERE username = ?
    )#";
    stmnt = SQLite::Statement(*databs, query);
    stmnt.bind(1, username);
    stmnt.exec();
}

BookStack Librarydb::getAllBooks() {
    BookStack books;
    auto query = R"#(
        SELECT [book_id], [title], [author], [quantity], [publisher],
            [pub_year], [description], [edition], [rating]
        FROM [books]
    )#";
    SQLite::Statement stmnt(*databs, query);
    while (stmnt.executeStep()) {
        books.push_back(extractBookInfo(stmnt).value());
    }
    return std::move(books);
}

Book Librarydb::getBook(std::size_t book_id) {
    auto query = R"#(
        SELECT [book_id], [title], [author], [quantity], [publisher],
                    [pub_year], [description], [edition], [rating]
            FROM [books]
                WHERE book_id = ?
    )#";

    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, static_cast<std::int64_t>(book_id));

    if (stmnt.executeStep()) {
        return extractBookInfo(stmnt).value();
    }

    return {};
}

std::optional<User> Librarydb::authenticate(const std::string username, const std::string password) {
    SQLite::Statement stmnt{*databs, "SELECT [username], [email], [type] FROM [Users] WHERE username = ? AND password = ?"};
    stmnt.bind(1, username);
    stmnt.bind(2, password);
    if (stmnt.executeStep()) {
        // correct credentials. Extract user data from columns
        return extractUserInfo(stmnt);
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

std::optional<Book> Librarydb::extractBookInfo(const SQLite::Statement& stmnt) {
    if(not stmnt.hasRow()) {
        return {};
    }
    /*
            CREATE TABLE IF NOT EXISTS [books]
                 (
                    [book_id] INTEGER NOT NULL PRIMARY KEY,
                    [title] VARCHAR(100) NOT NULL,
                    [author] VARCHAR(50) NOT NULL,
                    [quantity] INTEGER NOT NULL,
                    [publisher] VARCHAR(50),
                    [pub_year] INTEGER,
                    [description] VARCHAR(200),
                    [edition] INTEGER,
                    [rating] NUMERIC(2,1),
                    [file] BLOB
                )
    */

    Book bok;
     bok.book_id = stmnt.getColumn(0).getInt64();
    bok.title = stmnt.getColumn(1).getString();
    bok.author = stmnt.getColumn(2).getString();
    bok.quantity = stmnt.getColumn(3).getInt();
    bok.publisher = stmnt.getColumn(4).isNull() ? "" : stmnt.getColumn(4).getString();
    bok.pub_year = stmnt.getColumn(5).isNull() ? -1 : stmnt.getColumn(5).getInt();
    bok.description = stmnt.getColumn(6).isNull() ? "" : stmnt.getColumn(6).getString();
    bok.edition = stmnt.getColumn(7).isNull() ? -1 : stmnt.getColumn(7).getInt();
    bok.rating = stmnt.getColumn(8).isNull() ? -1.0 : stmnt.getColumn(8).getDouble();

    return std::move(bok);
}

void Librarydb::changePassword(std::string& username, std::string& password) {
    auto query = R"#(
        UPDATE [users] SET password = ? WHERE username = ?
    )#";
    SQLite::Statement stmnt(*databs, query);
    stmnt.bind(1, password);
    stmnt.bind(2, username);
    stmnt.exec();
}

void Librarydb::makeAdmin(const std::string& username) {
    unborrowAll(username);
    unfavouriteAll(username);
    SQLite::Statement stmnt(*databs, "UPDATE [users] SET [type] = 'Admin' WHERE [username] = ?");
    stmnt.bind(1, username);
    stmnt.exec();
}

void Librarydb::demoteAdmin(const std::string& username) {
    SQLite::Statement stmnt(*databs, "UPDATE [users] SET [type] = 'Regular' WHERE [username] = ?");
    stmnt.bind(1, username);
    stmnt.exec();
}
