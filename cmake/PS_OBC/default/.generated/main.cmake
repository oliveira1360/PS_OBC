include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(PS_OBC_default_library_list )

# Handle files with suffix s, for group default-XC32
if(PS_OBC_default_default_XC32_FILE_TYPE_assemble)
add_library(PS_OBC_default_default_XC32_assemble OBJECT ${PS_OBC_default_default_XC32_FILE_TYPE_assemble})
    PS_OBC_default_default_XC32_assemble_rule(PS_OBC_default_default_XC32_assemble)
    list(APPEND PS_OBC_default_library_list "$<TARGET_OBJECTS:PS_OBC_default_default_XC32_assemble>")

endif()

# Handle files with suffix S, for group default-XC32
if(PS_OBC_default_default_XC32_FILE_TYPE_assembleWithPreprocess)
add_library(PS_OBC_default_default_XC32_assembleWithPreprocess OBJECT ${PS_OBC_default_default_XC32_FILE_TYPE_assembleWithPreprocess})
    PS_OBC_default_default_XC32_assembleWithPreprocess_rule(PS_OBC_default_default_XC32_assembleWithPreprocess)
    list(APPEND PS_OBC_default_library_list "$<TARGET_OBJECTS:PS_OBC_default_default_XC32_assembleWithPreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC32
if(PS_OBC_default_default_XC32_FILE_TYPE_compile)
add_library(PS_OBC_default_default_XC32_compile OBJECT ${PS_OBC_default_default_XC32_FILE_TYPE_compile})
    PS_OBC_default_default_XC32_compile_rule(PS_OBC_default_default_XC32_compile)
    list(APPEND PS_OBC_default_library_list "$<TARGET_OBJECTS:PS_OBC_default_default_XC32_compile>")

endif()

# Handle files with suffix cpp, for group default-XC32
if(PS_OBC_default_default_XC32_FILE_TYPE_compile_cpp)
add_library(PS_OBC_default_default_XC32_compile_cpp OBJECT ${PS_OBC_default_default_XC32_FILE_TYPE_compile_cpp})
    PS_OBC_default_default_XC32_compile_cpp_rule(PS_OBC_default_default_XC32_compile_cpp)
    list(APPEND PS_OBC_default_library_list "$<TARGET_OBJECTS:PS_OBC_default_default_XC32_compile_cpp>")

endif()

# Handle files with suffix [cC], for group default-XC32
if(PS_OBC_default_default_XC32_FILE_TYPE_dependentObject)
add_library(PS_OBC_default_default_XC32_dependentObject OBJECT ${PS_OBC_default_default_XC32_FILE_TYPE_dependentObject})
    PS_OBC_default_default_XC32_dependentObject_rule(PS_OBC_default_default_XC32_dependentObject)
    list(APPEND PS_OBC_default_library_list "$<TARGET_OBJECTS:PS_OBC_default_default_XC32_dependentObject>")

endif()


# Main target for this project
add_executable(PS_OBC_default_image_1DoWoj8N ${PS_OBC_default_library_list})

set_target_properties(PS_OBC_default_image_1DoWoj8N PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${PS_OBC_default_output_dir}")
target_link_libraries(PS_OBC_default_image_1DoWoj8N PRIVATE ${PS_OBC_default_default_XC32_FILE_TYPE_link})

# Add the link options from the rule file.
PS_OBC_default_link_rule( PS_OBC_default_image_1DoWoj8N)

# Add bin2hex target for converting built file to a .hex file.
string(REGEX REPLACE [.]elf$ .hex PS_OBC_default_image_name_hex ${PS_OBC_default_image_name})
add_custom_target(PS_OBC_default_Bin2Hex ALL
    COMMAND ${MP_BIN2HEX} \"${PS_OBC_default_output_dir}/${PS_OBC_default_image_name}\"
    BYPRODUCTS ${PS_OBC_default_output_dir}/${PS_OBC_default_image_name_hex}
    COMMENT "Convert built file to .hex")
add_dependencies(PS_OBC_default_Bin2Hex PS_OBC_default_image_1DoWoj8N)



