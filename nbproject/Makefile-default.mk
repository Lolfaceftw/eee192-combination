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
FINAL_IMAGE=${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=src/drivers/gps_usart.c src/drivers/pm_usart.c src/main.c src/parsers/nmea_parse.c src/parsers/pms_parser.c src/terminal_ui.c platform/gpio.c platform/systick.c platform/usart.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/src/drivers/gps_usart.o ${OBJECTDIR}/src/drivers/pm_usart.o ${OBJECTDIR}/src/main.o ${OBJECTDIR}/src/parsers/nmea_parse.o ${OBJECTDIR}/src/parsers/pms_parser.o ${OBJECTDIR}/src/terminal_ui.o ${OBJECTDIR}/platform/gpio.o ${OBJECTDIR}/platform/systick.o ${OBJECTDIR}/platform/usart.o
POSSIBLE_DEPFILES=${OBJECTDIR}/src/drivers/gps_usart.o.d ${OBJECTDIR}/src/drivers/pm_usart.o.d ${OBJECTDIR}/src/main.o.d ${OBJECTDIR}/src/parsers/nmea_parse.o.d ${OBJECTDIR}/src/parsers/pms_parser.o.d ${OBJECTDIR}/src/terminal_ui.o.d ${OBJECTDIR}/platform/gpio.o.d ${OBJECTDIR}/platform/systick.o.d ${OBJECTDIR}/platform/usart.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/src/drivers/gps_usart.o ${OBJECTDIR}/src/drivers/pm_usart.o ${OBJECTDIR}/src/main.o ${OBJECTDIR}/src/parsers/nmea_parse.o ${OBJECTDIR}/src/parsers/pms_parser.o ${OBJECTDIR}/src/terminal_ui.o ${OBJECTDIR}/platform/gpio.o ${OBJECTDIR}/platform/systick.o ${OBJECTDIR}/platform/usart.o

# Source Files
SOURCEFILES=src/drivers/gps_usart.c src/drivers/pm_usart.c src/main.c src/parsers/nmea_parse.c src/parsers/pms_parser.c src/terminal_ui.c platform/gpio.c platform/systick.c platform/usart.c

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
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32CM5164LS00048
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
${OBJECTDIR}/src/drivers/gps_usart.o: src/drivers/gps_usart.c  .generated_files/flags/default/94d3d368ee3896581f2a5474c853a9ec9885608e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/gps_usart.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/gps_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/drivers/gps_usart.o.d" -o ${OBJECTDIR}/src/drivers/gps_usart.o src/drivers/gps_usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/pm_usart.o: src/drivers/pm_usart.c  .generated_files/flags/default/edc9390281a2c59ea67ca61fc9b7b9ea36d8dd53 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/pm_usart.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/pm_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/drivers/pm_usart.o.d" -o ${OBJECTDIR}/src/drivers/pm_usart.o src/drivers/pm_usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/main.o: src/main.c  .generated_files/flags/default/c087d02681c61fdffcb0cb1b59eaa1e66c3f52b .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src" 
	@${RM} ${OBJECTDIR}/src/main.o.d 
	@${RM} ${OBJECTDIR}/src/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/main.o.d" -o ${OBJECTDIR}/src/main.o src/main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/parsers/nmea_parse.o: src/parsers/nmea_parse.c  .generated_files/flags/default/a58fc780898015da757640a76f781cb3c422ef17 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/parsers" 
	@${RM} ${OBJECTDIR}/src/parsers/nmea_parse.o.d 
	@${RM} ${OBJECTDIR}/src/parsers/nmea_parse.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/parsers/nmea_parse.o.d" -o ${OBJECTDIR}/src/parsers/nmea_parse.o src/parsers/nmea_parse.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/parsers/pms_parser.o: src/parsers/pms_parser.c  .generated_files/flags/default/4dc03bd9cc51559f41a56daeebfb3d256d03c826 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/parsers" 
	@${RM} ${OBJECTDIR}/src/parsers/pms_parser.o.d 
	@${RM} ${OBJECTDIR}/src/parsers/pms_parser.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/parsers/pms_parser.o.d" -o ${OBJECTDIR}/src/parsers/pms_parser.o src/parsers/pms_parser.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/terminal_ui.o: src/terminal_ui.c  .generated_files/flags/default/beb08bf963d66ad3d14138e406a7755c70569b5f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src" 
	@${RM} ${OBJECTDIR}/src/terminal_ui.o.d 
	@${RM} ${OBJECTDIR}/src/terminal_ui.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/terminal_ui.o.d" -o ${OBJECTDIR}/src/terminal_ui.o src/terminal_ui.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/platform/gpio.o: platform/gpio.c  .generated_files/flags/default/4a0226d2a9db16588caee665537daa884735c0c3 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/platform" 
	@${RM} ${OBJECTDIR}/platform/gpio.o.d 
	@${RM} ${OBJECTDIR}/platform/gpio.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/platform/gpio.o.d" -o ${OBJECTDIR}/platform/gpio.o platform/gpio.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/platform/systick.o: platform/systick.c  .generated_files/flags/default/dcee7fe06a0707a961a1478043dc62bd2e605f43 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/platform" 
	@${RM} ${OBJECTDIR}/platform/systick.o.d 
	@${RM} ${OBJECTDIR}/platform/systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/platform/systick.o.d" -o ${OBJECTDIR}/platform/systick.o platform/systick.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/platform/usart.o: platform/usart.c  .generated_files/flags/default/3af5d39db4ac661f5870f872714d77813d5590f2 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/platform" 
	@${RM} ${OBJECTDIR}/platform/usart.o.d 
	@${RM} ${OBJECTDIR}/platform/usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/platform/usart.o.d" -o ${OBJECTDIR}/platform/usart.o platform/usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
else
${OBJECTDIR}/src/drivers/gps_usart.o: src/drivers/gps_usart.c  .generated_files/flags/default/5b2bf5f97d4c8f5e356ef2832cc6e91b329aff9e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/gps_usart.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/gps_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/drivers/gps_usart.o.d" -o ${OBJECTDIR}/src/drivers/gps_usart.o src/drivers/gps_usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/drivers/pm_usart.o: src/drivers/pm_usart.c  .generated_files/flags/default/ca627488b27cfaa930fa7c65d7df9c1cfd46bf5a .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/drivers" 
	@${RM} ${OBJECTDIR}/src/drivers/pm_usart.o.d 
	@${RM} ${OBJECTDIR}/src/drivers/pm_usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/drivers/pm_usart.o.d" -o ${OBJECTDIR}/src/drivers/pm_usart.o src/drivers/pm_usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/main.o: src/main.c  .generated_files/flags/default/4e550b151b152d2667572661870f6963617a4a72 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src" 
	@${RM} ${OBJECTDIR}/src/main.o.d 
	@${RM} ${OBJECTDIR}/src/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/main.o.d" -o ${OBJECTDIR}/src/main.o src/main.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/parsers/nmea_parse.o: src/parsers/nmea_parse.c  .generated_files/flags/default/bfb1995b0542a968b2fc01b96cc9e6a173975557 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/parsers" 
	@${RM} ${OBJECTDIR}/src/parsers/nmea_parse.o.d 
	@${RM} ${OBJECTDIR}/src/parsers/nmea_parse.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/parsers/nmea_parse.o.d" -o ${OBJECTDIR}/src/parsers/nmea_parse.o src/parsers/nmea_parse.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/parsers/pms_parser.o: src/parsers/pms_parser.c  .generated_files/flags/default/44b79974b519ee93f65373346d0f81d0f3c4feb0 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src/parsers" 
	@${RM} ${OBJECTDIR}/src/parsers/pms_parser.o.d 
	@${RM} ${OBJECTDIR}/src/parsers/pms_parser.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/parsers/pms_parser.o.d" -o ${OBJECTDIR}/src/parsers/pms_parser.o src/parsers/pms_parser.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/src/terminal_ui.o: src/terminal_ui.c  .generated_files/flags/default/39b327d9a55bf2dd79e934b7b5732b9c44e35dda .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/src" 
	@${RM} ${OBJECTDIR}/src/terminal_ui.o.d 
	@${RM} ${OBJECTDIR}/src/terminal_ui.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/src/terminal_ui.o.d" -o ${OBJECTDIR}/src/terminal_ui.o src/terminal_ui.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/platform/gpio.o: platform/gpio.c  .generated_files/flags/default/806c7dc37b16609edc3a4a99f1e87b3f032b5d5d .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/platform" 
	@${RM} ${OBJECTDIR}/platform/gpio.o.d 
	@${RM} ${OBJECTDIR}/platform/gpio.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/platform/gpio.o.d" -o ${OBJECTDIR}/platform/gpio.o platform/gpio.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/platform/systick.o: platform/systick.c  .generated_files/flags/default/d57d85933cb4b456d6afb603cca76e4d00f80497 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/platform" 
	@${RM} ${OBJECTDIR}/platform/systick.o.d 
	@${RM} ${OBJECTDIR}/platform/systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/platform/systick.o.d" -o ${OBJECTDIR}/platform/systick.o platform/systick.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
${OBJECTDIR}/platform/usart.o: platform/usart.c  .generated_files/flags/default/24bcdaa8e814d484265e1401e7d1f594e7e331a4 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/platform" 
	@${RM} ${OBJECTDIR}/platform/usart.o.d 
	@${RM} ${OBJECTDIR}/platform/usart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -O0 -fno-common -I"inc" -I"inc/parsers" -MP -MMD -MF "${OBJECTDIR}/platform/usart.o.d" -o ${OBJECTDIR}/platform/usart.o platform/usart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}/PIC32CM-LS00" ${PACK_COMMON_OPTIONS} 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o ${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/PIC32CM-LS00"
	
else
${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o ${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,${DISTDIR}/memoryfile.xml -mdfp="${DFP_DIR}/PIC32CM-LS00"
	${MP_CC_DIR}\\xc32-bin2hex ${DISTDIR}/eee_192_combined_final.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
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
