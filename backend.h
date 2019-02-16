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


        class User : public std::enable_shared_from_this<User> {
        private:
            const std::string password;
            std::unordered_set<std::shared_ptr<User>> followers;
            std::unordered_set<std::shared_ptr<User>> subscriptions;
            std::vector<std::shared_ptr<Notification>> pendingNotifications;
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

            void addSubscription(const std::shared_ptr<User> subscription) {
                subscriptions.insert(subscription);
                subscription->followers.insert(shared_from_this());
            }

            void deferNotification(const std::shared_ptr<Notification> notification) {
                pendingNotifications.push_back(notification);
            }

            void releasePendingNotifications() {
                pendingNotifications.clear();
            }

            const std::vector<std::shared_ptr<Notification>> &getPendingNotifications() {
                return pendingNotifications;
            }

            const std::unordered_set<std::shared_ptr<User>> &getFollowers() const {
                return followers;
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
            std::map<std::string, std::shared_ptr<User>> authTokens;

            DataStorage() = default;

        public:
            DataStorage(const DataStorage &) = delete;

            DataStorage(DataStorage &&) = delete;

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

            void setAuthorized(const std::string& token, const std::shared_ptr<User> user) {
                authTokens[token] = user;
            }

            const std::shared_ptr<User> getAuthorizedUser(const std::string& token) const {
                if (authTokens.find(token) == authTokens.end()) {
                    return nullptr;
                }
                return authTokens.at(token);
            }
        };

        class NotificationManager {
        private:
            std::map<std::string, std::vector<std::weak_ptr<ClientCallback>>> callbacks;

            NotificationManager() = default;

            const std::unordered_set<std::shared_ptr<ClientCallback>> getUserCallbacks(const std::string &userName) {
                std::unordered_set<std::shared_ptr<ClientCallback>> result;
                if (callbacks.count(userName) != 0) {
                    std::vector<std::weak_ptr<ClientCallback>> &weakCallbacks = callbacks[userName];
                    for (auto it = weakCallbacks.begin(); it < weakCallbacks.end(); ++it) {
                        std::shared_ptr<ClientCallback> callback = it->lock();
                        if (callback) {
                            result.insert(callback);
                        } else {
                            weakCallbacks.erase(it);
                        }
                    }
                }
                return result;
            }

        public:
            NotificationManager(const NotificationManager &) = delete;

            NotificationManager(NotificationManager &&) = delete;

            static NotificationManager &instance() {
                static NotificationManager manager;
                return manager;
            }

            void addUserCallback(const std::string &userName, const std::shared_ptr<ClientCallback> callback) {
                callbacks[userName].push_back(std::weak_ptr<ClientCallback>(callback));
            }

            void notify(const std::string &userName, const std::shared_ptr<Notification> notification) {
                for (const std::shared_ptr<ClientCallback> callback : getUserCallbacks(userName))
                    (*callback)(notification);
            }
        };

        class BackendImpl : public Backend {
        private:
            DataStorage &storage = DataStorage::instance();
            NotificationManager &notificationManager = NotificationManager::instance();

            std::shared_ptr<User> checkCredentials(const std::string &authToken) {
                const std::shared_ptr<User> user = storage.getAuthorizedUser(authToken);
                if (!user) {
                    throw NotAuthorizedException();
                }
                return user;
            }

            void
            pushPendingNotifications(const std::shared_ptr<User> user, const std::shared_ptr<ClientCallback> callback) {
                for (const std::shared_ptr<Notification> &notification: user->getPendingNotifications()) {
                    (*callback)(notification);
                }
            }

            void
            pushNotificationTo(const std::shared_ptr<User> user, const std::shared_ptr<Notification> notification) {
                notificationManager.notify(user->name, notification);
                user->deferNotification(notification);
            }

            void
            pushNotificationFrom(const std::shared_ptr<User> user, const std::shared_ptr<Notification> notification) {
                for (const std::shared_ptr<User> to : user->getFollowers()) {
                    pushNotificationTo(to, notification);
                }
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
                storage.setAuthorized(token, user);
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
                pushNotificationFrom(user, std::make_shared<Notification>(video));
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

            void
            setClientCallback(const std::string &authToken, const std::shared_ptr<ClientCallback> callback) override {
                std::shared_ptr<User> user = checkCredentials(authToken);
                notificationManager.addUserCallback(user->name, callback);
                pushPendingNotifications(user, callback);
            }

            void subscribeFor(const std::string &authToken, const std::string &userName) {
                std::shared_ptr<User> user = checkCredentials(authToken);
                std::shared_ptr<User> subscription = storage.findUser(userName);
                if (!subscription)
                    throw NoSuchUserException();
                user->addSubscription(subscription);
            }

            void releasePendingNotifications(const std::string &authToken) {
                std::shared_ptr<User> user = checkCredentials(authToken);
                user->releasePendingNotifications();
            }
        };


        class Proxy : public Backend {
        private:
            const std::vector<std::shared_ptr<Backend>> backends;
            size_t roundRobinIndex = 0;

            const std::shared_ptr<Backend> nextBackend() {
                return backends[(++roundRobinIndex) % backends.size()];
            }

        public:
            Proxy(const std::vector<std::shared_ptr<Backend>> &backends) : backends(backends) {}

            const std::string auth(const std::string &name, const std::string &password) override {
                return nextBackend()->auth(name, password);
            }

            const std::string downloadVideo(const std::string &id) override {
                return nextBackend()->downloadVideo(id);
            }

            void registerUser(const std::string &name, const std::string &password) override {
                nextBackend()->registerUser(name, password);
            }

            const std::vector<std::shared_ptr<Video>> searchVideos(const std::vector<std::string> &request) override {
                return nextBackend()->searchVideos(request);
            }

            void addVideo(const std::string &authToken, const std::string &name, const std::string &content) override {
                nextBackend()->addVideo(authToken, name, content);
            }

            const std::shared_ptr<Video> getVideo(const std::string &id) override {
                return nextBackend()->getVideo(id);
            }

            void leaveComment(const std::string &authToken, const std::string &videoId,
                              const std::string &comment) override {
                nextBackend()->leaveComment(authToken, videoId, comment);
            }

            void leaveComment(const std::string &authToken, const std::string &videoId, const std::string &comment,
                              size_t replyToIndex) override {
                nextBackend()->leaveComment(authToken, videoId, comment, replyToIndex);
            }

            void leaveLike(const std::string &authToken, const std::string &videoId) override {
                nextBackend()->leaveLike(authToken, videoId);
            }

            void leaveLike(const std::string &authToken, const std::string &videoId, size_t likeId) override {
                nextBackend()->leaveLike(authToken, videoId, likeId);
            }

            void
            setClientCallback(const std::string &authToken, const std::shared_ptr<ClientCallback> callback) override {
                nextBackend()->setClientCallback(authToken, callback);
            }

            void subscribeFor(const std::string &authToken, const std::string &userName) override {
                nextBackend()->subscribeFor(authToken, userName);
            }

            void releasePendingNotifications(const std::string &authToken) override {
                nextBackend()->releasePendingNotifications(authToken);
            }
        };
    }
}
