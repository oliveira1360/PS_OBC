set(DEPENDENT_MP_BIN2HEXPS_OBC_default_1DoWoj8N "c:/Program Files/Microchip/xc32/v5.10/bin/xc32-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFPS_OBC_default_1DoWoj8N ${CMAKE_CURRENT_LIST_DIR}/../../../../out/PS_OBC/default.elf)
set(DEPENDENT_TARGET_DIRPS_OBC_default_1DoWoj8N ${CMAKE_CURRENT_LIST_DIR}/../../../../out/PS_OBC)
set(DEPENDENT_BYPRODUCTSPS_OBC_default_1DoWoj8N ${DEPENDENT_TARGET_DIRPS_OBC_default_1DoWoj8N}/${sourceFileNamePS_OBC_default_1DoWoj8N}.c)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRPS_OBC_default_1DoWoj8N}/${sourceFileNamePS_OBC_default_1DoWoj8N}.c
    COMMAND ${DEPENDENT_MP_BIN2HEXPS_OBC_default_1DoWoj8N} --image ${DEPENDENT_DEPENDENT_TARGET_ELFPS_OBC_default_1DoWoj8N} --image-generated-c ${sourceFileNamePS_OBC_default_1DoWoj8N}.c --image-generated-h ${sourceFileNamePS_OBC_default_1DoWoj8N}.h --image-copy-mode ${modePS_OBC_default_1DoWoj8N} --image-offset ${addressPS_OBC_default_1DoWoj8N} 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRPS_OBC_default_1DoWoj8N}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFPS_OBC_default_1DoWoj8N})
add_custom_target(
    dependent_produced_source_artifactPS_OBC_default_1DoWoj8N 
    DEPENDS ${DEPENDENT_TARGET_DIRPS_OBC_default_1DoWoj8N}/${sourceFileNamePS_OBC_default_1DoWoj8N}.c
    )
