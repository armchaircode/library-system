#include "App.hpp"
#include <memory>

// TODO; implement details app class

#include <exception>

class Exit : public std::exception {};

int App::run() {
    try {
        if(active_user == nullptr) {
            login();
        }
        // something else

    }
    catch(Exit& e){
        // cleanup
    }
    //
    return 0;
}

void App::login() {
    User usr;
    usr.username = "praefect";
    usr.type = UserClass::ADMIN;
    bool exit;
    if (exit)
        throw Exit{};
    active_user = std::make_unique<User>(usr);
}
