#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)
COMPONENT_ADD_INCLUDEDIRS += component/cjson/include
COMPONENT_PRIV_INCLUDEDIRS += component/cjson/include
COMPONENT_ADD_LDFLAGS += -L$(IDF_PATH)/components/cjson/lib
COMPONENT_ADD_LDLIBS += -lcjson
