#include "App.hpp"
#include "Librarydb.hpp"

#include "User.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <exception>
#include <vector>

class Exit : public std::exception {};

int App::run() {
    // screens
    //screen = std::make_unique<ftxui::ScreenInteractive>();
    try {
        if(session.empty()) {
            login();
        }
        else{
            //TODO: attempt to restore the session
            auto usr = db->restoreSession(session);
            if(usr.username.empty()){
                // restore failed, session expired
                login();
            }
            else{
                active_user = std::make_unique<User>(usr);
                home();
            }
        }
    }
    catch(Exit& e){
        // cleanup
    }
    //
    return EXIT_SUCCESS;
}

void App::writeSession() {
    // Shouldn't be called when there is no active user
    if (not active_user)
        throw std::exception();
    // TODO: Derive session informatin from active_user and write it to session_file
    auto ofs = std::ofstream(session_file.c_str());
    // do the actual writing
    ofs.close();
}

void App::login() {
    std::string username, password, email;

    auto login_button = ftxui::Button("Login", [&](){
        //read button data and set active_user
        auto usr = db->authenticate(username, password);
        if (not usr) {
            // TODO: authentication failed
        }
        else {
            // loged in
            active_user = std::make_unique<User>(usr.value());
            screen.Exit();
        }
    });

    auto signup_button = ftxui::Button("Sign Up", [&](){
        // save sign-up info and set active_user
        auto usr = User{email, username, UserClass::NORMAL};
        db->addUser(usr, password);
        // signed up
        active_user = std::make_unique<User>(usr);
        screen.Exit();
    });

    auto quit_button = ftxui::Button("Quit", [](){ throw Exit{}; });

    auto login_screen_container = ftxui::Container::Vertical({
        ftxui::Input(username, "username"),
        ftxui::Input(password, "password"),
        login_button,
        signup_button,
        quit_button
    });
    screen.Loop(login_screen_container);
}

void App::home() {
    // TODO: home screen implemntation
}
