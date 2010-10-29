#ifndef OODLES_EVENT_EVENT_HPP
#define OODLES_EVENT_EVENT_HPP

// STL
#include <set>

namespace oodles {
namespace event {

class Publisher; // Forward declaration for Event
class Subscriber; // Forward declaration for Event

class Event
{
    public:
        /* Member functions/methods */
        Event();
        virtual ~Event() = 0;

        bool add_subscriber(Subscriber &s);
        bool remove_subscriber(Subscriber &s);
        void publish(const Publisher &p) const;

        /*
         * Templated cast operator will automatically,
         * given the correct (related) type, cast this
         * base class object to your derived subclass.
         *
         * Publisher::event() will call this below...
         */
        template<typename Derived>
        operator const Derived& () const
        {
            return *static_cast<const Derived*>(this);
        }
    private:
        /* Member variables/attributes */
        std::set<Subscriber*> subscribers;

        /* Friend class declarations */
        friend class Publisher; // Accesses subscribers
        friend class Subscriber; // Accesses subscribers
};

} // event
} // oodles

#endif
