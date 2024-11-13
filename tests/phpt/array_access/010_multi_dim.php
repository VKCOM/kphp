@ok
<?php
require_once 'kphp_tester_include.php';

/** @param $x mixed[]
 *  @return mixed
 */
function as_mix_obj($x) {
    return to_mixed(new Classes\LoggingLikeArray($x));
}

function test_straightforward() {
    // 3 x 3 x 3
    $x = as_mix_obj([
        [as_mix_obj([1, 2, 3]), [4, 5, 6], [7, 8, 9]],
        as_mix_obj([[111, 222, 333], [444, 555, 666], as_mix_obj(["s1", "s2", 0.42])]),
        [[888, 999, 1010], [0.5, 0.25, 0.125], ["str", 0, 0]]
    ]);
    $y = [
        [as_mix_obj([1, 2, 3]), [4, 5, 6], [7, 8, 9]],
        as_mix_obj([[111, 222, 333], [444, 555, 666], as_mix_obj(["s1", "s2", 0.42])]),
        [[888, 999, 1010], [0.5, 0.25, 0.125], ["str", 0, 0]]
    ];
    $z = new Classes\LoggingLikeArray([
        [as_mix_obj([1, 2, 3]), [4, 5, 6], [7, 8, 9]],
        as_mix_obj([[111, 222, 333], [444, 555, 666], as_mix_obj(["s1", "s2", 0.42])]),
        [[888, 999, 1010], [0.5, 0.25, 0.125], ["str", 0, 0]]
    ]);

    for ($i = 0; $i < 3; $i++) {
        for ($j = 0; $j < 3; $j++) {
            for ($k = 0; $k < 3; $k++) {
                if (($i + 3 * $j + 3 * 3 * $k) % 5 == 0) {
                    unset($x[$i][$j][$k]);
                    unset($y[$i][$j][$k]);
                    unset($z[$i][$j][$k]);
                } else {
                    $key = "new_key_" . strval($i) . "_" . strval($j) . "_" . strval($j);
                    $x[$i][$j][$k] = $key;
                    $y[$i][$j][$k] = $key;
                    $z[$i][$j][$k] = $key;
                }
                var_dump($x[$i][$j][$k]);
                var_dump($y[$i][$j][$k]);
                var_dump($z[$i][$j][$k]);
            }
        }
    }
}

function test_cyclic_ref() {
    $x = as_mix_obj([[0, 1, 2, 3, 4, 5]]);
    $y = as_mix_obj([$x, "str"]);
    $x[] = $y;

    assert_true($x[1][0][1][0][1][0][1][0][1][0][0][4] === 4);
    assert_true($x[1][0][1][0][1][0][1][0][1][0][1][1] === "str");
    assert_true($x[1][0][1][0][1][0][1][0][1][1] === "str");
    assert_true($x[1][0][1][0][1][0][1][1] === "str");
    assert_true($x[1][0][1][0][1][1] === "str");
    assert_true($x[1][0][1][1] === "str");
    assert_true($x[1][1] === "str");

    unset($x[1]);

    assert_true(!isset($x[1]));
}

function test_cyclic_assignment() {
    $x = as_mix_obj([42]);
    $y = as_mix_obj([$x, "str"]);
    $x[] = $y;

    $x[1][0][1][0][1][0][0] = $x[1][0][1][0][1][0][1][1];
    assert_true($x[1][0][1][0][1][0][1][0][0] === "str");
    assert_true($x[1][0][0] === "str");
    assert_true($x[1][0][1][0][1][0][1][1] === "str");
    assert_true($x[1][0][1][1] === "str");
    
    $x[1][0][1][0][1][0][1][0][] = $x[1][0][1][0][1][] = $x[1][0][1][0][] = "123";
    assert_true($x[2] == "123");
    assert_true($x[3] == "123");
    assert_true($x[1][2] == "123");
}

test_straightforward();
test_cyclic_ref();
test_cyclic_assignment();

