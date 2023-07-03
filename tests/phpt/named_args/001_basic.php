@php8 ok
<?php

function test($a, $b, $c = "c", $d = "d", $e = "e") {
    echo "a=$a, b=$b, c=$c, d=$d, e=$e\n";
}

$a = "A"; $b = "B"; $c = "C"; $d = "D"; $e = "E";

test("A", "B", "C", d: "D", e: "E");
test("A", "B", "C", e: "E", d: "D");
test(e: "E", a: "A", d: "D", b: "B", c: "C");
test("A", "B", "C", e: "E");

test("A", "B", e : "E");
test("A", "B", d : "D");
test("A", "B", d : "D", e : "E");

test2("A", "B", "C", d: "D", e: "E");
test2("A", "B", "C", e: "E", d: "D");
test2(e: "E", a: "A", d: "D", b: "B", c: "C");
test2("A", "B", "C", e: "E");

test($a, $b, $c, d: $d, e: $e);
test($a, $b, $c, e: $e, d: $d);
test(e: $e, a: $a, d: $d, b: $b, c: $c);
test(a: $a, b: $b, c: $c, e: $e);

test2($a, $b, $c, d: $d, e: $e);
test2($a, $b, $c, e: $e, d: $d);
test2(e: $e, a: $a, d: $d, b: $b, c: $c);
test2(a: $a, b: $b, c: $c, e: $e);

function test2($a, $b, $c = "c", $d = "d", $e = "e") {
    echo "a=$a, b=$b, c=$c, d=$d, e=$e\n";
}

