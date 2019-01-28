#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <functional>
#include "common-data.cpp"
#include "util.cpp"

namespace youtube {
    namespace backend {
        class UserAlreadyExistsException : public std::runtime_error {
        public:
            UserAlreadyExistsException()
                    : runtime_error("Exception: user already exists") {}
        };

        class NoSuchUserException : public std::runtime_error {
        public:
            NoSuchUserException()
                    : runtime_error("Exception: no such user") {}
        };

        class NotAuthorizedException : public std::runtime_error {
        public:
            NotAuthorizedException()
                    : runtime_error("Exception: not authorized") {}
        };

        class WrongPasswordException : public std::runtime_error {
        public:
            WrongPasswordException()
                    : runtime_error("Exception: wrong password") {}
        };

        template<class T>
        class SearchEngine {
        private:
            mutable std::function<const std::vector<std::shared_ptr<T>>(void)> supplier;

        public:
            SearchEngine(const std::function<const std::vector<std::shared_ptr<T>>()> &supplier)
                    : supplier(supplier) {}

            const std::vector<std::shared_ptr<T>> search(const std::vector<std::string> &request) const {
                const std::vector<std::shared_ptr<T>> data = supplier();
                const std::vector<std::shared_ptr<T>> result;
                for (const auto element : data) {
                    const auto info = elementInfo(element);
                    for (const auto &req : request) {
                        const auto foundBegin = info.find(req);
                        const auto foundEnd = foundBegin + req.size();
                        if (foundBegin == info.end())
                            break;
                        if (foundBegin != info.begin() && *(foundBegin - 1) != ' ')
                            break;
                        if (foundEnd != info.end() && *(foundEnd + 1) != ' ')
                            break;
                    }
                    result.push_back(element);
                }
                return result;
            }

            template<class T1>
            const std::remove_cv_t<decltype(std::declval<T1>().name)> elementInfo(const std::shared_ptr<T1> element) {
                return element->name;
            }

            template<class T1>
            const std::remove_cv_t<decltype(std::declval<T1>().title)> elementInfo(const std::shared_ptr<T1> element) {
                return element->title;
            }
        };

        class DataStorage {
        private:
            std::vector<std::shared_ptr<Video>> videos;
            std::map<std::string, std::string> videoContent;
            std::map<std::string, std::shared_ptr<User>> users;

        public:
            static DataStorage &instance() {
                static DataStorage storage;
                return storage;
            }

            std::shared_ptr<User> findUser(const std::string &name) {
                if (users.find(name) == users.end())
                    return nullptr;
                return users[name];
            }

            std::shared_ptr<User> createUser(const std::string &name, const std::string &password) {
                return users[name] = std::make_shared<User>(name, password);
            }

            std::shared_ptr<Video> createVideo(std::shared_ptr<User> owner,
                    const std::string& title, const std::string& content) {
                std::shared_ptr<Video> video =
                        std::make_shared<Video>(RandomSequenceGenerator::instance().nextRandomString(5), title);
                videoContent[video->id] = content;
            }

            const SearchEngine<Video> &getVideoSearchEngine() {
                static SearchEngine<Video> engine([this] {
                    return videos;
                });
                return engine;
            }
        };

        class BackendImpl : public Backend {
        private:
            DataStorage &storage = DataStorage::instance();
            std::map<std::string, std::shared_ptr<User>> authTokens;

        public:

            const std::string auth(const std::string &name, const std::string &password) override {
                const std::shared_ptr<User> user = storage.findUser(name);
                if (!user) {
                    throw NoSuchUserException();
                }
                if (!user->checkPassword(password)) {
                    throw WrongPasswordException();
                }
                const std::string token = RandomSequenceGenerator::instance().nextRandomString(7);
                authTokens[token] = user;
                return token;
            }

            std::shared_ptr<User> registerUser(const std::string &name, const std::string &password) override {
                const std::shared_ptr<User> user = storage.findUser(name);
                if (user)
                    throw UserAlreadyExistsException();
                return storage.createUser(name, password);
            }

            std::shared_ptr<Video> addVideo(const std::string &authToken,
                                            const std::string &name, const std::string &content) override {
                std::shared_ptr<User> user = checkCredentials(authToken);

                return video;
            }

            std::shared_ptr<User> checkCredentials(const std::string& authToken) {
                if (authTokens.find(authToken) == authTokens.end()) {
                    throw NotAuthorizedException();
                }
                return authTokens[authToken];
            }
        };

    }
}
