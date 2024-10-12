
# USE THIS AS AN EXAMPLE OF HOW TO ADD THE CMakeLists.txt FILE FOR NEW CUSTOM COMPONENTS
# ADDING IN A "COMPONENT" FOLDER WITH CUSTOM CODE IN HERE MEANS THE "MAIN" FOLDER NEEDS TO "REQUIRE" THE NAME OF THE COMPONENT FOLDER IN IT'S OWN CMakeLists.txt FILE

idf_component_register(
    SRCS "src/test.c"       # Add all source files here
    INCLUDE_DIRS "include"            # Include header files
    # REQUIRES spi_bus other_libs       # List other components required by this component
)
