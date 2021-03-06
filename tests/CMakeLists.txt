set(TESTS
    Animation
    Archive
    AudioLoading
    Buoyancy
    Character
    Chase
    Config
    Cutscene
    Data
    FileIndex
    GameData
    GameWorld
    Garage
    HitTest
    Input
    Items
    Lifetime
    LoaderDFF
    LoaderIDE
    LoaderIPL
    Logger
    Menu
    Object
    Payphone
    Pickup
    Renderer
    RWBStream
    SaveGame
    ScriptMachine
    State
    StringEncoding
    Sound
    Text
    TrafficDirector
    Vehicle
    VisualFX
    Weapon
    World
    ZoneData
    )

set(TEST_SOURCES
    main.cpp
    test_Globals.cpp
    test_Globals.hpp
    )

foreach(TEST ${TESTS})
    list(APPEND TEST_SOURCES "test_${TEST}.cpp")
endforeach()

add_executable(rwtests
    ${TEST_SOURCES}
    )

target_include_directories(rwtests
    PRIVATE
        "${PROJECT_SOURCE_DIR}/tests"
        "${PROJECT_SOURCE_DIR}/rwgame"
    )

target_link_libraries(rwtests
    PRIVATE
        Boost::unit_test_framework
        librwgame
    )

openrw_target_apply_options(
    TARGET rwtests
    CORE
    COVERAGE
    INSTALL INSTALL_PDB
    )

if(SEPARATE_TEST_SUITES)
    foreach(TEST ${TESTS})
        add_test(
            NAME "${TEST}"
            COMMAND "$<TARGET_FILE:rwtests>" "-t" "${TEST}Tests"
            )
        set_tests_properties("${TEST}"
            PROPERTIES
                TIMEOUT 300
            )
    endforeach()
else()
    add_test(NAME UnitTests
        COMMAND "$<TARGET_FILE:rwtests>"
        )
    set_tests_properties(UnitTests
        PROPERTIES
            TIMEOUT 300
        )
endif()

if(TEST_DATA)
    add_test(NAME DataTests
            COMMAND "$<TARGET_FILE:rwtests>" "--run_test=@data-test"
        )
endif()
