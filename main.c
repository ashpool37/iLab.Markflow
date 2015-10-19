#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include "wtrie.h"
#include "wsnippets.h"
#include <math.h>
#include <time.h>

#define _FAIL(X) goto X

int main (int argc, char* argv[])
{
    setlocale (LC_ALL, "en_US.utf-8");

    if (argc < 4) _FAIL (BADARGS);

    unsigned CONTEXT = strtoul (argv[1], NULL, 10);
    unsigned TLENGTH = strtoul (argv[2], NULL, 10);
    const char* in_fname      = argv[3];

    if ( !(TLENGTH && CONTEXT && in_fname) ) _FAIL (BADARGS);

    struct wTrie* WT = calloc (1, sizeof (struct wTrie));
    wTrie_init (WT, L'\0', 0, NULL, 0);

    wchar_t* wbuffer = wloadfile (in_fname, NULL);
    wchar_t* wbufpos = wbuffer;
    wchar_t* word = calloc (CONTEXT + 1, sizeof (wchar_t));

    struct wTrie* endnode = NULL;
    struct wTrie* newborn = NULL;
    struct wTrie* child = NULL;
    int nletters = 0;
    float fnum = 0.0;

    for (wbufpos; *(wbufpos + CONTEXT + 2) != L'\0'; wbufpos++)
    {
        wcsncpy (word, wbufpos, CONTEXT);
        endnode = wTrie_findword (WT, word);
        if (!endnode) endnode = wTrie_addword (WT, word);
        child = wTrie_child (endnode, *(wbufpos + CONTEXT), NULL);
        if (!child)
        {
            newborn = wTrie_spawn (1, endnode, *(wbufpos + CONTEXT), 0, NULL, sizeof (float));
            *(float*)(newborn -> meta) = (float)1.0;
        }
        else
        {
            (*(float*)(child -> meta)) += (float)1.0;
        }

        nletters = 0;
        for (child = endnode -> child; child; child = child -> sibling)
        {
            nletters += (int)*(float*)(child -> meta);
        }
        for (child = endnode -> child; child; child = child -> sibling)
        {
            fnum = *(float*)(child -> meta);
            *(float*)(child -> meta) = (float)(int)fnum + (float)(int)fnum / (float)nletters;
        }
    }

    srand (time (NULL));
    wchar_t* seed = calloc (CONTEXT, sizeof (wchar_t));
    wchar_t nwch = L'\0';
    float rndf = 0.0; float rndacc = 0.0;
    int i = 1; int j = 0;
    wcsncpy (seed, wbuffer, CONTEXT);

    for (int i = 0; i < TLENGTH; i++)
    {
        endnode = wTrie_findword (WT, seed);
        if (endnode)
        {
            rndf = (float)rand() / (float)((unsigned)RAND_MAX + 1);
            rndacc = 0.0;
            for (child = endnode -> child; child; child = child -> sibling)
            {
                rndacc += fmod (*(float*)(child -> meta), (float)1.0);
                if (!rndacc || rndacc >= rndf)
                {
                    nwch = child -> wc;
                    break;
                }
            }
            wprintf (L"%lc", nwch);
            for (int j = 0; j < CONTEXT; j++)
            {
                seed[j] = seed[j+1];
            }
            seed[CONTEXT-1] = nwch;
        }
        else
        {
            wcsncpy (seed, wbuffer, CONTEXT);
        }
    }

    //wTrie_dump (WT);
     
    wTrie_purge (WT);
    free (wbuffer);
    free (word);

    return 0;

//========[ ERRONEOUS TERMINATION ]========

BADARGS:
    printf ("Usage: %s <context length> <output length> <input file>\n", argv [0]);


}
