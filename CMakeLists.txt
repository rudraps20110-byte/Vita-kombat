cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your VitaSDK path.")
    endif()
endif()

project(VitaKombat LANGUAGES CXX C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Vita Kombat")
set(VITA_TITLEID  "VKMB00001")
set(VITA_VERSION  "01.00")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -s CONTENT_ID=HB0001-${VITA_TITLEID}_00-0000000000000000")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/characters.cpp
    src/audio.cpp
    src/combat.cpp
    src/fighter_gfx.cpp
    src/story.cpp
    src/renderer.cpp
)

target_link_libraries(${PROJECT_NAME}
    vita2d
    freetype
    png
    jpeg
    z
    m
    SceAppMgr_stub
    SceAudio_stub
    ScePgf_stub
    ScePvf_stub
    SceGxm_stub
    SceDisplay_stub
    SceCtrl_stub
    SceCommonDialog_stub
    SceSysmodule_stub
)

target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--defsym=__sce_headroom=0x10000")

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME "${VITA_APP_NAME}"
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/keyart.png assets/keyart.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_lava.png assets/stages/stage_lava.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_ice.png assets/stages/stage_ice.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_storm.png assets/stages/stage_storm.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_nether.png assets/stages/stage_nether.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_dojo.png assets/stages/stage_dojo.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_cyber.png assets/stages/stage_cyber.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_solar.png assets/stages/stage_solar.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_abyss.png assets/stages/stage_abyss.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_jade.png assets/stages/stage_jade.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_coliseum.png assets/stages/stage_coliseum.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_clockwork.png assets/stages/stage_clockwork.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/stages/stage_nexus.png assets/stages/stage_nexus.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_0.png assets/story/art_0.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_1.png assets/story/art_1.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_2.png assets/story/art_2.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_3.png assets/story/art_3.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_4.png assets/story/art_4.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_5.png assets/story/art_5.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_6.png assets/story/art_6.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_7.png assets/story/art_7.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_8.png assets/story/art_8.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_9.png assets/story/art_9.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_10.png assets/story/art_10.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/story/art_11.png assets/story/art_11.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_0.png assets/portraits/char_0.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_1.png assets/portraits/char_1.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_2.png assets/portraits/char_2.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_3.png assets/portraits/char_3.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_4.png assets/portraits/char_4.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_5.png assets/portraits/char_5.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_6.png assets/portraits/char_6.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_7.png assets/portraits/char_7.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_8.png assets/portraits/char_8.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_9.png assets/portraits/char_9.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_10.png assets/portraits/char_10.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_11.png assets/portraits/char_11.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_12.png assets/portraits/char_12.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_13.png assets/portraits/char_13.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_14.png assets/portraits/char_14.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_15.png assets/portraits/char_15.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_16.png assets/portraits/char_16.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_17.png assets/portraits/char_17.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_18.png assets/portraits/char_18.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_19.png assets/portraits/char_19.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_20.png assets/portraits/char_20.png
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/portraits/char_21.png assets/portraits/char_21.png
)

