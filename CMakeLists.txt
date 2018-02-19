CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_CXX_STANDARD 11)

PROJECT(esmeraldamfcc)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} dsp)
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

ADD_LIBRARY(esmeraldamfcc SHARED
    dsp/__program.c
    dsp/acf.c
    dsp/basics.c
    dsp/channel.c
    dsp/csp.c
    dsp/dct.c
    dsp/delay.c
    dsp/deriv.c
    dsp/dft.c
    dsp/dwt.c
    dsp/fextract.c
    dsp/fft.c
    dsp/filterbank.c
    dsp/lpc.c
    dsp/mel.c
    dsp/mfcc_1_4.c
    dsp/mfcc.c
    dsp/preemph.c
    dsp/sdft.c
    dsp/shift.c
    dsp/tirol.c
    dsp/vad.c
    dsp/window.c
    mx/basics.c
    mx/prob.c
    mx/matrix.c
    mx/histogram.c
    mx/vector.c
    mx/eigen.c
    mx/score.c
    mx/sum.c
    rs/hashq.c
    rs/io.c
    rs/memory.c
    rs/messages.c
    rs/name.c
    rs/queue.c
    rs/time.c
    rs/basics.c
    rs/stack.c
    lib/esmeraldamfcc.cpp
)
SET_TARGET_PROPERTIES(esmeraldamfcc PROPERTIES PUBLIC_HEADER "lib/esmeraldamfcc.h")
INSTALL(TARGETS esmeraldamfcc EXPORT esmeraldamfccConfig
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
EXPORT(TARGETS esmeraldamfcc FILE esmeraldamfccConfig.cmake)
INSTALL(EXPORT esmeraldamfccConfig DESTINATION lib/cmake/esmeraldamfcc)

ADD_EXECUTABLE(dsp_fex
    dsp/dsp_fex.c
)
TARGET_LINK_LIBRARIES(dsp_fex esmeraldamfcc)

ADD_EXECUTABLE(dsp_vad
    dsp/dsp_vad.c
)
TARGET_LINK_LIBRARIES(dsp_vad esmeraldamfcc)

ADD_EXECUTABLE(mfcctest
    util/mfcctest.cpp
)
TARGET_LINK_LIBRARIES(mfcctest esmeraldamfcc)

