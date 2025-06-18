@ok php8
<?php

class Foo {
	#[SimpleAttribute]
	public $a1 = "";

	#[AttributeWithArgs(100, SomeClass::class)]
	protected $a2 = null;

	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	var $a3 = null, $b1 = 100;

	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	private ?Foo $a4 = null;

	#[
		FirstLineAttribute(100, SomeClass::class),
		SecondLineAttribute()
	]
	public int|string $a5 = 10, $b, $c, $d;

	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
	var $a6 = 100;

	#[\WithSlash]
	public $a7 = null;

	#[\F\Q\N]
	protected $a8 = null;
}
