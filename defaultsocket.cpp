//
// Created by andre on 11/11/2021.
//

#include "defaultsocket.h"

namespace nut {
    namespace internal {

#ifdef WIN32
#define ssize_t long
#define FD_TYPE SOCKET

        inline int xread(FD_TYPE socket, char *buf, int size) {
            return recv(socket, buf, size, 0);
        }

        inline int xwrite(FD_TYPE socket, const char *buf, int size) {
            return send(socket, buf, size, 0);
        }

        inline int xclose(FD_TYPE socket) {
            return ::closesocket(socket);
        }

#endif
#ifndef WIN32
        #define FD_TYPE int
        inline int xread(FD_TYPE socket, char * buf, int size) {
            return ::read(socket, buf, size);
        }
        inline int xwrite(FD_TYPE socket, const char * buf, int size) {
            return ::write(socket, buf, size);
        }
        inline int xclose(FD_TYPE socket) {
            return ::close(socket);
        }
#endif

        class DefaultSocket : public AbstractSocket {
        public:
            DefaultSocket();

            ~DefaultSocket();

            void connect(const std::string &host, int port) override;

            void disconnect() override;

            bool isConnected() const override;

            size_t read(void *buf, size_t sz) override;

            size_t write(const void *buf, size_t sz) override;

            std::string  read() override;

            void write(const std::string & s) override;

        private:
            SOCKET _sock;
            struct timeval _tv;
            std::string _buffer; /* Received buffer, string because data should be text only. */
        };

#ifdef WIN32
        static bool WSAInitialised = false;
#endif

        DefaultSocket::DefaultSocket() :
                _sock(INVALID_SOCKET),
                _tv() {
            _tv.tv_sec = -1;
            _tv.tv_usec = 0;
        }

        DefaultSocket::~DefaultSocket() {
            disconnect();
            if (WSAInitialised) {
                WSACleanup();
                WSAInitialised = false;
            }
        }

        void DefaultSocket::connect(const std::string &host, int port) {
            FD_TYPE sock_fd;
            struct addrinfo hints, *res, *ai;
            char sport[NI_MAXSERV];
            int v;
            fd_set wfds;
            int error;
            socklen_t error_size;
            long fd_flags;

            _sock = -1;

            if (host.empty()) {
                throw nut::UnknownHostException();
            }

#ifdef WIN32
            if (!WSAInitialised) {
                WSADATA wsaData;
                auto iResult = WSAStartup(MAKEWORD(2, 0), &wsaData);
                if (iResult == 0) {
                    WSAInitialised = true;
                }
            }
#endif

            snprintf(sport, sizeof(sport), "%hu", static_cast<unsigned short int>(port));

            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            while ((v = getaddrinfo(host.c_str(), sport, &hints, &res)) != 0) {
                switch (v) {
                    case EAI_AGAIN:
                        continue;
                    case EAI_NONAME:
                        throw nut::UnknownHostException();
#ifndef WIN32
                        case EAI_SYSTEM:
            throw nut::SystemException();
#endif
                    case EAI_MEMORY:
                        throw nut::NutException("Out of memory");
                    default:
                        throw nut::NutException("Unknown error");
                }
            }

            for (ai = res; ai != nullptr; ai = ai->ai_next) {

                sock_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

                if (sock_fd < 0) {
                    switch (errno) {
                        case EAFNOSUPPORT:
                        case EINVAL:
                            break;
                        default:
                            throw nut::SystemException();
                    }
                    continue;
                }


#ifndef WIN32
                /* non blocking connect */
        if(hasTimeout()) {
            fd_flags = fcntl(sock_fd, F_GETFL);
            fd_flags |= O_NONBLOCK;
            fcntl(sock_fd, F_SETFL, fd_flags);
        }
#endif

                while ((v = ::connect(sock_fd, ai->ai_addr, ai->ai_addrlen)) < 0) {
                    if (errno == EINPROGRESS) {
                        FD_ZERO(&wfds);
                        FD_SET(sock_fd, &wfds);
                        select(sock_fd + 1, nullptr, &wfds, nullptr, hasTimeout() ? &_tv : nullptr);
                        if (FD_ISSET(sock_fd, &wfds)) {
                            error_size = sizeof(error);
#ifndef WIN32
                            getsockopt(sock_fd, SOL_SOCKET, SO_ERROR,
                            &error, &error_size);
#endif
#ifdef WIN32
                            getsockopt(sock_fd, SOL_SOCKET, SO_ERROR,
                                       (char *) &error, &error_size);
#endif
                            if (error == 0) {
                                /* connect successful */
                                v = 0;
                                break;
                            }
                            errno = error;
                        } else {
                            /* Timeout */
                            v = -1;
                            break;
                        }
                    }

                    switch (errno) {
                        case EAFNOSUPPORT:
                            break;
                        case EINTR:
                        case EAGAIN:
                            continue;
                        default:
//				ups->upserror = UPSCLI_ERR_CONNFAILURE;
//				ups->syserrno = errno;
                            break;
                    }
                    break;
                }

                if (v < 0) {
                    xclose(sock_fd);
                    continue;
                }

#ifndef WIN32
                /* switch back to blocking operation */
        if(hasTimeout()) {
            fd_flags = fcntl(sock_fd, F_GETFL);
            fd_flags &= ~O_NONBLOCK;
            fcntl(sock_fd, F_SETFL, fd_flags);
        }
#endif
                _sock = sock_fd;
//		ups->upserror = 0;
//		ups->syserrno = 0;
                break;
            }

            freeaddrinfo(res);

            if (_sock < 0) {
                throw nut::IOException("Cannot connect to host");
            }


#ifdef OLD
            struct hostent *hostinfo = nullptr;
    SOCKADDR_IN sin = { 0 };
    hostinfo = ::gethostbyname(host.c_str());
    if(hostinfo == nullptr) /* Host doesnt exist */
    {
        throw nut::UnknownHostException();
    }

    // Create socket
    _sock = ::socket(PF_INET, SOCK_STREAM, 0);
    if(_sock == INVALID_SOCKET)
    {
        throw nut::IOException("Cannot create socket");
    }

    // Connect
    sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;
    if(::connect(_sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        _sock = INVALID_SOCKET;
        throw nut::IOException("Cannot connect to host");
    }
#endif // OLD
        }

        void DefaultSocket::disconnect() {
            if (_sock != INVALID_SOCKET) {
                ::closesocket(_sock);
                _sock = INVALID_SOCKET;
            }
            _buffer.clear();
        }

        bool DefaultSocket::isConnected() const {
            return _sock != INVALID_SOCKET;
        }

        size_t DefaultSocket::read(void *buf, size_t sz) {
            if (!isConnected()) {
                throw nut::NotConnectedException();
            }

            if (_tv.tv_sec >= 0) {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(_sock, &fds);
                int ret = select(_sock + 1, &fds, nullptr, nullptr, &_tv);
                if (ret < 1) {
                    throw nut::TimeoutException();
                }
            }

            ssize_t res = xread(_sock, static_cast<char *>(buf), sz);
            if (res == -1) {
                disconnect();
                throw nut::IOException("Error while reading from socket");
            }
            return static_cast<size_t>(res);
        }

        size_t DefaultSocket::write(const void *buf, size_t sz) {
            if (!isConnected()) {
                throw nut::NotConnectedException();
            }

            if (_tv.tv_sec >= 0) {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(_sock, &fds);
                int ret = select(_sock + 1, nullptr, &fds, nullptr, &_tv);
                if (ret < 1) {
                    throw nut::TimeoutException();
                }
            }

            ssize_t res = xwrite(_sock, static_cast<const char *>(buf), sz);
            if (res == -1) {
                disconnect();
                throw nut::IOException("Error while writing on socket");
            }
            return static_cast<size_t>(res);
        }
        std::string DefaultSocket::read() {
            std::string res;
            char buff[256];

            while(true)
            {
                // Look at already read data in _buffer
                if(!_buffer.empty())
                {
                    size_t idx = _buffer.find('\n');
                    if(idx!=std::string::npos)
                    {
                        res += _buffer.substr(0, idx);
                        _buffer.erase(0, idx+1);
                        return res;
                    }
                    res += _buffer;
                }

                // Read new buffer
                size_t sz = read(&buff, 256);
                if(sz==0)
                {
                    disconnect();
                    throw nut::IOException("Server closed connection unexpectedly");
                }
                _buffer.assign(buff, sz);
            }
        }
        /*
         * Don't touch this.
         */
        void DefaultSocket::write(const std::string& s) {
            auto vs = s + '\n';
            auto data = vs.data();
            size_t nextPos = 0;
            while (nextPos < vs.size()) {
                size_t bw = write(reinterpret_cast<const void *>(&data[nextPos]), vs.size() - nextPos);
                if (bw == 0)
                    throw IOException("Writing string failed");
                nextPos += bw;
            }
        }

        std::shared_ptr<nut::AbstractSocket> defaultFactory(){
            return std::shared_ptr<AbstractSocket>(new internal::DefaultSocket());
        };
    }
}