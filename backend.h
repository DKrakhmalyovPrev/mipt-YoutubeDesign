#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <functional>
#include <memory>
#include "common-data.h"
#include "util.h"

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

        class NoSuchVideoException : public std::runtime_error {
        public:
            NoSuchVideoException()
                    : runtime_error("Exception: no such video") {}
        };

        class NoSuchCommentException : public std::runtime_error {
        public:
            NoSuchCommentException()
                    : runtime_error("Exception: no such comment") {}
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


        class User {
        private:
            const std::string password;
            std::vector<std::shared_ptr<User>> subscriptions;
            std::vector<std::shared_ptr<Video>> videos;

        public:
            const std::string name;

            explicit User(std::string name, std::string password)
                    : name(std::move(name)), password(std::move(password)) {}

            const bool checkPassword(const std::string &pass) const {
                return pass == password;
            }

            void addVideo(const std::shared_ptr<Video> video) {
                videos.push_back(video);
            }
        };

        class BackendLikeable : public Likeable {
        private:
            std::unordered_set<std::string> whoLiked;

        public:
            void like(const std::string &userName) {
                whoLiked.insert(userName);
            }

            const size_t getLikes() const override {
                return whoLiked.size();
            }
        };

        class BackendComment : public Comment, public BackendLikeable {
        public:
            BackendComment(std::string userName, std::string content)
                    : Comment(std::move(userName), std::move(content)) {}

            const size_t getLikes() const override {
                return BackendLikeable::getLikes();
            }

            void addReply(const std::shared_ptr<Comment> reply) {
                replies.push_back(reply);
            }
        };

        class BackendVideo : public Video, public BackendLikeable {
        public:
            BackendVideo(const std::string &id, const std::string &title) : Video(id, title) {}

            void addComment(const std::shared_ptr<Comment> &comment) {
                comments.push_back(comment);
            }

            const size_t getLikes() const override {
                return BackendLikeable::getLikes();
            }
        };


        template<class T>
        class SearchEngine {
        private:
            mutable std::function<const std::vector<std::shared_ptr<T>>(void)> supplier;

            template<class T1>
            const std::remove_cv_t<decltype(std::declval<T1>().name)>
            elementInfo(const std::shared_ptr<T1> element) const {
                return element->name;
            }

            template<class T1>
            const std::remove_cv_t<decltype(std::declval<T1>().title)>
            elementInfo(const std::shared_ptr<T1> element) const {
                return element->title;
            }

        public:
            explicit SearchEngine(const std::function<const std::vector<std::shared_ptr<T>>()> &supplier)
                    : supplier(supplier) {}

            const std::vector<std::shared_ptr<T>> search(const std::vector<std::string> &request) const {
                const std::vector<std::shared_ptr<T>> data = supplier();
                std::vector<std::shared_ptr<T>> result;
                for (const auto element : data) {
                    const auto info = elementInfo(element);
                    for (const auto &req : request) {
                        const size_t foundBegin = info.find(req);
                        const size_t foundEnd = foundBegin + req.size();
                        if (foundBegin == std::string::npos)
                            continue;
                        if (foundBegin != 0 && info[foundBegin - 1] != ' ')
                            continue;
                        if (foundEnd != info.size() && info[foundEnd] != ' ')
                            continue;

                        result.push_back(element);
                        break;
                    }
                }
                return result;
            }
        };

        class DataStorage {
        private:
            std::vector<std::shared_ptr<Video>> videos;
            std::map<std::string, std::shared_ptr<Video>> idVideoMap;
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

            std::shared_ptr<Video> createVideo(const std::shared_ptr<User> owner,
                                               const std::string &title, const std::string &content) {
                std::shared_ptr<Video> video =
                        std::make_shared<BackendVideo>(RandomSequenceGenerator::instance().nextRandomString(5), title);
                videoContent[video->id] = content;
                videos.push_back(video);
                idVideoMap[video->id] = video;
                owner->addVideo(video);
                return video;
            }

            const SearchEngine<Video> &getVideoSearchEngine() {
                static SearchEngine<Video> engine([this] {
                    return videos;
                });
                return engine;
            }

            const std::string &findVideoContent(const std::string &id) {
                return videoContent.at(id);
            }

            const std::shared_ptr<BackendVideo> findVideo(const std::string &id) {
                if (idVideoMap.count(id))
                    return std::dynamic_pointer_cast<BackendVideo>(idVideoMap[id]);
                return nullptr;
            }
        };

        class BackendImpl : public Backend {
        private:
            DataStorage &storage = DataStorage::instance();
            std::map<std::string, std::shared_ptr<User>> authTokens;


            std::shared_ptr<User> checkCredentials(const std::string &authToken) {
                if (authTokens.find(authToken) == authTokens.end()) {
                    throw NotAuthorizedException();
                }
                return authTokens[authToken];
            }

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

            void registerUser(const std::string &name, const std::string &password) override {
                const std::shared_ptr<User> user = storage.findUser(name);
                if (user)
                    throw UserAlreadyExistsException();
                storage.createUser(name, password);
            }

            void addVideo(const std::string &authToken,
                          const std::string &name, const std::string &content) override {
                std::shared_ptr<User> user = checkCredentials(authToken);
                std::shared_ptr<Video> video = storage.createVideo(user, name, content);
            }

            const std::vector<std::shared_ptr<Video>> searchVideos(const std::vector<std::string> &request) override {
                return storage.getVideoSearchEngine().search(request);
            }

            const std::string downloadVideo(const std::string &id) override {
                try {
                    return storage.findVideoContent(id);
                } catch (const std::out_of_range &exc) {
                    throw NoSuchVideoException();
                }
            }

            const std::shared_ptr<Video> getVideo(const std::string &id) override {
                const std::shared_ptr<BackendVideo> result = storage.findVideo(id);
                if (!result)
                    throw NoSuchVideoException();
                return result;
            }

            void leaveComment(const std::string &authToken, const std::string &videoId,
                              const std::string &comment) override {
                std::shared_ptr<User> user = checkCredentials(authToken);
                const std::shared_ptr<BackendVideo> video = storage.findVideo(videoId);
                if (!video)
                    throw NoSuchVideoException();
                video->addComment(std::make_shared<BackendComment>(user->name, comment));
            }

            void leaveComment(const std::string &authToken, const std::string &videoId,
                              const std::string &comment, const size_t replyToIndex) override {
                std::shared_ptr<User> user = checkCredentials(authToken);
                const std::shared_ptr<BackendVideo> video = storage.findVideo(videoId);
                if (!video)
                    throw NoSuchVideoException();
                const std::vector<std::shared_ptr<Comment>> &comments = video->getComments();
                if (comments.size() <= replyToIndex)
                    throw NoSuchCommentException();
                const std::shared_ptr<BackendComment> reply = std::make_shared<BackendComment>(user->name, comment);
                std::dynamic_pointer_cast<BackendComment>(comments[replyToIndex])->addReply(reply);
            }

            void leaveLike(const std::string &authToken, const std::string &videoId) override {
                std::shared_ptr<User> user = checkCredentials(authToken);
                const std::shared_ptr<BackendVideo> video = storage.findVideo(videoId);
                if (!video)
                    throw NoSuchVideoException();
                video->like(user->name);
            }

            void leaveLike(const std::string &authToken, const std::string &videoId, const size_t commentId) override {
                std::shared_ptr<User> user = checkCredentials(authToken);
                const std::shared_ptr<BackendVideo> video = storage.findVideo(videoId);
                if (!video)
                    throw NoSuchVideoException();
                const std::vector<std::shared_ptr<Comment>> &comments = video->getComments();
                if (comments.size() <= commentId)
                    throw NoSuchCommentException();
                std::dynamic_pointer_cast<BackendComment>(comments[commentId])->like(user->name);
            }
        };

    }
}
