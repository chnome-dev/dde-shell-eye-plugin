cmake_minimum_required(VERSION 3.16)
project(cartoon-eye-win VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network TextToSpeech)

add_executable(CartoonEye WIN32
    main.cpp
    learncore.h learncore.cpp
    eyewindow.h eyewindow.cpp
    learnwindow.h learnwindow.cpp
)

target_link_libraries(CartoonEye PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::TextToSpeech)

# Windows 资源（图标）
if(WIN32)
    target_sources(CartoonEye PRIVATE app.rc)
endif()

set_target_properties(CartoonEye PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "CartoonEye"
)
