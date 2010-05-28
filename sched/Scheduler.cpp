// oodles
#include "PageData.hpp"
#include "Scheduler.hpp"

#include <time.h> // For time()

// STL
using std::string;
using std::vector;
using std::ostream;

static
inline
oodles::sched::Node*
parent_of(oodles::sched::Node::Base &node)
{
    return static_cast<oodles::sched::Node*>(node.parent);
}

namespace oodles {
namespace sched {

Scheduler::Scheduler() : leaves(0), tree(new Node("ROOT"))
{
}

Scheduler::~Scheduler()
{
}

uint32_t
Scheduler::run()
{
    /*
     * In our priority queue, crawlers, the top most item will
     * be the one with the least amount of work (therefore able
     * to take on more). If that crawler has reached full capacity
     * we can do no more.
     */
    Node *n = NULL;
    uint32_t i = 0, j = crawlers.size(), t = 0;
    for (Crawler *c = crawlers.top() ; i < j ; c = crawlers.top(), ++i) {
#ifdef DEBUG_SCHED
        std::cerr << '[' << c->id() << "|PRE]: " << c->unit_size() << std::endl;
#endif

        if (c->online()) // Do not assign anything to offline Crawlers
            t += fill_crawler(*c, n); // Assign as much work (fill work unit)

#ifdef DEBUG_SCHED
        std::cerr << '[' << c->id() << "|PST]: " << c->unit_size() << std::endl;
#endif
        crawlers.pop(); // Pop the top of the queue (i.e. remove 'c')
        crawlers.push(c); // Push c back onto the queue forcing rank order
    }

    return t;
}

void
Scheduler::replay_run(ostream &s)
{
    static const NodeBase *root = static_cast<const NodeBase*>(&tree.root());
    typedef unsigned long address_t; // Memory location as an integer

    if (trail.empty() || root->size() == 0)
        return;

    size_t e = 0; // Edge
    const NodeBase *n = root; // Node
    address_t nid = 0, pid = 0; // Vertices
    BreadCrumbTrail::Point i = trail.gather_crumb(); // Discard first crumb

    while (!trail.empty()) {
        nid = reinterpret_cast<address_t>(n); // First, set the DOT node ID

        /*
         * Print the node ID with the label as n->value
         */
        if (n == root) {
            s << nid << " [label=\"ROOT\"];\n";
        } else {
            s << nid << " [label=\"";
            n->print(s);
            s << "\"];\n";

            /*
             * Label the edge with the trail sequence ID
             */
            s << pid << " -> " << nid << " [label=" << e << "];\n";
        }

        i = trail.gather_crumb(); // Now gather the next crumb

        /*
         * Determine the direction of travel; If the crumb path index is less
         * than that of the current node path index we're retreating back up
         * the way we came - back up the path to where we forked (in the road).
         */
        if (i.first < n->path_idx) {
            do {
                i = trail.gather_crumb(); // Gather until we reach the fork
                n = n->parent;
            } while (!trail.empty() && i.first < n->path_idx);

            if (trail.empty())
                break;

            nid = reinterpret_cast<address_t>(n); // Set the DOT node ID again
        }

        assert(i.second < n->size()); // The crumb cannot exceed #children
        n = &(n->child(i.second)); // Next node is located by the current crumb
        pid = nid;
        ++e;
    }
}

bool
Scheduler::register_crawler(Crawler &c)
{
    crawlers.push(&c); // TODO: Check for duplicates
    return true;
}

void
Scheduler::schedule_from_seed(const string &url)
{
    schedule(url, true);
}

void
Scheduler::schedule_from_crawl(const string &url)
{
    schedule(url, false);
}

Node*
Scheduler::traverse_branch(Node &n)
{
    Node *p = select_best_child(n);

    trail.drop_crumb(n.path_idx, n.child_idx); // Leave a breadcrumb trail

    /*
     * If we have been unable to consider any children as eligible for
     * crawling then we must mark the node as 'Red' - we do not
     * want to traverse it again... yet.
     */
    if (!p) {
        if (n.eligible()) // Return when we've found something crawlable
            return &n;

        p = parent_of(n); // Begin to backtrack

        if (!p)
            return NULL; // We've returned to the root node
    }

    return traverse_branch(*p); // Continue delving deeper
}

void
Scheduler::weigh_tree_branch(Node &n) const
{
    if (!n.parent)
        return;

    Node *parent = parent_of(n);

    parent->weight -= n.weight;
    n.weight = n.calculate_weight();
    parent->weight = parent->weight + (n.weight / (n.size() + 1));

    weigh_tree_branch(*parent);
}

Node*
Scheduler::select_best_child(Node &parent) const
{
    Node *n = NULL;

    for (size_t i = 0 ; i < parent.size() ; ++i) {
        Node &c = parent.child(i);

        if (c.visit_state == Node::Red) // Skip-over any visited branch
#ifdef DEBUG_SCHED
        {
            std::cerr << '[' << c.value << "]: marked RED\n";
#endif
            continue;
#ifdef DEBUG_SCHED
        }
#endif

        c.visit_state = Node::Amber; // Node visited but not all children

        if (c.page && !c.eligible()) // Ignore ineligible yet crawlable nodes
            continue;

        if (!n || (n && c.weight > n->weight))
            n = &c;
    }

    if (!n) { // No child was a candidate although all were considered
        parent.visit_state = Node::Red;

        if ((n = parent_of(parent)))
            ++n->visited;

        n = NULL;
    }

#ifdef DEBUG_SCHED
    if (n)
        std::cerr << '[' << n->value << "]: Selected\n";
#endif

    return n;
}

Crawler::unit_t
Scheduler::fill_crawler(Crawler &c, Node *&n)
{
    static const Node *root = static_cast<const Node*>(&tree.root());

    if (root->visit_state == Node::Red) // Every single node has been traversed
#ifdef DEBUG_SCHED
    {
        std::cerr << "[ROOT]: Entire tree marked RED!\n";
#endif
        return 0;
#ifdef DEBUG_SCHED
    }
#endif

    Node *p = NULL;
    Crawler::unit_t assigned = 0;
    while (c.unit_size() < Crawler::max_unit_size()) {
        if (!n)
            n = const_cast<Node*>(root); // At the top of the tree
        else
            n = parent_of(*n); // Begin to backtrack

        if (!n)
            break; // We've exhausted the tree (root's parent is NULL)

        p = n; // Keep a copy of our current position
        n = traverse_branch(*n); // Traverse to find best candidate for crawling

        if (n && n->eligible()) {
            n->page->assign_crawler(&c);
            ++assigned;
        } else if (!n) // traverse_branch() exhausted the tree too - we're done!
            break;
        else
            n = p; // Restore the copy of the previous node
    }

    /*
     * If our last node was a leaf node that was assigned to
     * a crawler, mark it's ancestors as Red where necessary.
     */
    if (n && n->leaf() && (n->page && n->page->crawler)) {
        for (p = parent_of(*n) ; p ; ) {
            if (p->visited < p->size()) 
                break; // Terminate the loop if any unvisited children

            p->visit_state = Node::Red;
            p = parent_of(*p);

            if (p)
                p->visited++; // Update the grandparent
        }
    }

    return assigned;
}

void
Scheduler::schedule(const string &url, bool from_seed)
{
    bool duplicate = true;
    PageData *page = new PageData(url);
    Node *node = static_cast<Node*> (tree.insert(page->url.begin_tree(),
                                                 page->url.end_tree()));

    if (node->leaf() && !node->page)
        duplicate = false;

    if (!duplicate) { // Newly inserted, unique URL
        ++leaves;
        node->page = page; // Ownership of page is implicitly transferred here
    } else {
        delete page;
        page = node->page;
    }

    if (!from_seed) {
        ++page->links; // If we're from a seed it doesn't count as a link!
        weigh_tree_branch(*node); // Calculates branch schedule index (weight)
    }
}

} // sched
} // oodles
