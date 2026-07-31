#include <standard/string.h>
#include <standard/assert.h>

int main(void) {
    char buf1[32];
    char buf2[32];

    /* Test memset */
    memset(buf1, 'A', 10);
    for (int i = 0; i < 10; i++) {
        assert(buf1[i] == 'A');
    }

    /* Test memcpy */
    memcpy(buf2, buf1, 10);
    for (int i = 0; i < 10; i++) {
        assert(buf2[i] == 'A');
    }

    /* Test memcmp */
    assert(memcmp(buf1, buf2, 10) == 0);
    buf2[5] = 'B';
    assert(memcmp(buf1, buf2, 10) != 0);

    /* Test memmove */
    char move_buf[16] = "abcdefgh";
    memmove(move_buf + 2, move_buf, 5); /* abcde -> offset 2, overlaps */
    assert(memcmp(move_buf, "ababcdeh", 8) == 0);

    /* Test strlen */
    assert(strlen("hello") == 5);
    assert(strlen("") == 0);

    /* Test strcmp and strncmp */
    assert(strcmp("abc", "abc") == 0);
    assert(strcmp("abc", "abd") < 0);
    assert(strncmp("abc", "abx", 2) == 0);
    assert(strncmp("abc", "abx", 3) != 0);

    return 0;
}
