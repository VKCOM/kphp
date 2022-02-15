@ok
<?php

/**
 * @param int $x
 * @return string|null
 */
function f1(int $x): ?string {
    return null;
}

/**
 * @param ?int[] $x
 * @param callable(int):void $cb
 * @return string[]
 */
function f2(?array $x, callable $cb): array {
    return [];
}

/**
 * @param (int|false)[] $p1
 * @param int[]|false[] $p2
 * @param null|int[] $p3
 * @param null|int[]|array $p4
 * @param null|float[]|int[] $p5
 * @return int[]|string[]
 */
function f3(array $p1, array $p2, ?array $p3, ?array $p4, ?array $p5): array {
    return [];
}

f1(1);
f2(null,function($x){});
f3([],[],[],[],[]);
