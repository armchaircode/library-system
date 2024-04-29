#include "Librarydb.hpp"
#include "User.hpp"
#include <memory>

void Librarydb::init() {
    if(db_path.empty()) {
        return;
    }
    databs = std::make_unique<SQLite::Database>(db_path, SQLite::OPEN_READWRITE);
    if(!databs->tableExists("users"))
        populate();
}

void Librarydb::populate(){
    // Create users table
    databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [users]
                 (
                    [username] VARCHAR(20) PRIMARY KEY NOT NULL,
                    [email] VARCHAR(50) NOT NULL,
                    [password] VARCHAR(50) NOT NULL,
                    [type] VARCHAR(7) NOT NULL DEFAULT 'Regular'
                 )
                 )#");
    // Default amin account
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
                    CONSTRAINT [pk_favourites] PRIMARY KEY ([username], [book_id]),
                    FOREIGN KEY ([username]) REFERENCES [users] ([username])
                        ON DELETE CASCADE ON UPDATE NO ACTION,
                    FOREIGN KEY ([book_id]) REFERENCES [books] ([book_id])
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");
    // Crate borrowed relationship table
     databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [borrows]
                 (
                    [username] VARCHAR(50) NOT NULL,
                    [book_id] INTEGER NOT NULL,
                    CONSTRAINT [pk_favourites] PRIMARY KEY ([username], [book_id]),
                    FOREIGN KEY ([username]) REFERENCES [users] ([username])
                        ON DELETE CASCADE ON UPDATE NO ACTION,
                    FOREIGN KEY ([book_id]) REFERENCES [books] ([book_id])
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");
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
    // TODO: remove user
}

void Librarydb::addBook(Book nuser){
    // TODO: actually add the user
}

void Librarydb::removeBook(std::string book_id) {
    // TODO: remove the book
}

void Librarydb::addFavourite(std::string username, std::string book_id) {
    // TODO: favourite the said book for the the said user
}

void Librarydb::borrow(std::string username, std::string book_id) {
    // TODO: add the book to borrowed books
}

void Librarydb::unborrow(std::string username, std::string book_id) {
    // TODO: remove the from borrowed books
}
