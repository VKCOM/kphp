@kphp_should_fail
/yield isn't supported/
<?php
function foo() {
    yield 1;
    yield from [2,3];
    yield from four_five();
    return yield from six_seven();
}

function four_five() {
    yield 4;
    yield from five();
}

function five() {
    yield 5;
}

function six_seven() {
    yield 6;
    yield 7;
}

foreach (foo() as $el) {
	echo $el;
}
