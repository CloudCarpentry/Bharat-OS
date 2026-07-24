#include <bharat/kernel/ds/bh_cpumask.h>
#include <assert.h>
#include <stdio.h>

void test_cpumask_basic() {
    bh_cpumask_t mask;
    bh_cpumask_zero(&mask);
    assert(bh_cpumask_empty(&mask));
    assert(bh_cpumask_weight(&mask) == 0);

    bh_cpumask_set(&mask, 0);
    bh_cpumask_set(&mask, 10);
    bh_cpumask_set(&mask, 63);

    assert(!bh_cpumask_empty(&mask));
    assert(bh_cpumask_weight(&mask) == 3);
    assert(bh_cpumask_test(&mask, 0));
    assert(bh_cpumask_test(&mask, 10));
    assert(bh_cpumask_test(&mask, 63));
    assert(!bh_cpumask_test(&mask, 1));
    assert(!bh_cpumask_test(&mask, 62));

    bh_cpumask_clear(&mask, 10);
    assert(bh_cpumask_weight(&mask) == 2);
    assert(!bh_cpumask_test(&mask, 10));

    bh_cpumask_fill(&mask);
    assert(bh_cpumask_weight(&mask) == BHARAT_MAX_CPUS);
    assert(bh_cpumask_test(&mask, 0));
    assert(bh_cpumask_test(&mask, BHARAT_MAX_CPUS - 1));

    printf("test_cpumask_basic passed\n");
}

void test_cpumask_ops() {
    bh_cpumask_t a, b, res;
    bh_cpumask_zero(&a);
    bh_cpumask_zero(&b);

    bh_cpumask_set(&a, 1);
    bh_cpumask_set(&a, 2);
    bh_cpumask_set(&b, 2);
    bh_cpumask_set(&b, 3);

    bh_cpumask_and(&res, &a, &b);
    assert(bh_cpumask_weight(&res) == 1);
    assert(bh_cpumask_test(&res, 2));

    bh_cpumask_or(&res, &a, &b);
    assert(bh_cpumask_weight(&res) == 3);
    assert(bh_cpumask_test(&res, 1));
    assert(bh_cpumask_test(&res, 2));
    assert(bh_cpumask_test(&res, 3));

    bh_cpumask_xor(&res, &a, &b);
    assert(bh_cpumask_weight(&res) == 2);
    assert(bh_cpumask_test(&res, 1));
    assert(bh_cpumask_test(&res, 3));

    bh_cpumask_andnot(&res, &a, &b);
    assert(bh_cpumask_weight(&res) == 1);
    assert(bh_cpumask_test(&res, 1));

    assert(bh_cpumask_intersects(&a, &b));
    bh_cpumask_clear(&a, 2);
    assert(!bh_cpumask_intersects(&a, &b));

    printf("test_cpumask_ops passed\n");
}

void test_cpumask_iteration() {
    bh_cpumask_t mask;
    bh_cpumask_zero(&mask);
    bh_cpumask_set(&mask, 5);
    bh_cpumask_set(&mask, 15);
    bh_cpumask_set(&mask, 40);

    assert(bh_cpumask_first(&mask) == 5);
    assert(bh_cpumask_next(&mask, 5) == 15);
    assert(bh_cpumask_next(&mask, 15) == 40);
    assert(bh_cpumask_next(&mask, 40) == -1);

    printf("test_cpumask_iteration passed\n");
}

void test_cpumask_checked() {
    bh_cpumask_t mask;
    bh_cpumask_zero(&mask);

    assert(bh_cpumask_set_checked(&mask, 0) == K_OK);
    assert(bh_cpumask_set_checked(&mask, BHARAT_MAX_CPUS) == K_ERR_INVALID_ARG);

    assert(bh_cpumask_clear_checked(&mask, 0) == K_OK);
    assert(bh_cpumask_clear_checked(&mask, BHARAT_MAX_CPUS) == K_ERR_INVALID_ARG);

    bool val;
    assert(bh_cpumask_test_checked(&mask, 0, &val) == K_OK);
    assert(val == false);
    assert(bh_cpumask_test_checked(&mask, BHARAT_MAX_CPUS, &val) == K_ERR_INVALID_ARG);

    printf("test_cpumask_checked passed\n");
}

int main() {
    test_cpumask_basic();
    test_cpumask_ops();
    test_cpumask_iteration();
    test_cpumask_checked();
    return 0;
}
