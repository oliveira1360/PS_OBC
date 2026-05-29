#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../bootloader/src/bootloader.c ../bootloader/src/flash_efc.c ../bootloader/src/main.c ../bootloader/src/ota_verify.c ../bootloader/src/qspi_boot.c ../bootloader/src/startup_samv71.c ../bootloader/src/system_samv71.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1191379593/bootloader.o ${OBJECTDIR}/_ext/1191379593/flash_efc.o ${OBJECTDIR}/_ext/1191379593/main.o ${OBJECTDIR}/_ext/1191379593/ota_verify.o ${OBJECTDIR}/_ext/1191379593/qspi_boot.o ${OBJECTDIR}/_ext/1191379593/startup_samv71.o ${OBJECTDIR}/_ext/1191379593/system_samv71.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1191379593/bootloader.o.d ${OBJECTDIR}/_ext/1191379593/flash_efc.o.d ${OBJECTDIR}/_ext/1191379593/main.o.d ${OBJECTDIR}/_ext/1191379593/ota_verify.o.d ${OBJECTDIR}/_ext/1191379593/qspi_boot.o.d ${OBJECTDIR}/_ext/1191379593/startup_samv71.o.d ${OBJECTDIR}/_ext/1191379593/system_samv71.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1191379593/bootloader.o ${OBJECTDIR}/_ext/1191379593/flash_efc.o ${OBJECTDIR}/_ext/1191379593/main.o ${OBJECTDIR}/_ext/1191379593/ota_verify.o ${OBJECTDIR}/_ext/1191379593/qspi_boot.o ${OBJECTDIR}/_ext/1191379593/startup_samv71.o ${OBJECTDIR}/_ext/1191379593/system_samv71.o

# Source Files
SOURCEFILES=../bootloader/src/bootloader.c ../bootloader/src/flash_efc.c ../bootloader/src/main.c ../bootloader/src/ota_verify.c ../bootloader/src/qspi_boot.c ../bootloader/src/startup_samv71.c ../bootloader/src/system_samv71.c

# Pack Options 
PACK_COMMON_OPTIONS=-I "${CMSIS_DIR}/CMSIS/Core/Include"



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=ATSAMV71Q21B
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1191379593/bootloader.o: ../bootloader/src/bootloader.c  .generated_files/flags/default/f05805aa82e60f19815bfb9dac0456890b8d2f48 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/bootloader.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/bootloader.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/bootloader.o.d" -o ${OBJECTDIR}/_ext/1191379593/bootloader.o ../bootloader/src/bootloader.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/flash_efc.o: ../bootloader/src/flash_efc.c  .generated_files/flags/default/60f67773b46e7f8f84b5d1ef70ba56c7668bb3b2 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/flash_efc.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/flash_efc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/flash_efc.o.d" -o ${OBJECTDIR}/_ext/1191379593/flash_efc.o ../bootloader/src/flash_efc.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/main.o: ../bootloader/src/main.c  .generated_files/flags/default/f7886d2ccdd534091c146a983df6122b5ebbc3ad .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/main.o.d" -o ${OBJECTDIR}/_ext/1191379593/main.o ../bootloader/src/main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/ota_verify.o: ../bootloader/src/ota_verify.c  .generated_files/flags/default/b2419d00d156a3d77267cf31e2fc886885d885d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/ota_verify.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/ota_verify.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/ota_verify.o.d" -o ${OBJECTDIR}/_ext/1191379593/ota_verify.o ../bootloader/src/ota_verify.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/qspi_boot.o: ../bootloader/src/qspi_boot.c  .generated_files/flags/default/c453149f69dc758f2a8f38a08bce15ae44c226b3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/qspi_boot.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/qspi_boot.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/qspi_boot.o.d" -o ${OBJECTDIR}/_ext/1191379593/qspi_boot.o ../bootloader/src/qspi_boot.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/startup_samv71.o: ../bootloader/src/startup_samv71.c  .generated_files/flags/default/52da40cb2a4d22314bddf11d4dcb6e5dff3161ba .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/startup_samv71.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/startup_samv71.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/startup_samv71.o.d" -o ${OBJECTDIR}/_ext/1191379593/startup_samv71.o ../bootloader/src/startup_samv71.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/system_samv71.o: ../bootloader/src/system_samv71.c  .generated_files/flags/default/46eb5f957836dd7d16b2950c22c294b4341ad83e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/system_samv71.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/system_samv71.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/system_samv71.o.d" -o ${OBJECTDIR}/_ext/1191379593/system_samv71.o ../bootloader/src/system_samv71.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
else
${OBJECTDIR}/_ext/1191379593/bootloader.o: ../bootloader/src/bootloader.c  .generated_files/flags/default/6ae7d33580712d3a8e3e656ff518eaa0e8089900 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/bootloader.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/bootloader.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/bootloader.o.d" -o ${OBJECTDIR}/_ext/1191379593/bootloader.o ../bootloader/src/bootloader.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/flash_efc.o: ../bootloader/src/flash_efc.c  .generated_files/flags/default/472c408036e74c9392aa34697c61a68c7dc76d53 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/flash_efc.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/flash_efc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/flash_efc.o.d" -o ${OBJECTDIR}/_ext/1191379593/flash_efc.o ../bootloader/src/flash_efc.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/main.o: ../bootloader/src/main.c  .generated_files/flags/default/6c546e854b0406c0738017cd0c5532894e58af7c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/main.o.d" -o ${OBJECTDIR}/_ext/1191379593/main.o ../bootloader/src/main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/ota_verify.o: ../bootloader/src/ota_verify.c  .generated_files/flags/default/52be5c25eb8b0f6c38ca1d03372d51a472347ed1 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/ota_verify.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/ota_verify.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/ota_verify.o.d" -o ${OBJECTDIR}/_ext/1191379593/ota_verify.o ../bootloader/src/ota_verify.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/qspi_boot.o: ../bootloader/src/qspi_boot.c  .generated_files/flags/default/3b5567a8ce228b29e9c45dd05c03ba9142bb381f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/qspi_boot.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/qspi_boot.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/qspi_boot.o.d" -o ${OBJECTDIR}/_ext/1191379593/qspi_boot.o ../bootloader/src/qspi_boot.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/startup_samv71.o: ../bootloader/src/startup_samv71.c  .generated_files/flags/default/116e68abdfe6bc4cd6df8054691081954ea716db .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/startup_samv71.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/startup_samv71.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/startup_samv71.o.d" -o ${OBJECTDIR}/_ext/1191379593/startup_samv71.o ../bootloader/src/startup_samv71.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/_ext/1191379593/system_samv71.o: ../bootloader/src/system_samv71.c  .generated_files/flags/default/52d18190f2008d5d419d29df2bf3140953f2e738 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/_ext/1191379593" 
	@${RM} ${OBJECTDIR}/_ext/1191379593/system_samv71.o.d 
	@${RM} ${OBJECTDIR}/_ext/1191379593/system_samv71.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"../bootloader/inc" -MP -MMD -MF "${OBJECTDIR}/_ext/1191379593/system_samv71.o.d" -o ${OBJECTDIR}/_ext/1191379593/system_samv71.o ../bootloader/src/system_samv71.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -T ../bootloader/linker/samv71q21_boot.ld -o ${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=_min_heap_size=0,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samv71b"
	
else
${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    ../../dist/default/production/PS_OBC.X.production.hex
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -T ../bootloader/linker/samv71q21_boot.ld -o ${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=_min_heap_size=0,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samv71b"
	${MP_CC_DIR}\\xc32-bin2hex ${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
	@echo "Creating unified hex file"
	@"C:/Program Files/Microchip/MPLABX/v6.30/mplab_platform/platform/../mplab_ide/modules/../../bin/hexmate" --edf="C:/Program Files/Microchip/MPLABX/v6.30/mplab_platform/platform/../mplab_ide/modules/../../dat/en_msgs.txt" ${DISTDIR}/PS_OBC_BOOT.X.${IMAGE_TYPE}.hex ../../dist/default/production/PS_OBC.X.production.hex -odist/${CND_CONF}/production/PS_OBC_BOOT.X.production.unified.hex

endif


# Subprojects
.build-subprojects:
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
	cd ../.. && ${MAKE}  -f Makefile CONF=default TYPE_IMAGE=DEBUG_RUN
else
	cd ../.. && ${MAKE}  -f Makefile CONF=default
endif


# Subprojects
.clean-subprojects:
	cd ../.. && rm -rf "build/default" "dist/default"

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
