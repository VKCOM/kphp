@ok
<?php

require_once 'kphp_tester_include.php';

function test_maxPolyfillSize() {
    assert_int_eq3(\UberH3::maxPolyfillSize([], [], 1), 13);
    assert_int_eq3(\UberH3::maxPolyfillSize([], [], 5), 13);

    assert_int_eq3(\UberH3::maxPolyfillSize([tuple(0.4, 0.5), tuple(0.1, 0.2), tuple(0.2, 0.4)], [], 5), 35);
    assert_int_eq3(\UberH3::maxPolyfillSize([tuple(0.1, 0.1), tuple(0.1, 0.2), tuple(0.1, 0.0)], [], 1), 15);

    assert_int_eq3(\UberH3::maxPolyfillSize(
        [tuple(0.0, 0.0), tuple(15.0, 0.0), tuple(0.0, 15.0), tuple(15.0, 15.0)],
        [], 5), 55269);
    assert_int_eq3(\UberH3::maxPolyfillSize(
        [tuple(0.0, 0.0), tuple(15.0, 0.0), tuple(0.0, 15.0), tuple(15.0, 15.0)],
        [], 1), 34);

    assert_int_eq3(\UberH3::maxPolyfillSize(
        [tuple(0.0, 0.0), tuple(15.0, 0.0), tuple(0.0, 15.0), tuple(15.0, 15.0)],
        [
             [tuple(0.0, 0.0), tuple(1.0, 0.0), tuple(0.0, 1.0), tuple(1.0, 1.0)],
             [tuple(5.0, 5.0), tuple(10.0, 5.0), tuple(5.0, 10.0), tuple(10.0, 10.0)],
        ], 5), 55269);
    assert_int_eq3(\UberH3::maxPolyfillSize(
        [tuple(0.0, 0.0), tuple(15.0, 0.0), tuple(0.0, 15.0), tuple(15.0, 15.0)],
        [
             [tuple(0.0, 0.0), tuple(1.0, 0.0), tuple(0.0, 1.0), tuple(1.0, 1.0)],
             [tuple(5.0, 5.0), tuple(10.0, 5.0), tuple(5.0, 10.0), tuple(10.0, 10.0)],
        ], 1), 34);
}

test_maxPolyfillSize();
