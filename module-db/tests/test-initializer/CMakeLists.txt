add_catch2_executable(
    NAME
        db-initializer
    SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/unittest.cpp"
        
    LIBS
        module-sys
        module-db
    
    USE_FS
)
