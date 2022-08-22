option(TRV_TEST_MULTITHREADED "Enable multithreading within the filtering logic" OFF)

string(COMPARE EQUAL ${CMAKE_PROJECT_NAME} ${PROJECT_NAME} being_built)

if(${being_built})
    set(default_val ON)
else()
    set(default_val OFF)
endif()

option(PACKAGE_TESTS "Specifies whether to build and run tests" ${default_val})

if(${PACKAGE_TESTS})
    set(INSTALL_GTEST OFF CACHE BOOL "Install GTest for the unit tests")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(test_files TestZlib.cpp TestImage.cpp TestFilter.cpp)

    enable_testing()

    add_executable(pngreader_test ${test_files})
    target_include_directories(pngreader_test PUBLIC ${PROJECT_SOURCE_DIR}/include)

    if(${TRV_TEST_MULTITHREADED})
        target_compile_definitions(pngreader_test PUBLIC TRV_TEST_MULTITHREADED)
    endif()

    target_link_libraries(pngreader_test gtest_main pngreader_static)

    install(TARGETS pngreader_test DESTINATION test)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/test/samples DESTINATION test/)

    if(MSVC)
        # Install pdb files when available
        install(FILES $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWIthDebInfo>>:$<TARGET_PDB_FILE:pngreader_test>> DESTINATION test)
    endif()

    include(GoogleTest)
    gtest_discover_tests(pngreader_test)
endif()
