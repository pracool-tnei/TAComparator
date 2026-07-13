cmake_minimum_required(VERSION 3.19)
project(TAComparator LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(TAComparator
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    src/model/Metadata.h src/model/ObjectGroup.h src/model/Signal.h src/model/Study.h src/model/Component.h
    src/parser/DiscoveryParser.h src/parser/ItfParser.h src/parser/PopulationParser.h
    src/parser/DiscoveryParser.cpp src/parser/ItfParser.cpp src/parser/PopulationParser.cpp

    src/reader/TextFileReader.h
    src/reader/TextFileReader.cpp
    src/model/ObjectGroup.cpp src/model/Metadata.h src/model/Metadata.cpp src/model/Component.h src/model/Component.cpp
    src/model/Study.cpp src/model/Signal.cpp
)

qt_add_translations(
    TARGETS TAComparator
    TS_FILES TAComparator_en_AS.ts
)

target_link_libraries(TAComparator
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS TAComparator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET TAComparator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
