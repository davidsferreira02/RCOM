cmake_minimum_required(VERSION 3.23)
project(RCOM_FINAL_main C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(RCOM_FINAL_main
        src/E_Application_layer.c
        include/E_Application_layer.h
        src/E_link_layer.c
        include/E_link_layer.h
        src/E_main.c
        src/R_main.c
        src/R_Application_layer.c
        include/R_Application_layer.h
        src/R_link_layer.c
        include/R_link_layer.h)
