#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>

#define DFROMBGN  1
#define DFROMEND -1

int fgetnwc (wchar_t* wbuff, int n, FILE* infile)
{
    assert (infile);

    int i = 0;
    wchar_t winp = (wchar_t)fgetwc (infile);
    for (i; winp != WEOF && (n? i < n : 1); i++)
    {
        *(wbuff + i) = winp;
        winp = (wchar_t)fgetwc (infile);
    }
    return i;
}

wchar_t* wloadfile (const char* in_fname, wchar_t* wbuffer) 
{
    // TODO: Handle files bigger than 4GB (size > unsigned long long int)
    FILE* infile = fopen (in_fname, "r");
    if (!infile) return NULL;

    struct stat infile_st;
    stat (in_fname, &infile_st);
    uintmax_t infile_size = infile_st.st_size;

    if (!wbuffer)
    {
        wbuffer = calloc (infile_size, sizeof (wchar_t));
    }
    else
    {
        wbuffer = realloc (wbuffer, infile_size * sizeof (wchar_t) + 16);
        wbuffer[infile_size + 1] = L'\0'; 
    }

    fgetnwc (wbuffer, 0, infile);
    
    fclose (infile);

    return wbuffer;
}

wchar_t** wsplitlines_exp (wchar_t* wbuffer, int* linecount)
{
    if (!wbuffer) return NULL;

    long int nmem = 1;
    wchar_t** lines = calloc (nmem, sizeof (wchar_t*));
    const char exp = 2;
    
    int numln = 0;
    wchar_t wch = NULL;
    wchar_t* wbufpos = wbuffer;
    char newline = 1;
    do
    {
        wch = *wbufpos; 
        if (wch == L'\n')
        {
            newline = 1;
            *wbufpos = L'\0';
        }
        else
        {
            if (newline)
            {
                newline = 0;
                numln++;
                if (numln > nmem)
                {
                    nmem *= exp;
                    lines = realloc (lines, nmem * sizeof (wchar_t*));
                }
                lines[numln - 1] = wbufpos; 
            }
        }
        wbufpos++;
    } while (wch != L'\0');
    numln--;

    lines = realloc (lines, numln * sizeof (wchar_t*));

    if (linecount) *linecount = numln;
    return lines;
}

wchar_t** wsplitlines (wchar_t* wbuffer, int* linecount)
{
    if (!wbuffer) return NULL;
    
    int nlcount = 1;
    wchar_t* wbufpos = wbuffer;
    while (*wbufpos != L'\0')
    {
        if (*wbufpos == L'\n') nlcount++;
        wbufpos++;
    }
    nlcount--;
    
    wchar_t* __wstate = NULL;
    wchar_t** lines = calloc (nlcount, sizeof (wchar_t*));
    lines[0] = wcstok (wbuffer, L"\n", &__wstate); 

    for (int i = 1; i < nlcount; i++)
    {
        lines[i] = wcstok (NULL, L"\n", &__wstate);
    }

    if (linecount) *linecount = nlcount;
    return lines;
}

int alpha_strcmp_r (const void* vstr1, const void* vstr2, void* vdir)
{
    const wchar_t* str1 = *(const wchar_t**)vstr1;
    const wchar_t* str2 = *(const wchar_t**)vstr2;
    char dir = *(char*)vdir;

    int slen1 = wcslen (str1);
    int slen2 = wcslen (str2);
    
    int i1    = ( dir == DFROMBGN ? 0     : slen1 - 1 );
    int i2    = ( dir == DFROMBGN ? 0     : slen2 - 1 );
    int i1e   = ( dir == DFROMBGN ? slen1 : -1        ); 
    int i2e   = ( dir == DFROMBGN ? slen2 : -1        );
    int step  = dir;

    for (i1, i2; (i1 != i1e) && (i2 != i2e); )
    {
        if (!iswalpha (str1[i1]))
        {
            i1 += step;
            continue;
        }
        else if (!iswalpha (str2[i2]))
        {
            i2 += step;
            continue;
        }
        else
        {
            if      (towlower(str1[i1]) > towlower(str2[i2])) return 1;
            else if (towlower(str1[i1]) < towlower(str2[i2])) return -1;
            else    
                { i1 += step; i2 += step; }
        }
    }
    return 0;
}

