cmake_minimum_required (VERSION 3.6)

project(pg_module CXX)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
)

set(ASSETS
    assets/DGLogo.png
)

file(GLOB_RECURSE SRC_SOURCE
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)
	
file(GLOB_RECURSE ENGINE_SOURCE 
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/engine/src/*.*
	${CMAKE_CURRENT_SOURCE_DIR}/engine/inc/*.*)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_SOURCE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_SOURCE})

set(SOURCE ${SOURCE} ${SRC_SOURCE})
set(SOURCE ${SOURCE} ${ENGINE_SOURCE})

set(PROJECT_NAME_TMP "pg_module")

add_sample_app(${PROJECT_NAME_TMP} "playground" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

#set_target_properties(${PROJECT_NAME_TMP} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
#set_target_properties(${PROJECT_NAME_TMP} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-c ./conf/crytek-sponza.3dgep")

target_link_libraries(${PROJECT_NAME_TMP} PRIVATE Diligent-TextureLoader 
			assimp
			)

set (DE_TOP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

target_link_directories(${PROJECT_NAME_TMP} PRIVATE ""
			)

target_include_directories(${PROJECT_NAME_TMP} PRIVATE 
						${DE_TOP_PATH}/thirdparty/
						${DE_TOP_PATH}/thirdparty/glm-0.9.9.6/
						${DE_TOP_PATH}/thirdparty/assimp-5.0.0/include
						${CMAKE_BINARY_DIR}/thirdparty/assimp-5.0.0/include # assimp generated config.h			
						${DE_TOP_PATH}/thirdparty/entt/src
						${DE_TOP_PATH}/thirdparty/fibertaskinglib/include
						${DE_TOP_PATH}/thirdparty/taskflow
						)

					
