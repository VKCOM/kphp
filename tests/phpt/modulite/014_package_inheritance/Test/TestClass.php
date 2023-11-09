<?php

namespace Test;

use VK\Strings\Utils014\Append014;

class TestClass
{
    public static function test()
    {
       $_= Append014::concatStr("test","test");
       $_= \PackageExtender::concatStr("test","test");
    }
}
