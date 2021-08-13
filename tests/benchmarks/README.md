## Benchmarks

To use benchmarks
* [Install golang](https://golang.org/doc/install)
* Clone and build [ktest](https://github.com/quasilyte/ktest) 

```
$ git clone https://github.com/quasilyte/ktest
$ cd ktest
$ go build -o ktest ./cmd/ktest/
```
* Enjoy with the benchmarks
```
$ KPHP_ROOT=/path/to/repo/kphp ./ktest bench-vs-php /path/to/repo/kphp/tests/benchmarks/
```
