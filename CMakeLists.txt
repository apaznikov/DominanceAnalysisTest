cmake_minimum_required(VERSION 3.22)
project(DominanceAnalysisTest C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

####################################################################################

#list(APPEND CMAKE_PROGRAM_PATH "/Users/ap/CLionProjects/llvm-project/llvm/build/bin")
list(APPEND CMAKE_PROGRAM_PATH "/home/ap/dev/llvm-project/llvm/build/bin")

find_program(LLVM_LINK_EXECUTABLE llvm-link)
find_program(OPT_EXECUTABLE opt)
find_program(CLANG_EXECUTABLE clang)
find_program(CLANG_CXX_EXECUTABLE clang++)

# Check if the programs were found
if(NOT LLVM_LINK_EXECUTABLE OR NOT OPT_EXECUTABLE OR NOT CLANG_EXECUTABLE)
        message(FATAL_ERROR "Could not find llvm-link, opt, or clang. Please adjust your PATH or provide the correct paths.")
endif()

message(STATUS "Found clang: ${CLANG_EXECUTABLE}")
message(STATUS "Found clang++: ${CLANG_CXX_EXECUTABLE}")
message(STATUS "Found opt: ${OPT_EXECUTABLE}")
message(STATUS "Found llvm-link: ${LLVM_LINK_EXECUTABLE}")

####################################################################################

set(CMAKE_C_LINK_EXECUTABLE
        "${LLVM_LINK_EXECUTABLE} <LINK_FLAGS> -S <OBJECTS> -o <TARGET>.ll <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE
        "${LLVM_LINK_EXECUTABLE} <LINK_FLAGS> -S <OBJECTS> -o <TARGET>.ll <LINK_LIBRARIES>")

function(add_target target_name source_file)
        add_executable(${target_name} ${source_file})

        target_compile_options(${target_name}
                PRIVATE -O0 -S -g -emit-llvm
                -Wno-unused-command-line-argument
                -Wno-return-stack-address
                -Xclang -disable-O0-optnone
        )

#        add_custom_command(TARGET ${target_name} POST_BUILD
#                COMMAND rm -f st_summary.txt
#                COMMAND ${OPT_EXECUTABLE}
#                -S ${target_name}.ll
#                -o ${target_name}_mem2reg.ll
##                -passes=mem2reg
#                COMMAND ${OPT_EXECUTABLE}
#                -S -passes='print<${analysis_pass}>'
#                -debug-only=${analysis_pass}
#                ${target_name}_mem2reg.ll -disable-output
#                DEPENDS ${target_name}.ll
#                COMMENT "Running ${analysis_pass} Printer"
#        )
#-passes=mem2reg

        # Run TSan
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CLANG_EXECUTABLE}
                -S -g -emit-llvm -fsanitize=thread
                -fno-inline-functions
                -mllvm -tsan-use-dominance-analysis
                -mllvm -debug-only=tsan,
                -Wno-return-stack-address
                ../${source_file}
                -o ${target_name}_TSan_instr.ll
                COMMENT "Running TSan"
        )
endfunction()

add_target(DominanceAnalysisTest DominanceAnalysisTest.c)

####################################################################################

#function(add_tsan_target target_name source_file)
#        if(${source_file} MATCHES "\\.cpp$")
#                set(compiler ${CLANG_CXX_EXECUTABLE})
#        elseif(${source_file} MATCHES "\\.c$")
#                set(compiler ${CLANG_EXECUTABLE})
#        else()
#                message(FATAL_ERROR "Unsupported source file extension: ${source_file}")
#        endif()
#
#
#        add_executable(${target_name} ${source_file})
#
#        set_target_properties(${target_name} PROPERTIES COMPILER ${compiler})
#
#        target_compile_options(${target_name} PRIVATE
#                -O0
#                -S -emit-llvm
#                -o ${target_name}_instr.ll
#                -fsanitize=thread
#                "SHELL:-mllvm -tsan-use-escape-analysis-global"
#                "SHELL:-mllvm -tsan-use-lock-ownership"
#                "SHELL:-mllvm -debug-only=ea,tsan-ea,tsan"
#                -Wno-unused-command-line-argument
#                -Wno-return-stack-address
#                -Xclang -disable-O0-optnone
#        )
#endfunction()

#"SHELL:-mllvm -debug-only=tsan,tsan-ea,ea"
#add_tsan_target(TSanTestCPP TSanTest.cpp)
#add_tsan_target(TSanTestC TSanTest.c)

#-S -passes=mem2reg EscapeAnalysisTest.ll
#-mllvm -stats
#-mllvm -debug-only=tsan,tsan-ea
