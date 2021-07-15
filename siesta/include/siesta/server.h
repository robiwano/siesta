#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common.h"

namespace siesta
{
    namespace server
    {
        class Token
        {
        public:
            virtual ~Token() = default;
        };

        class TokenHolder
        {
            std::vector<std::unique_ptr<Token>> routes_;

        public:
            void operator+=(std::unique_ptr<Token> route);
        };

        namespace rest
        {
            class Request
            {
            public:
                virtual ~Request()                         = default;
                virtual const std::string& getUri() const  = 0;
                virtual const HttpMethod getMethod() const = 0;
                virtual const std::map<std::string, std::string>&
                getUriParameters() const = 0;
                virtual const std::map<std::string, std::string>& getQueries()
                    const                                                   = 0;
                virtual std::string getHeader(const std::string& key) const = 0;
                virtual std::string getBody() const                         = 0;
            };

            class Response
            {
            public:
                virtual ~Response()                              = default;
                virtual void addHeader(const std::string& key,
                                       const std::string& value) = 0;
                // Set the body of the response
                virtual void setBody(const void* data, size_t size) = 0;
                virtual void setBody(const std::string& data)       = 0;
            };

            using Handler =
                std::function<void(const rest::Request&, rest::Response&)>;
        }  // namespace rest

        namespace websocket
        {
            /**
             * Reader interface, implemented by websocket handlers
             */
            class Reader
            {
            public:
                virtual ~Reader()                                = default;
                virtual void onReadData(const std::string& data) = 0;
            };

            /**
             * Writer interface, passed to websocket factory
             */
            class Writer
            {
            public:
                virtual ~Writer()                               = default;
                virtual void writeData(const std::string& data) = 0;
            };

            /** Websocket handler factory type */
            using Factory = std::function<Reader*(Writer&)>;
        }  // namespace websocket

        class Server
        {
        public:
            virtual ~Server() = default;

            /**
             * Adds a REST route.
             *
             * @param method    HTTP method (GET, PUT etc.)
             * @param uri       Route URI
             * @param handler   Handler for route
             * @returns A token. Hold on to returned token to keep route
             * "alive". When token goes out of scope, route is removed.
             */
            NO_DISCARD virtual std::unique_ptr<Token> addRoute(
                HttpMethod method,
                const std::string& uri,
                rest::Handler handler) = 0;

            /**
             * Adds serving of static folder.
             *
             * @param uri   Directory URI
             * @param path  Filesystem path of directory to server.
             * @returns A token. Hold on to returned token to keep directory
             * "alive". When token goes out of scope, directory is removed.
             */
            NO_DISCARD virtual std::unique_ptr<Token> addDirectory(
                const std::string& uri,
                const std::string& path) = 0;

            /**
             * Adds websocket handler for text mode websocket.
             *
             * @param uri                   Websocket URI
             * @param factory               Factory for websocket handler.
             * @param max_num_connections   Max # of concurrent sessions for
             * websocket. Set to zero for no limit (default).
             * @returns A token. Hold on to returned token to keep websocket
             * "alive". When token goes out of scope, websocket is removed.
             */
            NO_DISCARD virtual std::unique_ptr<Token> addTextWebsocket(
                const std::string& uri,
                websocket::Factory factory,
                const size_t max_num_connections = 0) = 0;

            /**
             * Adds websocket handler for binary mode websocket.
             *
             * @param uri                   Websocket URI
             * @param factory               Factory for websocket handler.
             * @param max_num_connections   Max # of concurrent sessions for
             * websocket. Set to zero for no limit (default).
             * @returns A token. Hold on to returned token to keep websocket
             * "alive". When token goes out of scope, websocket is removed.
             */
            NO_DISCARD virtual std::unique_ptr<Token> addBinaryWebsocket(
                const std::string& uri,
                websocket::Factory factory,
                const size_t max_num_connections = 0) = 0;

            /**
             * Add a certificate. Used when TLS is enabled. Must be called
             * before server is started
             *
             * @param cert      Certificate
             * @param key       Private key for certificate
             * @param passwd    Password
             * @returns void
             */
            virtual void addCertificate(const std::string& cert,
                                        const std::string& key,
                                        const std::string& passwd = "") = 0;
            /**
             * Starts the server
             *
             * @returns void
             */
            virtual void start() = 0;

            /**
             * Get the port number for the server. Only valid after server has
             * been started.
             *
             * @returns int
             */
            virtual int port() const = 0;
        };

        /**
         * Create a server instance
         *
         * @param address   Address, f.i. "http://127.0.0.1/9080"
         * @returns A server instance
         */
        std::shared_ptr<Server> createServer(const std::string& address);

    }  // namespace server
}  // namespace siesta
