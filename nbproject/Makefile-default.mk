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
FINAL_IMAGE=${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=src/app/init.c src/app/mission.c src/app/modeSelecter.c src/app/sensors.c src/app/stateCheck.c src/drivers/i2c_driver.c src/drivers/qspi_driver.c src/drivers/spi_driver.c src/drivers/usart_driver.c src/hal/hal_gpio.c src/hal/hal_i2c.c src/hal/hal_peripherals_init.c src/hal/hal_qspi.c src/hal/hal_spi.c src/hal/hal_system.c src/hal/hal_usart.c src/hal/hal_debug_uart.c src/hal/hal_systick.c src/peripherals/eps.c src/peripherals/flexiforce.c src/peripherals/gnss.c src/peripherals/imu.c src/peripherals/pressure.c src/peripherals/propulsor.c src/peripherals/temperature.c src/peripherals/ttc.c src/peripherals/ext_memory.c src/main.c src/testsForBoard/deterministic_test.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/src/app/init.o ${OBJECTDIR}/src/app/mission.o ${OBJECTDIR}/src/app/modeSelecter.o ${OBJECTDIR}/src/app/sensors.o ${OBJECTDIR}/src/app/stateCheck.o ${OBJECTDIR}/src/drivers/i2c_driver.o ${OBJECTDIR}/src/drivers/qspi_driver.o ${OBJECTDIR}/src/drivers/spi_driver.o ${OBJECTDIR}/src/drivers/usart_driver.o ${OBJECTDIR}/src/hal/hal_gpio.o ${OBJECTDIR}/src/hal/hal_i2c.o ${OBJECTDIR}/src/hal/hal_peripherals_init.o ${OBJECTDIR}/src/hal/hal_qspi.o ${OBJECTDIR}/src/hal/hal_spi.o ${OBJECTDIR}/src/hal/hal_system.o ${OBJECTDIR}/src/hal/hal_usart.o ${OBJECTDIR}/src/hal/hal_debug_uart.o ${OBJECTDIR}/src/hal/hal_systick.o ${OBJECTDIR}/src/peripherals/eps.o ${OBJECTDIR}/src/peripherals/flexiforce.o ${OBJECTDIR}/src/peripherals/gnss.o ${OBJECTDIR}/src/peripherals/imu.o ${OBJECTDIR}/src/peripherals/pressure.o ${OBJECTDIR}/src/peripherals/propulsor.o ${OBJECTDIR}/src/peripherals/temperature.o ${OBJECTDIR}/src/peripherals/ttc.o ${OBJECTDIR}/src/peripherals/ext_memory.o ${OBJECTDIR}/src/main.o ${OBJECTDIR}/src/testsForBoard/deterministic_test.o
POSSIBLE_DEPFILES=${OBJECTDIR}/src/app/init.o.d ${OBJECTDIR}/src/app/mission.o.d ${OBJECTDIR}/src/app/modeSelecter.o.d ${OBJECTDIR}/src/app/sensors.o.d ${OBJECTDIR}/src/app/stateCheck.o.d ${OBJECTDIR}/src/drivers/i2c_driver.o.d ${OBJECTDIR}/src/drivers/qspi_driver.o.d ${OBJECTDIR}/src/drivers/spi_driver.o.d ${OBJECTDIR}/src/drivers/usart_driver.o.d ${OBJECTDIR}/src/hal/hal_gpio.o.d ${OBJECTDIR}/src/hal/hal_i2c.o.d ${OBJECTDIR}/src/hal/hal_peripherals_init.o.d ${OBJECTDIR}/src/hal/hal_qspi.o.d ${OBJECTDIR}/src/hal/hal_spi.o.d ${OBJECTDIR}/src/hal/hal_system.o.d ${OBJECTDIR}/src/hal/hal_usart.o.d ${OBJECTDIR}/src/hal/hal_debug_uart.o.d ${OBJECTDIR}/src/hal/hal_systick.o.d ${OBJECTDIR}/src/peripherals/eps.o.d ${OBJECTDIR}/src/peripherals/flexiforce.o.d ${OBJECTDIR}/src/peripherals/gnss.o.d ${OBJECTDIR}/src/peripherals/imu.o.d ${OBJECTDIR}/src/peripherals/pressure.o.d ${OBJECTDIR}/src/peripherals/propulsor.o.d ${OBJECTDIR}/src/peripherals/temperature.o.d ${OBJECTDIR}/src/peripherals/ttc.o.d ${OBJECTDIR}/src/peripherals/ext_memory.o.d ${OBJECTDIR}/src/main.o.d ${OBJECTDIR}/src/testsForBoard/deterministic_test.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/src/app/init.o ${OBJECTDIR}/src/app/mission.o ${OBJECTDIR}/src/app/modeSelecter.o ${OBJECTDIR}/src/app/sensors.o ${OBJECTDIR}/src/app/stateCheck.o ${OBJECTDIR}/src/drivers/i2c_driver.o ${OBJECTDIR}/src/drivers/qspi_driver.o ${OBJECTDIR}/src/drivers/spi_driver.o ${OBJECTDIR}/src/drivers/usart_driver.o ${OBJECTDIR}/src/hal/hal_gpio.o ${OBJECTDIR}/src/hal/hal_i2c.o ${OBJECTDIR}/src/hal/hal_peripherals_init.o ${OBJECTDIR}/src/hal/hal_qspi.o ${OBJECTDIR}/src/hal/hal_spi.o ${OBJECTDIR}/src/hal/hal_system.o ${OBJECTDIR}/src/hal/hal_usart.o ${OBJECTDIR}/src/hal/hal_debug_uart.o ${OBJECTDIR}/src/hal/hal_systick.o ${OBJECTDIR}/src/peripherals/eps.o ${OBJECTDIR}/src/peripherals/flexiforce.o ${OBJECTDIR}/src/peripherals/gnss.o ${OBJECTDIR}/src/peripherals/imu.o ${OBJECTDIR}/src/peripherals/pressure.o ${OBJECTDIR}/src/peripherals/propulsor.o ${OBJECTDIR}/src/peripherals/temperature.o ${OBJECTDIR}/src/peripherals/ttc.o ${OBJECTDIR}/src/peripherals/ext_memory.o ${OBJECTDIR}/src/main.o ${OBJECTDIR}/src/testsForBoard/deterministic_test.o

# Source Files
SOURCEFILES=src/app/init.c src/app/mission.c src/app/modeSelecter.c src/app/sensors.c src/app/stateCheck.c src/drivers/i2c_driver.c src/drivers/qspi_driver.c src/drivers/spi_driver.c src/drivers/usart_driver.c src/hal/hal_gpio.c src/hal/hal_i2c.c src/hal/hal_peripherals_init.c src/hal/hal_qspi.c src/hal/hal_spi.c src/hal/hal_system.c src/hal/hal_usart.c src/hal/hal_debug_uart.c src/hal/hal_systick.c src/peripherals/eps.c src/peripherals/flexiforce.c src/peripherals/gnss.c src/peripherals/imu.c src/peripherals/pressure.c src/peripherals/propulsor.c src/peripherals/temperature.c src/peripherals/ttc.c src/peripherals/ext_memory.c src/main.c src/testsForBoard/deterministic_test.c

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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

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
${OBJECTDIR}/src/app/init.o: src/app/init.c  .generated_files/flags/default/7c572c48657dfbf2f5fc5feb664ae293be05c994 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/init.o.d 
	@${RM} ${OBJECTDIR}/src/app/init.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/init.o.d" -o ${OBJECTDIR}/src/app/init.o src/app/init.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/mission.o: src/app/mission.c  .generated_files/flags/default/792ca1ae7ea9b523544e7cc390efea6405030fb9 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/mission.o.d 
	@${RM} ${OBJECTDIR}/src/app/mission.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/mission.o.d" -o ${OBJECTDIR}/src/app/mission.o src/app/mission.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/modeSelecter.o: src/app/modeSelecter.c  .generated_files/flags/default/b75cc861d65118818537128a356b895c85d5ac28 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/modeSelecter.o.d 
	@${RM} ${OBJECTDIR}/src/app/modeSelecter.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/modeSelecter.o.d" -o ${OBJECTDIR}/src/app/modeSelecter.o src/app/modeSelecter.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/sensors.o: src/app/sensors.c  .generated_files/flags/default/4411995d2d95fb276d5242c54d603e4c3117e787 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/sensors.o.d 
	@${RM} ${OBJECTDIR}/src/app/sensors.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/sensors.o.d" -o ${OBJECTDIR}/src/app/sensors.o src/app/sensors.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/stateCheck.o: src/app/stateCheck.c  .generated_files/flags/default/3902e9b38910f65cb99eb5389e262ca979c0213a .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/stateCheck.o.d 
	@${RM} ${OBJECTDIR}/src/app/stateCheck.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/stateCheck.o.d" -o ${OBJECTDIR}/src/app/stateCheck.o src/app/stateCheck.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/i2c_driver.o: src/drivers/i2c_driver.c  .generated_files/flags/default/1d165d346c73d7402fd79424d9f74013000ac6c3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/i2c_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/i2c_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/i2c_driver.o.d" -o ${OBJECTDIR}/src/drivers/i2c_driver.o src/drivers/i2c_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/qspi_driver.o: src/drivers/qspi_driver.c  .generated_files/flags/default/d6494540eeac803d6f9ce44a7efb653918d4dc1f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/qspi_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/qspi_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/qspi_driver.o.d" -o ${OBJECTDIR}/src/drivers/qspi_driver.o src/drivers/qspi_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/spi_driver.o: src/drivers/spi_driver.c  .generated_files/flags/default/8f9ff95c5552835e8a4baa0e86b6e8157227bddf .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/spi_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/spi_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/spi_driver.o.d" -o ${OBJECTDIR}/src/drivers/spi_driver.o src/drivers/spi_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/usart_driver.o: src/drivers/usart_driver.c  .generated_files/flags/default/e6fcd3c1d180af821b88687b52971ec78eaeb56e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/usart_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/usart_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/usart_driver.o.d" -o ${OBJECTDIR}/src/drivers/usart_driver.o src/drivers/usart_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_gpio.o: src/hal/hal_gpio.c  .generated_files/flags/default/922dc8675186b9f1fb881ee2f693e82f445b37e5 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_gpio.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_gpio.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_gpio.o.d" -o ${OBJECTDIR}/src/hal/hal_gpio.o src/hal/hal_gpio.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_i2c.o: src/hal/hal_i2c.c  .generated_files/flags/default/fe6fabfa2eed6308988c1df62faab15775618349 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_i2c.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_i2c.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_i2c.o.d" -o ${OBJECTDIR}/src/hal/hal_i2c.o src/hal/hal_i2c.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_peripherals_init.o: src/hal/hal_peripherals_init.c  .generated_files/flags/default/a58817f447813cc8080789a842b3cf36ccfb3958 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_peripherals_init.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_peripherals_init.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_peripherals_init.o.d" -o ${OBJECTDIR}/src/hal/hal_peripherals_init.o src/hal/hal_peripherals_init.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_qspi.o: src/hal/hal_qspi.c  .generated_files/flags/default/ff91e2075f4af3384b40d6f2f28c8101e90c734f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_qspi.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_qspi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_qspi.o.d" -o ${OBJECTDIR}/src/hal/hal_qspi.o src/hal/hal_qspi.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_spi.o: src/hal/hal_spi.c  .generated_files/flags/default/75c2d28421eb4cfc371349eafaf649d14c21afd3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_spi.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_spi.o.d" -o ${OBJECTDIR}/src/hal/hal_spi.o src/hal/hal_spi.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_system.o: src/hal/hal_system.c  .generated_files/flags/default/30cd121eb90418740ebad4aa8b8c4ab57941bbd4 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_system.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_system.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_system.o.d" -o ${OBJECTDIR}/src/hal/hal_system.o src/hal/hal_system.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_usart.o: src/hal/hal_usart.c  .generated_files/flags/default/4dbb7b9a5381a49c46b6e532cf859292c2755efc .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_usart.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_usart.o.d" -o ${OBJECTDIR}/src/hal/hal_usart.o src/hal/hal_usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_debug_uart.o: src/hal/hal_debug_uart.c  .generated_files/flags/default/15d68e8b8d3af1224a9a318ff32587cafec74ca3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_debug_uart.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_debug_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_debug_uart.o.d" -o ${OBJECTDIR}/src/hal/hal_debug_uart.o src/hal/hal_debug_uart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_systick.o: src/hal/hal_systick.c  .generated_files/flags/default/a6b1f016c3c4d62e851d334cfe3956f4c6aa0969 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_systick.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_systick.o.d" -o ${OBJECTDIR}/src/hal/hal_systick.o src/hal/hal_systick.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/eps.o: src/peripherals/eps.c  .generated_files/flags/default/5fc593d8c5267e1945c25ab98c9f25338a5a928f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/eps.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/eps.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/eps.o.d" -o ${OBJECTDIR}/src/peripherals/eps.o src/peripherals/eps.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/flexiforce.o: src/peripherals/flexiforce.c  .generated_files/flags/default/8db7bccfabb35e665104701c22fe13a697a3cfe8 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/flexiforce.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/flexiforce.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/flexiforce.o.d" -o ${OBJECTDIR}/src/peripherals/flexiforce.o src/peripherals/flexiforce.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/gnss.o: src/peripherals/gnss.c  .generated_files/flags/default/d81cb3722855bab76c8dccb2b53cca267e5f7e75 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/gnss.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/gnss.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/gnss.o.d" -o ${OBJECTDIR}/src/peripherals/gnss.o src/peripherals/gnss.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/imu.o: src/peripherals/imu.c  .generated_files/flags/default/3bf047957198e59d5cd7cb3fa331a3ab9dc5ae5c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/imu.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/imu.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/imu.o.d" -o ${OBJECTDIR}/src/peripherals/imu.o src/peripherals/imu.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/pressure.o: src/peripherals/pressure.c  .generated_files/flags/default/dca5d9686995053a10c315b435ab15df0127cd08 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/pressure.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/pressure.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/pressure.o.d" -o ${OBJECTDIR}/src/peripherals/pressure.o src/peripherals/pressure.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/propulsor.o: src/peripherals/propulsor.c  .generated_files/flags/default/98d83dfe4973d6ed7bf61eaf918d87bbbd7178dc .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/propulsor.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/propulsor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/propulsor.o.d" -o ${OBJECTDIR}/src/peripherals/propulsor.o src/peripherals/propulsor.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/temperature.o: src/peripherals/temperature.c  .generated_files/flags/default/20d65f473437f6d0c228b330f3dd27d7ea7b13fe .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/temperature.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/temperature.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/temperature.o.d" -o ${OBJECTDIR}/src/peripherals/temperature.o src/peripherals/temperature.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/ttc.o: src/peripherals/ttc.c  .generated_files/flags/default/efbe92e9dc006b18c5f2482f0fb74f059833b90e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/ttc.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/ttc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/ttc.o.d" -o ${OBJECTDIR}/src/peripherals/ttc.o src/peripherals/ttc.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/ext_memory.o: src/peripherals/ext_memory.c  .generated_files/flags/default/85cdd33f36ef939e8aa48e4818c249df43dc890 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/ext_memory.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/ext_memory.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/ext_memory.o.d" -o ${OBJECTDIR}/src/peripherals/ext_memory.o src/peripherals/ext_memory.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/main.o: src/main.c  .generated_files/flags/default/8a188b08a633074782faef21d758229d6b0a6267 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src" 
	@${RM} ${OBJECTDIR}/src/main.o.d 
	@${RM} ${OBJECTDIR}/src/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/main.o.d" -o ${OBJECTDIR}/src/main.o src/main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/testsForBoard/deterministic_test.o: src/testsForBoard/deterministic_test.c  .generated_files/flags/default/e5e97f6abdaf8f7dd1f7d527e252dc448c5673fe .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/testsForBoard" 
	@${RM} ${OBJECTDIR}/src/testsForBoard/deterministic_test.o.d 
	@${RM} ${OBJECTDIR}/src/testsForBoard/deterministic_test.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/testsForBoard/deterministic_test.o.d" -o ${OBJECTDIR}/src/testsForBoard/deterministic_test.o src/testsForBoard/deterministic_test.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
else
${OBJECTDIR}/src/app/init.o: src/app/init.c  .generated_files/flags/default/34e0eebc471248466919cbef33e46dad0aee29a3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/init.o.d 
	@${RM} ${OBJECTDIR}/src/app/init.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/init.o.d" -o ${OBJECTDIR}/src/app/init.o src/app/init.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/mission.o: src/app/mission.c  .generated_files/flags/default/5f8df0ad66825a427cce3b250df92271d0d0033a .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/mission.o.d 
	@${RM} ${OBJECTDIR}/src/app/mission.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/mission.o.d" -o ${OBJECTDIR}/src/app/mission.o src/app/mission.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/modeSelecter.o: src/app/modeSelecter.c  .generated_files/flags/default/c9c87fb3d2b8ffc5913a364e8bdb77a274323ff0 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/modeSelecter.o.d 
	@${RM} ${OBJECTDIR}/src/app/modeSelecter.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/modeSelecter.o.d" -o ${OBJECTDIR}/src/app/modeSelecter.o src/app/modeSelecter.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/sensors.o: src/app/sensors.c  .generated_files/flags/default/bd56276ece74a5bc560f796db5a2b96f67536b4d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/sensors.o.d 
	@${RM} ${OBJECTDIR}/src/app/sensors.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/sensors.o.d" -o ${OBJECTDIR}/src/app/sensors.o src/app/sensors.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/app/stateCheck.o: src/app/stateCheck.c  .generated_files/flags/default/777fddbf625a409e6626626169c20e67f2395653 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/app" 
	@${RM} ${OBJECTDIR}/src/app/stateCheck.o.d 
	@${RM} ${OBJECTDIR}/src/app/stateCheck.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/app/stateCheck.o.d" -o ${OBJECTDIR}/src/app/stateCheck.o src/app/stateCheck.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/i2c_driver.o: src/drivers/i2c_driver.c  .generated_files/flags/default/5942540c2e63cbea2b6476158d95822e064ea48f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/i2c_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/i2c_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/i2c_driver.o.d" -o ${OBJECTDIR}/src/drivers/i2c_driver.o src/drivers/i2c_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/qspi_driver.o: src/drivers/qspi_driver.c  .generated_files/flags/default/371e304dff9faa9bab4ab615e2875be6f6ab1bcf .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/qspi_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/qspi_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/qspi_driver.o.d" -o ${OBJECTDIR}/src/drivers/qspi_driver.o src/drivers/qspi_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/spi_driver.o: src/drivers/spi_driver.c  .generated_files/flags/default/ced2547372dc996fa82fdbc510e6a52b55be8ad .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/spi_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/spi_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/spi_driver.o.d" -o ${OBJECTDIR}/src/drivers/spi_driver.o src/drivers/spi_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/usart_driver.o: src/drivers/usart_driver.c  .generated_files/flags/default/c6ff79bb3f70bd7a2e45bf4f96ee5b939912dde2 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/usart_driver.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/usart_driver.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/drivers/usart_driver.o.d" -o ${OBJECTDIR}/src/drivers/usart_driver.o src/drivers/usart_driver.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_gpio.o: src/hal/hal_gpio.c  .generated_files/flags/default/276e4bb2d74a2c84800a5160bd31e1cc2a6e1825 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_gpio.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_gpio.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_gpio.o.d" -o ${OBJECTDIR}/src/hal/hal_gpio.o src/hal/hal_gpio.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_i2c.o: src/hal/hal_i2c.c  .generated_files/flags/default/5f5b3c0abb59e98e504a31aa1fed9844d14db57e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_i2c.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_i2c.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_i2c.o.d" -o ${OBJECTDIR}/src/hal/hal_i2c.o src/hal/hal_i2c.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_peripherals_init.o: src/hal/hal_peripherals_init.c  .generated_files/flags/default/f472befc572fd1b7d2928da37c5e115417728d88 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_peripherals_init.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_peripherals_init.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_peripherals_init.o.d" -o ${OBJECTDIR}/src/hal/hal_peripherals_init.o src/hal/hal_peripherals_init.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_qspi.o: src/hal/hal_qspi.c  .generated_files/flags/default/eeea993b5eaaf953b4d99e0ae572a5058751b4c0 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_qspi.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_qspi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_qspi.o.d" -o ${OBJECTDIR}/src/hal/hal_qspi.o src/hal/hal_qspi.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_spi.o: src/hal/hal_spi.c  .generated_files/flags/default/658474f1b648e0459c8c6fcbabe14e2fcb3bcf6c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_spi.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_spi.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_spi.o.d" -o ${OBJECTDIR}/src/hal/hal_spi.o src/hal/hal_spi.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_system.o: src/hal/hal_system.c  .generated_files/flags/default/669188ba607808f07ec447c2477824324d31e93a .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_system.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_system.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_system.o.d" -o ${OBJECTDIR}/src/hal/hal_system.o src/hal/hal_system.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_usart.o: src/hal/hal_usart.c  .generated_files/flags/default/e17668f9115af9e0800d63bbd5fc35dc8be7b427 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_usart.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_usart.o.d" -o ${OBJECTDIR}/src/hal/hal_usart.o src/hal/hal_usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_debug_uart.o: src/hal/hal_debug_uart.c  .generated_files/flags/default/bfae9613201fd0806aa973e495dde8f9af2350c1 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_debug_uart.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_debug_uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_debug_uart.o.d" -o ${OBJECTDIR}/src/hal/hal_debug_uart.o src/hal/hal_debug_uart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/hal/hal_systick.o: src/hal/hal_systick.c  .generated_files/flags/default/b84910bc9a7a3fff7f4f825a4a523fa8f1793f62 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/hal" 
	@${RM} ${OBJECTDIR}/src/hal/hal_systick.o.d 
	@${RM} ${OBJECTDIR}/src/hal/hal_systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/hal/hal_systick.o.d" -o ${OBJECTDIR}/src/hal/hal_systick.o src/hal/hal_systick.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/eps.o: src/peripherals/eps.c  .generated_files/flags/default/2d9820d2cc21b3bf21c4e0ba7bcd7cfca077952b .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/eps.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/eps.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/eps.o.d" -o ${OBJECTDIR}/src/peripherals/eps.o src/peripherals/eps.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/flexiforce.o: src/peripherals/flexiforce.c  .generated_files/flags/default/1855c26a6ef8848cd4c97d459343454de4ffeed4 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/flexiforce.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/flexiforce.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/flexiforce.o.d" -o ${OBJECTDIR}/src/peripherals/flexiforce.o src/peripherals/flexiforce.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/gnss.o: src/peripherals/gnss.c  .generated_files/flags/default/c9cacec1f259b60f530035d9568a7abf0ff0cd1e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/gnss.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/gnss.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/gnss.o.d" -o ${OBJECTDIR}/src/peripherals/gnss.o src/peripherals/gnss.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/imu.o: src/peripherals/imu.c  .generated_files/flags/default/a4d20f37dad6bff0a22eb7163edb2ded6cdbf950 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/imu.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/imu.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/imu.o.d" -o ${OBJECTDIR}/src/peripherals/imu.o src/peripherals/imu.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/pressure.o: src/peripherals/pressure.c  .generated_files/flags/default/aeb34113117042653d5dbd29e3607a4847cd9aa0 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/pressure.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/pressure.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/pressure.o.d" -o ${OBJECTDIR}/src/peripherals/pressure.o src/peripherals/pressure.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/propulsor.o: src/peripherals/propulsor.c  .generated_files/flags/default/de2fbc47abae022f287100985f22b8bd9686af1d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/propulsor.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/propulsor.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/propulsor.o.d" -o ${OBJECTDIR}/src/peripherals/propulsor.o src/peripherals/propulsor.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/temperature.o: src/peripherals/temperature.c  .generated_files/flags/default/250c8228c803cd158f7d5eed596487c9e46aa613 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/temperature.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/temperature.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/temperature.o.d" -o ${OBJECTDIR}/src/peripherals/temperature.o src/peripherals/temperature.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/ttc.o: src/peripherals/ttc.c  .generated_files/flags/default/a79956e647006035e4808d482b588439d4127677 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/ttc.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/ttc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/ttc.o.d" -o ${OBJECTDIR}/src/peripherals/ttc.o src/peripherals/ttc.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/peripherals/ext_memory.o: src/peripherals/ext_memory.c  .generated_files/flags/default/3a487d251c278605b8b7d552a5e1666fdcc3ce21 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/peripherals" 
	@${RM} ${OBJECTDIR}/src/peripherals/ext_memory.o.d 
	@${RM} ${OBJECTDIR}/src/peripherals/ext_memory.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/peripherals/ext_memory.o.d" -o ${OBJECTDIR}/src/peripherals/ext_memory.o src/peripherals/ext_memory.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/main.o: src/main.c  .generated_files/flags/default/61061b04ee96d991caa7f731142e6dea8a1d2be6 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src" 
	@${RM} ${OBJECTDIR}/src/main.o.d 
	@${RM} ${OBJECTDIR}/src/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/main.o.d" -o ${OBJECTDIR}/src/main.o src/main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/testsForBoard/deterministic_test.o: src/testsForBoard/deterministic_test.c  .generated_files/flags/default/25c1b8b2b983894800afd62754233a786f461eef .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/testsForBoard" 
	@${RM} ${OBJECTDIR}/src/testsForBoard/deterministic_test.o.d 
	@${RM} ${OBJECTDIR}/src/testsForBoard/deterministic_test.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -ffunction-sections -fdata-sections -O1 -fno-common -I"inc" -I"inc/app" -I"inc/config" -I"inc/drivers" -I"inc/hal" -I"inc/peripherals" -MP -MMD -MF "${OBJECTDIR}/src/testsForBoard/deterministic_test.o.d" -o ${OBJECTDIR}/src/testsForBoard/deterministic_test.o src/testsForBoard/deterministic_test.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/samv71b" ${PACK_COMMON_OPTIONS} 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -T ota/bootloader/linker/samv71q21_app.ld -o ${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,--defsym=_min_heap_size=0,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samv71b"
	
else
${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -T ota/bootloader/linker/samv71q21_app.ld -o ${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=_min_heap_size=0,--gc-sections,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/samv71b"
	${MP_CC_DIR}\\xc32-bin2hex ${DISTDIR}/PS_OBC.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

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
