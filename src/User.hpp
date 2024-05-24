#pragma once

#include <memory> // shared_ptr
#include <string> // string
#include <vector> // vector

enum class UserClass{
    ADMIN,
    NORMAL
};

struct User {
    std::string email;
    std::string username;
    UserClass type;
};

typedef std::shared_ptr<User> UserPtr;
typedef std::vector<UserPtr> Users;
