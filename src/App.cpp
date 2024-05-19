#include "App.hpp"
#include "Book.hpp"
#include "Librarydb.hpp"
#include "User.hpp"

#include "SQLiteCpp/Exception.h"

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <exception>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <thread>
#include <regex>
#include <vector>

class Exit : public std::exception {};

int App::run() {
    try {
        login();
    }
    catch(const Exit& e){
        screen.Exit();
        // cleanup
    }
    catch (const SQLite::Exception& e) {
        screen.Exit();
        std::cerr<<"[ERROR] Database engine error. <"<<e.what()<<">"<<std::endl;
        return EXIT_FAILURE;
    }
    catch(const std::exception& e) {
        screen.Exit();
        std::cerr<<"[Error] Unknown error. <"<<e.what()<<">"<<std::endl;
        return EXIT_FAILURE;
    }
    //
    return EXIT_SUCCESS;
}

std::size_t readSession(const std::string& filename) {
    std::size_t session;
    std::ifstream ifs(filename);
    ifs>>session;
    ifs.close();
    return session;
}

void App::writeSessionFile(const std::size_t session) {
    auto ofs = std::ofstream(session_file);
    ofs<<session;
    ofs.close();
}

void App::attemptRestore() {
    auto usr = db->restoreSession(readSession(session_file));
    if( !usr){
        // restore failed, session expired
        return;
    }
    else{
        active_user = std::make_unique<User>(usr.value());
    }
}

void flip(bool& flag) {
    auto th = std::thread([&flag] {
        flag = not flag;
        std::this_thread::sleep_for(std::chrono::seconds{1});;
        flag = not flag;
    });
    th.detach();
}

void App::saveSession() {
    // Shouldn't be called when there is no active user
    if (not active_user)
        throw std::exception();

    std::size_t session = std::hash<std::string>{}(active_user->username);
    writeSessionFile(session);
    db->newSession(active_user->username, session);
}

void App::clearSession() {
    clearSessionFile();
    db->clearSession(active_user->username);
}

void App::clearSessionFile() {
    std::ofstream ofs(session_file, std::ios::trunc);
}

void App::login() {
    using namespace ftxui;

    if(newSession) {
        clearSessionFile();
    }
    else {
        attemptRestore();
        if (active_user) {
            home();
        }
    }

    bool signup_ok = false;
    std::string login_username, signup_username, signup_password, login_password, email, signup_error_message;
    auto signup_status = Renderer([&] {
        return text(signup_error_message) | color(Color::Red);
    }) | Maybe([&] {
            bool show_error = true;
            if ( !signup_username.empty() && signup_username.size() < 4) {
                signup_error_message = "Username too short!";
            }
            else if ( !signup_username.empty() && db->usernameExists(signup_username)) {
                signup_error_message = "Username is taken!";
            }
            else if ( !signup_password.empty() && signup_password.size() < 4) {
                signup_error_message = "Password is too short!";
            }
            else if (!email.empty() && db->emailIsUsed(email)) {
                signup_error_message = "Email is used before!";
            }
            else {
                show_error = false;
            }
            signup_ok = !(show_error || signup_username.empty() || signup_password.empty() || email.empty());
            return show_error;
        });

    bool show_failed_authentication = false;
    auto failed_login = Renderer([&] {
        return text("Login failed. Wrong credentials!") | color(Color::Red);
    }) | Maybe(&show_failed_authentication);

    auto signup_action = [&] {
        if (! signup_ok)
            return;

        // save sign-up info and set active_user
        auto usr = User{email, signup_username, UserClass::NORMAL};
        db->addUser(usr, signup_password);
        // signed up
        signup_ok = false;
        signup_password.clear();
        signup_username.clear();
        email.clear();
        active_user = std::make_unique<User>(usr);
        saveSession();
        home();
    };

    auto login_action = [&] {
        auto usr = db->authenticate(login_username, login_password);
        if (not usr) {
            login_username.clear();
            login_password.clear();
            flip(show_failed_authentication);
            }
        else {
            // loged in
            login_password.clear();
            login_username.clear();
            active_user = std::make_unique<User>(usr.value());
            saveSession();
            home();
        }
    };

    // input boxes
    ftxui::InputOption password_option;
    password_option.password = true;

    auto login_screen_container = Container::Vertical({
        Input(&login_username, "Username") | size(ftxui::WIDTH, ftxui::EQUAL, 40) | border,
        Input(&login_password, "Password", password_option) | size(ftxui::WIDTH, ftxui::EQUAL, 40) | border,
        failed_login,
        Container::Horizontal({
            Button("Login", login_action, ButtonOption::Ascii()),
            Button("Quit", [] { throw Exit(); }, ButtonOption::Ascii())
        })
    });

    auto signup_screen_container = Container::Vertical({
        Input(&email, "Email") | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&signup_username, "Username") | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&signup_password, "Password", password_option) | size(WIDTH, ftxui::EQUAL, 40) | ftxui::border,
        signup_status,
        Container::Horizontal({
            Button("Sign Up", signup_action, ButtonOption::Ascii()),
            Button("Quit", [&] { throw Exit(); }, ButtonOption::Ascii())
       })
    });

    std::vector<std::string> toggle_labels{"Login", "Signup"};
    int login_signup_selected = 0;

    auto login_signup_screen = Container::Vertical({
        Toggle(&toggle_labels, &login_signup_selected),
        Container::Tab({login_screen_container, signup_screen_container },
            &login_signup_selected)
    });


    auto login_signup_renderer = Renderer(login_signup_screen, [&] {
        return vbox({
            hbox(
                filler(),
                text("Welcome to Library Management System") | bold,
                filler()
            ),
            separator(),
            filler(),
            hbox(
                filler(),
                login_signup_screen->Render(),
                filler()
            ),
            filler()
        }) | border;
    });

    screen.Loop(login_signup_renderer);
}

void App::adminHome() {
    using namespace ftxui;
    std::string username = active_user->username;

    std::vector<std::string> main_selection {
        "Add a book",
        "Book management",
        "User management",
        "My account"
    };

    BookStack all_books = db->getAllBooks();
    Users all_users = db->getAllUsers();

    std::string searchString;

    // Main manu selector
    int main_menu_selected = 0;
    auto main_menu = Menu(&main_selection, &main_menu_selected);

    // All books main menu item
    int all_book_selected = 0;
    auto all_book_menu = Container::Vertical({}, &all_book_selected);
    if (all_books.empty()) {
        all_book_menu->Add(
            Renderer([]{
                return vbox({
                    filler(),
                    text("No books in the library"),
                    filler()
                    });
            })
        );
    }
    else {
        for(int i = 0; i<all_books.size(); ++i){
            all_book_menu->Add(
                MenuEntry(all_books[i].author + "_" + all_books[i].title) | Maybe([&, i] {
                    return searchString.empty() || isSearchResult(all_books[i], searchString);
                })
            );
        }
    }
    all_book_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    int all_user_selected = 0;
    auto all_user_menu = Container::Vertical({}, &all_user_selected);
    if (all_users.empty()) {
        all_user_menu->Add(
            Renderer([]{
                return vbox({
                    filler(),
                    text("No users yet"),
                    filler()
                    });
            })
        );
    }
    else {
        for(int i = 0; i<all_users.size(); ++i){
            all_user_menu->Add(
                MenuEntry(all_users[i].username + "_" + all_users[i].email) | Maybe([&, i] {
                    return searchString.empty() || isSearchResult(all_users[i], searchString);
                })
            );
        }
    }
    all_user_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // search Area container creator
    auto searchArea = [&searchString] {
        return Container::Horizontal({
            Renderer([] { return filler(); }),
            Renderer([] { return text("Search: "); }),
            Input(&searchString, "  here  ") | size(ftxui::WIDTH, ftxui::EQUAL, 10)
        });
    };

    bool add_book_ok = false, show_add_click_error = false, add_book_successfull = false;
    std::string add_book_title, add_book_author, add_book_quantity, add_book_publisher,
            add_book_pub_year, add_book_edition, add_book_description, add_book_error_message;
    int text_label_size = 20;

    // action performed when add book function is asked
    auto add_book_action = [&]{
            if ( ! add_book_ok)
                return;

            bool required_field_missing_error = true;
            if(add_book_title.empty()) {
                add_book_error_message = "Title is required";
            }
            else if (add_book_author.empty()) {
                add_book_error_message = "Author name is required";
            }
            else if(add_book_quantity.empty()) {
                add_book_error_message = "Quantity is required";
            }
            else {
                required_field_missing_error = false;
            }

            if(required_field_missing_error) {
                flip(show_add_click_error);
                return;
            }

            // all is good
            Book book;
            book.book_id = std::hash<std::string>{}(
                book.title + book.author + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())
            );
            book.title = add_book_title;
            book.author = add_book_author;
            book.quantity = std::stoi(add_book_quantity);
            book.publisher = add_book_publisher;
            add_book_pub_year.empty() ? book.pub_year = -1 : book.pub_year = std::stoi(add_book_pub_year);
            add_book_edition.empty() ? book.edition = -1 : book.edition = std::stoi(add_book_edition);

            // add the new book to the database, working copy and menu
            db->addBook(book);
            auto indx = all_books.size();
            all_books.push_back(book);
            /*
            //TODO: This part is not working
            all_book_menu->Add(
                MenuEntry(book.author + "_" + book.title) | Maybe([&, indx] {
                    return searchString.empty() || isSearchResult(all_books[indx], searchString);
                })
            );
            */

            // show success message
            flip(add_book_successfull);

            //clear things up
            add_book_title.clear();
            add_book_author.clear();
            add_book_quantity.clear();
            add_book_publisher.clear();
            add_book_pub_year.clear();
            add_book_edition.clear();
            add_book_description.clear();
       };

    auto add_book_alert = Renderer([&add_book_error_message] { return text(add_book_error_message) | color(Color::Red); }) |
        Maybe([&] {
            add_book_ok = false;
            auto isItDigit = [](const char ch) { return std::isdigit(ch); };
            if (!add_book_quantity.empty() && ! std::ranges::all_of(add_book_quantity, isItDigit)) {
                add_book_error_message = "Invalid quantity input";
            }
            else if (!add_book_pub_year.empty() && ! std::ranges::all_of(add_book_pub_year, isItDigit)) {
                add_book_error_message = "Invalid publication year";
            }
            else if (!add_book_edition.empty() && ! std::ranges::all_of(add_book_edition, isItDigit)) {
                add_book_error_message = "Invalid edition number";
            }
            else {
                add_book_ok = true;
            }
            return !add_book_ok || show_add_click_error;
        });

    auto add_book_container = Container::Vertical({
        Container::Horizontal({
            Renderer([]{ return filler(); }),
            Container::Vertical({
                label("Title"),
                label("Author"),
                label("Quantity"),
                label("Publisher"),
                label("Pub. Year"),
                label("Edition"),
                label("Description")
            }),
            Renderer([]{ return separator(); }),
            Container::Vertical({
                Input(&add_book_title, "title"),
                Input(&add_book_author, "author"),
                Input(&add_book_quantity, "quantity"),
                Input(&add_book_publisher, "publisher"),
                Input(&add_book_pub_year, "year"),
                Input(&add_book_edition, "edition"),
                Input(&add_book_description, "description")
            }),
            Renderer([]{ return filler(); })
        }),
        add_book_alert,
        Renderer([] { return text("Book added successfully") | color(Color::Green); }) | Maybe(&add_book_successfull),
        Container::Horizontal({
            Renderer([]{ return filler(); }),
            Button("Add Book", add_book_action, ButtonOption::Ascii()),
            Renderer([]{ return filler(); })
        })
    });

    // Main entries and the selected entry info containers holder
    auto main_tab = Container::Tab({
        add_book_container,
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                all_book_menu
            }),
            Renderer([] { return separator(); }),
            bookDetail(all_books, all_book_selected)
        }),
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                all_user_menu,
            }),
            Renderer([] { return separator(); }),
            userDetail(all_users, all_user_selected)
        }),
        Container::Vertical({
            Button("Change passoword", []{}, ButtonOption::Ascii())
        })
    }, &main_menu_selected);

    // Main menu on left most side
    auto main_menu_container = Container::Vertical({
        Renderer([&username]{
            return hbox({
                filler(),
                text(username) | color(Color::Red),
                filler()
            });
        }),
        main_menu,
        Renderer([] { return filler(); }),
        Renderer([] { return separator(); }),
        Container::Horizontal({
            Button("Logout", [&]{
                clearSession();
                active_user = nullptr;
                screen.Exit();
            }, ButtonOption::Ascii()),
            Button("Quit", [&] { throw Exit(); }, ButtonOption::Ascii())
        })
    });

    // Outer most container
    auto home_screen = Container::Horizontal({
        main_menu_container,
        Renderer([] { return separator(); }),
        main_tab
    }) | border;

    screen.Loop(home_screen);
}

void App::normalHome() {
    using namespace ftxui;
    std::string username = active_user->username;

    BookStack all_books = db->getAllBooks();
    BookStack borrowed = db->getBorrowed(username);
    BookStack favourites = db->getFavourites(username);

    std::string searchString;
    std::vector<std::string> main_selection {
        "All books",
        "Borrowed",
        "Favourites",
        "My Account"
    };

    // Main manu selector
    int main_menu_selected = 0;
    auto main_menu = Menu(&main_selection, &main_menu_selected);

    // All books main menu item
    int all_book_selected = 0;
    auto all_book_menu = Container::Vertical({}, &all_book_selected);
    if (all_books.empty()) {
        all_book_menu->Add(
            Renderer([]{
                return vbox({
                    filler(),
                    text("No books in the library"),
                    filler()
                    });
            })
        );
    }
    else {
        for(int i = 0; i<all_books.size(); ++i){
            all_book_menu->Add(
                MenuEntry(all_books[i].author + "_" + all_books[i].title) | Maybe([&, i] {
                    return searchString.empty() || isSearchResult(all_books[i], searchString);
                })
            );
        }
    }
    all_book_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // Favourite books main menu item
    int favourite_book_selected = 0;
    auto favourites_menu = Container::Vertical({}, &favourite_book_selected);
    if (favourites.empty()) {
        favourites_menu->Add(
            Renderer([]{
                return vbox({
                    filler(),
                    text("You haven't liked any book yet"),
                    filler()
                });
            }));
    }
    else {
        for(int i = 0; i<favourites.size(); ++i){
            favourites_menu->Add(
                MenuEntry(favourites[i].author + "_" + favourites[i].title) | Maybe([&, i] {
                    return searchString.empty() || isSearchResult(favourites[i], searchString);
                })
            );
        }
    }
   favourites_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // borrowed books main manu item
    int borrowed_book_selected = 0;
    auto borrowed_menu = Container::Vertical({}, &borrowed_book_selected);
    if (borrowed.empty()) {
        borrowed_menu->Add(
            Renderer([]{
                return vbox({
                    filler(),
                    text("You haven't borrowed any book yet"),
                    filler()
                });
            }));
    }
    else {
        for(int i = 0; i<borrowed.size(); ++i) {
            borrowed_menu->Add(
                MenuEntry(borrowed[i].author + "_" + borrowed[i].title) | Maybe([&, i] {
                    return searchString.empty() || isSearchResult(borrowed[i], searchString);
                })
            );
        }
    }
    borrowed_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // search Area container creator
    auto searchArea = [&searchString] {
        return Container::Horizontal({
            Renderer([] { return filler(); }),
            Renderer([] { return text("Search: "); }),
            Input(&searchString, "  here  ") | size(ftxui::WIDTH, ftxui::EQUAL, 10)
        });
    };

    auto borrow_button_action = [&] {
        db->borrow(username, all_books[all_book_selected].book_id);
        if(borrowed.empty()) {
            // there is no menu
            borrowed_menu->ChildAt(0)->Detach();
        }
        // one borrowed, minus one from available books
        --all_books[all_book_selected].quantity;

        auto indx = borrowed.size();
        borrowed.push_back(all_books[all_book_selected]);
        borrowed_menu->Add(
            MenuEntry(borrowed[indx].author + "_" + borrowed[indx].title) | Maybe([&, indx] {
                return searchString.empty() || isSearchResult(borrowed[indx], searchString);
            })
        );
    };

    auto borrow_button = Button("Borrow", borrow_button_action, ButtonOption::Ascii()) | Maybe([&] {
        return std::ranges::none_of(borrowed, [&](const Book& book) {
                return book.book_id == all_books[all_book_selected].book_id; })
            && all_books[all_book_selected].quantity > 0;
    });

    auto like_button_action = [&] {
        db->addFavourite(username, all_books[all_book_selected].book_id);
        if (favourites.empty()) {
            favourites_menu->ChildAt(0)->Detach();
        }

        auto indx = favourites.size();
        favourites.push_back(all_books[all_book_selected]);
        favourites_menu->Add(
            MenuEntry(favourites[indx].author + "_" + favourites[indx].title) | Maybe([&, indx] {
                return searchString.empty() || isSearchResult(favourites[indx], searchString);
            })
        );
    };

    auto like_button = Button("Like", like_button_action, ButtonOption::Ascii());


    // Main entries and the selected entry info containers holder
    auto main_tab = Container::Tab({
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                all_book_menu,
            }),
            Renderer([] { return separator(); }),
            Container::Vertical({
                bookDetail(all_books, all_book_selected),
                Renderer([] { return filler();}),
                Container::Horizontal({
                    Renderer([] { return filler();}),
                    borrow_button,
                    like_button,
                    Renderer([] { return filler();})
                })
            }),
        }),
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                borrowed_menu,
            }),
            Renderer([] { return separator(); }),
            bookDetail(borrowed, borrowed_book_selected)}),
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                favourites_menu,
            }),
            Renderer([] { return separator(); }),
            bookDetail(favourites, favourite_book_selected)}),
        Container::Vertical({
            Button("Change passoword", []{}, ButtonOption::Ascii())
        })
    }, &main_menu_selected);

    // Main menu on left most side
    auto main_menu_container = Container::Vertical({
        Renderer([&username]{
            return hbox({
                filler(),
                text(username) | color(Color::Green),
                filler()
            });
        }),
        main_menu,
        Renderer([] { return filler(); }),
        Renderer([] { return separator(); }),
        Container::Horizontal({
            Button("Logout", [&]{
                clearSession();
                active_user = nullptr;
                screen.Exit();
            }, ButtonOption::Ascii()),
            Button("Quit", [&] { throw Exit(); }, ButtonOption::Ascii())
        })
    });

    // Outer most container
    auto home_screen = Container::Horizontal({
        main_menu_container,
        Renderer([] { return separator(); }),
        main_tab
    }) | border;

    screen.Loop(home_screen);
}

void App::home() {
    if(active_user->type == UserClass::NORMAL)
        normalHome();
    else {
        adminHome();
    }
}

ftxui::Component App::bookDetail(const BookStack& books, const int& selector) {
    using namespace ftxui;
    if (books.empty()) {
        return Container::Horizontal({
            Renderer([] { return filler(); }),
            Renderer([] { return text("Nothing selected"); }),
            Renderer([] { return filler(); }),
        });
    }

    return Container::Vertical({
        Renderer([&] {
            return text("Titile: " + books[selector].title);
        }),
        Renderer([&] {
            return text("Author: " + books[selector].author);
        }),
        Renderer([&] {
            return text("Publisher: " + books[selector].publisher);
        }) | Maybe([&] { return ! books[selector].publisher.empty(); }),
        Renderer([&] {
                return text("Pub. Year: " + std::to_string(books[selector].pub_year));
        }) | Maybe([&] { return books[selector].pub_year > 0; }),
        Renderer([&] {
            return text("Edition: " + std::to_string(books[selector].edition));
        }) | Maybe([&] { return books[selector].edition > 0; }),
        Renderer([&] {
                return text("Rating: " + std::to_string(books[selector].rating).substr(0,3));
            }) | Maybe([&] { return books[selector].rating >= 0; }),
        Renderer([&] {
            return text("Availablity: " + std::string(books[selector].quantity > 0 ? "Available" : "Not Available"));
        }),
        Renderer([&] {
            return vbox({
                text("Description") | bold,
                paragraph(books[selector].description)
            });
        }) | Maybe([&] { return ! books[selector].description.empty(); })
    });
}

ftxui::Component App::userDetail(const Users& users, const int& selector) {
    using namespace ftxui;
    if (users.empty()) {
        return Container::Horizontal({
            Renderer([] { return filler(); }),
            Renderer([] { return text("Nothing selected"); }),
            Renderer([] { return filler(); }),
        });
    }

    return Container::Vertical({
        Renderer([&] {
            return text("Username: " + users[selector].username);
        }),
        Renderer([&] {
            return text("Email: " + users[selector].email);
        }),
        Renderer([&] {
            return text(std::string("Category: ") +
                        (users[selector].type == UserClass::NORMAL ? "Regular" : "Admin"));
        })
    });
}


bool App::isSearchResult(const Book& book, const std::string& searchString) {
    std::regex pattern {".*" + searchString + ".*", std::regex_constants::icase};
    return std::regex_match(book.title, pattern) || std::regex_match(book.author, pattern);
}

bool App::isSearchResult(const User& usr, const std::string& searchString) {
    std::regex pattern {".*" + searchString + ".*", std::regex_constants::icase};
    return std::regex_match(usr.username, pattern) || std::regex_match(usr.email, pattern);
}

ftxui::Component App::label(const std::string txt) {
    using namespace ftxui;
    return Container::Horizontal({
            Renderer([]{ return filler(); }),
            Renderer([&txt]{ return text(txt); })
        }) | size(ftxui::WIDTH, ftxui::EQUAL, 20);
};

