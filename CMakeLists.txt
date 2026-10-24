cmake_minimum_required(VERSION 3.16)
project(PCD4 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_library(pcd4lib
    src/common.c
    src/species.c
    src/text.c
    src/pk4.c
    src/pcd.c
)
target_include_directories(pcd4lib PUBLIC include)

add_executable(pcd4 src/cli.c)
target_link_libraries(pcd4 PRIVATE pcd4lib)

enable_testing()

add_executable(tests tests/test_main.c)
target_link_libraries(tests PRIVATE pcd4lib)
add_test(NAME tests COMMAND tests)

add_test(
    NAME cli_create
    COMMAND pcd4 create
            --type member-card
            --title "Member Card"
            --description "Members can enter the Harbor Inn."
            --card-id 18
            --games d,pt
            --redistribution 255
            --icons darkrai,none,dialga
            --received 2009-08-03
            --output cli_test.pcd
            --force
)
set_tests_properties(cli_create PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PASS_REGULAR_EXPRESSION "wrote cli_test.pcd"
)

add_test(NAME cli_validate COMMAND pcd4 validate cli_test.pcd)
set_tests_properties(cli_validate PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS cli_create
    PASS_REGULAR_EXPRESSION "valid"
)

add_test(NAME cli_inspect COMMAND pcd4 inspect cli_test.pcd)
set_tests_properties(cli_inspect PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS cli_create
    PASS_REGULAR_EXPRESSION "Icons: Darkrai, None, Dialga"
)

add_test(
    NAME cli_edit
    COMMAND pcd4 edit cli_test.pcd
            --title "Edited Card"
            --description "Edited description."
            --games pt,hg
            --icons mewtwo,mew,chikorita
            --output cli_edited.pcd
            --force
)
set_tests_properties(cli_edit PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS cli_create
    PASS_REGULAR_EXPRESSION "wrote cli_edited.pcd"
)

add_test(NAME cli_edit_inspect COMMAND pcd4 inspect cli_edited.pcd)
set_tests_properties(cli_edit_inspect PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS cli_edit
    PASS_REGULAR_EXPRESSION "Title: Edited Card"
)
