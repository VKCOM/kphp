@ok php8
<?php

class Foo {
	public function f1(#[SimpleAttribute] string|int $a) {}
	public function f2(
		#[SimpleAttribute] string|int $a,
		#[AttributeWithArgs(100, SomeClass::class)] string|int $b,
		#[
			FirstLineAttribute(100, SomeClass::class),
			SecondLineAttribute()
		]
		$c,
		#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
		#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
		Foo|string $d = "100",
		#[
			FirstAttribute(100, SomeClass::class),
			SecondAttribute(),
		]
		#[
			SecondGroupFirstAttribute(SomeClass::class),
			SecondGroupSecondAttribute
		]
		$e = 100,
		#[\F\Q\N] string|int $f = 100
	) {}
}

function f1(#[SimpleAttribute] $a) {}

function f2(
	#[SimpleAttribute] string|int $a,
	#[AttributeWithArgs(100, SomeClass::class)] string|int $b,
	#[
		FirstLineAttribute(100, SomeClass::class),
		SecondLineAttribute()
	]
	$c,
	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
	\Foo|string $d = "100",
	#[
		FirstAttribute(100, SomeClass::class),
		SecondAttribute(),
	]
	#[
		SecondGroupFirstAttribute(SomeClass::class),
		SecondGroupSecondAttribute
	]
	$e = 100,
	#[\F\Q\N] string|int $f = 100
) {}
