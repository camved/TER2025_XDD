/*
 *	Resource management classes for XDD
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2004-8, IRIT UPS.
 *
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef OTAWA_XDD__NODE_H__
#define OTAWA_XDD__NODE_H__

#include <elm/data/Vector.h>
#include <elm/data/HashSet.h>
#include <elm/data/HashMap.h>
#include <climits>

// uncomment it to enable parallel computation
#define XDD_PARA

namespace otawa {
    namespace xengine {

        typedef uint64_t XddVar;
        typedef int XddVal;

        struct SpecialVals {
            static const int BOT = INT_MIN;
            static const int TOP = INT_MAX;
            static const int NoVal = -999999;
            static const unsigned NoVar = (unsigned) -1;
        };

        class Node {
        public:

            explicit Node(XddVal val) :
                    left(nullptr),
                    right(nullptr),
                    _var(SpecialVals::NoVar),
                    _val(val),
                    _max(val),
                    _min(val) {}; //TODO add mutex dans les attributs

            Node(XddVar v, const Node *left, const Node *right) :
                    left(left),
                    right(right),
                    _var(v),
                    _val(SpecialVals::NoVal),
                    _max(elm::max(left->_max, right->_max)),
                    _min(elm::min(left->_min, right->_min)) {};

            inline int countSubNodes() const { return nodesInTopoOrd().count(); };
            inline XddVal maxLeaf() const { return _max; };
            inline XddVal minLeaf() const { return _min; };
            inline const Node *low() const { return left; };
            inline const Node *high() const { return right; };

            inline XddVal val() const {
                //can't get a value if this is a NODE
                ASSERTP(isLeaf(), "ERROR: can't get the time(val) for a NODE!, Abort \n");
                return _val;
            };

            inline XddVar var() const {
                ASSERTP(!isLeaf(), "ERROR: can't get the var for a LEAF!, Abort \n");
                ASSERTP(_var > 0, " The variable index must be > 0, check you VarManager, Abort \n")
                return _var;
            };

            inline bool isLeaf() const { return _var == SpecialVals::NoVar; };
			unsigned height() const;
            void print(elm::io::Output &output) const;

            static void
            topologicalSorting(const Node *n, elm::HashSet<const Node *> &visited, elm::Vector<const Node *> &res);
            friend elm::io::Output &operator<<(elm::io::Output &output, const Node &x);

            inline bool operator==(const Node &n2) const {
                return _var == n2._var
                       && _val == n2._val
                       && left == n2.left
                       && right == n2.right;
            };

            inline int compare(const Node &n2) const {
                if (this == &n2)
                    return 0;
                if (isLeaf() && !n2.isLeaf())
                    return -1;
                if (!isLeaf() && n2.isLeaf())
                    return 1;
                if (isLeaf() && n2.isLeaf())
                    return val() - n2.val();

                //Both not leaves
                if (var() > n2.var())
                    return 1;
                else if (var() < n2.var())
                    return -1;
                else {
                    int diff = high()->compare(*n2.high());
                    if (diff != 0)
                        return diff;
                    else
                        return low()->compare(*n2.low());
                }
            }

            inline int countLeaves() const {
                return getLeaves().count();
            };

            elm::Vector<XddVal> getLeavesVal() const;
            elm::Vector<const Node *> getLeaves() const;
            elm::Vector<const Node *> nodesInTopoOrd() const;
            void printLeafs(elm::io::Output &output) const;

            inline XddVal right_most_leaf() const {
                if (this->isLeaf()) {
                    return _val;
                } else {
                    return right->right_most_leaf();
                }
            }


        private:
            const Node *left;
            const Node *right;
            XddVar _var;
            XddVal _val;
            XddVal _max;
            XddVal _min;
        };

    }
}


namespace elm {

    /**
     * @class Comparator<const otawa::xengine::Node*>
     * the comparator of Node
     */
    template<>
    class Comparator<const otawa::xengine::Node *> {
    public:
        static inline int compare(const otawa::xengine::Node *n1, const otawa::xengine::Node *n2) {
            if (n1->isLeaf() && n2->isLeaf())
                return int(n1->val() - n2->val());
            else if (n1->isLeaf() && !n2->isLeaf())
                return 1;
            else if (!n1->isLeaf() && n2->isLeaf())
                return -1;
            else {// both not leaves
                // if not same var in the node
                if (n1->var() != n2->var())
                    // return the difference
                    return int(n1->var() - n2->var());
                else { // same var
                    // if they are the same
                    if (n1->low() == n2->low() && n1->high() == n2->high())
                        return 0;
                    else { // same var but not the same tree
                        // compare low first
                        auto res = compare(n1->low(), n2->low());
                        if (res == 0) // same low tree => return high diff
                            return compare(n1->high(), n2->high());
                        else // diff low tree => just return the diff (low tree has priority)
                            return res;
                    }
                }
            }
        };

        int doCompare(const otawa::xengine::Node *n1, const otawa::xengine::Node *n2) const {
            return compare(n1, n2);
        }
    };


    // TODO now the key is 64 bits, should change the hash
    /**
     * @class HashKey<const Node*>
     * HashKey for xdd Node, using ? hash technique
     * @ingroup xdd
     */
    template<>
    class HashKey<const otawa::xengine::Node *> {
    public:
        inline t::hash computeHash(const otawa::xengine::Node *key) const { return hash(key); };

        inline bool isEqual(const otawa::xengine::Node *key1, const otawa::xengine::Node *key2) const {
            return equals(key1, key2);
        };
        const static uint32_t c1 = 0xcc9e2d51;
        const static uint32_t c2 = 0x1b873593;

        inline static t::hash subHash(const t::uint64 i, t::hash _hash) {
            t::uint32 k1 = i & 0xFFFFFFFF;
            k1 *= c1;
            k1 = rotl32(k1, 15);
            k1 *= c2;

            _hash ^= k1;
            _hash = rotl32(_hash, 13);
            _hash = _hash * 5 + 0xe6546b64;

            t::uint32 k2 = i >> 32;
            k2 *= c1;
            k2 = rotl32(k1, 15);
            k2 *= c2;

            _hash ^= k2;
            _hash = rotl32(_hash, 13);
            _hash = _hash * 5 + 0xe6546b64;
            return _hash;
        }

        inline static t::hash hash(const otawa::xengine::Node *key) {
            if (key->isLeaf()) {
                return subHash(key->val(), seed);
            } else {
                t::hash _hash = seed;
                _hash = subHash(reinterpret_cast<t::uint64>(key->low()), _hash);
                _hash = subHash(reinterpret_cast<t::uint64>(key->high()), _hash);
                _hash = subHash(key->var(), _hash);
                return _hash;
            }
        };

        inline static bool equals(const otawa::xengine::Node *key1, const otawa::xengine::Node *key2) {
            return (*key1) == (*key2);
        };

        inline static uint32_t rotl32(uint32_t x, int8_t r) {
            return (x << r) | (x >> (32 - r));
        }

        static const uint32_t seed = 13;
    };
}

namespace otawa {
    namespace xengine {

        class NodeManager {
        public:

            NodeManager() : _unique_table(XDD_HASH_SIZE), bot(new Node(SpecialVals::BOT)),
                            top(new Node(SpecialVals::TOP)), zero(new Node(0)) {};

            ~NodeManager() {
                for (auto i = _unique_table.begin(); i != _unique_table.end(); i++)
                    delete *i;
            }

            static inline uint64_t index(const Node *n) {
                if (n->isLeaf()) {
                    return 0;
                } else {
                    return n->var();
                }
            }

            inline const Node *mK(XddVar var, const Node *left, const Node *right) {

                ASSERTP(left != nullptr && right != nullptr,
                        "Panic: You may want to call mK(int val) to create a leaf");

                ASSERTP(var != SpecialVals::NoVar,
                        "Panic: Impossible to create None variable. "
                        "-999999 is reserved for None, maybe it is the problem?")

                // According to the mathematic definition of XDD,
                // if an event doesn't have any impact on leaves, it shouldn't exist
                if (left == right) {
                    return left;
                }

                // search if this node exists
                const Node _tmp(var, left, right);
                const Node *find = _unique_table.get(&_tmp, nullptr);

                // use it if exists, returns
                if (find != nullptr) {
                    return find;
                }

                // it doesn't exist, create it
                //lock TODO
                const Node *newNode = new Node(var, left, right);

                // register in the unique table
                _unique_table[(newNode)] = newNode;
                //delock TODO
                return newNode;
            }

            inline const Node *mK(const XddVal val) {

                ASSERTP(val != SpecialVals::NoVal,
                        "Panic: Can not initialize node with None value!\n");

                if (val == SpecialVals::TOP)
                    return top;

                if (val == SpecialVals::BOT)
                    return bot;

                if (val == 0)
                    return zero;

                //create the Leaf
                const Node _tmp(val);

                //check in the UniqueTable, return nullptr if not found
                const Node *find = _unique_table.get(&_tmp, nullptr);

                // already exists, use it
                if (find != nullptr)
                    return find;

                // it doesn't exist, create it
                const Node *newNode = new Node(val);

                // register in the hash table
                _unique_table[newNode] = newNode;
                return newNode;
            }

            // count total number of nodes in HashTable
            inline int countNodes() { return _unique_table.count(); };

            //Used for stats of hash function on unique_table, abandoned
            //#	ifdef ELM_STAT
            //		int minEntry(void) const { return  _unique_table.minEntry();}
            //		int maxEntry(void) const { return  _unique_table.maxEntry();}
            //		int zeroEntry(void) const { return _unique_table.zeroEntry(); }
            //		int size(void) const { return _unique_table.size(); }
            //#	endif


        private:
            elm::HashMap<const Node *, const Node *> _unique_table;
            const static int XDD_HASH_SIZE = 65551;

        public:
            const Node *bot;
            const Node *top;
            const Node *zero;
        };
    }
}
#endif // OTAWA_XDD__NODE_H__
