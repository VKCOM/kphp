include_guard(GLOBAL)

prepend(TLO_PARSING_OBJECTS ${COMMON_DIR}/tlo-parsing/
        flat-optimization.cpp
        remove-odd-types.cpp
        replace-anonymous-args.cpp
        tl-dependency-graph.cpp
        tl-objects.cpp
        tl-scheme-final-check.cpp
        tlo-parser.cpp)
vk_add_library(tlo_parsing_src OBJECT ${TLO_PARSING_OBJECTS})
