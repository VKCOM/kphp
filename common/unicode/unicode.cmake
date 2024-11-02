set(UNICODE_DIR ${COMMON_DIR}/unicode)
set(UNICODE_AUTO_DATA_DIR ${AUTO_DIR}/common/unicode/data)
set(UNICODE_AUTO_RAW_DATA_DIR ${AUTO_DIR}/common/unicode/raw_data)

prepend(UNICODE_DATA_LIST ${UNICODE_AUTO_DATA_DIR}/
        UnicodeCaseFolding.txt
        UnicodeGeneralCategory.txt
        UnicodeNFKD.txt
        UnicodeUppercase.txt
        UnicodeLowercase.txt)

add_executable(prepare_unicode_data ${UNICODE_DIR}/prepare-unicode-data.cpp)
add_executable(generate_unicode_utils ${UNICODE_DIR}/generate-unicode-utils.cpp)

file(MAKE_DIRECTORY ${UNICODE_AUTO_DATA_DIR} ${UNICODE_AUTO_RAW_DATA_DIR})

add_custom_command(
        OUTPUT ${UNICODE_DATA_LIST}
        DEPENDS ${UNICODE_DIR}/raw_data/unicode-data.tgz
        COMMAND tar xzf ${UNICODE_DIR}/raw_data/unicode-data.tgz -C ${UNICODE_AUTO_RAW_DATA_DIR}/
        COMMAND prepare_unicode_data ${UNICODE_AUTO_RAW_DATA_DIR}/CaseFolding.txt ${UNICODE_AUTO_DATA_DIR}/UnicodeCaseFolding.txt
        COMMAND prepare_unicode_data ${UNICODE_AUTO_RAW_DATA_DIR}/GeneralCategory.txt ${UNICODE_AUTO_DATA_DIR}/UnicodeGeneralCategory.txt
        COMMAND prepare_unicode_data -c 0,13 ${UNICODE_AUTO_RAW_DATA_DIR}/UnicodeData.txt ${UNICODE_AUTO_DATA_DIR}/UnicodeLowercase.txt
        COMMAND prepare_unicode_data -c 0,12 ${UNICODE_AUTO_RAW_DATA_DIR}/UnicodeData.txt ${UNICODE_AUTO_DATA_DIR}/UnicodeUppercase.txt
        COMMAND prepare_unicode_data -a -c 0,5 ${UNICODE_AUTO_RAW_DATA_DIR}/UnicodeData.txt ${UNICODE_AUTO_DATA_DIR}/UnicodeNFKD.txt
        COMMENT "unicode raw data generation")

add_custom_command(
        OUTPUT ${AUTO_DIR}/common/unicode-utils-auto.h
        COMMAND generate_unicode_utils ${UNICODE_DATA_LIST} ${AUTO_DIR}/common/unicode-utils-auto.h
        DEPENDS ${UNICODE_DATA_LIST}
        COMMENT "unicode-utils-auto.h generation")

set(UNICODE_SOURCES unicode-utils.cpp utf8-utils.cpp)

if (COMPILE_RUNTIME_LIGHT)
	set(UNICODE_SOURCES_FOR_COMP "${UNICODE_SOURCES}")
	configure_file(${BASE_DIR}/compiler/unicode_sources.h.in ${AUTO_DIR}/compiler/unicode_sources.h)
endif()

prepend(UNICODE_SOURCES ${UNICODE_DIR}/ ${UNICODE_SOURCES})

if (COMPILE_RUNTIME_LIGHT)
	vk_add_library(light-unicode OBJECT ${UNICODE_SOURCES} ${AUTO_DIR}/common/unicode-utils-auto.h)
	set_property(TARGET light-unicode PROPERTY POSITION_INDEPENDENT_CODE ON)

	target_compile_options(light-unicode PUBLIC -stdlib=libc++ -fPIC)
	target_link_options(light-unicode PUBLIC -stdlib=libc++)
endif()

vk_add_library(unicode OBJECT ${UNICODE_SOURCES} ${AUTO_DIR}/common/unicode-utils-auto.h)
