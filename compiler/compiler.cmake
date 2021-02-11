set(KPHP_COMPILER_DIR ${BASE_DIR}/compiler)

set(KPHP_COMPILER_AUTO_DIR ${AUTO_DIR}/compiler)
set(KEYWORDS_SET ${KPHP_COMPILER_AUTO_DIR}/keywords_set.hpp)
set(KEYWORDS_GPERF ${KPHP_COMPILER_DIR}/keywords.gperf)

prepend(KPHP_COMPILER_COMMON ${COMMON_DIR}/
        dl-utils-lite.cpp
        wrappers/fmt_format.cpp
        termformat/termformat.cpp
        tl2php/combinator-to-php.cpp
        tl2php/constructor-to-php.cpp
        tl2php/function-to-php.cpp
        tl2php/php-classes.cpp
        tl2php/tl-to-php-classes-converter.cpp)

prepend(KPHP_COMPILER_THREADING_SOURCES threading/
        allocator.cpp
        profiler.cpp
        thread-id.cpp)

prepend(KPHP_COMPILER_SCHEDULER_SOURCES scheduler/
        node.cpp
        one-thread-scheduler.cpp
        scheduler-base.cpp
        scheduler.cpp)

prepend(KPHP_COMPILER_MAKE_SOURCES make/
        hardlink-or-copy.cpp
        make-runner.cpp
        make.cpp
        target.cpp)

prepend(KPHP_COMPILER_DATA_SOURCES data/
        class-data.cpp
        class-members.cpp
        define-data.cpp
        function-data.cpp
        lambda-class-data.cpp
        lambda-generator.cpp
        lambda-interface-generator.cpp
        lambda-implicit-uses.cpp
        lib-data.cpp
        performance-inspections.cpp
        src-file.cpp
        var-data.cpp
        vars-collector.cpp
        virtual-method-generator.cpp)

prepend(KPHP_COMPILER_INFERRING_SOURCES inferring/
        expr-node.cpp
        ifi.cpp
        key.cpp
        lvalue.cpp
        multi-key.cpp
        node-recalc.cpp
        node.cpp
        primitive-type.cpp
        public.cpp
        restriction-isset.cpp
        restriction-match-phpdoc.cpp
        restriction-non-void.cpp
        restriction-stacktrace-finder.cpp
        type-data.cpp
        type-inferer.cpp
        type-hint-recalc.cpp
        type-node.cpp
        var-node.cpp)

prepend(KPHP_COMPILER_CODEGEN_SOURCES code-gen/
        code-gen-task.cpp
        code-generator.cpp
        declarations.cpp
        files/function-header.cpp
        files/function-source.cpp
        files/global_vars_memory_stats.cpp
        files/init-scripts.cpp
        files/lib-header.cpp
        files/tl2cpp/tl-combinator.cpp
        files/tl2cpp/tl-constructor.cpp
        files/tl2cpp/tl-function.cpp
        files/tl2cpp/tl-module.cpp
        files/tl2cpp/tl-type-expr.cpp
        files/tl2cpp/tl-type.cpp
        files/tl2cpp/tl2cpp-utils.cpp
        files/tl2cpp/tl2cpp.cpp
        files/type-tagger.cpp
        files/vars-cpp.cpp
        files/vars-reset.cpp
        includes.cpp
        raw-data.cpp
        vertex-compiler.cpp
        writer-data.cpp)

prepend(KPHP_COMPILER_PIPES_SOURCES pipes/
        analyzer.cpp
        analyze-performance.cpp
        calc-actual-edges.cpp
        calc-bad-vars.cpp
        calc-const-types.cpp
        calc-empty-functions.cpp
        calc-func-dep.cpp
        calc-locations.cpp
        calc-real-defines-values.cpp
        calc-rl.cpp
        calc-val-ref.cpp
        cfg-end.cpp
        cfg.cpp
        check-abstract-function-defaults.cpp
        check-access-modifiers.cpp
        check-classes.cpp
        check-conversions.cpp
        check-function-calls.cpp
        check-modifications-of-const-vars.cpp
        check-nested-foreach.cpp
        check-tl-classes.cpp
        check-ub.cpp
        clone-strange-const-params.cpp
        code-gen.cpp
        collect-const-vars.cpp
        collect-forkable-types.cpp
        collect-main-edges.cpp
        collect-required-and-classes.cpp
        convert-list-assignments.cpp
        erase-defines-declarations.cpp
        extract-async.cpp
        extract-resumable-calls.cpp
        file-to-tokens.cpp
        filter-only-actually-used.cpp
        final-check.cpp
        fix-returns.cpp
        gen-tree-postprocess.cpp
        generate-virtual-methods.cpp
        inline-defines-usages.cpp
        inline-simple-functions.cpp
        load-files.cpp
        optimization.cpp
        parse.cpp
        parse-and-apply-phpdoc.cpp
        preprocess-break.cpp
        preprocess-eq3.cpp
        preprocess-exceptions.cpp
        preprocess-function.cpp
        register-defines.cpp
        register-kphp-configuration.cpp
        register-variables.cpp
        remove-empty-function-calls.cpp
        resolve-self-static-parent.cpp
        sort-and-inherit-classes.cpp
        split-switch.cpp
        transform-to-smart-instanceof.cpp
        type-inferer.cpp
        write-files.cpp)

prepend(KPHP_COMPILER_SOURCES ${KPHP_COMPILER_DIR}/
        ${KPHP_COMPILER_CODEGEN_SOURCES}
        ${KPHP_COMPILER_DATA_SOURCES}
        ${KPHP_COMPILER_INFERRING_SOURCES}
        ${KPHP_COMPILER_MAKE_SOURCES}
        ${KPHP_COMPILER_PIPES_SOURCES}
        ${KPHP_COMPILER_SCHEDULER_SOURCES}
        ${KPHP_COMPILER_THREADING_SOURCES}
        class-assumptions.cpp
        compiler-core.cpp
        compiler.cpp
        composer.cpp
        cpp-dest-dir-initializer.cpp
        debug.cpp
        compiler-settings.cpp
        gentree.cpp
        index.cpp
        lexer.cpp
        name-gen.cpp
        operation.cpp
        phpdoc.cpp
        stage.cpp
        stats.cpp
        type-hint.cpp
        tl-classes.cpp
        vertex.cpp)

if(APPLE)
    set_source_files_properties(${KPHP_COMPILER_DIR}/lexer.cpp PROPERTIES COMPILE_FLAGS -Wno-deprecated-register)
endif()

list(APPEND KPHP_COMPILER_SOURCES
     ${KPHP_COMPILER_COMMON}
     ${KEYWORDS_SET})

vk_add_library(kphp2cpp_src OBJECT ${KPHP_COMPILER_SOURCES})

file(MAKE_DIRECTORY ${KPHP_COMPILER_AUTO_DIR})
add_custom_command(OUTPUT ${KEYWORDS_SET}
                   COMMAND gperf -CGD -c -N get_type -Z KeywordsSet -K keyword -L C++ -t ${KEYWORDS_GPERF} > ${KEYWORDS_SET}
                   DEPENDS ${KEYWORDS_GPERF}
                   COMMENT "keywords.gperf generation")

prepend(VERTEX_AUTO_GENERATED ${KPHP_COMPILER_AUTO_DIR}/vertex/
        vertex-types.h
        vertex-all.h
        foreach-op.h
        is-base-of.h)

add_custom_command(OUTPUT ${VERTEX_AUTO_GENERATED}
                   COMMAND ${Python3_EXECUTABLE} ${KPHP_COMPILER_DIR}/vertex-gen.py --auto ${AUTO_DIR} --config ${KPHP_COMPILER_DIR}/vertex-desc.json
                   DEPENDS ${KPHP_COMPILER_DIR}/vertex-gen.py ${KPHP_COMPILER_DIR}/vertex-desc.json
                   COMMENT "vertices generation")
add_custom_target(auto_vertices_generation_target DEPENDS ${VERTEX_AUTO_GENERATED})

set_property(SOURCE ${KPHP_COMPILER_DIR}/kphp2cpp.cpp
             APPEND
             PROPERTY COMPILE_DEFINITIONS
             DEFAULT_KPHP_PATH="${DEFAULT_KPHP_PATH}/"
             KPHP_HAS_NO_PIE="${NO_PIE}")

add_executable(kphp2cpp ${KPHP_COMPILER_DIR}/kphp2cpp.cpp)
target_include_directories(kphp2cpp PUBLIC ${KPHP_COMPILER_DIR})

prepare_cross_platform_libs(COMPILER_LIBS yaml-cpp re2)
set(COMPILER_LIBS vk::kphp2cpp_src vk::tlo_parsing_src vk::popular_common ${COMPILER_LIBS} fmt::fmt OpenSSL::Crypto pthread)

target_link_libraries(kphp2cpp PRIVATE ${COMPILER_LIBS})
target_link_options(kphp2cpp PRIVATE ${NO_PIE})
set_target_properties(kphp2cpp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

add_dependencies(kphp2cpp_src auto_vertices_generation_target)
