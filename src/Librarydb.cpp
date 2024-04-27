#include "Librarydb.hpp"
#include <memory>

// TODO: Implement database class

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
                    [email] VARCHAR(50) PRIMARY KEY NOT NULL,
                    [name] VARCHAR(20) NOT NULL DEFAULT 'USER',
                    [password] VARCHAR(50) NOT NULL,
                    [type] VARCHAR(7) NOT NULL DEFAULT 'Regular'
                 )
                 )#");
    // Default amin account
    databs->exec(R"#(
                 INSERT INTO [users] (email, name, password, type)
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
                    [email] VARCHAR(50) NOT NULL,
                    [book_id] INTEGER NOT NULL,
                    CONSTRAINT [pk_favourites] PRIMARY KEY ([email], [book_id]),
                    FOREIGN KEY ([email]) REFERENCES [users] ([email])
                        ON DELETE CASCADE ON UPDATE NO ACTION,
                    FOREIGN KEY ([book_id]) REFERENCES [books] ([book_id])
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");
    // Crate borrowed relationship table
     databs->exec(R"#(
                 CREATE TABLE IF NOT EXISTS [borrows]
                 (
                    [email] VARCHAR(50) NOT NULL,
                    [book_id] INTEGER NOT NULL,
                    CONSTRAINT [pk_favourites] PRIMARY KEY ([email], [book_id]),
                    FOREIGN KEY ([email]) REFERENCES [users] ([email])
                        ON DELETE CASCADE ON UPDATE NO ACTION,
                    FOREIGN KEY ([book_id]) REFERENCES [books] ([book_id])
                        ON DELETE CASCADE ON UPDATE NO ACTION
                 )
                 )#");
}

SQLite::Statement Librarydb::getfavourites(std::string& username) {
    std::string query = "";
    //query += username;
    return SQLite::Statement{*databs, query};
}

SQLite::Statement Librarydb::searchBook(std::string& val) {
    // TODO: prepare query
    std::string query = "";
    //query += isbn;
    return SQLite::Statement{*databs, query};
}

SQLite::Statement Librarydb::searchUser(std::string& val) {
    // TODO: prepare query
    std::string query = "";
    //query += isbn;
    return SQLite::Statement{*databs, query};
}

void Librarydb::addUser(User& nuser){
    // TODO: actually add the user
}

void Librarydb::removeUser(std::string& username){
    // TODO: remove user
}

void Librarydb::addBook(Book& nuser){
    // TODO: actually add the user
}

void Librarydb::removeBook(std::string& isbn) {
    // TODO: remove the book
}

void Librarydb::addFavourite(std::string& username, std::string& isbn) {
    // TODO: favourite the said book for the the said user
}

void Librarydb::borrow(std::string& username, std::string& isbn) {
    // TODO: add the book to borrowed books
}

void Librarydb::unborrow(std::string& username, std::string& isbn) {
    // TODO: remove the from borrowed books
}
