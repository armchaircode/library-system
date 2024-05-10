#include "App.hpp"
#include "Librarydb.hpp"

#include "User.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <ftxui/dom/elements.hpp>
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

    auto signup_action = [&] {
        // save sign-up info and set active_user
        auto usr = User{email, username, UserClass::NORMAL};
        db->addUser(usr, password);
        // signed up
        active_user = std::make_unique<User>(usr);
        screen.Exit();

    };

    auto login_action = [&] {
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
    };

    auto quit_action = [&] {
        throw Exit{};
    };

    // buttons
    auto signup_button = ftxui::Button("Sign Up", signup_action);
    auto login_button = ftxui::Button("Login", login_action);
    auto quit_button = ftxui::Button("Quit", quit_action);

    // input boxes
    ftxui::InputOption password_option;
    password_option.password = true;
    auto password_box = ftxui::Input(&password, "password", password_option);
    auto username_box = ftxui::Input(&username, "username");
    auto email_box = ftxui::Input(&email, "email");

    auto welcome_text = ftxui::text("Welcome to Library Management System") | ftxui::bold;

    /*
    auto signup_screen_container = ftxui::Container::Vertical({
        email_box,
        username_box,
        password_box,
        ftxui::Container::Horizontal({
            signup_button,
            quit_button
        })
    });
    */

    auto login_screen_container = ftxui::Container::Vertical({
        username_box,
        password_box,
        ftxui::Container::Horizontal({
            login_button,
            quit_button
        })
    });

    auto login_renderer = ftxui::Renderer(login_screen_container, [&] {
        return ftxui::vbox({
            welcome_text,
            ftxui::separator(),
            login_screen_container->Render()
        }
        ) | ftxui::border;
    });

    screen.Loop(login_renderer);
}

void App::home() {
    // TODO: home screen implemntation
}
