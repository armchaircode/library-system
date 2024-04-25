#include "Librarydb.hpp"
#include <memory>

// TODO: Implement database class

void Librarydb::init() {
    if(db_path.empty()) {
        return;
    }

    databs = std::make_unique<SQLite::Database>(db_path);
    if(!databs->tableExists("user"))
        populate();
}

void Librarydb::populate(){
    // TODO: make bunch of queries to make tables
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
