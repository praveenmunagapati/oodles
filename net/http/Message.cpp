// oodles
#include "Message.hpp"

// STL
#include <sstream>
#include <algorithm>

// libc
#include <string.h> // For memchr() etc.
#include <assert.h> // For assert() etc.
#include <strings.h> // For strncasecmp() etc.

// STL
using std::string;
using std::vector;
using std::lower_bound;
using std::ostringstream;

namespace {
 
static const string EMPTY;

template<typename T>
inline
string
to_string(const T &t)
{
    ostringstream ss;
    ss << t;
    return ss.str();
}

} // anonymous

namespace oodles {
namespace net {
namespace http {

// Message::Header
bool
Message::Header::operator< (const Header &other) const
{
    return strncasecmp(key.c_str(), other.key.c_str(), key.size()) < 0;
}

bool
Message::Header::operator== (const Header &other) const
{
    return strncasecmp(key.c_str(), other.key.c_str(), key.size()) == 0;
}

bool
Message::Header::operator< (const string &key) const
{
    return strncasecmp(this->key.c_str(), key.c_str(), this->key.size()) < 0;
}

bool
Message::Header::operator== (const string &other) const
{
    return strncasecmp(this->key.c_str(), key.c_str(), this->key.size()) == 0;
}

// Message
const char Message::CR = '\r';
const char Message::LF = '\n';
const string Message::protocol("HTTP");

Message::Message(int fd, size_t size) :
    fd(fd),
    mode(-1),
    version(0),
    buffered(0),
    body_offset(0),
    body_size(size)
{
    if (size > 0)
        add_header("content-length", to_string<size_t>(size));
}

uint16_t
Message::response_code() const
{
    if (headers_received() && !start_line[1].empty())
        return atoi(start_line[1].c_str());

    return 0;
}

const string&
Message::response_phrase() const
{
    if (headers_received() && !start_line[2].empty())
        return start_line[2];

    return EMPTY;
}

void
Message::request(const string &method, const string &URI)
{
    version = 1.1f; // Always use 1.1 in Oodles
    
    start_line[0] = method;
    start_line[1] = URI.empty() ? "*" : URI;
    start_line[2] = protocol + "/" + to_string<float>(version);
}

const string&
Message::request_URI() const
{
    if (headers_received() && !start_line[1].empty())
        return start_line[1];

    return EMPTY;
}

const string&
Message::request_method() const
{
    if (headers_received() && !start_line[0].empty())
        return start_line[0];

    return EMPTY;
}

void
Message::respond(uint16_t code, const string &phrase)
{
    version = 1.1f; // Always use 1.1 in Oodles
    
    start_line[0] = protocol + "/" + to_string<float>(version);
    start_line[1] = to_string<uint16_t>(code);
    start_line[2] = phrase;
}

size_t
Message::size() const
{
    return body_offset + body_size;
}

const string&
Message::find_header(const string &key) const
{
    vector<Header>::const_iterator x = lower_bound(headers.begin(),
                                                   headers.end(),
                                                   key);

    if (!(x != headers.end() && *x == key))
        return EMPTY;

    return x->value;
}

bool
Message::add_header(const string &key, const string &value)
{
    vector<Header>::iterator x = lower_bound(headers.begin(),
                                             headers.end(),
                                             key);
    if (x != headers.end() && *x == key)
        return false; // Header indexed by key already exists
    
    headers.insert(x, Header(key, value));

    return true;
}

bool
Message::complete() const
{
    if (mode == Outbound)
        return headers_sent() && pending() == 0;
    
    return headers_received() && pending() == 0;
}

size_t
Message::to_buffer(char *buffer, size_t max)
{
    size_t used = 0;
    
    if (!headers_sent())
        used = write_headers(buffer, max);

    if (used < max)
        used += write_body(buffer + used, max - used);

    buffered += used;
    
    return used;
}

size_t
Message::from_buffer(const char *buffer, size_t max)
{
    size_t used = 0;
    
    if (!headers_received())
        used = read_headers(buffer, max);
    
    if (used < max)
        used += read_body(buffer + used, max - used);
    
    buffered += used;

    return used;
}

size_t
Message::pending() const
{
    size_t n = 0;
    
    if (mode == Inbound) {
        const uint16_t code = atoi(start_line[1].c_str());
        
        if (!(code < 200 || code == 204 || code == 304)) { // Refer to RFC2616
            const string &h = find_header("content-length");

            if (!h.empty())
                n = static_cast<size_t>(atoll(h.c_str()));
            else {
                /* 
                 * TODO: Try alternatives. In fact, fully implement section
                 * section 4.4 of RFC2616 inc. transfer length precedence.
                 */
            }
        }
    } else if (mode == Outbound) {
        n = body_size;
    }
    
    return n == 0 ? 0 : n - (buffered - body_offset);
}

size_t
Message::read_body(const char *buffer, size_t max)
{
    if (fd == -1)
        return 0;
    
    size_t x = pending(), n = write(fd, buffer, x < max ? x : max);

    body_size += n;

    return n;
}

void
Message::read_start_line(const char *buffer, size_t max)
{
    static const char DL = ' ';
    string s(buffer, max);
    bool last = false;

    mode = Inbound;
    
    for (size_t i = 0, j = 0, k = 0 ; i < max ; ++i)  {
        if (s[i] == DL || last) {
            start_line[k++] = s.substr(j, (last ? max : i) - j);

            if (k < 3) {
                if (k == 2)
                    last = true;
                
                j = i + 1;
            } else
                i = max;
        }
    }

    int i = start_line[0].size() - (protocol.size() + 1),
        j = start_line[2].size() - (protocol.size() + 1);

    if (i < 0) // If negative, indicates start_line cannot be Response
        s = start_line[2].substr(protocol.size() + 1, j);
    else if (j < 0) // If negative, indicates start_line cannot be Request
        s = start_line[0].substr(protocol.size() + 1, i);
    else
        ; // TODO

    version = atof(s.c_str());
}

size_t
Message::read_headers(const char *buffer, size_t max) throw (HeaderError)
{
    static const char DL = ':';
    size_t i = 0, j = 0;
    
    while (!(headers_received() || i == max)) {
        if (buffer[i] != LF) {
            ++i;
            continue;
        }
        
        if (buffer[i - 3] ==  CR && buffer[i - 2] == LF && buffer[i - 1] == CR)
            body_offset = i + 1; // Header/body split located
        else if (buffer[i - 1] != CR)
            throw HeaderError("Message::identify_header", 0,
                              "Expected CRLF terminator but none found.");

        if (j == 0) { // The start-line.
            read_start_line(buffer, i - 1);
        } else if (!body_offset) { // A Message header
            const char *p = buffer + j;
            const char *d = static_cast<const char*>(memchr(p, DL, i - j));

            if (!d)
                throw HeaderError("Message::identify_header", 0,
                                  "Delimiter '%c' not found in header.", DL);
            
            Header h;
            vector<Header>::iterator b(headers.begin()), e(headers.end());

            h.key.assign(p, d - p);
            h.value.assign(d + 1, i - j - (h.key.size() + 2));

            headers.insert(lower_bound(b, e, h), h);
        }

        j = 1 + i++;
    }

    return i;
}

size_t
Message::write_body(char *buffer, size_t max)
{
    if (fd == -1)
        return 0;

    size_t x = pending();
    return read(fd, buffer, x < max ? x : max);
}

size_t
Message::write_headers(char *buffer, size_t max)
{
    static const char DL = ':';
    size_t used = 0, n = 0, x = 0;
    vector<Header>::iterator i(headers.begin()), j(headers.end()), k;
    
    if (start_line.size() > 0)
        if ((used = write_start_line(buffer, max)) == 0)
            return 0; // Not enough buffer space even for the request line

    for (x = used ; i != j ; ) {
        /*
         * 3 = DL + CR + LF
         * 2 = CRLF indicating end of headers
         */
        k = i + 1;
        n = i->key.size() + i->value.size() + 3 + (k == j ? 2 : 0);

        if (n > max)
            break; // We can't fit any more full-line headers

        memcpy(buffer + x, i->key.c_str(), i->key.size());
        x += i->key.size();
        memcpy(buffer + x, &DL, 1);
        x += 1;
        memcpy(buffer + x, i->value.c_str(), i->value.size());
        x += i->value.size();
        
        memcpy(buffer + x, &CR, 1);
        memcpy(buffer + x + 1, &LF, 1);
        x += 2;

        headers.erase(i);
        i = k;
    }

    /*
     * Add the end-of-headers indicator, if all were written.
     */
    if (headers.empty()) {
        memcpy(buffer + x, &CR, 1);
        memcpy(buffer + x + 1, &LF, 1);
        x += 2;
    }
    
    body_offset = x;

    return x;
}

size_t
Message::write_start_line(char *buffer, size_t max)
{
    static const char DL = ' ';
    size_t n = start_line.size() + 4, x = 0; // 4 = 2xDL + CR + LF
    
    mode = Outbound;
    
    if (max < n)
        return 0; // Only write a complete request line or none at all

    for (uint16_t i = 0 ; i < 3 ; ++i) {
        string &s = start_line[i];
        
        memcpy(buffer + x, s.c_str(), s.size());
        x += s.size();
        memcpy(buffer + x, &DL, 1);
        x += 1;

        s.clear();
    }

    memcpy(buffer + (x - 1), &CR, 1); // Overwrite that last DL!
    memcpy(buffer + x, &LF, 1);

    return n;
}

} // http
} // net
} // oodles
