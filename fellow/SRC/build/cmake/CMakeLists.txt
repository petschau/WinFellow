cmake_minimum_required(VERSION 3.25)
project(NullFellow)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wno-deprecated-non-prototype -DHAVE_UNISTD_H=1")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -DHAVE_UNISTD_H=1")

set(FELLOW_ROOT_DIR ../..)

set(FELLOW_INCLUDE_DIR ${FELLOW_ROOT_DIR})
set(FELLOW_LIBS_XDMS_INCLUDE_DIR ${FELLOW_ROOT_DIR}/fellow/libs/xdms/src)
set(FELLOW_LIBS_ZLIB_INCLUDE_DIR ${FELLOW_ROOT_DIR}/fellow/libs/zlib/src)

set(FELLOW_LIBS_XDMS_DIR ${FELLOW_ROOT_DIR}/fellow/libs/xdms/src)
set(FELLOW_LIBS_ZLIB_DIR ${FELLOW_ROOT_DIR}/fellow/libs/zlib/src)

set(FELLOW_OS_DIR ${FELLOW_ROOT_DIR}/fellow/os/null)
set(FELLOW_API_DIR ${FELLOW_ROOT_DIR}/fellow/api)
set(FELLOW_APPLICATION_DIR ${FELLOW_ROOT_DIR}/fellow/application)
set(FELLOW_AUTOMATION_DIR ${FELLOW_ROOT_DIR}/fellow/automation)
set(FELLOW_CHIPSET_DIR ${FELLOW_ROOT_DIR}/fellow/chipset)
set(FELLOW_CONFIGURATION_DIR ${FELLOW_ROOT_DIR}/fellow/configuration)
set(FELLOW_CPU_DIR ${FELLOW_ROOT_DIR}/fellow/cpu)
set(FELLOW_DEBUG_DIR ${FELLOW_ROOT_DIR}/fellow/debug)
set(FELLOW_HARDFILE_DIR ${FELLOW_ROOT_DIR}/fellow/hardfile)
set(FELLOW_HUD_DIR ${FELLOW_ROOT_DIR}/fellow/hud)
set(FELLOW_MEMORY_DIR ${FELLOW_ROOT_DIR}/fellow/memory)
set(FELLOW_SCHEDULER_DIR ${FELLOW_ROOT_DIR}/fellow/scheduler)
set(FELLOW_SERVICE_DIR ${FELLOW_ROOT_DIR}/fellow/service)
set(FELLOW_TIME_DIR ${FELLOW_ROOT_DIR}/fellow/time)
set(FELLOW_VM_DIR ${FELLOW_ROOT_DIR}/fellow/vm)
set(FELLOW_XDMS_DIR ${FELLOW_ROOT_DIR}/fellow/libs/xdms)
set(FELLOW_ZLIB_DIR ${FELLOW_ROOT_DIR}/fellow/libs/zlib)

set(FELLOW_LIBS_XDMS_SOURCE_FILES
        ${FELLOW_LIBS_XDMS_DIR}/crc_csum.c
        ${FELLOW_LIBS_XDMS_DIR}/getbits.c
        ${FELLOW_LIBS_XDMS_DIR}/maketbl.c
        ${FELLOW_LIBS_XDMS_DIR}/pfile.c
        ${FELLOW_LIBS_XDMS_DIR}/tables.c
        ${FELLOW_LIBS_XDMS_DIR}/u_deep.c
        ${FELLOW_LIBS_XDMS_DIR}/u_heavy.c
        ${FELLOW_LIBS_XDMS_DIR}/u_init.c
        ${FELLOW_LIBS_XDMS_DIR}/u_medium.c
        ${FELLOW_LIBS_XDMS_DIR}/u_quick.c
        ${FELLOW_LIBS_XDMS_DIR}/u_rle.c
        )

set(FELLOW_LIBS_ZLIB_SOURCE_FILES
        ${FELLOW_LIBS_ZLIB_DIR}/adler32.c
        ${FELLOW_LIBS_ZLIB_DIR}/compress.c
        ${FELLOW_LIBS_ZLIB_DIR}/crc32.c
        ${FELLOW_LIBS_ZLIB_DIR}/deflate.c
        ${FELLOW_LIBS_ZLIB_DIR}/gzclose.c
        ${FELLOW_LIBS_ZLIB_DIR}/gzlib.c
        ${FELLOW_LIBS_ZLIB_DIR}/gzread.c
        ${FELLOW_LIBS_ZLIB_DIR}/gzread.c
        ${FELLOW_LIBS_ZLIB_DIR}/gzwrite.c
        ${FELLOW_LIBS_ZLIB_DIR}/infback.c
        ${FELLOW_LIBS_ZLIB_DIR}/inffast.c
        ${FELLOW_LIBS_ZLIB_DIR}/inflate.c
        ${FELLOW_LIBS_ZLIB_DIR}/inftrees.c
        ${FELLOW_LIBS_ZLIB_DIR}/trees.c
        ${FELLOW_LIBS_ZLIB_DIR}/uncompr.c
        ${FELLOW_LIBS_ZLIB_DIR}/zutil.c
        )

set(FELLOW_API_SOURCE_FILES
        ${FELLOW_API_DIR}/Services.cpp
        ${FELLOW_API_DIR}/Drivers.cpp
        ${FELLOW_API_DIR}/drivers/IniValues.cpp
        )

set(FELLOW_APPLICATION_SOURCE_FILES
        ${FELLOW_APPLICATION_DIR}/DisplayMode.cpp
        ${FELLOW_APPLICATION_DIR}/Fellow.cpp
        ${FELLOW_APPLICATION_DIR}/FellowFilesys.cpp
        ${FELLOW_APPLICATION_DIR}/HostRenderConfiguration.cpp
        ${FELLOW_APPLICATION_DIR}/HostRenderer.cpp
        ${FELLOW_APPLICATION_DIR}/HostRenderRuntimeSettings.cpp
        ${FELLOW_APPLICATION_DIR}/HostRenderStatistics.cpp
        ${FELLOW_APPLICATION_DIR}/Interrupt.cpp
        ${FELLOW_APPLICATION_DIR}/ListTree.cpp
        ${FELLOW_APPLICATION_DIR}/modrip.cpp
        )

set(FELLOW_AUTOMATION_SOURCE_FILES
        ${FELLOW_AUTOMATION_DIR}/Automator.cpp
        ${FELLOW_AUTOMATION_DIR}/Script.cpp
        )

set(FELLOW_CHIPSET_SOURCE_FILES
        ${FELLOW_CHIPSET_DIR}/BitplaneDMA.cpp
        ${FELLOW_CHIPSET_DIR}/BitplaneDMAFetchPrograms.cpp
        ${FELLOW_CHIPSET_DIR}/BitplaneRegisters.cpp
        ${FELLOW_CHIPSET_DIR}/BitplaneShifter.cpp
        ${FELLOW_CHIPSET_DIR}/Blitter.cpp
        ${FELLOW_CHIPSET_DIR}/ChipsetInfo.cpp
        ${FELLOW_CHIPSET_DIR}/Cia.cpp
        ${FELLOW_CHIPSET_DIR}/CopperCommon.cpp
        ${FELLOW_CHIPSET_DIR}/CopperRegisters.cpp
        ${FELLOW_CHIPSET_DIR}/CycleExactCopper.cpp
        ${FELLOW_CHIPSET_DIR}/CycleExactSprites.cpp
        ${FELLOW_CHIPSET_DIR}/DDFStateMachine.cpp
        ${FELLOW_CHIPSET_DIR}/DIWXStateMachine.cpp
        ${FELLOW_CHIPSET_DIR}/DIWYStateMachine.cpp
        ${FELLOW_CHIPSET_DIR}/DMAController.cpp
        ${FELLOW_CHIPSET_DIR}/draw_interlace_control.cpp
        ${FELLOW_CHIPSET_DIR}/draw_pixelrenderers.cpp
        ${FELLOW_CHIPSET_DIR}/DualPlayfieldMapper.cpp
        ${FELLOW_CHIPSET_DIR}/Floppy.cpp
        ${FELLOW_CHIPSET_DIR}/Gameport.cpp
        ${FELLOW_CHIPSET_DIR}/Graphics.cpp
        ${FELLOW_CHIPSET_DIR}/HostFrame.cpp
        ${FELLOW_CHIPSET_DIR}/HostFrameCopier.cpp
        ${FELLOW_CHIPSET_DIR}/HostFrameDelayedRenderer.cpp
        ${FELLOW_CHIPSET_DIR}/HostFrameImmediateRenderer.cpp
        ${FELLOW_CHIPSET_DIR}/Kbd.cpp
        ${FELLOW_CHIPSET_DIR}/LineExactCopper.cpp
        ${FELLOW_CHIPSET_DIR}/LineExactSprites.cpp
        ${FELLOW_CHIPSET_DIR}/Planar2ChunkyDecoder.cpp
        ${FELLOW_CHIPSET_DIR}/rtc.cpp
        ${FELLOW_CHIPSET_DIR}/RtcOkiMsm6242rs.cpp
        ${FELLOW_CHIPSET_DIR}/Sound.cpp
        ${FELLOW_CHIPSET_DIR}/Sprite.cpp
        ${FELLOW_CHIPSET_DIR}/SpriteDMA.cpp
        ${FELLOW_CHIPSET_DIR}/SpriteMerger.cpp
        ${FELLOW_CHIPSET_DIR}/SpriteP2CDecoder.cpp
        ${FELLOW_CHIPSET_DIR}/SpriteRegisters.cpp
        ${FELLOW_CHIPSET_DIR}/uart.cpp
        ${FELLOW_CHIPSET_DIR}/Wav.cpp
        )

set(FELLOW_CONFIGURATION_SOURCE_FILES
        ${FELLOW_CONFIGURATION_DIR}/Configuration.cpp
        ${FELLOW_CONFIGURATION_DIR}/HostRenderConfigurationMapper.cpp
        )

set(FELLOW_CPU_SOURCE_FILES
        ${FELLOW_CPU_DIR}/CpuIntegration.cpp
        ${FELLOW_CPU_DIR}/CpuModule.cpp
        ${FELLOW_CPU_DIR}/CpuModule_Disassembler.cpp
        ${FELLOW_CPU_DIR}/CpuModule_EffectiveAddress.cpp
        ${FELLOW_CPU_DIR}/CpuModule_Exceptions.cpp
        ${FELLOW_CPU_DIR}/CpuModule_Flags.cpp
        ${FELLOW_CPU_DIR}/CpuModule_Instructions.cpp
        ${FELLOW_CPU_DIR}/CpuModule_InternalState.cpp
        ${FELLOW_CPU_DIR}/CpuModule_Interrupts.cpp
        ${FELLOW_CPU_DIR}/CpuModule_Logging.cpp
        ${FELLOW_CPU_DIR}/CpuModule_StackFrameGen.cpp
        )

set(FELLOW_DEBUG_SOURCE_FILES
        ${FELLOW_DEBUG_DIR}/commands/ChipsetRegisterCommand.cpp
        ${FELLOW_DEBUG_DIR}/commands/CPURegisterCommand.cpp
        ${FELLOW_DEBUG_DIR}/commands/CPUStepCommand.cpp
        ${FELLOW_DEBUG_DIR}/commands/DisassembleCommand.cpp
        ${FELLOW_DEBUG_DIR}/commands/LogCommand.cpp
        ${FELLOW_DEBUG_DIR}/commands/MemoryCommand.cpp
        ${FELLOW_DEBUG_DIR}/commands/MemoryMapCommand.cpp
        ${FELLOW_DEBUG_DIR}/console/ChipsetRegisterHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/ConsoleDebugger.cpp
        ${FELLOW_DEBUG_DIR}/console/CPURegisterHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/CPUStepHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/CPUWriterUtils.cpp
        ${FELLOW_DEBUG_DIR}/console/DebugWriter.cpp
        ${FELLOW_DEBUG_DIR}/console/DisassembleHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/HelpHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/LogHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/MemoryHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/MemoryMapHandler.cpp
        ${FELLOW_DEBUG_DIR}/console/ParameterUtils.cpp
        ${FELLOW_DEBUG_DIR}/log/BitplaneShifterLogEntry.cpp
        ${FELLOW_DEBUG_DIR}/log/ChipBusLogEntry.cpp
        ${FELLOW_DEBUG_DIR}/log/DebugLogEntry.cpp
        ${FELLOW_DEBUG_DIR}/log/DebugLogHandler.cpp
        ${FELLOW_DEBUG_DIR}/log/DiagnosticFeatures.cpp
        ${FELLOW_DEBUG_DIR}/log/DIWXLogEntry.cpp
        ${FELLOW_DEBUG_DIR}/log/DIWYLogEntry.cpp
        ${FELLOW_DEBUG_DIR}/log/EventQueueLogEntry.cpp
        ${FELLOW_DEBUG_DIR}/log/LineChangeLogEntry.cpp
        )

set(FELLOW_HARDFILE_SOURCE_FILES
        ${FELLOW_HARDFILE_DIR}/hunks/AdditionalHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/BSSHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/CodeHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/DataHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/EndHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/FileImage.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/HeaderHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/HunkFactory.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/HunkParser.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/HunkRelocator.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/HunkSize.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/InitialHunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/RawDataReader.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/Reloc32Hunk.cpp
        ${FELLOW_HARDFILE_DIR}/hunks/Reloc32OffsetTable.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/ChecksumCalculator.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDB.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDBFileReader.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDBFileSystemHandler.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDBFileSystemHeader.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDBHandler.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDBLSegBlock.cpp
        ${FELLOW_HARDFILE_DIR}/rdb/RDBPartition.cpp
        ${FELLOW_HARDFILE_DIR}/HardfileHandler.cpp
        ${FELLOW_HARDFILE_DIR}/HardfileStructs.cpp
        )

set(FELLOW_HUD_SOURCE_FILES
        ${FELLOW_HUD_DIR}/HudPropertyProvider.cpp
        )

set(FELLOW_MEMORY_SOURCE_FILES
        ${FELLOW_MEMORY_DIR}/Memory.cpp
        )

set(FELLOW_SCHEDULER_SOURCE_FILES
        ${FELLOW_SCHEDULER_DIR}/Scheduler.cpp
        ${FELLOW_SCHEDULER_DIR}/SchedulerEvent.cpp
        ${FELLOW_SCHEDULER_DIR}/SchedulerQueue.cpp
        )

set(FELLOW_SERVICE_SOURCE_FILES
        ${FELLOW_SERVICE_DIR}/CoreServicesFactory.cpp
        ${FELLOW_SERVICE_DIR}/FSWrapper.cpp
        ${FELLOW_SERVICE_DIR}/HUD.cpp
        ${FELLOW_SERVICE_DIR}/Log.cpp
        ${FELLOW_SERVICE_DIR}/RetroPlatformWrapper.cpp
        )

set(FELLOW_TIME_SOURCE_FILES
        ${FELLOW_TIME_DIR}/Timestamps.cpp
        )

set(FELLOW_VM_SOURCE_FILES
        ${FELLOW_VM_DIR}/Chipset.cpp
        ${FELLOW_VM_DIR}/Clocks.cpp
        ${FELLOW_VM_DIR}/Copper.cpp
        ${FELLOW_VM_DIR}/Display.cpp
        ${FELLOW_VM_DIR}/DisplayRegisters.cpp
        ${FELLOW_VM_DIR}/EventLog.cpp
        ${FELLOW_VM_DIR}/M68K.cpp
        ${FELLOW_VM_DIR}/MemorySystem.cpp
        ${FELLOW_VM_DIR}/VM.cpp
        )

set(FELLOW_XDMS_SOURCE_FILES
        ${FELLOW_XDMS_DIR}/xdmswrap.c
        )

set(FELLOW_ZLIB_SOURCE_FILES
        ${FELLOW_ZLIB_DIR}/zlibwrap.cpp
        )

set(FELLOW_OS_SOURCE_FILES
        ${FELLOW_OS_DIR}/SoundDriver.cpp
        ${FELLOW_OS_DIR}/GraphicsDriver.cpp
        ${FELLOW_OS_DIR}/TimerDriver.cpp
        ${FELLOW_OS_DIR}/KeyboardDriver.cpp
        ${FELLOW_OS_DIR}/MouseDriver.cpp
        ${FELLOW_OS_DIR}/JoystickDriver.cpp
        ${FELLOW_OS_DIR}/IniDriver.cpp
        )

set (SOURCE_FILES
        ${FELLOW_API_SOURCE_FILES}
        ${FELLOW_APPLICATION_SOURCE_FILES}
        ${FELLOW_AUTOMATION_SOURCE_FILES}
        ${FELLOW_CHIPSET_SOURCE_FILES}
        ${FELLOW_CONFIGURATION_SOURCE_FILES}
        ${FELLOW_CPU_SOURCE_FILES}
        ${FELLOW_DEBUG_SOURCE_FILES}
        ${FELLOW_HARDFILE_SOURCE_FILES}
        ${FELLOW_HUD_SOURCE_FILES}
        ${FELLOW_MEMORY_SOURCE_FILES}
        ${FELLOW_SCHEDULER_SOURCE_FILES}
        ${FELLOW_SERVICE_SOURCE_FILES}
        ${FELLOW_TIME_SOURCE_FILES}
        ${FELLOW_VM_SOURCE_FILES}
        ${FELLOW_XDMS_SOURCE_FILES}
        ${FELLOW_ZLIB_SOURCE_FILES}

        ${FELLOW_LIBS_XDMS_SOURCE_FILES}
        ${FELLOW_LIBS_ZLIB_SOURCE_FILES}
        ${FELLOW_OS_SOURCE_FILES}
        )

include_directories(${FELLOW_INCLUDE_DIR} ${FELLOW_OS_DIR} ${FELLOW_LIBS_XDMS_INCLUDE_DIR} ${FELLOW_LIBS_ZLIB_INCLUDE_DIR})

add_executable(NullFellow ${SOURCE_FILES})
