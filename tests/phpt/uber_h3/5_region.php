@ok k2_skip
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

function test_polyfill() {
    assert_array_int_eq3(\UberH3::polyfill([], [], 1), []);
    assert_array_int_eq3(\UberH3::polyfill([], [], 5), []);

    assert_array_int_eq3(
        \UberH3::polyfill([tuple(0.4, 0.5), tuple(0.1, 0.2), tuple(0.2, 0.4)], [], 5),
        [601042442497556479, 601042410285301759]
    );

    assert_array_int_eq3(
        \UberH3::polyfill([tuple(0.4, 0.5), tuple(0.1, 0.2), tuple(0.2, 0.4)],
        [[tuple(0.4, 0.5), tuple(0.1, 0.2), tuple(0.2, 0.4)]], 5),
        []
    );

    assert_array_int_eq3(
        \UberH3::polyfill([tuple(0.0, 0.0), tuple(15.0, 0.0), tuple(0.0, 15.0), tuple(15.0, 15.0)],
            [[tuple(0.0, 0.0), tuple(1.0, 0.0), tuple(0.0, 1.0), tuple(1.0, 1.0)]], 1),
        [583264530256101375, 582521260395724799, 582525658442235903, 582530056488747007, 583031433791012863]
    );

    assert_array_int_eq3(
        \UberH3::polyfill([tuple(0.0, 0.0), tuple(15.0, 0.0), tuple(0.0, 15.0), tuple(15.0, 15.0)],
            [
                 [tuple(0.0, 0.0), tuple(1.0, 0.0), tuple(0.0, 1.0), tuple(1.0, 1.0)],
                 [tuple(5.0, 5.0), tuple(10.0, 5.0), tuple(5.0, 10.0), tuple(10.0, 10.0)],
            ], 1),
        [583264530256101375, 582521260395724799, 582525658442235903, 582530056488747007, 583031433791012863]
    );
}

test_maxPolyfillSize();
test_polyfill();
