prepend(RUNTIME_LANGUAGE_SRC core/globals/ php-script-globals.cpp)

prepend(RUNTIME_KPHP_CORE_CONTEXT_SRC core/kphp-core-impl/
        kphp-core-context.cpp)

set(RUNTIME_LIGHT_CORE_SRC ${RUNTIME_LANGUAGE_SRC}
                           ${RUNTIME_KPHP_CORE_CONTEXT_SRC})
