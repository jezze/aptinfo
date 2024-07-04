#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CMDS                        7
#define MAX_ENTRIES                     0x20000
#define FIELDBUFFER_SIZE                0x1000
#define LETTERS_UPSTREAM                "~.+-:"
#define LETTERS_REVISION                 "~.+"

enum state
{

    STATE_BEGIN = 1,
    STATE_NAME = 2,
    STATE_ARCH = 3,
    STATE_RELATION = 4,
    STATE_VERSION = 5,
    STATE_END = 6

};

enum relation
{

    RELATION_NONE = 1,
    RELATION_EQ = 2,
    RELATION_GT = 3,
    RELATION_GTEQ = 4,
    RELATION_LT = 5,
    RELATION_LTEQ = 6

};

enum compare
{

    COMPARE_CONTINUE = 0,
    COMPARE_VALID = 1,
    COMPARE_INVALID = 2

};

struct snippet
{

    char *data;
    unsigned int length;

};

struct substring
{

    unsigned int offset;
    unsigned int end;

};

struct vstring
{

    char *data;
    unsigned int length;
    struct snippet name;
    struct snippet arch;
    struct snippet relation;
    struct snippet version;

};

struct command
{

    char *name;
    int (*handle)(int argc, char **argv);

};

struct entry
{

    char name[64];
    char version[64];
    char arch[24];
    unsigned int size;
    char *filename;
    unsigned int offset;
    unsigned int matched;

};

static void substring_init(struct substring *substring)
{

    substring->offset = 0;
    substring->end = 0;

}

static void snippet_init(struct snippet *snippet, char *data, unsigned int length)
{

    snippet->data = data;
    snippet->length = length;

}

static void vstring_init(struct vstring *vstring, char *data, unsigned int length, struct substring *name, struct substring *arch, struct substring *relation, struct substring *version)
{

    vstring->data = data;
    vstring->length = length;

    snippet_init(&vstring->name, data + name->offset, (name->end > name->offset) ? name->end - name->offset + 1 : 0);
    snippet_init(&vstring->arch, data + arch->offset, (arch->end > arch->offset) ? arch->end - arch->offset + 1 : 0);
    snippet_init(&vstring->relation, data + relation->offset, (relation->end > relation->offset) ? relation->end - relation->offset + 1 : 0);
    snippet_init(&vstring->version, data + version->offset, (version->end > version->offset) ? version->end - version->offset + 1 : 0);

}

static unsigned int snippet_match(struct snippet *snippet, struct snippet *snippet2)
{

    return (snippet->length == snippet2->length) && !strncmp(snippet->data, snippet2->data, snippet->length);

}

static unsigned int snippet_matchentry(struct snippet *snippet, struct entry *entry)
{

    return (snippet->length == strlen(entry->name)) && !strncmp(snippet->data, entry->name, snippet->length);

}

static unsigned int findfirst(char *version, unsigned int length, char c, unsigned int offset)
{

    unsigned int i;

    for (i = offset; i < length; i++)
    {

        if (version[i] == c)
            return i;

    }

    return 0;

}

static unsigned int findlast(char *version, unsigned int length, char c, unsigned int offset)
{

    unsigned int last = length;
    unsigned int i;

    for (i = offset; i < length; i++)
    {

        if (version[i] == c)
            last = i;

    }

    return last;

}

static unsigned int isnumerical(unsigned int v)
{

    return (v >= '0' && v <= '9');

}

static unsigned int isalphabetical(unsigned int v)
{

    return (v >= 'a' && v <= 'z') || (v >= 'A' && v <= 'Z');

}

static unsigned int isspecial(unsigned int v, char *letters)
{

    unsigned int i;

    for (i = 0; i < strlen(letters); i++)
    {

        if (letters[i] == v)
            return 1;

    }

    return 0;

}

static unsigned int tonumerical(char *input, unsigned int length, unsigned int base, unsigned int offset)
{

    unsigned int value = 0;
    unsigned int m = 1;
    unsigned int i;

    for (i = length; i > 0; i--)
    {

        int v = input[offset + i - 1] - '0';

        value += v * m;
        m *= base;

    }

    return value;

}

static unsigned int tolexical(unsigned int c, char *letters)
{

    unsigned int offset = 0;
    unsigned int i;

    if (c == '~')
        return offset;

    offset += 1;

    if (c == 0)
        return offset;

    offset += 1;

    if (c >= 'A' && c <= 'Z')
        return offset + (c - 'A');

    offset += 26;

    if (c >= 'a' && c <= 'z')
        return offset + (c - 'a');

    offset += 26;

    for (i = 0; i < strlen(letters); i++)
    {

        if (letters[i] == c)
            return offset + i;

    }

    offset += strlen(letters);

    return offset;

}

static unsigned int numoptions(char *data, unsigned int length)
{

    unsigned int count = 0;
    unsigned int i;

    for (i = 0; i < length; i++)
    {

        if (data[i] == '|')
            count++;

    }

    return count + 1;

}

static unsigned int parsevstring(struct vstring *vstring, char *data, unsigned int length)
{

    unsigned int state = STATE_BEGIN;
    struct substring name;
    struct substring arch;
    struct substring relation;
    struct substring version;
    unsigned int i;

    substring_init(&name);
    substring_init(&arch);
    substring_init(&relation);
    substring_init(&version);

    for (i = 0; i < length; i++)
    {

        int c = data[i];

        if (c == ' ')
            continue;

        switch (state)
        {

        case STATE_BEGIN:
            name.offset = i;
            state = STATE_NAME;

            break;

        case STATE_NAME:
            switch (c)
            {

            case '|':
            case ',':
            case '\n':
            case '\0':
                state = STATE_END;

                break;

            case ':':
                arch.offset = i + 1;
                state = STATE_ARCH;

                break;

            case '(':
                relation.offset = i + 1;
                state = STATE_RELATION;

                break;

            default:
                name.end = i;

                break;

            }

            break;

        case STATE_ARCH:
            switch (c)
            {

            case '|':
            case ',':
            case '\n':
            case '\0':
                state = STATE_END;

                break;

            case '(':
                relation.offset = i + 1;
                state = STATE_RELATION;

                break;

            default:
                arch.end = i;

                break;

            }

            break;

        case STATE_RELATION:
            switch (c)
            {

            case '=':
            case '<':
            case '>':
                relation.end = i;

                break;

            default:
                version.offset = i;
                state = STATE_VERSION;

                break;

            }

            break;

        case STATE_VERSION:
            switch (c)
            {

            case ')':
                state = STATE_END;

                break;

            default:
                version.end = i;

                break;

            }

            break;

        }

    }

    vstring_init(vstring, data, length, &name, &arch, &relation, &version);

    return 1;

}

static unsigned int readfield(struct entry *entry, char *data, unsigned int count, char *field)
{

    FILE *fp = fopen(entry->filename, "r");
    unsigned int length = strlen(field);
    char *line = NULL;
    size_t len = 0;
    ssize_t n;
    unsigned int ret = 0;

    if (fp)
    {

        fseek(fp, entry->offset, SEEK_SET);

        while ((n = getline(&line, &len, fp)) != -1)
        {

            if (line[0] == '\n')
                break;

            if (length < n && line[length] == ':')
            {

                if (!strncmp(line, field, length))
                {

                    strcpy(data, line + length + 1);

                    ret = n - length - 1;

                    break;

                }

            }

        }

        fclose(fp);

    }

    return ret;

}

static unsigned int eachcomma(char *data, unsigned int length, unsigned int offset)
{

    unsigned int i;

    for (i = offset; i < length; i++)
    {

        if (data[i] == ',' || data[i] == '\n' || data[i] == '\0')
            return i + 1 - offset;

    }

    return 0;

}

static unsigned int eachpipe(char *data, unsigned int length, unsigned int offset)
{

    unsigned int i;

    for (i = offset; i < length; i++)
    {

        if (data[i] == '|' || data[i] == ',' || data[i] == '\n' || data[i] == '\0')
            return i + 1 - offset;

    }

    return 0;

}

static void printentry(FILE *file, char *fmt, struct entry *entry)
{

    unsigned int length = strlen(fmt);
    unsigned int c = 0;
    unsigned int i;
    char result[4096];

    for (i = 0; i < length; i++)
    {

        if (fmt[i] == '%')
        {

            i++;

            switch (fmt[i])
            {

            case 'n':
                strncpy(result + c, entry->name, strlen(entry->name));

                c += strlen(entry->name);

                break;

            case 'a':
                strncpy(result + c, entry->arch, strlen(entry->arch));

                c += strlen(entry->arch);

                break;

            case 'v':
                strncpy(result + c, entry->version, strlen(entry->version));

                c += strlen(entry->version);

                break;

            case 'A':
                strncpy(result + c, entry->name, strlen(entry->name));

                c += strlen(entry->name);

                strcpy(result + c, ":");

                c += 1;

                strncpy(result + c, entry->arch, strlen(entry->arch));

                c += strlen(entry->arch);

                strcpy(result + c, " (= ");

                c += 4;

                strncpy(result + c, entry->version, strlen(entry->version));

                c += strlen(entry->version);

                strcpy(result + c, ")");

                c += 1;

                break;

            }

        }

        else
        {

            result[c] = fmt[i];
            c++;

        }

    }

    result[c] = '\0';

    fprintf(file, result);

}

static void printvstringextra(FILE *file, char *fmt, struct vstring *vstring)
{

    unsigned int length = strlen(fmt);
    unsigned int c = 0;
    unsigned int i;
    char result[4096];

    for (i = 0; i < length; i++)
    {

        if (fmt[i] == '%')
        {

            i++;

            switch (fmt[i])
            {

            case 'n':
                strncpy(result + c, vstring->name.data, vstring->name.length);

                c += vstring->name.length;

                break;

            case 'a':
                strncpy(result + c, vstring->arch.data, vstring->arch.length);

                c += vstring->arch.length;

                break;

            case 'r':
                strncpy(result + c, vstring->relation.data, vstring->relation.length);

                c += vstring->relation.length;

                break;

            case 'v':
                strncpy(result + c, vstring->version.data, vstring->version.length);

                c += vstring->version.length;

                break;

            case 'A':
                strncpy(result + c, vstring->name.data, vstring->name.length);

                c += vstring->name.length;

                if (vstring->arch.length)
                {

                    strcpy(result + c, ":");

                    c += 1;

                    strncpy(result + c, vstring->arch.data, vstring->arch.length);

                    c += vstring->arch.length;

                }

                if (vstring->relation.length && vstring->version.length)
                {

                    strcpy(result + c, " (");

                    c += 2;

                    strncpy(result + c, vstring->relation.data, vstring->relation.length);

                    c += vstring->relation.length;

                    strcpy(result + c, " ");

                    c += 1;

                    strncpy(result + c, vstring->version.data, vstring->version.length);

                    c += vstring->version.length;

                    strcpy(result + c, ")");

                    c += 1;

                }

                break;

            }

        }

        else
        {

            result[c] = fmt[i];
            c++;

        }

    }

    result[c] = '\0';

    fprintf(file, result);

}

static void printvstring(FILE *file, char *fmt, char *data, unsigned int length)
{

    struct vstring vstring;

    if (parsevstring(&vstring, data, length))
        printvstringextra(file, fmt, &vstring);

}

static void printcsv(FILE *file, char *data, unsigned int count)
{

    unsigned int offset;
    unsigned int length;

    for (offset = 0; (length = eachcomma(data, count, offset)); offset += length)
        printvstring(file, "%A\n", data + offset, length);

}

static unsigned int getrelation(char *relation, unsigned int length)
{

    switch (length)
    {

    case 0:
        return RELATION_NONE;

    case 1:
        if (relation[0] == '=')
            return RELATION_EQ;

        break;

    case 2:
        if (relation[0] == '<' && relation[1] == '<')
            return RELATION_LT;
        
        if (relation[0] == '<' && relation[1] == '=')
            return RELATION_LTEQ;
 
        if (relation[0] == '>' && relation[1] == '>')
            return RELATION_GT;

        if (relation[0] == '>' && relation[1] == '=')
            return RELATION_GTEQ;

        break;

    }

    return 0;

}

static unsigned int readnumerical(char *version, unsigned int length, unsigned int offset)
{

    unsigned int i;

    for (i = offset; i <= length; i++)
    {

        if (!isnumerical(version[i]))
            return i - offset;

    }

    return length - offset + 1;

}

static unsigned int readlexical(char *version, unsigned int length, unsigned int offset, char *letters)
{

    unsigned int i;

    for (i = offset; i <= length; i++)
    {

        if (!isalphabetical(version[i]) && !isspecial(version[i], letters))
            return i - offset;

    }

    return length - offset + 1;

}

static int comparelexical(char *data1, unsigned int offset1, unsigned int length1, char *data2, unsigned int offset2, unsigned int length2, char *letters)
{

    unsigned int length = (length1 > length2) ? length1 : length2;
    unsigned int i;

    //printf("comparelex: %.*s %.*s\n", length1, data1 + offset1, length2, data2 + offset2);

    for (i = 0; i < length; i++)
    {

        unsigned int p1 = tolexical(i < length1 ? data1[offset1 + i] : 0, letters);
        unsigned int p2 = tolexical(i < length2 ? data2[offset2 + i] : 0, letters);

        if (p1 < p2)
            return -1;

        if (p1 > p2)
            return 1;

    }

    return 0;

}

static int comparenumerical(char *data1, unsigned int offset1, unsigned int length1, char *data2, unsigned int offset2, unsigned int length2)
{

    unsigned int n1 = tonumerical(data1, length1, 10, offset1);
    unsigned int n2 = tonumerical(data2, length2, 10, offset2);

    //printf("comparenum: %.*s %.*s (%d, %d)\n", length1, data1 + offset1, length2, data2 + offset2, n1, n2);

    if (n1 < n2)
        return -1;

    if (n1 > n2)
        return 1;

    return 0;

}

static unsigned int checkrelation(unsigned int relation, unsigned int c)
{

    switch (relation)
    {

    case RELATION_EQ:
        if (c == -1)
            return COMPARE_INVALID;

        if (c == 1)
            return COMPARE_INVALID;

        break;

    case RELATION_LT:
    case RELATION_LTEQ:
        if (c == -1)
            return COMPARE_VALID;

        if (c == 1)
            return COMPARE_INVALID;

        break;

    case RELATION_GT:
    case RELATION_GTEQ:
        if (c == -1)
            return COMPARE_INVALID;

        if (c == 1)
            return COMPARE_VALID;

        break;

    }

    return COMPARE_CONTINUE;

}

static unsigned int compareversions(unsigned int relation, char *version1, unsigned int length1, char *version2, unsigned int length2)
{

    unsigned int offset1 = 0;
    unsigned int offset2 = 0;
    unsigned int colon1 = findfirst(version1, length1, ':', offset1);
    unsigned int colon2 = findfirst(version2, length2, ':', offset2);
    unsigned int dash1 = findlast(version1, length1, '-', offset1);
    unsigned int dash2 = findlast(version2, length2, '-', offset2);
    unsigned int lex1;
    unsigned int lex2;
    unsigned int num1;
    unsigned int num2;
    unsigned int v;

    if (relation == RELATION_NONE)
        return 1;

    v = checkrelation(RELATION_EQ, comparenumerical(version1, offset1, colon1, version2, offset2, colon2));

    if (v)
        return (v == COMPARE_VALID);

    offset1 += colon1;
    offset2 += colon2;

    do
    {

        lex1 = readlexical(version1, dash1, offset1, LETTERS_UPSTREAM);
        lex2 = readlexical(version2, dash2, offset2, LETTERS_UPSTREAM);

        v = checkrelation(relation, comparelexical(version1, offset1, lex1, version2, offset2, lex2, LETTERS_UPSTREAM));

        if (v)
            return (v == COMPARE_VALID);

        offset1 += lex1;
        offset2 += lex2;
        num1 = readnumerical(version1, dash1, offset1);
        num2 = readnumerical(version2, dash2, offset2);

        v = checkrelation(relation, comparenumerical(version1, offset1, num1, version2, offset2, num2));

        if (v)
            return (v == COMPARE_VALID);

        offset1 += num1;
        offset2 += num2;

    } while (lex1 || lex2 || num1 || num2);

    offset1 = dash1 + 1;
    offset2 = dash2 + 1;

    do
    {

        lex1 = readlexical(version1, length1, offset1, LETTERS_REVISION);
        lex2 = readlexical(version2, length2, offset2, LETTERS_REVISION);

        v = checkrelation(relation, comparelexical(version1, offset1, lex1, version2, offset2, lex2, LETTERS_REVISION));

        if (v)
            return (v == COMPARE_VALID);

        offset1 += lex1;
        offset2 += lex2;
        num1 = readnumerical(version1, length1, offset1);
        num2 = readnumerical(version2, length2, offset2);
        v = checkrelation(relation, comparenumerical(version1, offset1, num1, version2, offset2, num2));

        if (v)
            return (v == COMPARE_VALID);

        offset1 += num1;
        offset2 += num2;

    } while (lex1 || lex2 || num1 || num2);

    switch (relation)
    {

    case RELATION_EQ:
    case RELATION_LTEQ:
    case RELATION_GTEQ:
        return 1;

    }

    return 0;

}

static struct entry *findentry(struct vstring *vstring, struct entry *entries, unsigned int nentries)
{

    unsigned int relation = getrelation(vstring->relation.data, vstring->relation.length);
    unsigned int i;

    for (i = 0; i < nentries; i++)
    {

        struct entry *current = &entries[i];

        if (snippet_matchentry(&vstring->name, current))
        {

            if (compareversions(relation, current->version, strlen(current->version), vstring->version.data, vstring->version.length))
                return current;

        }

    }

    return 0;

}

static struct entry *findentryprovides(struct vstring *vstring, struct entry *entries, unsigned int nentries)
{

    unsigned int relation = getrelation(vstring->relation.data, vstring->relation.length);
    unsigned int i;

    for (i = 0; i < nentries; i++)
    {

        struct entry *current = &entries[i];
        char fieldbuffer[FIELDBUFFER_SIZE];
        unsigned int count = readfield(current, fieldbuffer, FIELDBUFFER_SIZE, "Provides");
        unsigned int offset;
        unsigned int length;

        for (offset = 0; (length = eachcomma(fieldbuffer, count, offset)); offset += length)
        {

            struct vstring provided;

            if (parsevstring(&provided, fieldbuffer + offset, length))
            {

                if (snippet_match(&vstring->name, &provided.name))
                {

                    if (compareversions(relation, provided.version.data, provided.version.length, vstring->version.data, vstring->version.length))
                        return current;

                }

            }

        }

    }

    return 0;

}

static struct entry *findmatch(char *data, unsigned int length, struct entry *entries, unsigned int nentries)
{

    struct vstring vstring;

    return (parsevstring(&vstring, data, length)) ? findentry(&vstring, entries, nentries) : 0;

}

static struct entry *findmatchincludeprovides(char *data, unsigned int length, struct entry *entries, unsigned int nentries)
{

    struct vstring vstring;

    if (parsevstring(&vstring, data, length))
    {

        struct entry *entry = findentry(&vstring, entries, nentries);

        if (!entry)
            entry = findentryprovides(&vstring, entries, nentries);

        return entry;

    }

    return 0;

}

static unsigned int addmatched(struct entry *entry, struct entry **matched, unsigned int maxmatched, unsigned int nmatched)
{

    if (nmatched < maxmatched)
    {

        if (!entry->matched)
        {

            matched[nmatched] = entry;
            nmatched++;

            entry->matched = 1;

        }

    }

    return nmatched;

}

static unsigned int resolve(struct entry *entry, struct entry *entries, unsigned int nentries, char *field, struct entry **matched, unsigned int maxmatched, unsigned int nmatched)
{

    unsigned int i;

    nmatched = addmatched(entry, matched, maxmatched, nmatched);

    for (i = 0; i < nmatched; i++)
    {

        char fieldbuffer[FIELDBUFFER_SIZE];
        unsigned int count = readfield(matched[i], fieldbuffer, FIELDBUFFER_SIZE, field);
        unsigned int offset;
        unsigned int length;

        for (offset = 0; (length = eachcomma(fieldbuffer, count, offset)); offset += length)
        {

            char *data = fieldbuffer + offset;
            unsigned int noptions = numoptions(data, length);

            if (noptions > 1)
            {

                unsigned int offset2;
                unsigned int length2;
                unsigned int found = 0;

                for (offset2 = 0; (length2 = eachpipe(data, length, offset2)); offset2 += length2)
                {

                    struct entry *child = findmatchincludeprovides(data + offset2, length2, entries, nentries);

                    if (child && child->matched)
                    {

                        found = 1;

                        break;

                    }

                }

                if (found)
                    continue;

                fprintf(stderr, "WARNING: found no match for [");

                for (offset2 = 0; (length2 = eachpipe(data, length, offset2)); offset2 += length2)
                {

                    if (offset2)
                        printvstring(stderr, " | %A", data + offset2, length2);
                    else
                        printvstring(stderr, "%A", data + offset2, length2);

                }

                fprintf(stderr, "]\n");

            }

            else
            {

                struct entry *child = findmatchincludeprovides(data, length, entries, nentries);

                if (child)
                    nmatched = addmatched(child, matched, maxmatched, nmatched);
                else
                    printvstring(stderr, "WARNING: found no match for %A\n", data, length);

            }

        }

    }

    return nmatched;

}

static unsigned int parsefile(char *filename, struct entry *entries, unsigned int maxentries)
{

    FILE *fp = fopen(filename, "r");

    if (fp)
    {

        struct entry *current = &entries[0];
        unsigned int offset = 0;
        unsigned int nentries = 0;
        char *line = NULL;
        size_t len = 0;
        ssize_t n;

        current->filename = filename;
        current->offset = 0;

        while ((n = getline(&line, &len, fp)) != -1)
        {

            if (line[0] == '\n')
            {

                if (nentries < maxentries)
                {

                    current++;
                    nentries++;

                    current->filename = filename;
                    current->offset = offset + 1;

                }

                else
                {

                    fprintf(stderr, "WARNING: max number of entries reached (%u)\n", nentries);

                    return nentries;

                }

            }

            else if (!strncmp(line, "Package: ", 9))
            {

                strncpy(current->name, line + 9, n - 10);

            }

            else if (!strncmp(line, "Version: ", 9))
            {

                strncpy(current->version, line + 9, n - 10);

            }

            else if (!strncmp(line, "Architecture: ", 14))
            {

                strncpy(current->arch, line + 14, n - 15);

            }

            else if (!strncmp(line, "Size: ", 6))
            {

                current->size = tonumerical(line, n, 10, 6);

            }

            offset += n;

        }

        fclose(fp);

        return nentries;

    }

    return 0;

}

static unsigned int parsefiles(int argc, char **argv, struct entry *entries, unsigned int maxentries)
{

    unsigned int nentries = 0;
    unsigned int i;

    for (i = 0; i < argc; i++)
        nentries += parsefile(argv[i], entries + nentries, maxentries - nentries);

    return nentries;

}

static struct entry entries[MAX_ENTRIES];
static struct entry *matched[MAX_ENTRIES];

static int command_compare(int argc, char **argv)
{

    if (argc == 3)
    {

        unsigned int relation = getrelation(argv[1], strlen(argv[1]));

        if (relation)
        {

            unsigned int valid = compareversions(relation, argv[0], strlen(argv[0]), argv[2], strlen(argv[2]));

            printf("%s %s %s [%s]\n", argv[0], argv[1], argv[2], valid ? "OK" : "NOT OK");

        }

        else
        {

            fprintf(stderr, "ERROR: Unknown comparison operator %s\n", argv[1]);

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("compare <v1> <op> <v2>\n\n");
        printf("Compare the two debian version strings <v1> and <v2> using the comparison operator <op>\n");
        printf("  v1: [epoch:]upstream-version[-debian-revision]\n");
        printf("  v2: [epoch:]upstream-version[-debian-revision]\n");
        printf("  op: One of =, <<, >>, <=, >=\n");

    }

    return EXIT_SUCCESS;

}

static int command_list(int argc, char **argv)
{

    if (argc >= 1)
    {

        unsigned int nentries = parsefiles(argc, &argv[0], entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int i;

            for (i = 0; i < nentries; i++)
            {

                struct entry *current = &entries[i];

                printentry(stdout, "%A\n", current);

            }

        }

        else
        {

            fprintf(stderr, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("list <index-file>...\n\n");
        printf("List all packages\n");

    }

    return EXIT_SUCCESS;

}

static int command_depends(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc - 1, &argv[1], entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int offset;
            unsigned int length;

            for (offset = 0; (length = eachcomma(argv[0], strlen(argv[0]) + 1, offset)); offset += length)
            {

                struct entry *entry = findmatch(argv[0] + offset, length, entries, nentries);

                if (entry)
                {

                    char fieldbuffer[FIELDBUFFER_SIZE];
                    unsigned int count = readfield(entry, fieldbuffer, FIELDBUFFER_SIZE, "Depends");

                    printcsv(stdout, fieldbuffer, count);

                }

                else
                {

                    printvstring(stderr, "ERROR: No entry with the name '%A' was found\n", argv[0] + offset, length);

                    return EXIT_FAILURE;

                }

            }

        }

        else
        {

            fprintf(stderr, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("depends <package-expression> <index-file>...\n\n");
        printf("Show dependencies of packages that matches the package expression\n");

    }

    return EXIT_SUCCESS;

}

static int command_raw(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc - 1, &argv[1], entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int offset;
            unsigned int length;

            for (offset = 0; (length = eachcomma(argv[0], strlen(argv[0]) + 1, offset)); offset += length)
            {

                struct entry *entry = findmatch(argv[0] + offset, length, entries, nentries);

                if (entry)
                {

                    FILE *fp = fopen(entry->filename, "r");

                    if (fp)
                    {

                        char *line = NULL;
                        size_t len = 0;
                        ssize_t n;

                        fseek(fp, entry->offset, SEEK_SET);

                        if (offset)
                            printf("\n");

                        while ((n = getline(&line, &len, fp)) != -1)
                        {

                            if (line[0] == '\n')
                                break;

                            printf("%s", line);

                        }

                        fclose(fp);

                    }

                }

                else
                {

                    printvstring(stderr, "ERROR: No entry with the name '%A' was found\n", argv[0] + offset, length);

                    return EXIT_FAILURE;

                }

            }

        }

        else
        {

            fprintf(stderr, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("raw <package-expression> <index-file>...\n\n");
        printf("Show raw data of packages that matches the package expression\n");

    }

    return EXIT_SUCCESS;

}

static int command_rdepends(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc - 1, &argv[1], entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int offset;
            unsigned int length;

            for (offset = 0; (length = eachcomma(argv[0], strlen(argv[0]) + 1, offset)); offset += length)
            {

                struct entry *entry = findmatch(argv[0] + offset, length, entries, nentries);

                if (entry)
                {

                    unsigned int i;

                    for (i = 0; i < nentries; i++)
                    {

                        struct entry *current = &entries[i];
                        char fieldbuffer[FIELDBUFFER_SIZE];
                        unsigned int count = readfield(current, fieldbuffer, FIELDBUFFER_SIZE, "Depends");
                        unsigned int offset;
                        unsigned int length;

                        for (offset = 0; (length = eachcomma(fieldbuffer, count, offset)); offset += length)
                        {

                            struct vstring dependency;

                            if (parsevstring(&dependency, fieldbuffer + offset, length))
                            {

                                if (snippet_matchentry(&dependency.name, entry))
                                {

                                    unsigned int relation = getrelation(dependency.relation.data, dependency.relation.length);

                                    if (compareversions(relation, entry->version, strlen(entry->version), dependency.version.data, dependency.version.length))
                                        printentry(stdout, "%A\n", current);

                                }

                            }

                        }

                    }

                }

                else
                {

                    printvstring(stderr, "ERROR: No entry with the name '%A' was found\n", argv[0] + offset, length);

                    return EXIT_FAILURE;

                }

            }

        }

        else
        {

            fprintf(stderr, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("rdepends <package-expression> <index-file>...\n\n");
        printf("Show packages having dependencies that matches the package expression\n");


    }

    return EXIT_SUCCESS;

}

static int command_resolve(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc - 1, &argv[1], entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int nmatched = 0;
            unsigned int offset;
            unsigned int length;
            unsigned int i;

            for (offset = 0; (length = eachcomma(argv[0], strlen(argv[0]) + 1, offset)); offset += length)
            {

                struct entry *entry = findmatch(argv[0] + offset, length, entries, nentries);

                if (entry)
                {

                    nmatched = resolve(entry, entries, nentries, "Depends", matched, MAX_ENTRIES, nmatched);

                }

                else
                {

                    printvstring(stderr, "ERROR: No entry with the name '%A' was found\n", argv[0] + offset, length);

                    return EXIT_FAILURE;

                }

            }

            for (i = nmatched; i > 0; i--)
                printentry(stdout, "%A\n", matched[i - 1]);

        }

        else
        {

            fprintf(stderr, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("resolve <package-expression> <index-file>...\n\n");
        printf("Recursively resolve all dependencies of packages that matches the package expression\n");

    }

    return EXIT_SUCCESS;

}

static int command_show(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc - 1, &argv[1], entries, MAX_ENTRIES);

        if (nentries)
        {

            struct entry *entry = findmatch(argv[0], strlen(argv[0]), entries, nentries);

            if (entry)
            {

                char *fields[8] = {
                    "Pre-Depends",
                    "Depends",
                    "Recommends",
                    "Suggests",
                    "Conflicts",
                    "Replaces",
                    "Breaks",
                    "Provides",
                };
                unsigned int i;

                for (i = 0; i < 8; i++)
                {

                    char fieldbuffer[FIELDBUFFER_SIZE];
                    unsigned int count = readfield(entry, fieldbuffer, FIELDBUFFER_SIZE, fields[i]);

                    if (count)
                    {

                        fprintf(stdout, "# %s:\n", fields[i]);
                        printcsv(stdout, fieldbuffer, count);

                    }


                }

            }

            else
            {

                printvstring(stderr, "ERROR: No entry with the name '%A' was found\n", argv[0], strlen(argv[0]));

                return EXIT_FAILURE;

            }

        }

        else
        {

            fprintf(stderr, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        printf("show <package> <index-file>...\n\n");
        printf("Show information about a package\n");

    }

    return EXIT_SUCCESS;

}

int main(int argc, char **argv)
{

    unsigned int i;

    static struct command commands[NUM_CMDS] = {
        {"compare", command_compare},
        {"depends", command_depends},
        {"list", command_list},
        {"raw", command_raw},
        {"rdepends", command_rdepends},
        {"resolve", command_resolve},
        {"show", command_show}
    };

    if (argc < 2)
    {

        printf("aptinfo <command> [<args>]\n\n");
        printf("commands:\n");

        for (i = 0; i < NUM_CMDS; i++)
        {

            struct command *command = &commands[i];

            printf("  %s\n", command->name);

        }

    }

    else
    {

        for (i = 0; i < NUM_CMDS; i++)
        {

            struct command *command = &commands[i];

            if (!strcmp(argv[1], command->name))
                return command->handle(argc - 2, &argv[2]);

        }

        fprintf(stderr, "ERROR: Unknown command %s\n", argv[1]);

        return EXIT_FAILURE;

    }

    return EXIT_SUCCESS;

}

