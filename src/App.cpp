#include "App.hpp"
#include "Book.hpp"
#include "Librarydb.hpp"
#include "User.hpp"

#include "SQLiteCpp/Exception.h"

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"
#include "ftxui/component/event.hpp"

#include <algorithm> // all_of, none_of, any_of
#include <cctype> // isdigit
#include <chrono> // system_clock
#include <cstddef> // size_t
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <fstream> // ifstream, ofstream
#include <functional> // hash
#include <iostream> // cerr
#include <memory> // make_unique
#include <exception> // exception
#include <stdexcept> // runtime_error
#include <string> // string
#include <thread> // thread
#include <regex> // regex, regex_match
#include <vector> // vector

using Action = std::function<void()>;

class Exit : public std::exception {};

ftxui::MenuEntryOption menuEntryOption(){
    using namespace ftxui;
    auto option = MenuEntryOption();
    option.transform = [](const EntryState& state) {
        Element e;
        if (state.active) {
            e = text("> " + state.label) | bold;
        }
        if (state.focused) {
            e = text("< " + state.label + " >") | bold;
        }
        if (!state.focused && !state.active) {
            e = text("  " + state.label);
        }
        return e;
    };

    return std::move(option);
}

ftxui::MenuOption menuOption() {
    using namespace ftxui;
    auto option = MenuOption();

    option.entries_option.transform = [](const EntryState& state) {
        Element e;
        if (state.focused) {
            e = text("> " + state.label);
        }
        if (state.active) {
            e = text("< " + state.label + " >") | bold;
        }
        if (!state.focused && !state.active) {
            e = text("  " + state.label) | dim;
        }
        return e;
    };

    return std::move(option);
}

// Centrally defined input behaviour
ftxui::InputOption inputOption() {
    using namespace ftxui;

    auto option = InputOption();
    option.multiline = false;

    option.transform = [](InputState state) {
        //state.element |= color(Color::White);
        if (state.is_placeholder) {
            state.element |= dim;
        }

        if (state.focused) {
            state.element |= bgcolor(Color::GrayDark);
        } else if (state.hovered) {
            state.element |= bold;
        }

        return state.element;
    };

    return std::move(option);
}

// Input option which is also a password input
ftxui::InputOption passwordInputOption() {
    auto option = inputOption();
    option.password = true;

    return std::move(option);
}

// Button behaviour definition
ftxui::ButtonOption buttonOption() {
    return ftxui::ButtonOption::Ascii();
}

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
        std::cerr<<"[ERROR] Unknown error. <"<<e.what()<<">"<<std::endl;
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
    if(usr){
        active_user = std::make_unique<User>(*usr);
    }
}

void App::flip(bool& flag) {
    auto th = std::thread([&flag] {
        flag = not flag;
        std::this_thread::sleep_for(std::chrono::seconds{2});;
        screen.PostEvent(ftxui::Event::Custom);
        flag = not flag;
    });
    th.detach();
}

void App::saveSession() {
    // Shouldn't be called when there is no active user
    if (not active_user)
        throw std::runtime_error{"Asked to save session when there is no active session. This shouldn't be happening!"};

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

    // SIGNUP stuff here
    bool signup_ok = false;
    std::string login_username, signup_username, signup_password, login_password, email, signup_error_message;
    auto signup_status = Renderer([&] {
        return text(signup_error_message) | color(Color::Red);
    }) | Maybe([&] {
            bool show_error = true;
            if ( !signup_username.empty() && signup_username.size() < 4) {
                signup_error_message = "Username is too short!";
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


    // When a login fails, this comes up.
    bool show_failed_authentication = false;
    auto failed_login = Renderer([&] {
        return text("Login failed. Wrong credentials!") | color(Color::Red);
    }) | Maybe(&show_failed_authentication);

    std::vector<std::string> toggle_labels{"Login", "Signup"};
    int login_signup_selected = 0;

    // Funcion called when sign-up is requested
    auto signup_action = [&] {
        if (not signup_ok)
            return;

        // save sign-up info and set active_user
        auto usr = User{email, signup_username, UserClass::NORMAL};
        db->addUser(std::make_shared<User>(usr), signup_password);

        // return flag to its original place
        signup_ok = false;

        //clear buffers
        signup_password.clear();
        signup_username.clear();
        email.clear();

        // Select signup page once signed up. Otherwise, sign up is still displayed upon logout
        login_signup_selected = 0;

        active_user = std::make_unique<User>(usr);
        saveSession();
        home();
    };

    // This is invoked for login
    auto login_action = [&] {
        auto usr = db->authenticate(login_username, login_password);
        if (usr) {
            // loged in
            login_password.clear();
            login_username.clear();
            active_user = std::make_unique<User>(*usr);
            saveSession();
            home();
        }
        else {
            // login failed
            login_password.clear();
            flip(show_failed_authentication);
        }
    };

    // Container for widgets in login tab
    auto login_screen_container = Container::Vertical({
        Input(&login_username, "Username", inputOption()) | size(ftxui::WIDTH, ftxui::EQUAL, 40) | border,
        Input(&login_password, "Password", passwordInputOption()) | size(ftxui::WIDTH, ftxui::EQUAL, 40) | border,
        failed_login,
        Container::Horizontal({
            Button("Login", login_action, buttonOption()),
            Button("Quit", [] { throw Exit(); }, buttonOption())
        })
    }) | CatchEvent([&](Event e) {          /* Invoke login_action when Enter is pressed */
            if (e == Event::Return){
                login_action();
                return true;
            }
            return false;
        });

    // Container for widgets in signup page
    auto signup_screen_container = Container::Vertical({
        Input(&email, "Email", inputOption()) | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&signup_username, "Username", inputOption()) | size(WIDTH, ftxui::EQUAL, 40) | border,
        Input(&signup_password, "Password", passwordInputOption()) | size(WIDTH, ftxui::EQUAL, 40) | ftxui::border,
        signup_status,
        Container::Horizontal({
            Button("Sign Up", signup_action, buttonOption()),
            Button("Quit", [&] { throw Exit(); }, buttonOption())
       })
    }) | CatchEvent([&](Event e) {
            if (e == Event::Return) {
                signup_action();    /* Signup on ENTER */
                return true;
            }
            return false;
        });

    // Whole of login-Signup screen. A sort of super container
    auto login_signup_screen = Container::Vertical({
        Toggle(&toggle_labels, &login_signup_selected),
        Container::Tab({login_screen_container, signup_screen_container },
            &login_signup_selected)
    });

    // Fine tune rendering of login-signup widgets.
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

    // Finally, keep it all in the loop
    screen.Loop(login_signup_renderer);
}

void App::adminHome() {
    // Admin accounts stuff
    using namespace ftxui;
    std::string username = active_user->username;

    std::vector<std::string> main_selection {
        "Add a book",
        "Book management",
        "User management",
        "My account"
    };

    // Fetch data from database
    BookStack all_books = db->getAllBooks();
    Users all_users = db->getAllUsers();

    // Buffer for search text
    std::string searchString;

    // Main manu selector
    int main_menu_selected = 0;
    auto main_menu = Menu(&main_selection, &main_menu_selected, menuOption());

    // All books main menu item
    int all_book_selected = 0;
    auto all_book_menu = Container::Vertical({}, &all_book_selected);

    // Add books to the book management menu
    for(int i = 0; i<all_books.size(); ++i){
        all_book_menu->Add(
            MenuEntry(all_books[i]->author + "_" + all_books[i]->title, menuEntryOption()) | Maybe([&, i] {
                return searchString.empty() || isSearchResult(all_books[i], searchString);
            })
        );
    }

    all_book_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // Users management menu
    int all_user_selected = 0;
    auto all_user_menu = Container::Vertical({}, &all_user_selected);

    // Put users data on the menu, for users management
    for(int i = 0; i<all_users.size(); ++i){
        all_user_menu->Add(
            MenuEntry(all_users[i]->username + "_" + all_users[i]->email, menuEntryOption()) | Maybe([&, i] {
                return searchString.empty() || isSearchResult(all_users[i], searchString);
            })
        );
    }

    all_user_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // search Area container creator
    auto searchArea = [&searchString] {
        return Container::Horizontal({
            Renderer([] { return filler(); }),
            Renderer([] { return text("Search: "); }),
            Input(&searchString, "  here  ", inputOption()) | size(ftxui::WIDTH, ftxui::EQUAL, 10)
        });
    };

    // This is filter maker for input fields that should only take digits, like quantity
    auto digit_filter = []{
        return CatchEvent([](Event e) {
            return e.is_character() && !std::isdigit(e.character()[0]);
        });
    };

    // Buffers for book information, used when adding and editng books
    std::string add_book_title, add_book_author, add_book_quantity, add_book_publisher,
            add_book_pub_year, add_book_edition, add_book_description;
    int text_label_size = 20;

    // Success message dialog
    bool show_success_alert = false;
    std::string success_message;
    auto success_alert = [&] {
         return Container::Horizontal({
            Renderer([]{ return filler(); }),
            Renderer([&success_message] { return text(success_message) | color(Color::Green); }),
            Renderer([]{ return filler(); })
        }) | Maybe(&show_success_alert);
    };

    // Error message dialog
    bool show_error_alert = false;
    std::string error_message;
    auto add_edit_book_alert = [&] {
         return Container::Horizontal({
            Renderer([]{ return filler(); }),
            Renderer([&error_message] { return text(error_message) | color(Color::Red); }),
            Renderer([]{ return filler(); })
        }) | Maybe(&show_error_alert);
    };


    // action performed when add book function is asked
    auto add_book_action = [&]{
        bool required_field_missing_error = true;
        // Check if any of required fields is missing
        if(add_book_title.empty()) {
            error_message = "Title is required";
        }
        else if (add_book_author.empty()) {
            error_message = "Author name is required";
        }
        else if(add_book_quantity.empty()) {
            error_message = "Quantity is required";
        }
        else {
            required_field_missing_error = false;
        }

        // If missing, DARN
        if(required_field_missing_error) {
            flip(show_error_alert);
            return;
        }

        // ALL IS GOOD
        // Copy data from buffers to book object
        auto book = std::make_shared<Book>();
        book->book_id = std::hash<std::string>{}(
            book->title + book->author + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())
        );
        book->title = add_book_title;
        book->author = add_book_author;
        book->quantity = std::stoi(add_book_quantity);
        book->publisher = add_book_publisher;
        book->pub_year = add_book_pub_year.empty() ? -1 : std::stoi(add_book_pub_year);
        book->edition = add_book_edition.empty() ? -1 : std::stoi(add_book_edition);

        // add the new book to the database, working copy and menu
        db->addBook(book);
        auto indx = all_books.size();
        all_books.push_back(book);
        all_book_menu->ChildAt(0)->Add(
            MenuEntry(book->author + "_" + book->title, menuEntryOption()) | Maybe([&, indx] {
                return searchString.empty() || isSearchResult(all_books[indx], searchString);
            })
        );

        // show success message
        success_message = "Book added successfully";
        flip(show_success_alert);

        //clear things up
        add_book_title.clear();
        add_book_author.clear();
        add_book_quantity.clear();
        add_book_publisher.clear();
        add_book_pub_year.clear();
        add_book_edition.clear();
        add_book_description.clear();
    };

    // Simple label maker. Meant to avoid rewritting the same code ten places
    auto make_label = [](const std::string txt) {
        using namespace ftxui;
        return Container::Horizontal({
                Renderer([]{ return filler(); }),
                Renderer([txt]{ return text(txt); })
            }) | size(ftxui::WIDTH, ftxui::EQUAL, 20);
    };

    // Bunch of input fields, with their labes, for book information
    auto book_detail_inputs = [&]{
        return Container::Horizontal({
            Renderer([]{ return filler(); }),
            Container::Vertical({
                make_label("Title"),
                make_label("Author"),
                make_label("Quantity") ,
                make_label("Publisher"),
                make_label("Pub. Year"),
                make_label("Edition"),
                make_label("Description")
            }),
            Renderer([]{ return separator(); }),
            Container::Vertical({
                Input(&add_book_title, "title", inputOption()),
                Input(&add_book_author, "author", inputOption()),
                Input(&add_book_quantity, "quantity", inputOption()) | digit_filter(),
                Input(&add_book_publisher, "publisher", inputOption()),
                Input(&add_book_pub_year, "year", inputOption()) | digit_filter(),
                Input(&add_book_edition, "edition", inputOption()) | digit_filter(),
                Input(&add_book_description, "description", inputOption())
                    | size(ftxui::WIDTH, ftxui::LESS_THAN, 50)
            }),
            Renderer([]{ return filler(); })
        });
    };

    // Container for add books tab
    auto add_book_container = Container::Vertical({
        book_detail_inputs(),
        add_edit_book_alert(),
        success_alert(),
        Container::Horizontal({
            Renderer([]{ return filler(); }),
            Button("Add Book", add_book_action, buttonOption()),
            Renderer([]{ return filler(); })
        })
    }) | CatchEvent([&](Event e) {
            if (e == Event::Return){
                add_book_action();
                return true;
            }
            return false;
        });

    // Flag to control whn edit book window is displayed
    bool show_book_edit_dialog = false;

    // Kick off editing books
    auto edit_button_action = [&] {
        // Find selected book
        auto book = all_books[all_book_selected];

        // Copy book data to buffers. The user only has to edit the wanted bits
        // and not write everything from scratch
        add_book_title = book->title;
        add_book_author = book->author;
        add_book_quantity = std::to_string(book->quantity);
        add_book_publisher = book->publisher;
        add_book_pub_year = book->pub_year < 0 ? "" : std::to_string(book->pub_year);
        add_book_edition = book->edition < 0 ? "" : std::to_string(book->edition);
        add_book_description = book->description;

        // Display editing window. Let the editing begin
        show_book_edit_dialog = true;
    };
    auto edit_book_button = Button("Edit", edit_button_action, buttonOption());

    // House cleaning after editing is done, or cancelled
    auto leave_edit_dialog_action = [&] {
        // clear up
        add_book_title.clear();
        add_book_author.clear();
        add_book_quantity.clear();
        add_book_publisher.clear();
        add_book_pub_year.clear();
        add_book_edition.clear();
        add_book_description.clear();

        // Hide book editing little window
        show_book_edit_dialog = false;
    };

    // Make the actual changes
    auto save_chages_button_action = [&] {
        // Check if anything is missing
        bool required_field_missing_error = true;
        if(add_book_title.empty()) {
            error_message = "Title is required";
        }
        else if (add_book_author.empty()) {
            error_message = "Author name is required";
        }
        else if(add_book_quantity.empty()) {
            error_message = "Quantity is required";
        }
        else {
            required_field_missing_error = false;
        }

        // If something is missing, TOO BAD
        if(required_field_missing_error) {
            flip(show_error_alert);
            return;
        }

        // Find the book in question
        auto book = all_books[all_book_selected];

        // Edit the book data
        book->title = add_book_title;
        book->author = add_book_author;
        book->quantity = std::stoi(add_book_quantity);
        book->publisher = add_book_publisher;
        book->pub_year = add_book_pub_year.empty() ? -1 : std::stoi(add_book_pub_year);
        book->edition = add_book_edition.empty() ? -1 : std::stoi(add_book_edition);
        book->description = add_book_description;

        // Make the changes permanent, in database
        db->updateBook(book);
        // Finally, clean the house and editing is over
        leave_edit_dialog_action();
    };

    // Book editing widgets in one house
    auto edit_book_container = Container::Vertical({
        book_detail_inputs(),
        Renderer([]{ return separator(); }),
        add_edit_book_alert(),
        success_alert(),
        Container::Horizontal({
            Renderer([]{ return filler(); }),
            Button("Update", save_chages_button_action, buttonOption()),
            Renderer([]{ return filler(); })
        })
    }) | border | CatchEvent([&](Event e){          /* ESCAPE means cancel */
            if (e == Event::Escape) {
                leave_edit_dialog_action();
                return true;
            }
            else if (e == Event::Return) {          /* ENTER means save changes */
                save_chages_button_action();
                return true;
            }

            return false;
        });

    // Remove a book a book with this action
    auto remove_book_button_action = [&] {
        db->removeBook(all_books[all_book_selected]->book_id);
        all_books.erase(all_books.begin() + all_book_selected);
        // Remove from book menu
        all_book_menu->ChildAt(0)->ChildAt(all_user_selected)->Detach();
    };
    auto remove_book_button = Button("Remove", remove_book_button_action, buttonOption());

    // KICK a user out
    auto remove_user_button_action = [&] {
        db->removeUser(all_users[all_user_selected]->username);
        all_users.erase(all_users.begin() + all_user_selected);
        //Remove from the menu
        all_user_menu->ChildAt(0)->ChildAt(all_user_selected)->Detach();
    };
    auto remove_user_button = Button("Remove", remove_user_button_action, buttonOption());

    // BIG promotion for a user. Only admins can promote
    auto grant_privelege_button_action = [&] {
        db->makeAdmin(all_users[all_user_selected]->username);
        all_users[all_user_selected]->type = UserClass::ADMIN;
    };
    auto grant_privelege_button = Button("Grant Admin Rights", grant_privelege_button_action, buttonOption()) |
        Maybe([&] { return all_users[all_user_selected]->type == UserClass::NORMAL; });

    // Degrade an admin to a normal user
    auto revoke_privelege_button_action = [&] {
        db->demoteAdmin(all_users[all_user_selected]->username);
        all_users[all_user_selected]->type = UserClass::NORMAL;
        // If an admin demoted himself, logout because he is no longer an admin
        if (all_users[all_user_selected]->username == active_user->username) {
            active_user = nullptr;
            screen.Exit();
        }
    };
    auto revoke_privelege_button = Button("Revoke Admin Rights", revoke_privelege_button_action, buttonOption()) |
        Maybe([&] { return all_users[all_user_selected]->type == UserClass::ADMIN; });


    // Password changing buffers and flags
    std::string new_password;
    bool deleting_account = false, password_change_success = false;

    // Main entries and the selected entry info containers holder
    auto main_tab = Container::Tab({
        add_book_container,
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                all_book_menu
            }) | Maybe([&] { return ! all_books.empty(); }),
            Renderer([] { return separator(); }),
            Container::Vertical({
                bookDetail(all_books, all_book_selected),
                Renderer([] { return filler(); }),
                Container::Horizontal({
                    Renderer([] { return filler(); }),
                    edit_book_button,
                    remove_book_button,
                    Renderer([] { return filler(); })
                })
            })
        }) | Maybe([&] { return ! all_books.empty(); }) | Modal(edit_book_container, &show_book_edit_dialog),

        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                all_user_menu,
            }),
            Renderer([] { return separator(); }),
            Container::Vertical({
                userDetail(all_users, all_user_selected),
                Renderer([] { return filler(); }),
                Container::Horizontal({
                    Renderer([] { return filler(); }),
                    remove_user_button, grant_privelege_button, revoke_privelege_button,
                    Renderer([] { return filler(); })
                })
            })
        }) | Maybe([&] { return ! all_users.empty(); }),

        accountMgmtScreen(new_password, password_change_success, deleting_account)

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
                main_tab->DetachAllChildren();
                active_user = nullptr;
                screen.Exit();
            }, buttonOption()),
            Button("Quit", [&] { throw Exit(); }, buttonOption())
        })
    });

    // Outer most container
    auto home_screen = Container::Horizontal({
        main_menu_container,
        Renderer([] { return separator(); }),
        main_tab
    }) | border;

    // Off we go. It is all displayed
    screen.Loop(home_screen);
}

void App::normalHome() {
    // This is regular user
    using namespace ftxui;

    std::string username = active_user->username;

    // Fetch all books from database
    BookStack all_books = db->getAllBooks();

    auto findBook = [&](const std::size_t book_id) -> BookPtr {
        auto ptr = std::ranges::find_if(all_books, [&](const BookPtr& book) {
            return book->book_id == book_id;
        });

        return *ptr;
    };

    // Fetch borrowed books
    BookStack borrowed_books = db->getBorrowed(username);
    BookStack borrowed;
    for(auto book : borrowed_books) {
        borrowed.push_back(findBook(book->book_id));
    }
    borrowed_books.clear();

    // Fetch favourite books
    BookStack favourites_books = db->getFavourites(username);
    BookStack favourites;
    for (auto book : favourites_books) {
        favourites.push_back(findBook(book->book_id));
    }
    favourites_books.clear();

    std::string searchString;
    std::vector<std::string> main_selection {
        "All books",
        "Borrowed",
        "Favourites",
        "My Account"
    };

    // Main manu selector
    int main_menu_selected = 0;
    auto main_menu = Menu(&main_selection, &main_menu_selected, menuOption());

    // All books main menu item
    int all_book_selected = 0;
    auto all_book_menu = Container::Vertical({}, &all_book_selected);

    // Add books to the menu
    for(int i = 0; i<all_books.size(); ++i){
        all_book_menu->Add(
            MenuEntry(all_books[i]->author + "_" + all_books[i]->title, menuEntryOption()) | Maybe([&, i] {
                return searchString.empty() || isSearchResult(all_books[i], searchString);
            })
        );
    }

    all_book_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // Favourite books main menu item
    int favourite_book_selected = 0;
    auto favourites_menu = Container::Vertical({}, &favourite_book_selected);

    // Favaourites in the menu
    for(int i = 0; i<favourites.size(); ++i){
        favourites_menu->Add(
            MenuEntry(favourites[i]->author + "_" + favourites[i]->title, menuEntryOption()) | Maybe([&, i] {
                return searchString.empty() || isSearchResult(favourites[i], searchString);
            })
        );
    }

   favourites_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // borrowed books main manu item
    int borrowed_book_selected = 0;
    auto borrowed_menu = Container::Vertical({}, &borrowed_book_selected);

    // Add borrowed books to the borrowed books menu
    for(int i = 0; i<borrowed.size(); ++i) {
        borrowed_menu->Add(
            MenuEntry(borrowed[i]->author + "_" + borrowed[i]->title, menuEntryOption()) | Maybe([&, i] {
                return searchString.empty() || isSearchResult(borrowed[i], searchString);
            })
        );
    }

    borrowed_menu |= size(ftxui::WIDTH, ftxui::EQUAL, entryMenuSize);

    // search Area container creator
    auto searchArea = [&searchString] {
        return Container::Horizontal({
            Renderer([] { return filler(); }),
            Renderer([] { return text("Search: "); }),
            Input(&searchString, "  here  ", inputOption()) | size(ftxui::WIDTH, ftxui::EQUAL, 10)
        });
    };

    // What happens when a book is borrowed
    auto borrow_button_action = [&] {
        BookPtr book;
        // The book borrowing can be done from all_books tab or favourites tab.
        // Appropriate book must be selected based on condition
        if (main_menu_selected == 0) {
            book = all_books[all_book_selected];
        }
        else {
            book = favourites[favourite_book_selected];
        }

        // Try borrowing. If a book is already borrowed, an error is thrown.
        // In that case, just quietly return. Meaning DO NOTHING.
        try {
            db->borrow(username, book->book_id);
        }
        catch(const SQLite::Exception& e){
            //TODO: if UNIQUE constraint error, return. Else throw.
            //e.getErrorCode();
            return;
        }

        // one borrowed, minus one from available books
        --book->quantity;

        // Add newly borrowed book to the in-memory catalog of borrowed books
        auto indx = borrowed.size();
        borrowed.push_back(book);

        // New book is borrowed. Put it on the borrowed menu
        borrowed_menu->ChildAt(0)->Add(
            MenuEntry(borrowed[indx]->author + "_" + borrowed[indx]->title, menuEntryOption()) | Maybe([&, indx] {
                return searchString.empty() || isSearchResult(borrowed[indx], searchString);
            })
        );
    };

    // This finds out if a book is already borrowed
    auto isBorrowed = [&](const BookPtr& book) -> bool {
        return std::ranges::any_of(borrowed, [&](const BookPtr& bok) { return book == bok; });
    };

    // The button to borrow books
    auto borrow_button = Button("Borrow", borrow_button_action, buttonOption()) | Renderer([&](Element borrow) {
        if (isBorrowed(all_books[all_book_selected])) {
            // When borrowed, say "borrowed"
            return text("Borrowed ");
        }
        else {
            // Else keep it as is
            return std::move(borrow);
        }
    });

    // What does it mean to like a book?
    auto like_button_action = [&] {
        BookPtr book;
        // Liking can be from borrowed tab or all_books tab. Act accordingly
        if (main_menu_selected == 0) {
            book = all_books[all_book_selected];
        }
        else {
            book = borrowed[borrowed_book_selected];
        }

        // Try liking. If error, it is already liked. Ignore that.
        try {
            db->addFavourite(username, book->book_id);
        }
        catch(const SQLite::Exception& e){
            //TODO: if UNIQUE constraint error, return. Else throw.
            //e.getErrorCode();
            return;
        }

        // We have a new like at hand. Place it in the ranks of favourites in memory
        auto indx = favourites.size();
        favourites.push_back(book);

        // In the menu too
        favourites_menu->ChildAt(0)->Add(
            MenuEntry(favourites[indx]->author + "_" + favourites[indx]->title, menuEntryOption()) | Maybe([&, indx] {
                return searchString.empty() || isSearchResult(favourites[indx], searchString);
            })
        );
    };

    // Is it liked? How would we know?
    auto isFavourite = [&](const BookPtr& book) -> bool {
        return std::ranges::any_of(favourites, [&](const BookPtr& bok) { return book == bok; });
    };

    // Like button.
    auto like_button = Button("Like", like_button_action, buttonOption()) | Renderer([&](Element like) {
        if (isFavourite(all_books[all_book_selected])) {
            return text(" Liked");
        }
        else {
            return std::move(like);
        }
    });

    // This is return action. This is THE WAY to return books
    auto unborrow_button_action = [&] {
        // Register with the database
        db->unborrow(username, borrowed[borrowed_book_selected]->book_id);

        // The is book is return. Increase the available quantity
        ++borrowed[borrowed_book_selected]->quantity;

        // delete from borrowed books working copy
        borrowed.erase(borrowed.begin() + borrowed_book_selected);
        // Remove from the menu
        borrowed_menu->ChildAt(0)->ChildAt(borrowed_book_selected)->Detach();
    };
    auto unborrow_button = Button("Return", unborrow_button_action, buttonOption());

    // No longer like this book. Banish it from liked books.
    auto unlike_button_action = [&] {
        // remove from database
        db->removeFavourite(username, favourites[favourite_book_selected]->book_id);
        // remove from working copy of favourites
        favourites.erase(favourites.begin() + favourite_book_selected);
        // Remove from the favourites menu
        favourites_menu->ChildAt(0)->ChildAt(favourite_book_selected)->Detach();
    };
    auto unlike_button = Button("Unlike", unlike_button_action, buttonOption());

    // New password buffer and flags
    std::string new_password;
    bool deleting_account = false, password_change_success = false;

    // Rating stuff
    bool show_rate_dialog = false;

    // Rate button maker with their number of stars
    auto rate_button = [&](int n) {
        return Button(std::string(n, '*'), [&, n]{
            auto book = borrowed[borrowed_book_selected];
            book->rating = db->rateBook(book->book_id, n);
            show_rate_dialog = false;
        }, buttonOption());
    };

    // This is the little floating RATE window
    auto rate_dialog = Container::Vertical({
        Renderer([&] { return text("Rate: " + borrowed[borrowed_book_selected]->title); }),
        Renderer([&] { return separator(); }),
        Container::Horizontal({
            rate_button(1), rate_button(2), rate_button(3), rate_button(4), rate_button(5)
        })
    }) | border | size(ftxui::WIDTH, EQUAL, 30) | size(ftxui::HEIGHT, EQUAL, 5)
        | CatchEvent([&](Event e) {
            if (e == Event::Escape){            /* ESCAPE-ing  is cancelling */
                show_rate_dialog = false;
                return true;
            }
            return false;
        });

    // Main entries and the selected entry info containers holder
    auto main_tab = Container::Tab({
        // all books tab
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
        }) | Maybe([&] { return ! all_books.empty(); }),

        // borrowed books tab
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                borrowed_menu,
            }),
            Renderer([] { return separator(); }),
            Container::Vertical({
                bookDetail(borrowed, borrowed_book_selected),
                Renderer([] { return filler();}),
                Container::Horizontal({
                    Renderer([] { return filler();}),
                    unborrow_button,
                    Button("Like", like_button_action, buttonOption()) | Renderer([&](Element like) {
                        if (isFavourite(borrowed[borrowed_book_selected])) {
                            return text(" Liked ");
                        }
                        else {
                            return std::move(like);
                        }
                    }),
                    Button("Rate", [&] { show_rate_dialog = true; }, buttonOption()),
                    Renderer([] { return filler();})
                })
            })
        }) | Modal(rate_dialog, &show_rate_dialog) | Maybe([&] { return ! borrowed.empty(); }),

        // favourites tab
        Container::Horizontal({
            Container::Vertical({
                searchArea(),
                Renderer([]{ return separator(); }),
                favourites_menu,
            }),
            Renderer([] { return separator(); }),
            Container::Vertical({
                bookDetail(favourites, favourite_book_selected),
                Renderer([] { return filler();}),
                Container::Horizontal({
                    Renderer([] { return filler();}),
                    unlike_button,
                    Button("Borrow", borrow_button_action, buttonOption()) | Renderer([&](Element borrow) {
                        if (isBorrowed(favourites[favourite_book_selected])) {
                            return text(" Borrowed");
                        }
                        else {
                            return std::move(borrow);
                        }
                    }),
                    Renderer([] { return filler();})
                })
            })
        }) | Maybe([&] { return ! favourites.empty(); }),

        // account tab
        accountMgmtScreen(new_password, password_change_success, deleting_account)

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
                main_tab->DetachAllChildren();
                active_user = nullptr;
                screen.Exit();
            }, buttonOption()),
            Button("Quit", [] { throw Exit(); }, buttonOption())
        })
    });

    // Outer most container
    auto home_screen = Container::Horizontal({
        main_menu_container,
        Renderer([] { return separator(); }),
        main_tab
    }) | border;

    // All done, now loop it
    screen.Loop(home_screen);
}

void App::home() {
    // WHO is this newly logged-in user?
    if(active_user->type == UserClass::NORMAL){
        normalHome();
    }
    else {
        adminHome();
    }
}

// That section always on the right side, displaying things about books
ftxui::Component App::bookDetail(const BookStack& books, const int& selector) {
    using namespace ftxui;

    return Container::Vertical({
        Renderer([&] {
            return text("Titile: " + books[selector]->title);
        }),
        Renderer([&] {
            return text("Author: " + books[selector]->author);
        }),
        Renderer([&] {
            return text("Publisher: " + books[selector]->publisher);
        }) | Maybe([&] { return ! books[selector]->publisher.empty(); }),
        Renderer([&] {
            return text("Pub. Year: " + std::to_string(books[selector]->pub_year));
        }) | Maybe([&] { return books[selector]->pub_year > 0; }),
        Renderer([&] {
            return text("Edition: " + std::to_string(books[selector]->edition));
        }) | Maybe([&] { return books[selector]->edition > 0; }),
        Renderer([&] {
            return text("Rating: " + std::to_string(books[selector]->rating).substr(0,3));
        }),
        Renderer([&] {
            return (active_user->type == UserClass::NORMAL)
                ? (text("Availablity: " + std::string(books[selector]->quantity > 0 ? "Available" : "Not Available")))
                : text("Quantity: " + std::to_string(books[selector]->quantity));
        }),
        Renderer([&] {
            return vbox({
                text("Description") | bold,
                paragraph(books[selector]->description)
            });
        }) | Maybe([&] { return ! books[selector]->description.empty(); })
    });
}

// This windows talks about users instead
ftxui::Component App::userDetail(const Users& users, const int& selector) {
    using namespace ftxui;

    return Container::Vertical({
        Renderer([&] {
            return text("Username: " + users[selector]->username);
        }),
        Renderer([&] {
            return text("Email: " + users[selector]->email);
        }),
        Renderer([&] {
            return text(std::string("Category: ") +
                        (users[selector]->type == UserClass::NORMAL ? "Regular" : "Admin"));
        })
    });
}

// Does a book meet search criteria?
bool App::isSearchResult(const BookPtr& book, const std::string& searchString) {
    std::regex pattern {".*" + searchString + ".*", std::regex_constants::icase};
    return std::regex_match(book->title, pattern) || std::regex_match(book->author, pattern);
}

// Does a user meet search criteria?
bool App::isSearchResult(const UserPtr& usr, const std::string& searchString) {
    std::regex pattern {".*" + searchString + ".*", std::regex_constants::icase};
    return std::regex_match(usr->username, pattern) || std::regex_match(usr->email, pattern);
}

// Changing account information
ftxui::Component App::accountMgmtScreen(std::string& new_password, bool& password_change_success, bool& deleting_account) {
    using namespace ftxui;
    // Succes alert after password change. Defining here is saves from rewritting in two places later
    static auto success_alert = [&] {
         return Container::Horizontal({
            Renderer([]{ return filler(); }),
            Renderer([] { return text("Password changed successfully") | color(Color::Green); }),
            Renderer([]{ return filler(); })
        });
    };

    // ERROR. Defining here is saves from rewritting in two places later
    static auto error_alert = [&](const std::string txt) {
         return Container::Horizontal({
            Renderer([]{ return filler(); }),
            Renderer([txt] { return text(txt) | color(Color::Red); }),
            Renderer([]{ return filler(); })
        });
    };

    // THIS is done to change password
    static auto change_password_action = [&] {
        if(new_password.size() < 4)
            return;
        db->changePassword(active_user->username, new_password);
        new_password.clear();
        flip(password_change_success);
    };

    // Root is special. Can't be removed, even by root itself.
    if (active_user->username == "root") {
        return Container::Vertical({
            Input(&new_password, " password", passwordInputOption()),
            error_alert("Password too short") | Maybe([&] {
                return !new_password.empty() && new_password.size() < 4;
            }),
            success_alert() | Maybe(&password_change_success),
            Button("Change passoword", change_password_action, buttonOption())
            }) | CatchEvent([&](Event e) {
                if (e == Event::Return) {
                    change_password_action();
                    return true;
                }

                return false;
                });
    }

    // For other users
    return Container::Vertical({
        Input(&new_password, "            password", passwordInputOption()),
        error_alert("Password too short") | Maybe([&] {
            return !new_password.empty() && new_password.size() < 4;
        }),
        success_alert() | Maybe(&password_change_success),
        Container::Horizontal({
            Button("Change passoword", change_password_action, buttonOption()),
            Button("Delete Account", [&]{
                deleting_account = true;
            }, buttonOption())
        }),
        Container::Vertical({
            error_alert("This action is irreversible. Are you sure?"),
            Container::Horizontal({
                Renderer([] { return filler(); }),
                Button("Yes", [&] {
                    db->removeUser(active_user->username);
                    clearSession();
                    active_user = nullptr;
                    screen.Exit();
                }, buttonOption()),
                Button("No", [&] {
                    deleting_account = false;
                }, buttonOption()),
                Renderer([] { return filler(); })
            })
        }) | Maybe(&deleting_account)
    }) | CatchEvent([&](Event e) {
        if (e == Event::Return) {
            change_password_action();
            return true;
        }

        return false;
    });
}
