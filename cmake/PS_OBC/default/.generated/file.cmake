# The following variables contains the files used by the different stages of the build process.
set(PS_OBC_default_default_XC32_FILE_TYPE_assemble)
set_source_files_properties(${PS_OBC_default_default_XC32_FILE_TYPE_assemble} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${PS_OBC_default_default_XC32_FILE_TYPE_assemble})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(PS_OBC_default_default_XC32_FILE_TYPE_assembleWithPreprocess)
set_source_files_properties(${PS_OBC_default_default_XC32_FILE_TYPE_assembleWithPreprocess} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${PS_OBC_default_default_XC32_FILE_TYPE_assembleWithPreprocess})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(PS_OBC_default_default_XC32_FILE_TYPE_compile
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app/init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app/mission.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app/modeSelecter.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app/sensors.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/app/stateCheck.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/drivers/i2c_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/drivers/qspi_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/drivers/spi_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/drivers/usart_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_i2c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_peripherals_init.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_qspi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_spi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_system.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hal/hal_usart.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/eps.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/flexiforce.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/gnss.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/imu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/pressure.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/propulsor.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/temperature.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/peripherals/ttc.c")
set_source_files_properties(${PS_OBC_default_default_XC32_FILE_TYPE_compile} PROPERTIES LANGUAGE C)
set(PS_OBC_default_default_XC32_FILE_TYPE_compile_cpp)
set_source_files_properties(${PS_OBC_default_default_XC32_FILE_TYPE_compile_cpp} PROPERTIES LANGUAGE CXX)
set(PS_OBC_default_default_XC32_FILE_TYPE_link)
set(PS_OBC_default_image_name "default.elf")
set(PS_OBC_default_image_base_name "default")

# The output directory of the final image.
set(PS_OBC_default_output_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../out/PS_OBC")

# The full path to the final image.
set(PS_OBC_default_full_path_to_image ${PS_OBC_default_output_dir}/${PS_OBC_default_image_name})
