add_executable(llm_test test_llm.cpp)

target_link_libraries(llm_test PRIVATE CoCo)
setup_sanitizers(llm_test)
