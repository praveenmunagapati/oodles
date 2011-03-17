#ifndef OODLES_NET_ENDPOINT_HPP
#define OODLES_NET_ENDPOINT_HPP

// oodles
#include "Buffer.hpp"
#include "common/page.hpp"

// Boost.shared
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

// Boost.asio
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>

// libc
#include <stdint.h> // For uint16_t

namespace oodles {

class Dispatcher; // Forward declaration for Endpoint

namespace net {

static const uint16_t NBS = OODLES_PAGE_SIZE; // Network buffer size

/* Free function for returning the local hostname of this endpoint */
inline std::string hostname() { return boost::asio::ip::host_name(); }

class SessionHandler; // Forward declaration for Endpoint
class ProtocolHandler; // Forward declaration for Endpoint

class Endpoint : public boost::enable_shared_from_this<Endpoint>
{
    public:
        /* Dependent typedefs */
        typedef boost::shared_ptr<Endpoint> Connection;

        /* Member functions/methods */
        ~Endpoint();

        static
        Endpoint*
        create(Dispatcher &d)
        {
            return new Endpoint(d);
        }

        bool online() const { return tcp_socket.is_open(); }
        boost::asio::ip::tcp::socket& socket() { return tcp_socket; }

        void stop(); // Close socket cancelling pending handlers
        void start(); // Must be called to register reads/writes

        void set_session(SessionHandler *s); // Pair session with endpoint
        SessionHandler* get_session() const { return session; }
        void set_protocol(ProtocolHandler *p); // Pair protocol with endpoint
        ProtocolHandler* get_protocol() const { return protocol; }
    private:
        struct Metric 
        {
            bool bootstrap;
            time_t last_transfer;
            double transfer_rate;
            size_t transferred_bytes, intermediate;

            Metric() :
                bootstrap(true),
                last_transfer(0),
                transfer_rate(0.0f),
                transferred_bytes(0)
            {}

            void update(size_t bytes);
        };
        
        /* Member variables/attributes */
        Buffer<NBS> inbound;
        Buffer<NBS> outbound;
        Metric recv_rate, send_rate;
        
        SessionHandler *session;
        ProtocolHandler *protocol;
        boost::asio::ip::tcp::socket tcp_socket;

        /* Member functions/methods */
        Endpoint(Dispatcher &d);

        Endpoint();
        Endpoint(const Endpoint &e);
        Endpoint& operator= (const Endpoint &e);

        /*
         * Asynchronous operations - registers the handlers/callbacks with
         * the io_service which are executed (the handlers) when a read and/or
         * write has been completed on our behalf by the OS.
         */
        void async_recv(char *ptr, size_t max);
        void async_send(const char *ptr, size_t max);

        /*
         * Handler executed by io_service when read operation is performed.
         * e denotes an error code should the read have failed
         * b contains the number of bytes transferred
         */
        void
        raw_recv_callback(const boost::system::error_code& e, size_t b);

        /*
         * Handler executed by io_service when write operation is performed.
         * e denotes an error code should the write have failed
         * b contains the number of bytes transferred
         */
        void
        raw_send_callback(const boost::system::error_code& e, size_t b);

        /* Friend class declarations */
        friend class ProtocolHandler; // Needs access to inbound and outbound
};

} // net
} // oodles

#endif

