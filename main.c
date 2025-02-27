#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys.h"

#define NUM_CMDS                        8
#define MAX_ENTRIES                     0x20000
#define FIELDBUFFER_SIZE                0x4000
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

    char namedata[64];
    char versiondata[64];
    char archdata[24];
    struct vstring vstring;
    unsigned int size;
    unsigned int isize;
    unsigned int count;
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

static unsigned int snippet_match(struct snippet *snippet, struct snippet *snippet2)
{

    return (snippet->length == snippet2->length) && !memcmp(snippet->data, snippet2->data, snippet->length);

}

static void vstring_init(struct vstring *vstring, char *data, unsigned int length, struct substring *name, struct substring *arch, struct substring *relation, struct substring *version)
{

    snippet_init(&vstring->name, data + name->offset, (name->end > name->offset) ? name->end - name->offset + 1 : 0);
    snippet_init(&vstring->arch, data + arch->offset, (arch->end > arch->offset) ? arch->end - arch->offset + 1 : 0);
    snippet_init(&vstring->relation, data + relation->offset, (relation->end > relation->offset) ? relation->end - relation->offset + 1 : 0);
    snippet_init(&vstring->version, data + version->offset, (version->end > version->offset) ? version->end - version->offset + 1 : 0);

}

static unsigned int append(char *s1, char *s2, unsigned int length, unsigned int offset)
{

    memcpy(s1 + offset, s2, length);

    return offset + length;

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

static unsigned int eachnewline(char *data, unsigned int length, unsigned int offset)
{

    unsigned int i;

    for (i = offset; i < length; i++)
    {

        if (data[i] == '\n' || data[i] == '\0')
            return i + 1 - offset;

    }

    return 0;

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

static unsigned int readfield(struct entry *entry, char *data, unsigned int count, char *field)
{

    unsigned int fd = sys_open(entry->filename);
    unsigned int offset = 0;

    if (fd)
    {

        char buffer[FIELDBUFFER_SIZE];
        unsigned int count;

        sys_seek(fd, entry->offset);

        count = sys_read(fd, buffer, entry->count);

        if (count)
        {

            unsigned int length = strlen(field);
            unsigned int length2;
            unsigned int offset2;

            for (offset2 = 0; (length2 = eachnewline(buffer, count, offset2)); offset2 += length2)
            {

                if (length2 == 1 && buffer[offset2] == '\n')
                    break;

                if (length < length2 && buffer[offset2 + length] == ':' && !memcmp(buffer + offset2, field, length))
                {

                    offset = append(data, buffer + offset2 + length + 1, length2 - length - 1, offset);

                    break;

                }

            }

        }

        sys_close(fd);

    }

    return offset;

}

static void dprintvstring(unsigned int fd, char *fmt, struct vstring *vstring)
{

    unsigned int length = strlen(fmt);
    unsigned int offset = 0;
    char result[4096];
    unsigned int i;

    for (i = 0; i < length; i++)
    {

        if (fmt[i] == '%')
        {

            i++;

            switch (fmt[i])
            {

            case 'n':
                offset = append(result, vstring->name.data, vstring->name.length, offset);

                break;

            case 'a':
                offset = append(result, vstring->arch.data, vstring->arch.length, offset);

                break;

            case 'r':
                offset = append(result, vstring->relation.data, vstring->relation.length, offset);

                break;

            case 'v':
                offset = append(result, vstring->version.data, vstring->version.length, offset);

                break;

            case 'A':
                offset = append(result, vstring->name.data, vstring->name.length, offset);

                if (vstring->arch.length)
                {

                    offset = append(result, ":", 1, offset);
                    offset = append(result, vstring->arch.data, vstring->arch.length, offset);

                }

                if (vstring->relation.length && vstring->version.length)
                {

                    offset = append(result, " (", 2, offset);
                    offset = append(result, vstring->relation.data, vstring->relation.length, offset);
                    offset = append(result, " ", 1, offset);
                    offset = append(result, vstring->version.data, vstring->version.length, offset);
                    offset = append(result, ")", 1, offset);

                }

                break;

            }

        }

        else
        {

            offset = append(result, fmt + i, 1, offset);

        }

    }

    offset = append(result, "\0", 1, offset);

    dprintf(fd, "%s", result);

}

static void dprintcsv(unsigned int fd, char *data, unsigned int count)
{

    unsigned int offset;
    unsigned int length;

    for (offset = 0; (length = eachcomma(data, count, offset)); offset += length)
    {

        struct vstring vstring;

        if (parsevstring(&vstring, data + offset, length))
            dprintvstring(fd, "%A\n", &vstring);

    }

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

    for (i = 0; i < length; i++)
    {

        unsigned int p1 = tolexical(i < length1 ? data1[offset1 + i] : 0, letters);
        unsigned int p2 = tolexical(i < length2 ? data2[offset2 + i] : 0, letters);

        if (p1 != p2)
            return p1 - p2;

    }

    return 0;

}

static int comparenumerical(char *data1, unsigned int offset1, unsigned int length1, char *data2, unsigned int offset2, unsigned int length2)
{

    unsigned int n1 = tonumerical(data1, length1, 10, offset1);
    unsigned int n2 = tonumerical(data2, length2, 10, offset2);

    return n1 - n2;

}

static unsigned int checkrelation(unsigned int relation, int c)
{

    switch (relation)
    {

    case RELATION_EQ:
        if (c < 0)
            return COMPARE_INVALID;

        if (c > 0)
            return COMPARE_INVALID;

        break;

    case RELATION_LT:
    case RELATION_LTEQ:
        if (c < 0)
            return COMPARE_VALID;

        if (c > 0)
            return COMPARE_INVALID;

        break;

    case RELATION_GT:
    case RELATION_GTEQ:
        if (c < 0)
            return COMPARE_INVALID;

        if (c > 0)
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

    if (v != COMPARE_CONTINUE)
        return v;

    offset1 += colon1;
    offset2 += colon2;

    do
    {

        lex1 = readlexical(version1, dash1, offset1, LETTERS_UPSTREAM);
        lex2 = readlexical(version2, dash2, offset2, LETTERS_UPSTREAM);

        v = checkrelation(relation, comparelexical(version1, offset1, lex1, version2, offset2, lex2, LETTERS_UPSTREAM));

        if (v != COMPARE_CONTINUE)
            return v;

        offset1 += lex1;
        offset2 += lex2;
        num1 = readnumerical(version1, dash1, offset1);
        num2 = readnumerical(version2, dash2, offset2);

        v = checkrelation(relation, comparenumerical(version1, offset1, num1, version2, offset2, num2));

        if (v != COMPARE_CONTINUE)
            return v;

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

        if (v != COMPARE_CONTINUE)
            return v;

        offset1 += lex1;
        offset2 += lex2;
        num1 = readnumerical(version1, length1, offset1);
        num2 = readnumerical(version2, length2, offset2);
        v = checkrelation(relation, comparenumerical(version1, offset1, num1, version2, offset2, num2));

        if (v != COMPARE_CONTINUE)
            return v;

        offset1 += num1;
        offset2 += num2;

    } while (lex1 || lex2 || num1 || num2);

    switch (relation)
    {

    case RELATION_EQ:
    case RELATION_LTEQ:
    case RELATION_GTEQ:
        return COMPARE_VALID;

    }

    return COMPARE_INVALID;

}

static struct entry *findentry(struct vstring *vstring, struct entry *entries, unsigned int nentries)
{

    unsigned int relation = getrelation(vstring->relation.data, vstring->relation.length);
    unsigned int i;

    for (i = 0; i < nentries; i++)
    {

        struct entry *current = &entries[i];

        if (snippet_match(&vstring->name, &current->vstring.name))
        {

            if (compareversions(relation, current->vstring.version.data, current->vstring.version.length, vstring->version.data, vstring->version.length) == COMPARE_VALID)
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

                    if (compareversions(relation, provided.version.data, provided.version.length, vstring->version.data, vstring->version.length) == COMPARE_VALID)
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

                dprintf(SYS_FD_STDERR, "WARNING: found no match for [");

                for (offset2 = 0; (length2 = eachpipe(data, length, offset2)); offset2 += length2)
                {

                    struct vstring vstring;

                    if (parsevstring(&vstring, data + offset2, length2))
                        dprintvstring(SYS_FD_STDERR, offset2 ? " | %A" : "%A", &vstring);

                }

                dprintf(SYS_FD_STDERR, "]\n");

            }

            else
            {

                struct entry *child = findmatchincludeprovides(data, length, entries, nentries);

                if (child)
                    nmatched = addmatched(child, matched, maxmatched, nmatched);
                else
                    dprintf(SYS_FD_STDERR, "WARNING: found no match for %.*s\n", length, data);

            }

        }

    }

    return nmatched;

}

static void entry_init(struct entry *current, char *filename, unsigned int offset)
{

    snippet_init(&current->vstring.name, current->namedata, 0);
    snippet_init(&current->vstring.version, current->versiondata, 0);
    snippet_init(&current->vstring.relation, "=", 1);
    snippet_init(&current->vstring.arch, current->archdata, 0);

    current->filename = filename;
    current->offset = offset;

}

static unsigned int parsefile(char *filename, struct entry *entries, unsigned int maxentries)
{

    unsigned int fd = sys_open(filename);

    if (fd)
    {

        struct entry *current = &entries[0];
        unsigned int offset = 0;
        unsigned int nentries = 0;
        unsigned int count;
        char buffer[FIELDBUFFER_SIZE];

        entry_init(current, filename, 0);

        while ((count = sys_read(fd, buffer, FIELDBUFFER_SIZE)))
        {

            unsigned int length2;
            unsigned int offset2;

            for (offset2 = 0; (length2 = eachnewline(buffer, count, offset2)); offset2 += length2)
            {

                if (length2 == 1 && buffer[offset2] == '\n')
                {

                    if (nentries < maxentries)
                    {

                        current->count = offset2;

                        current++;
                        nentries++;

                        offset += offset2 + length2;

                        entry_init(current, filename, offset);
                        sys_seek(fd, offset);

                        break;

                    }

                    else
                    {

                        dprintf(SYS_FD_STDERR, "WARNING: max number of entries reached (%u)\n", nentries);

                        return nentries;

                    }

                }

                else if (!memcmp(buffer + offset2, "Package: ", 9))
                {

                    snippet_init(&current->vstring.name, current->namedata, append(current->namedata, buffer + offset2 + 9, length2 - 10, 0));

                }

                else if (!memcmp(buffer + offset2, "Version: ", 9))
                {

                    snippet_init(&current->vstring.version, current->versiondata, append(current->versiondata, buffer + offset2 + 9, length2 - 10, 0));

                }

                else if (!memcmp(buffer + offset2, "Architecture: ", 14))
                {

                    snippet_init(&current->vstring.arch, current->archdata, append(current->archdata, buffer + offset2 + 14, length2 - 15, 0));

                }

                else if (!memcmp(buffer + offset2, "Size: ", 6))
                {

                    current->size = tonumerical(buffer + offset2, length2 - 7, 10, 6);

                }

                else if (!memcmp(buffer + offset2, "Installed-Size: ", 16))
                {

                    current->isize = tonumerical(buffer + offset2, length2 - 17, 10, 16);

                }

            }

        }

        sys_close(fd);

        return nentries;

    }

    return 0;

}

static unsigned int parsefiles(int argc, char **argv, struct entry *entries, unsigned int maxentries)
{

    unsigned int nentries = 0;
    unsigned int i;

    for (i = argc - 1; i < argc; i++)
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

            dprintf(SYS_FD_STDOUT, "%s %s %s [%s]\n", argv[0], argv[1], argv[2], valid == COMPARE_VALID ? "OK" : "NOT OK");

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: Unknown comparison operator %s\n", argv[1]);

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "compare <v1> <op> <v2>\n\n");
        dprintf(SYS_FD_STDOUT, "Compare the two debian version strings <v1> and <v2> using the comparison operator <op>\n");
        dprintf(SYS_FD_STDOUT, "  v1: [epoch:]upstream-version[-debian-revision]\n");
        dprintf(SYS_FD_STDOUT, "  v2: [epoch:]upstream-version[-debian-revision]\n");
        dprintf(SYS_FD_STDOUT, "  op: One of =, <<, >>, <=, >=\n");

    }

    return EXIT_SUCCESS;

}

static int command_depends(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

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

                    dprintcsv(SYS_FD_STDOUT, fieldbuffer, count);

                }

                else
                {

                    dprintf(SYS_FD_STDERR, "ERROR: No entry with the name '%.*s' was found\n", length, argv[0] + offset);

                    return EXIT_FAILURE;

                }

            }

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "depends <package-expression> <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "Show dependencies of packages that matches the package expression\n");

    }

    return EXIT_SUCCESS;

}

static int command_list(int argc, char **argv)
{

    if (argc >= 1)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int i;

            for (i = 0; i < nentries; i++)
            {

                struct entry *current = &entries[i];

                dprintvstring(SYS_FD_STDOUT, "%A\n", &current->vstring);

            }

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "list <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "List all packages\n");

    }

    return EXIT_SUCCESS;

}

static int command_raw(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int offset;
            unsigned int length;

            for (offset = 0; (length = eachcomma(argv[0], strlen(argv[0]) + 1, offset)); offset += length)
            {

                struct entry *entry = findmatch(argv[0] + offset, length, entries, nentries);

                if (entry)
                {

                    unsigned int fd = sys_open(entry->filename);

                    if (fd)
                    {

                        char buffer[FIELDBUFFER_SIZE];
                        unsigned int count;

                        sys_seek(fd, entry->offset);

                        count = sys_read(fd, buffer, entry->count);

                        if (count)
                        {

                            unsigned int length2;
                            unsigned int offset2;

                            for (offset2 = 0; (length2 = eachnewline(buffer, count, offset2)); offset2 += length2)
                            {

                                if (length2 == 1 && buffer[offset2] == '\n')
                                    break;

                                dprintf(SYS_FD_STDOUT, "%.*s", length2, buffer + offset2);

                            }

                        }

                        sys_close(fd);

                    }

                }

                else
                {

                    dprintf(SYS_FD_STDERR, "ERROR: No entry with the name '%.*s' was found\n", length, argv[0] + offset);

                    return EXIT_FAILURE;

                }

            }

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "raw <package-expression> <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "Show raw data of packages that matches the package expression\n");

    }

    return EXIT_SUCCESS;

}

static int command_rdepends(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

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

                                if (snippet_match(&dependency.name, &entry->vstring.name))
                                {

                                    unsigned int relation = getrelation(dependency.relation.data, dependency.relation.length);

                                    if (compareversions(relation, entry->vstring.version.data, entry->vstring.version.length, dependency.version.data, dependency.version.length) == COMPARE_VALID)
                                        dprintvstring(SYS_FD_STDOUT, "%A\n", &current->vstring);

                                }

                            }

                        }

                    }

                }

                else
                {

                    dprintf(SYS_FD_STDERR, "ERROR: No entry with the name '%.*s' was found\n", length, argv[0] + offset);

                    return EXIT_FAILURE;

                }

            }

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "rdepends <package-expression> <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "Show packages having dependencies that matches the package expression\n");


    }

    return EXIT_SUCCESS;

}

static int command_resolve(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

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

                    dprintf(SYS_FD_STDERR, "ERROR: No entry with the name '%.*s' was found\n", length, argv[0] + offset);

                    return EXIT_FAILURE;

                }

            }

            for (i = nmatched; i > 0; i--)
                dprintvstring(SYS_FD_STDOUT, "%A\n", &matched[i - 1]->vstring);

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "resolve <package-expression> <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "Recursively resolve all dependencies of packages that matches the package expression\n");

    }

    return EXIT_SUCCESS;

}

static int command_show(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

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

                        dprintf(SYS_FD_STDOUT, "# %s:\n", fields[i]);
                        dprintcsv(SYS_FD_STDOUT, fieldbuffer, count);

                    }


                }

            }

            else
            {

                dprintf(SYS_FD_STDERR, "ERROR: No entry with the name '%s' was found\n", argv[0]);

                return EXIT_FAILURE;

            }

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "show <package> <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "Show information about a package\n");

    }

    return EXIT_SUCCESS;

}

static int command_size(int argc, char **argv)
{

    if (argc >= 2)
    {

        unsigned int nentries = parsefiles(argc, argv, entries, MAX_ENTRIES);

        if (nentries)
        {

            unsigned int size = 0;
            unsigned int isize = 0;
            unsigned int offset;
            unsigned int length;

            for (offset = 0; (length = eachcomma(argv[0], strlen(argv[0]) + 1, offset)); offset += length)
            {

                struct entry *entry = findmatch(argv[0] + offset, length, entries, nentries);

                if (entry)
                {

                    size += entry->size;
                    isize += entry->isize;

                }

                else
                {

                    dprintf(SYS_FD_STDERR, "ERROR: No entry with the name '%.*s' was found\n", length, argv[0] + offset);

                    return EXIT_FAILURE;

                }

            }

            dprintf(SYS_FD_STDOUT, "Size: %u\n", size);
            dprintf(SYS_FD_STDOUT, "Installed-Size: %u\n", isize);

        }

        else
        {

            dprintf(SYS_FD_STDERR, "ERROR: No entries found in package file(s)\n");

            return EXIT_FAILURE;

        }

    }

    else
    {

        dprintf(SYS_FD_STDOUT, "size <package-expression> <index-file>...\n\n");
        dprintf(SYS_FD_STDOUT, "Show the total size of packages that matches the package expression\n");

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
        {"show", command_show},
        {"size", command_size}
    };

    if (argc < 2)
    {

        dprintf(SYS_FD_STDOUT, "aptinfo <command> [<args>]\n\n");
        dprintf(SYS_FD_STDOUT, "commands:\n");

        for (i = 0; i < NUM_CMDS; i++)
        {

            struct command *command = &commands[i];

            dprintf(SYS_FD_STDOUT, "  %s\n", command->name);

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

        dprintf(SYS_FD_STDERR, "ERROR: Unknown command %s\n", argv[1]);

        return EXIT_FAILURE;

    }

    return EXIT_SUCCESS;

}

