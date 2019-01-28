#pragma once

#include <utility>
#include <vector>
#include <map>
#include <unordered_set>
#include <memory>


namespace youtube {

    class User;

    class Video;

    class Comment;

    template<bool backend_version>
    class Likeable {
    public:
        virtual std::enable_if_t<backend_version, void> like(std::shared_ptr<User> user) = 0;

        virtual std::enable_if_t<!backend_version, void> like() = 0;

        virtual void getLikes() const = 0;
    };


    class Comment {
    private:
    public:
        const std::shared_ptr<User> user;
        const std::string content;

        explicit Comment(std::shared_ptr<User> user, std::string content)
                : user(std::move(user)), content(std::move(content)) {}
    };

    class Video {
    private:
        std::vector<std::shared_ptr<Comment>> comments;
        std::unordered_set<std::shared_ptr<User>> likes;

    public:
        const std::string id;
        const std::string title;

        explicit Video(std::string id, std::string title)
                : id(std::move(id)), title(std::move(title)) {}

//            void like() {
//                ++likes;
//            }
//
//            const int getLikes() const {
//                return likes;
//            }
    };

    class User {
    private:
        const std::string name;
        const std::string password;
        std::vector<std::shared_ptr<User>> subscriptions;
        std::vector<std::shared_ptr<Video>> videos;

    public:
        explicit User(std::string name, std::string password)
                : name(std::move(name)), password(std::move(password)) {}

        const bool checkPassword(const std::string &pass) const {
            return pass == password;
        }
    };


    class Backend {
    public:
        virtual const std::string auth(const std::string &name, const std::string &password) = 0;
        virtual std::shared_ptr<User> registerUser(const std::string &name, const std::string &password) = 0;
        virtual std::shared_ptr<Video> addVideo(const std::string& authToken,
                const std::string& name, const std::string& content) = 0;
    };
}
