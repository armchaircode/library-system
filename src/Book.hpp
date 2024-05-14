#pragma once

#include <string>

struct Book {
    std::size_t book_id;
    std::string title;
    std::string author;
    int quantity;
    std::string publisher;
    int pub_year;
    std::string description;
    int edtion;
    double rating;
};
