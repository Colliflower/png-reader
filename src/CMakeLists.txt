set(src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Zlib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Chunk.cpp
)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

add_library(pngreader_static STATIC ${src_files})
add_library(pngreader SHARED ${src_files})

set(warnings $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_compile_options(pngreader_static PRIVATE ${warnings})

target_compile_options(pngreader PRIVATE ${warnings})

target_include_directories(pngreader_static PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(pngreader PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_definitions(pngreader_static PUBLIC BUILDING_DLL)
target_compile_definitions(pngreader PUBLIC BUILDING_DLL)

install(TARGETS pngreader_static DESTINATION lib)

install(TARGETS pngreader DESTINATION lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION .)

if(MSVC)
    # Install pdb files when available
    install(FILES $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWIthDebInfo>>:$<TARGET_PDB_FILE:pngreader>> DESTINATION lib)
    set_target_properties(pngreader_static PROPERTIES
        COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG ${CMAKE_INSTALL_PREFIX}/lib
        COMPILE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_INSTALL_PREFIX}/lib
        COMPILE_PDB_NAME pngreader_static
    )
endif()
