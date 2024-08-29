@ok php8
<?php

class Foo {
	#[SimpleAttribute]
	public const A = 100;

	#[AttributeWithArgs(100, SomeClass::class)]
	private const A1 = 10;

	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	protected const A2 = "a";

	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	public const A3 = 100;

	#[
		FirstLineAttribute(100, SomeClass::class),
		SecondLineAttribute()
	]
	public const A4 = 100;

	#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
	#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
	public const A5 = 100;

	#[\WithSlash]
	protected const A6 = 100;

	#[\F\Q\N]
	public const A7 = 100;
}
