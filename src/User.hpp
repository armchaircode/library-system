#pragma once

#include <string>

enum class UserClass{
    ADMIN,
    NORMAL
};

struct User {
    std::string email;
    std::string username;
    UserClass type;
};
