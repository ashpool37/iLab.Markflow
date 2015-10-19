/*
* Copyright (c) 2015 Artyom Zhurikhin 
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

/**
 * @file wtrie.h
 * @author Artyom Zhurikhin a.k.a.Thurisaz
 * @date 17 Oct 2015
 * @brief Header file containing wTrie data structure
 *
 * wTrie is an implementation of Trie (a.k.a. Prefix Tree) data structure with support for
 * wide characters (wchar_t) and additional meta pointers. It was initially created for maintaining
 * strings list for a Markov model based text generator.
 *
 * @see https://en.wikipedia.org/wiki/Trie
 * @see https://github.com/p00tis
 * @see https://vk.com/thrsz
 */

#ifndef _WTRIE_H_
#define _WTRIE_H_


#include <wchar.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "wtrerrno.h"
#include "precond.h"

/**
 * @brief The wTrie data structure.
 *
 * wTrie implements a Prefix Tree (Trie), which provides a convenient interface for storing lots of
 * words (strings) with lower memory consumption than array and linear search time. Nodes in tree represent
 * symbols, words always start from the root node (the root node itself is not a part of any word) and 
 * end with a terminating node (the terminating node is always included and contains the last symbol). 
 * Terminating nodes are not necessarily leaves of the tree and may exist in the midst of any other word.
 * The data structure is recursive, each node is an instance if wTrie structure and can be regarded as a
 * subtree or branch.
 *
 * Subnodes of a node are called @b children of that node, the node then is called @b parent of those
 * subnodes. Children of the same node are called @siblings. For the reasons of more effective memory
 * management, each node has only two pointer members: one to a child and one to a sibling. That way,
 * only the first child is linked directly to its parent, all the other children form a singly linked
 * list by pointing to the next sibling. Pointers to parents are not supported.
 *
 * @warning Developers should consider modifying the structure and adding new fields if necessary 
 * rather than using void* meta extensively.
 *
 */ 

struct wTrie
{
    wchar_t       wc;       //!< The wide charater of the node.
    struct wTrie* sibling;  //!< The next sibling. Shall be NULL if the node is the last child.
    struct wTrie* child;    //!< The first child. Shall be NULL if the node has no children.
    bool          term;     //!< The flag showing if the node is terminating. 
    void*         meta;     //!< Pointer to any value, just in case.
};

/** @brief Recursive consistency check.
 *
 * @return @b 1 if check succeeds, @b 0 otherwise.
 *
 */

int wTrie_rOK (struct wTrie* wtr)
{
    return wtr && 
           (wtr -> sibling ? wTrie_rOK (wtr -> sibling) : 1) &&
           (wtr -> child   ? wTrie_rOK (wtr -> child)   : 1);
}

/** @brief Constructor of wTries and new nodes. Should ALWAYS be called after creating a wTrie.
 *
 * Initializes the wTrie instance pointed by @c wtr with supplied values. Sibling and child pointers
 * are set to @c NULL. 
 *
 * @return @b 0 if supplied with a NULL @c wtr pointer, 1 otherwise.
 */

int wTrie_init (struct wTrie* wtr, //!< Pointer to initialize at
                wchar_t wc,        //!< Wide character of the node
                bool term,         //!< Terminating node flag
                void* meta,        //!< Pointer to meta. Should be NULL if meta_sz is non-zero
                size_t meta_sz     //!< Size of memory to allocate for metadata. Ignores supplied void* meta
                )
{
    _PRECONDITION (wtr, E_WTRIE_NULLPOINTER, return 0);

    wtr -> sibling = NULL;
    wtr -> child   = NULL;
    wtr -> wc      = wc;
    wtr -> term    = term;
    if (meta_sz > 0)
    {
        if (meta) errno = W_WTRIE_METANOTSET;
        wtr -> meta = calloc (1, meta_sz);
    }
    else
    {
        wtr -> meta = meta;
    }
    return 1;
}

/** @brief Prints wTrie structure contents recursively to standard output.
 *
 * Useful for debugging. Performs consistency check with @c wTrie_rOK before dumping. Terminating nodes
 * are marked with '.', nodes containing non-NULL meta pointers are marked with '*'. Nodes containing 
 * newline and null characters are printed as {n} and {0}.
 *
 * @warning The function uses a static variable and is therefore thread unsafe. Using it for other purposes
 * than debugging is deprecated.
 */

void wTrie_dump (struct wTrie* wtr)
{
    _PRECONDITION (wTrie_rOK, E_WTRIE_CORRUPT, return);

    static int level = 0;

    for (int i = 0; i < level - 1; i++) wprintf (L"    ");
    if (level > 0) wprintf (L" `--");
    if (wtr -> wc == L'\0')
        wprintf (L"{0}\n");
    else if (wtr -> wc == L'\n')
        wprintf (L"{n}\n");
    else
        wprintf (L"[%lc]%lc%lc\n", wtr -> wc,
                                (wtr -> term ? L'.' : L' '),
                                (wtr -> meta ? L'*' : L' '));

    level++;

    for (struct wTrie* child = wtr -> child; child; child = child -> sibling)
        wTrie_dump (child);

    level--;    
}

/** @brief The poisonous node destructor.
 *
 * Fills the memory allocated for the supplied node with @c C3 ("poison") byte, then frees the memory.
 *
 * @return @b 0 if supplied with a NULL @c wtr pointer, @b 1 otherwise.
 * @warning Operation is non-recursive and discards only one given node. Any children and siblings of
 * discared node will become orphaned and unaccessible. Use wTrie_purge for subtree removing.
 *
 */

int wTrie_discard (struct wTrie* wtr)
{
    _PRECONDITION (wtr, E_WTRIE_NULLPOINTER, return 0);

    if (wtr -> meta) free (wtr -> meta);
    memset (wtr, 0xC3, sizeof (struct wTrie));
    free (wtr);

    return 1;
}

/** @brief Search child of a node by given symbol.
 *
 * Performs list traversal of given node's (parent) children, comparing their @b wc with supplied @b wch.
 * Returns pointer to the found child. If @b lsibling_p is not NULL, puts there a pointer to the previous
 * sibling of found node, if any. Does not perform consistency check before operation.
 *
 * @return NULL if no children were found or parent supplied was NULL. Pointer to the found child otherwise. 
 */

struct wTrie* wTrie_child (struct wTrie* parent, wchar_t wch, struct wTrie** lsibling_p)
{
    _PRECONDITION (parent, E_WTRIE_ORPHAN, return NULL);

    struct wTrie* lostchild = NULL;
    struct wTrie* lsib = NULL;
    
    for (struct wTrie* child = parent -> child; child && !lostchild; child = child -> sibling)
    {
        if (child -> wc == wch)
        {
            lostchild = child;
            if (lsibling_p) *lsibling_p = lsib;
        }
        if (child != parent -> child)
        {
            lsib = child;
        }
    }
   
    return lostchild;
}

/** @brief Spawns a child with given parameters to a given node.
 *
 * Creates a new child to the supplied @b parent node and initializes it with supplied wch, term, and meta 
 * values. Newly created child is linked directly to the parent, and other children of the parent, if any, 
 * are linked to the created node. If finds a child with the same @b wch value and if strict flag is 
 * non-zero, function fails. If strict is zero, overwrites the found child with supplied values, not 
 * changing its position. Fails if parent is NULL.
 *
 * @return NULL if fails, pointer to the created (overwritted) node otherwise.
 */

struct wTrie* wTrie_spawn (bool strict, struct wTrie* parent,
                           wchar_t wch, bool term, void* meta, size_t meta_sz)
{
    _PRECONDITION (parent, E_WTRIE_ORPHAN, return NULL);

    struct wTrie* child = wTrie_child (parent, wch, NULL); 
    struct wTrie* newborn = NULL;
    if (!child)
    {
        newborn = calloc (1, sizeof (struct wTrie));
        wTrie_init (newborn, wch, term, meta, meta_sz);
        if (parent -> child) newborn -> sibling = parent -> child;
        parent -> child = newborn;
    }
    else
    {
        if (strict)
        {
            errno = E_WTRIE_DUPCHILD;
            return NULL;
        }
        else
        {
            child -> term = term;
            child -> meta = meta;
            return child;
        }
    }
    
    if (newborn) return newborn;
    else
    {
        errno = E_WTRIE_SPAWNFAILED;
        return NULL;
    }
}

/** @brief Recursively poisonously discards a node, its siblings and children. */

void wTrie_purge (struct wTrie* wtr)
{
    _PRECONDITION (wTrie_rOK, E_WTRIE_CORRUPT, return);

    struct wTrie* sib = wtr -> sibling;
    struct wTrie* child = wtr -> child;

    wTrie_discard (wtr);

    if (sib) wTrie_purge (sib);
    if (child) wTrie_purge (child);
}

/** @brief Purges a child containing the given wch symbol, preserving its siblings.
 *
 * Function locates the child which contains the given @b wch symbol. After that, it links its sibling, if
 * any, to parent if the child was linked to parent, or to the previous sibling otherwise. Finally, 
 * it issues recursive purge on the child (cutting off its sibling first).
 *
 * @return 0 if fails, 1 otherwise.
 */

int wTrie_collapse (struct wTrie* parent, wchar_t wch)
{
    _PRECONDITION (parent, E_WTRIE_ORPHAN, return 0);

    struct wTrie* child = NULL;
    struct wTrie* lsib = NULL; 
    child = wTrie_child (parent, wch, &lsib);
    if (!child)
    {
        errno = E_WTRIE_NOSUCHNODE;
        return 0;
    }

    if (child -> sibling)
    {
        if (lsib)
        {
            lsib -> sibling = child -> sibling;
        }
        else
        {
            parent -> child = child -> sibling;
        }
    }
    
    child -> sibling = NULL;
    wTrie_purge (child);
    return 1;
}

/** @brief Inserts new word into the tree.
 *
 * Traverses the tree, adding new nodes for the symbols of wstring. The last node is set as terminating.
 *
 * @return pointer to the last added node if succeds, NULL otherwise.
 *
 */

struct wTrie* wTrie_addword (struct wTrie* wtr, const wchar_t* wstring)
{
    _PRECONDITION ((wstring && wtr), E_WTRIE_NULLPOINTER, return NULL);
    _PRECONDITION (*wstring,         E_WTRIE_EMPTYWORD,   return NULL);

    if (*(wstring + 1) != L'\0')
    {
        struct wTrie* nwtr = wTrie_spawn (0, wtr, *wstring, 0, NULL, 0);
        return wTrie_addword (nwtr, wstring + 1);
    }
    else return wTrie_spawn (0, wtr, *wstring, 1, NULL, 0);
}

/** @brief Same as addword, but adds only first n symbols of wstring */

struct wTrie* wTrie_addnword (struct wTrie* wtr, const wchar_t* wstring, int n)
{
    _PRECONDITION ((wstring && wtr),     E_WTRIE_NULLPOINTER, return NULL);
    _PRECONDITION ((*wstring && n >= 1), E_WTRIE_EMPTYWORD,   return NULL);

    if (*(wstring + 1) != L'\0' && n > 1)
    {
        struct wTrie* nwtr = wTrie_spawn (0, wtr, *wstring, 0, NULL, 0);
        return wTrie_addnword (nwtr, wstring + 1, n - 1);
    }
    else return wTrie_spawn (0, wtr, *wstring, 1, NULL, 0);
}

/** @brief Finds the given word in the tree.
 *
 * @return Pointer to the terminating node of the found word, NULL of fails or doesn't find the given word.
 *
 */

struct wTrie* wTrie_findword (struct wTrie* wtr, const wchar_t* wstring)
{
    _PRECONDITION ((wstring && wtr), E_WTRIE_NULLPOINTER, return NULL);
    _PRECONDITION (*wstring,         E_WTRIE_EMPTYWORD,   return NULL);
    
    struct wTrie* child = wTrie_child (wtr, *wstring, NULL);
    if (child)
    {
        if (child -> term && *(wstring + 1) == L'\0')
        {
            return child;
        }
        else if (*(wstring + 1) != L'\0')
        {
            return wTrie_findword (child, wstring + 1);
        }
    }
    errno = I_WTRIE_NOSUCHWORD;
    return NULL;
}

/** @brief Same as findword, but puts parent and previous sibling into the given addresses if those are non-NULL
 */

struct wTrie* wTrie_findword_rel (struct wTrie* wtr, const wchar_t* wstring,
                                  struct wTrie** parent_p, struct wTrie** lsibling_p)
{
    _PRECONDITION ((wstring && wtr), E_WTRIE_NULLPOINTER, return NULL);
    _PRECONDITION (*wstring,         E_WTRIE_EMPTYWORD,   return NULL);

    struct wTrie* child = wTrie_child (wtr, *wstring, *(wstring + 1) == L'\0' ? lsibling_p : NULL);
    if (child)
    {
        if (child -> term && *(wstring + 1) == L'\0')
        {
            return child;
        }
        else if (*(wstring + 1) != L'\0')
        {
            if (*(wstring + 2) == L'\0')
            {
                struct wTrie* mbchild = NULL;
                struct wTrie* mbparent = child;
                struct wTrie* mblsibling = NULL;

                mbchild = wTrie_findword_rel (child, wstring + 1, &mbparent, &mblsibling);
                if (mbchild)
                {
                    if (parent_p) *parent_p = mbparent;
                    if (lsibling_p) *lsibling_p = mblsibling;
                    return mbchild;
                }
            }
            else
            {
                return wTrie_findword_rel (child, wstring + 1, parent_p, lsibling_p);
            }
        }
    }
    errno = I_WTRIE_NOSUCHWORD;
    return NULL;
}

/** @brief Service function for determining if the given node has more than 1 child. */

bool wTrie_hasmultichild (struct wTrie* wtr)
{
    _PRECONDITION (wtr, E_WTRIE_NULLPOINTER, return false);

    if (wtr -> child)
    {
        if (wtr -> child -> sibling)
            return true;
        else
            return false;
    }
    else
        return false;
}

/** @brief Finds a leaf of the given word in the tree.
 *
 * If the word is found, returns a pointer to the node from which on all nodes of the word are belonging
 * to a leaf terminated by the terminating node of the word. Puts its parent to the @b parent_p
 * address if parent_p is not NULL.
 *
 * @return Pointer to the root of the leaf, NULL of fails or doesn't find such word or leaf. */

struct wTrie* wTrie_leaf (struct wTrie* wtr, const wchar_t* wstring, struct wTrie** parent_p)
{
    _PRECONDITION ((wstring && wtr), E_WTRIE_NULLPOINTER, return NULL);
    _PRECONDITION (*wstring,         E_WTRIE_EMPTYWORD,   return NULL);

    struct wTrie* endnode = wTrie_findword (wtr, wstring);
    _PRECONDITION (endnode,          E_WTRIE_NOSUCHWORD,  return NULL);

    struct wTrie* leaf = NULL;
    struct wTrie* node = wtr; int i = 0;
    struct wTrie* parent = NULL;
    struct wTrie* child = NULL;
     
    bool fin = 0;
    do
    {
        if (node == endnode) fin = 1;
        
        child = wTrie_child (node, *(wstring + i++), NULL); 

        if (!fin)
        {
            if (!wTrie_hasmultichild (child))
            {
                if (!leaf) leaf = child;
                if (!parent) parent = node;
            }
            else
            {
                leaf = NULL;
                parent = NULL;
            }
        }
        else
        {
            if (node -> child)
            {
                leaf = NULL; parent = NULL;
            }
        }

        node = child;

    } while (!fin);

    if (parent_p) *parent_p = parent;
    return leaf;
}

/** @brief Removes the given word from the tree.
 *
 * Deletes the leaf of the given word. If word is not terminating a leaf, unsets its terminating node.
 *
 * @return 1 on success, 0 on failure.*/

int wTrie_rmword (struct wTrie* wtr, const wchar_t* wstring)
{
    struct wTrie* endnode = wTrie_findword (wtr, wstring);
    _PRECONDITION (endnode,          E_WTRIE_NOSUCHWORD,  return NULL);

    struct wTrie* parent = NULL;
    struct wTrie* leaf = wTrie_leaf (wtr, wstring, &parent);
    if (leaf && !(endnode -> child))
    {
        wTrie_collapse (parent, leaf -> wc);
    }
    else
    {
        endnode -> term = false;
    }
    return 1;
}

#endif
