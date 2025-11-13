@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_kRing() {
  assert_true(\UberH3::kRing(603537747495354367, -1) === false);

  assert_array_int_eq3(\UberH3::kRing(603537747495354367, 0), [603537747495354367]);
  assert_array_int_eq3(\UberH3::kRing(603537747495354367, 1), [
    603537747495354367,
    603537753803587583,
    603537753266716671,
    603537746958483455,
    603537746690047999,
    603537747226918911,
    603537751387668479
  ]);
}

function test_compact() {
  assert_array_int_eq3(\UberH3::compact([]), []);

  assert_array_int_eq3(\UberH3::compact([614053082818412543]), [614053082818412543]);
  assert_array_int_eq3(\UberH3::compact([614053082818412543, 614053082818412543]),
                       [614053082818412543, 614053082818412543]);

  assert_array_int_eq3(\UberH3::compact([
    618556682443948031,
    618556682444210175,
    618556682444472319,
    618556682444734463,
    618556682444996607,
    618556682445258751,
    618556682445520895
  ]), [614053082818412543]);
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

test_kRing();
test_compact();
test_polyfill();
