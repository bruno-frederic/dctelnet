/* test/test_dsr.c -- ANSI Device Status Report requests. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dsr.h"

#define FIND(s, str, k) Dsr_Find(s, (const UBYTE *)(str), sizeof(str) - 1, k)

/* Absinthe and 20 For Beers ask where the cursor is and wait: DCTelnet did
 * not answer, the BBS stalled. */
static void test_a_position_request_is_found_after_the_text_before_it(void) {
    struct DsrScan s;
    int kind;

    Dsr_Init(&s);
    assert(FIND(&s, "hello\x1b[6nworld", &kind) == 9 && kind == DSR_POSITION);
    Dsr_Init(&s);
    assert(FIND(&s, "\x9b" "5n", &kind) == 3 && kind == DSR_STATUS);   /* 8-bit CSI */
}

static void test_a_request_split_between_reads_is_found(void) {
    struct DsrScan s;
    int kind;

    Dsr_Init(&s);
    assert(FIND(&s, "text\x1b", &kind) == 5 && kind == DSR_NONE);
    assert(FIND(&s, "[6", &kind) == 2 && kind == DSR_NONE);
    assert(FIND(&s, "nmore", &kind) == 1 && kind == DSR_POSITION);
}

static void test_other_sequences_are_not_requests(void) {
    struct DsrScan s;
    int kind;
    static const char seqs[] = "\x1b[6m\x1b[2J\x1b[?6n\x1b[1;6n\x1b[16n";

    Dsr_Init(&s);
    assert(Dsr_Find(&s, (const UBYTE *)seqs, sizeof(seqs) - 1, &kind) == sizeof(seqs) - 1 && kind == DSR_NONE);
}

static void test_the_answers(void) {
    char out[16];

    static const char pos[] = "\x1b[12;40R", ok[] = "\x1b[0n";

    assert(Dsr_Answer(DSR_POSITION, 12, 40, out, sizeof(out)) == sizeof(pos) - 1 && memcmp(out, pos, sizeof(pos) - 1) == 0);
    assert(Dsr_Answer(DSR_STATUS, 0, 0, out, sizeof(out)) == sizeof(ok) - 1 && memcmp(out, ok, sizeof(ok) - 1) == 0);
}

/* A parameter past 249 saturates: CSI 2566 n wrapped round to 6 (a
 * position request) in the byte it is kept in. */
static void test_a_huge_parameter_is_no_request(void) {
    struct DsrScan s;
    int kind;
    const UBYTE a[] = "\x1b[2566n", b[] = "\x1b[2565n", c[] = "\x1b[256n";

    Dsr_Init(&s);
    assert(Dsr_Find(&s, a, sizeof(a) - 1, &kind) == sizeof(a) - 1 && kind == DSR_NONE);
    assert(Dsr_Find(&s, b, sizeof(b) - 1, &kind) == sizeof(b) - 1 && kind == DSR_NONE);
    assert(Dsr_Find(&s, c, sizeof(c) - 1, &kind) == sizeof(c) - 1 && kind == DSR_NONE);
}

int main(void) {
    test_a_position_request_is_found_after_the_text_before_it();
    test_a_request_split_between_reads_is_found();
    test_other_sequences_are_not_requests();
    test_the_answers();
    test_a_huge_parameter_is_no_request();
    printf("dsr: all assertions passed\n");
    return 0;
}
