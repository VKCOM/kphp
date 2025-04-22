import zstandard
import random
import string
import os
import gzip
import pytest

from python.lib.testcase import WebServerAutoTestCase


@pytest.mark.k2_skip_suite
class TestZstd(WebServerAutoTestCase):
    # size of this string is 460
    some_string = """
        KPHP is a PHP compiler. It compiles a limited subset of PHP to a native binary running faster than PHP.
        KPHP takes your PHP source code and converts it to a C++ equivalent, 
        then compiles the generated C++ code and runs it within an embedded HTTP server. 
        You could call KPHP a transpiler, but we call it a compiler.
        KPHP is not JIT-oriented: all types are inferred at compile time. It has no “slow startup” phase.
    """.encode()

    @classmethod
    def _make_name(cls, file_name):
        return os.path.join(cls.web_server_working_dir, file_name)

    @classmethod
    def make_dict(cls, file_name, samples):
        train_set = [samples[i % len(samples)] for i in range(512)]
        d = zstandard.train_dictionary(1024 * 1024, train_set)
        with open(cls._make_name(file_name), 'wb') as f:
            f.write(d.as_bytes())
        return d

    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--hard-memory-limit": "350m"
        })
        cls.dict = cls.make_dict("dict", (cls.some_string, b"is", b"PHP", b"KPHP", b"call"))
        cls.dict_other = cls.make_dict("dict.other", (b"ab " * 10, b"foo " * 100, b"bar " * 1000, b"baz " * 100))
        cls.dict_bad = b"foo bar baz"
        with open(cls._make_name("dict.bad"), 'wb') as f:
            f.write(cls.dict_bad)
        cls.huge_random_string = ''.join(random.choice(string.ascii_letters) for _ in range(1024 * 1024 * 17)).encode()
        cls.examples = (
            (b"hello world", zstandard.MAX_COMPRESSION_LEVEL + 1, 1),
            (cls.some_string, zstandard.MAX_COMPRESSION_LEVEL + 1, 1),
            (cls.some_string * 100000, 15, 3),
            (cls.huge_random_string, 15, 4)
        )

    @classmethod
    def extra_class_teardown(cls):
        for f in ("dict", "dict.other", "dict.bad", "in.dat", "out.dat"):
            try:
                os.unlink(cls._make_name(f))
            except:
                pass

    def _call_php(self, test_type, expected_code=200, expected_msg="OK", level=0, dictionary="dict"):
        resp = self.web_server.http_get("/test_zstd?type={}&level={}&dict={}".format(test_type, level, dictionary))
        self.assertEqual(resp.status_code, expected_code)
        self.assertEqual(resp.text, expected_msg)

    def _write_in_file(self, body):
        with open(self._make_name("in.dat"), 'wb') as f:
            f.write(body)

    def _read_out_file(self):
        with open(self._make_name("out.dat"), 'rb') as f:
            return f.read()

    def test_uncompress(self):
        for s, max_level, step in self.examples:
            for i in range(1, max(2, max_level), step):
                cxt = zstandard.ZstdCompressor(level=i)
                self._write_in_file(cxt.compress(s))
                self._call_php("uncompress")
                self.assertEqual(self._read_out_file(), s)

    def test_uncompress_data_compressed_by_other_algo(self):
        self._write_in_file(gzip.compress(self.some_string))
        self._call_php("uncompress")
        self.web_server.assert_log(["Warning: zstd_uncompress: it was not compressed by zstd"])
        self.assertEqual(self._read_out_file(), b"false")

    def test_uncompress_non_compressed_data(self):
        self._write_in_file(self.some_string)
        self._call_php("uncompress")
        self.web_server.assert_log(["Warning: zstd_uncompress: it was not compressed by zstd"])
        self.assertEqual(self._read_out_file(), b"false")

    def test_uncompress_compressed_data_with_dict(self):
        ctx = zstandard.ZstdCompressor(dict_data=self.dict)
        self._write_in_file(ctx.compress(self.some_string))
        self._call_php("uncompress")
        self.web_server.assert_log(["Warning: zstd_uncompress: got zstd error: Dictionary mismatch"])
        self.assertEqual(self._read_out_file(), b"false")

    def test_compress(self):
        ctx = zstandard.ZstdDecompressor()
        for s, max_level, step in self.examples:
            for i in range(1, max(2, max_level), step):
                self._write_in_file(s)
                self._call_php("compress", level=i)
                self.assertEqual(ctx.decompress(self._read_out_file()), s)

    def test_compress_oom(self):
        self._write_in_file(self.huge_random_string)
        self._call_php("compress", 500, "ERROR", zstandard.MAX_COMPRESSION_LEVEL)
        self.web_server.assert_log([
            "Warning: Can't allocate \\d+ bytes",
            "Critical error during script execution: memory limit exit"
        ])

    def test_compress_bad_level(self):
        self._write_in_file(self.some_string)
        bad_level = zstandard.MAX_COMPRESSION_LEVEL+1
        self._call_php("compress", level=bad_level)
        self.web_server.assert_log([
            "zstd_compress: compression level \\({}\\) must be within -\\d*..22 or equal to 0".format(bad_level),
        ])
        self.assertEqual(self._read_out_file(), b"false")

    def test_uncompress_dict(self):
        for s, max_level, step in self.examples:
            for i in range(1, max(2, max_level), step):
                ctx = zstandard.ZstdCompressor(dict_data=self.dict, level=i)
                self._write_in_file(ctx.compress(s))
                self._call_php("uncompress_dict")
                self.assertEqual(self._read_out_file(), s)

    def test_uncompress_dict_compressed_with_other_dict(self):
        ctx = zstandard.ZstdCompressor(dict_data=self.dict_other)
        self._write_in_file(ctx.compress(self.some_string))
        self._call_php("uncompress_dict")
        self.web_server.assert_log(["Warning: zstd_uncompress: got zstd error: Dictionary mismatch"])
        self.assertEqual(self._read_out_file(), b"false")

    def test_uncompress_dict_compressed_without_dict(self):
        ctx = zstandard.ZstdCompressor()
        self._write_in_file(ctx.compress(self.some_string))
        self._call_php("uncompress_dict")
        self.assertEqual(self._read_out_file(), self.some_string)

    def test_compress_dict(self):
        ctx = zstandard.ZstdDecompressor(dict_data=self.dict)
        for s, _, _ in self.examples:
            self._write_in_file(s)
            self._call_php("compress_dict")
            self.assertEqual(ctx.decompress(self._read_out_file()), s)

    def test_compress_bad_dict(self):
        ctx = zstandard.ZstdDecompressor()
        self._write_in_file(self.some_string)
        self._call_php("compress_dict", dictionary="dict.bad")
        self.assertEqual(ctx.decompress(self._read_out_file()), self.some_string)
