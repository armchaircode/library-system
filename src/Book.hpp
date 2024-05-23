#pragma once

#include <string>
#include <vector>
#include <memory>

struct Book {
    std::size_t book_id;
    std::string title;
    std::string author;
    int quantity;
    std::string publisher;
    int pub_year;
    std::string description;
    int edition;
    double rating;
};

typedef std::shared_ptr<Book> BookPtr;
typedef std::vector<BookPtr> BookStack;
