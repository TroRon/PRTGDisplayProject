cmake_minimum_required(VERSION 3.16)
set(PROJECT_VER "1.0.3")
# Public releases must not embed the developer's absolute filesystem paths.
if(DEFINED PUBLIC_SOURCE_ROOT)
    add_compile_options("-ffile-prefix-map=${PUBLIC_SOURCE_ROOT}=." "-fmacro-prefix-map=${PUBLIC_SOURCE_ROOT}=.")
endif()
add_compile_options("-ffile-prefix-map=W:/=." "-fmacro-prefix-map=W:/=.")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(eaglenet_lcd5b)
# Hardware-only LVGL allocator. Simulator sources and lv_conf.h stay unchanged.
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_include_directories(${lvgl_lib} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/main")
target_compile_definitions(${lvgl_lib} PRIVATE
    LV_MEM_CUSTOM_ALLOC=panel_lvgl_alloc
    LV_MEM_CUSTOM_REALLOC=panel_lvgl_realloc
    LV_MEM_CUSTOM_FREE=panel_lvgl_free)
