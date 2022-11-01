#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/_ext/359267392/browse5.o \
	${OBJECTDIR}/src/cvffmpeg/cvffmpeg.o \
	${OBJECTDIR}/src/browse.o \
	${OBJECTDIR}/src/cvffmpeg/abbrevs.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -lcxcore -lcv -lhighgui -lcvaux -lml -lpq

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-Debug.mk dist/sunar-hmi

dist/sunar-hmi: ${OBJECTFILES}
	${MKDIR} -p dist
	${LINK.cc} -o ${CND_DISTDIR}/sunar-hmi ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/main.o: src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/opencv -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main.o src/main.cpp

${OBJECTDIR}/_ext/359267392/browse5.o: /home/chmelarp/Projects/sunar/sunar-hmi/src/browse5.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/359267392
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/opencv -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/359267392/browse5.o /home/chmelarp/Projects/sunar/sunar-hmi/src/browse5.cpp

${OBJECTDIR}/src/cvffmpeg/cvffmpeg.o: src/cvffmpeg/cvffmpeg.c 
	${MKDIR} -p ${OBJECTDIR}/src/cvffmpeg
	${RM} $@.d
	$(COMPILE.c) -g -I/usr/local/include/opencv -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cvffmpeg/cvffmpeg.o src/cvffmpeg/cvffmpeg.c

${OBJECTDIR}/src/browse.o: src/browse.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/local/include/opencv -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/browse.o src/browse.cpp

${OBJECTDIR}/src/cvffmpeg/abbrevs.o: src/cvffmpeg/abbrevs.c 
	${MKDIR} -p ${OBJECTDIR}/src/cvffmpeg
	${RM} $@.d
	$(COMPILE.c) -g -I/usr/local/include/opencv -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/cvffmpeg/abbrevs.o src/cvffmpeg/abbrevs.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} dist/sunar-hmi

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
