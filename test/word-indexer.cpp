// oodles
#include "utility/file-ops.hpp"
#include "utility/IndexedSet.hpp"

// STL
#include <iostream>

#include <ctype.h> // For isspace()

// IO streams
using std::cout;
using std::cerr;
using std::endl;

// Containers
using std::string;

// STL exception
using std::exception;

static
bool
tokenise_words(const string &path)
{
    try {
        string s;
        size_t n = oodles::read_file_data(path, s);

        if (n != s.size())
            return false;

        string word;
        oodles::IndexedSet<string> words;
        string::const_iterator i = s.begin(), j = s.end();

        for (n = 0 ; i != j ; ++i) {
            if (isspace(*i)) {
                if (!word.empty()) {
                    words.insert(word);
                    word.clear();
                    ++n;
                }
            } else
                word += *i;
        }

        if (words.size() != n)
            return false;
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Provide the location (file path) to a raw, ASCII text file.\n";
        return 1;
    }

    int rc = 0;

    for (int i = 1 ; i < argc ; ++i) {
        cout << "IndexedSet test#" << i << " (" << argv[i] << ")...";

        if (tokenise_words(argv[i])) {
            cout << "PASS.";
        } else {
            cout << "FAIL.";
            rc = 1;
        }

        cout << endl;
    }

    return rc;
}
