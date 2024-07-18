import os
import subprocess
import pytest
import re

from python.lib.testcase import KphpCompilerAutoTestCase

class TestFFI(KphpCompilerAutoTestCase):
    def should_use_nocc(self):
        # compiling FFI tests needs special libs to be installed, whey aren't installed on nocc agents
        return False

    def is_shared_lib_installed(self, name):
        if 'GITHUB_ACTIONS' in os.environ:
            return False
        cmd = ['ldconfig', '-p']
        result = subprocess.run(cmd, stdout=subprocess.PIPE)
        output = result.stdout.decode('utf-8')
        return name in output

    def build_shared_lib(self, name):
        dir = self.kphp_build_working_dir
        obj_file_name = os.path.join(dir, name + '.o')
        c_file_name = os.path.join(self.test_dir, 'c', name + '.c')
        lib_file_name = os.path.join(dir, 'lib' + name + '.so')
        cmd = ['gcc', '-c', '-fpic', '-o', obj_file_name, c_file_name]
        ret_code = subprocess.call(cmd, cwd=self.test_dir)
        if ret_code != 0:
            raise Exception("failed to compile " + c_file_name)
        cmd = ['gcc', '-shared', '-o', lib_file_name, obj_file_name]
        ret_code = subprocess.call(cmd, cwd=self.test_dir)
        if ret_code != 0:
            raise Exception("failed to create a shared lib " + lib_file_name)

    def ffi_build_and_compare_with_php(self, php_script_path, shared_libs=[]):
        os.environ['LD_LIBRARY_PATH'] = self.kphp_build_working_dir
        os.environ.pop('PHP_FFI_LOAD1', None)
        os.environ.pop('PHP_FFI_LOAD2', None)
        os.environ.pop('PHP_FFI_LOAD3', None)
        i = 1
        for lib in shared_libs:
            self.build_shared_lib(lib)
            os.environ['PHP_FFI_LOAD' + str(i)] = lib
            i += 1
        once_runner = self.make_kphp_once_runner(php_script_path)
        php_options = [
            ('zend_extension', 'opcache'),
            ('extension', 'ffi'),
            ('ffi.enable', 'preload'),
            ('opcache.enable', 1),
            ('opcache.enable_cli', 1),
            ('opcache.preload', os.path.join(self.test_dir, 'php', 'preload.php')),
        ]
        self.assertTrue(once_runner.run_with_php(php_options), "Got PHP error")
        self.assertTrue(once_runner.compile_with_kphp({}))
        self.assertTrue(once_runner.run_with_kphp(), "Got KPHP runtime error")
        self.assertTrue(once_runner.compare_php_and_kphp_stdout(), "Got PHP and KPHP diff")

    def codegen_find(self, patterns):
        results = {}
        for pat in patterns:
            results[pat] = []
        path = os.path.join(self.kphp_build_working_dir, "kphp_build", "kphp")
        for f in os.listdir(path):
            full_name = os.path.join(path, f)
            if not os.path.isdir(full_name):
                continue
            if f.startswith('o_globals_') or f.startswith('o_const_'):
                continue
            for f2 in os.listdir(full_name):
                with open(os.path.join(full_name, f2), 'r') as cpp_file:
                    for l in cpp_file.readlines():
                        l = str(l).strip()
                        if l.startswith('//'):
                            continue
                        for pat in patterns:
                            if re.search(pat, l):
                                results[pat].append((f2, l))
        return results

    def log_print(self, msg):
        with open("log.txt", "a") as f:
            f.write(msg + "\n")

    def assertCodegen(self, expect):
        patterns = []
        values = []
        for e in expect:
            patterns.append(e[0])
            values.append(e[1])
        matches = self.codegen_find(patterns)
        i = 0
        while i < len(patterns):
            pat = patterns[i]
            m = matches[pat]
            if len(m) != 0 and values[i] == 0:
                (filename, l) = m[0]
                self.fail(filename + ' matches /' + pat + '/: ' + l)
            elif len(m) != values[i]:
                self.fail('got ' + str(len(m)) + ' matches for /' + pat + '/, expected ' + str(values[i]))
            i += 1

    # optimization tests: ensure that we generate efficient code

    def test_optimization_c2php_elim(self):
        self.ffi_build_and_compare_with_php('php/optimization/c2php_elim.php')
        self.assertCodegen([(r'ffi_c2php', 3)])

    # typing tests: ensure that FFI types info is preserved in different contexts

    def test_typing_vector_closure_arg(self):
        self.ffi_build_and_compare_with_php('php/typing/vector_closure_arg.php', shared_libs=['vector'])

    def test_typing_bool_struct_field(self):
        self.ffi_build_and_compare_with_php('php/typing/bool_struct_field.php')

    def test_typing_char_struct_field(self):
        self.ffi_build_and_compare_with_php('php/typing/char_struct_field.php')

    def test_typing_complex_expr(self):
        self.ffi_build_and_compare_with_php('php/typing/complex_expr.php')

    def test_typing_cdata_type_hint(self):
        self.ffi_build_and_compare_with_php('php/typing/cdata_type_hint.php')

    def test_typing_cdata_ptr_type_hint(self):
        self.ffi_build_and_compare_with_php('php/typing/cdata_ptr_type_hint.php')

    # nested structs test: check that C struct fields are working correctly

    def test_nested_value_read(self):
        self.ffi_build_and_compare_with_php('php/nested/value_read.php')

    def test_nested_value_read2(self):
        self.ffi_build_and_compare_with_php('php/nested/value_read2.php')

    def test_nested_value_assign_struct(self):
        self.ffi_build_and_compare_with_php('php/nested/value_assign_struct.php')

    def test_nested_ptr_assign(self):
        self.ffi_build_and_compare_with_php('php/nested/ptr_assign.php', ['vector'])

    # scope tests: check the FFI scope/library objects correctness

    def test_scope_func_arg(self):
        self.ffi_build_and_compare_with_php('php/scope/scope_func_arg.php', shared_libs=['vector'])

    def test_scope_class_field(self):
        self.ffi_build_and_compare_with_php('php/scope/scope_class_field.php', shared_libs=['vector'])

    # numtype tests: check that C<->PHP numerical types work correctly

    def test_numtype_simple(self):
        self.ffi_build_and_compare_with_php('php/numtype/simple.php')

    def test_numtype_array(self):
        self.ffi_build_and_compare_with_php('php/numtype/array.php')

    def test_numtype_func_arg(self):
        self.ffi_build_and_compare_with_php('php/numtype/func_arg.php')

    def test_numtype_truncate(self):
        self.ffi_build_and_compare_with_php('php/numtype/truncate.php')

    def test_numtype_arith(self):
        self.ffi_build_and_compare_with_php('php/numtype/arith.php')

    def test_numtype_new_scalar(self):
        self.ffi_build_and_compare_with_php('php/numtype/new_scalar.php')

    # ref tests: check that CDataRef works correctly

    def test_ref_field_write(self):
        self.ffi_build_and_compare_with_php('php/ref/field_write.php')

    def test_ref_field_write_struct(self):
        self.ffi_build_and_compare_with_php('php/ref/field_write_struct.php')

    def test_ref_aliasing(self):
        self.ffi_build_and_compare_with_php('php/ref/aliasing.php')

    # pointer tests: check some C pointer-specific things

    def test_pointer_cstr(self):
        self.ffi_build_and_compare_with_php('php/pointer/cstr.php', shared_libs=['pointers'])

    def test_pointer_implicit_deref(self):
        self.ffi_build_and_compare_with_php('php/pointer/implicit_deref.php', shared_libs=['pointers'])

    def test_pointer_null_as_ptr(self):
        self.ffi_build_and_compare_with_php('php/pointer/null_as_ptr.php', shared_libs=['pointers'])

    def test_pointer_null_cdata(self):
        self.ffi_build_and_compare_with_php('php/pointer/null_cdata.php', shared_libs=['pointers'])

    def test_pointer_primitive_ptr_arg(self):
        self.ffi_build_and_compare_with_php('php/pointer/primitive_ptr_arg.php', shared_libs=['pointers'])

    def test_pointer_ptr2(self):
        self.ffi_build_and_compare_with_php('php/pointer/ptr2.php', shared_libs=['pointers'])

    def test_pointer_cast(self):
        self.ffi_build_and_compare_with_php('php/pointer/cast.php', shared_libs=['pointers'])

    # vector tests: check libvector.so functions

    def test_vector_default_value(self):
        self.ffi_build_and_compare_with_php('php/vector/default_value.php', shared_libs=['vector'])

    # syslib tests: some example bindings to libraries

    def test_syslib_h3(self):
        if self.is_shared_lib_installed('libh3.so'):
            self.ffi_build_and_compare_with_php('php/syslib/uberh3_example.php')
        else:
            pytest.skip('libh3.so not found')

    def test_syslib_libc(self):
        if self.is_shared_lib_installed('libc.so.6'):
            self.ffi_build_and_compare_with_php('php/syslib/libc_example.php')
        else:
            pytest.skip('libc.so.6 not found')

    def test_syslib_libgmp(self):
        if self.is_shared_lib_installed('libgmp.so'):
            self.ffi_build_and_compare_with_php('php/syslib/libgmp_example.php')
        else:
            pytest.skip('libgmp.so not found')

    def test_syslib_libimagick(self):
        if self.is_shared_lib_installed('libMagickWand-6.Q16.so'):
            self.ffi_build_and_compare_with_php('php/syslib/imagick_example.php')
        else:
            pytest.skip('libMagickWand-6.Q16.so not found')

    def test_syslib_libgd(self):
        if self.is_shared_lib_installed('libgd.so'):
            self.ffi_build_and_compare_with_php('php/syslib/libgd_example.php')
        else:
            pytest.skip('libgd.so')

    # util tests: check sizeof, memcpy and other functions

    def test_util_sizeof(self):
        self.ffi_build_and_compare_with_php('php/util/sizeof.php')

    def test_util_memcmp(self):
        self.ffi_build_and_compare_with_php('php/util/memcmp.php')

    def test_util_memcpy(self):
        self.ffi_build_and_compare_with_php('php/util/memcpy.php')

    def test_util_memset(self):
        self.ffi_build_and_compare_with_php('php/util/memset.php')

    def test_util_isnull(self):
        self.ffi_build_and_compare_with_php('php/util/isnull.php')

    def test_util_string(self):
        self.ffi_build_and_compare_with_php('php/util/string.php')

    # multi-file tests

    def test_multifile(self):
        self.ffi_build_and_compare_with_php('php/multifile/main.php', shared_libs=['vector', 'pointers'])

    # staticlib tests: check that static FFI libraries work properly

    def test_staticlib_implicit(self):
        self.ffi_build_and_compare_with_php('php/staticlib/implicit.php')

    # other tests

    def test_callbacks(self):
        self.ffi_build_and_compare_with_php('php/callbacks.php', shared_libs=['vector'])

    def test_unmanaged_memory(self):
        self.ffi_build_and_compare_with_php('php/unmanaged_memory.php', shared_libs=['pointers'])


    def test_dynamic_size_arrays(self):
        self.ffi_build_and_compare_with_php('php/dynamic_size_arrays.php', shared_libs=['pointers'])

    def test_fixed_size_arrays(self):
        self.ffi_build_and_compare_with_php('php/fixed_size_arrays.php', shared_libs=['pointers'])

    def test_pointer_ops(self):
        self.ffi_build_and_compare_with_php('php/pointer_ops.php', shared_libs=['pointers'])

    def test_vararg(self):
        self.ffi_build_and_compare_with_php('php/vararg.php')

    def test_vararg2(self):
        self.ffi_build_and_compare_with_php('php/vararg2.php')

    def test_vararg3(self):
        self.ffi_build_and_compare_with_php('php/vararg3.php', shared_libs=['vector'])

    def test_vararg4(self):
        self.ffi_build_and_compare_with_php('php/vararg4.php', shared_libs=['vector'])

    def test_clone(self):
        self.ffi_build_and_compare_with_php('php/clone.php')

    def test_cdef_global(self):
        self.ffi_build_and_compare_with_php('php/cdef_global.php')

    def test_cdata_getclass(self):
        self.ffi_build_and_compare_with_php('php/cdata_getclass.php')

    def test_multiple_cdef_in_one_file(self):
        self.ffi_build_and_compare_with_php('php/multiple_cdef_in_one_file.php')

    def test_comments(self):
        self.ffi_build_and_compare_with_php('php/comments.php')

    def test_typedef(self):
        self.ffi_build_and_compare_with_php('php/cdef_typedef.php')

    def test_vector_double(self):
        self.ffi_build_and_compare_with_php('php/vector_double.php', shared_libs=['vector'])

    def test_vector_float(self):
        self.ffi_build_and_compare_with_php('php/vector_float.php', shared_libs=['vector'])

    def test_vector_func_arg(self):
        self.ffi_build_and_compare_with_php('php/vector_func_arg.php', shared_libs=['vector'])

    def test_vector_class_field(self):
        self.ffi_build_and_compare_with_php('php/vector_class_field.php', shared_libs=['vector'])

    def test_vector_array(self):
        self.ffi_build_and_compare_with_php('php/vector_array.php', shared_libs=['vector'])

    def test_vector_reassign(self):
        self.ffi_build_and_compare_with_php('php/vector_reassign.php', shared_libs=['vector'])

    def test_vector_array_free_queue(self):
        self.ffi_build_and_compare_with_php('php/vector_array_free_queue.php', shared_libs=['vector'])

    def test_enums(self):
        self.ffi_build_and_compare_with_php('php/enums.php')

    def test_unions(self):
        self.ffi_build_and_compare_with_php('php/unions.php')

    def test_unions2(self):
        self.ffi_build_and_compare_with_php('php/unions2.php')

    def test_unions3(self):
        self.ffi_build_and_compare_with_php('php/unions3.php')

    def test_multiple_shared_lib_refs(self):
        self.ffi_build_and_compare_with_php('php/multiple_shared_lib_refs.php')
