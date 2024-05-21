prepend(RUNTIME_LANGUAGE_SRC ${BASE_DIR}/runtime-light/core/globals/
        php-script-globals.cpp)

prepend(RUNTIME_KPHP_CORE_CONTEXT_SRC ${BASE_DIR}/runtime-light/core/kphp-core-impl/
        kphp-core-context.cpp)


set(RUNTIME_CORE_SRC ${RUNTIME_LANGUAGE_SRC}
                    ${RUNTIME_KPHP_CORE_CONTEXT_SRC})