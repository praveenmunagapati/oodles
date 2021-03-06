// oodles
#include "URL.hpp"
#include "utility/hash.hpp"

// STL
#include <sstream>

// libc
#include <assert.h> // For assert()

// STL
using std::copy;
using std::string;
using std::vector;
using std::ostream;
using std::ostringstream;
using std::ostream_iterator;

/*
 * Here, the anonymous namespace holds only the old IDGenerator template
 * class. Instead of being a class member however, or even instances of it,
 * we use it local to this unit only and return the hash_t values to an
 * instance of a simpler class, ID.
 */
namespace {

typedef oodles::url::URL::hash_t hash_t;

// Define a local-only template function, hasher
template<class Container>
hash_t
hasher(const Container &input, hash_t seed);

// Provide the two necessary specialisations of hasher:
template<>
inline
hash_t
hasher<string>(const string &input, hash_t seed)
{
#ifdef HAS_64_BITS
    return oodles::fnv64(input.data(), input.size(), seed);
#else
    return oodles::fnv32(input.data(), input.size(), seed);
#endif
}

template<>
inline
hash_t
hasher<vector<string> >(const vector<string> &input, hash_t seed)
{
    vector<string>::const_iterator i = input.begin();

    for ( ; i != input.end() ; ++i)
        seed = hasher(*i, seed);

    return seed;
}

template<class Type>
class IDGenerator
{
    public:
        /* Member functions/methods */
        IDGenerator(const Type &value) : hash(0), content(value) {}
        hash_t id(hash_t seed = 0) { return compute_hash(seed); }
    private:
        /* Member functions/methods */
        hash_t compute_hash(hash_t seed)
        {
            if (!hash)
                hash = hasher(content, seed);

            return hash;
        }

        /* Member variables/attributes */
        hash_t hash;
        const Type &content;
};

} // anonymous

namespace oodles {
namespace url {

URL::URL()
{
}

URL::URL(const string &url) : id(tokenise(url))
{
}

URL::URL(const URL &url) : attributes(url.attributes), id(url.id)
{
}

URL&
URL::operator= (const URL &url)
{
    if (this != &url) {
        id = url.id;
        attributes = url.attributes;
    }

    return *this;
}

bool
URL::operator==(const URL &rhs) const
{
    return page_id() == rhs.page_id();
}

bool
URL::operator!=(const URL &rhs) const
{
    return !(operator==(rhs));
}

URL::hash_t
URL::page_id() const
{
    return id.page;
}

URL::hash_t
URL::path_id() const
{
    return id.path;
}

URL::hash_t
URL::domain_id() const
{
    return id.domain;
}

string
URL::host() const
{
    string s;
    
    if (!attributes.domain.empty()) {
        vector<string>::const_iterator i = attributes.domain.begin(),
                                       j = attributes.domain.end() - 1;

        while (i != j) {
            s += *i + ".";
            ++i;
        }

        s += *j;
    }

    return s;
}

string
URL::resource() const
{
    string s("/");
    
    if (!attributes.path.empty()) {
        vector<string>::const_iterator i = attributes.path.begin(),
                                       j = attributes.path.end() - 1;

        while (i != j) {
            s += *i + "/";
            ++i;
        }

        s += *j + "/";
    }

    s += attributes.page;

    return s;
}

string
URL::to_string() const
{
    ostringstream stream;
    to_stream(stream);
    
    return stream.str();
}

void
URL::print(ostream &stream) const
{
    to_stream(stream);
}

void
URL::to_stream(ostream &stream) const
{
    if (!attributes.scheme.empty())
        stream << attributes.scheme << "://";

    if (!attributes.username.empty()) {
        stream << attributes.username;
        
        if (!attributes.password.empty())
            stream << ':' << attributes.password;
        
        stream << '@';
    }

    stream << host();

    if (!attributes.port.empty())
        stream << ':' << attributes.port;

    stream << resource();
}

URL::ID
URL::tokenise(const string &url) throw(ParseError)
{
    Parser p;
    IDGenerator<string> page(attributes.page);
    IDGenerator<vector<string> > path(attributes.path),
                                 domain(attributes.domain);

    if (!p.parse(url, attributes))
       throw ParseError("URL::tokenise", 0, "Failed to parse input '%s'.",
                        url.c_str());

    const ID x = {domain.id(),
                  path.id(domain.id()),
                  page.id(path.id(domain.id()))};
    return x;
}

} // url
} // oodles
