@ok php8
<?php

#[AttributeWithArgs(100, SomeClass::class)]
function f1() {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
function f2(): void {}

#[
	FirstLineAttribute(100, SomeClass::class),
	SecondLineAttribute(),
]
function f3() {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
function f4(): int|string {}

#[\WithSlash]
function f5() {}

#[\F\Q\N]
function f6(): \Foo|string {}

