cmake_minimum_required (VERSION 3.6)

project(SSAOSample CXX)

set(SOURCE
    src/SSAOSample.cpp
)

set(INCLUDE
    src/SSAOSample.h
)

set(SHADERS
)

file(GLOB SPONZA_MODEL LIST_DIRECTORIES false assets/sponza/*.*)

set(ASSETS
    ${SPONZA_MODEL}
)

add_sample_app("SSAOSample" "Samples" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")
target_link_libraries(SSAOSample PRIVATE AssetLoader DiligentFX)


set_source_files_properties(${SPONZA_MODEL} PROPERTIES
    VS_DEPLOYMENT_LOCATION "sponza"
    MACOSX_PACKAGE_LOCATION "Resources/sponza"
)
source_group("assets" FILES ${ASSETS})
source_group("assets\\models\\Sponza" FILES ${SPONZA_MODEL})
