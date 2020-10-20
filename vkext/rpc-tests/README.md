Before test running you need to clone engine repo  
and apply memcache-rpc-tl-test.patch to it (git apply).

Run from vkext/rpc-tests directory with the following command:

`./run-tests.py --engine-repo <path-to-engine-repo>`

It's possible to run with valgrind:

`run-tests.py --valgrind --engine-repo <path-to-engine-repo>`

Output of it can be found in `vkext/rpc-tests/valgrind.out`.

Useful [link](http://www.phpinternalsbook.com/php7/memory_management/memory_debugging.html) about memory debug in PHP.
