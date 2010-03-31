#ifndef OODLES_SCHED_NODE_HPP
#define OODLES_SCHED_NODE_HPP

// oodles
#include "url/URL.hpp"
#include "utility/Node.hpp"

namespace oodles {
namespace sched {

class PageData; // Forward declration for Node

class Node : public oodles::Node<url::value_type>
{
    public:
        /* Dependent typedefs */
        typedef url::value_type value_type;

        /* Member functions/methods */
        Node(const value_type &v);
        ~Node();

        bool eligible() const { return !assigned && page != NULL; }

        /* Member variables/attributes */
        float weight; // Weight of this *branch* inc. this node
        bool assigned; // Is this (leaf) node assigned a crawler?
        PageData *page; // Only used with leaf nodes, NULL otherwise
    private:
        /* Member functions/methods */
        void visit();
        Node* new_node(const value_type &v) const;
};

} // sched
} // oodles

#endif

