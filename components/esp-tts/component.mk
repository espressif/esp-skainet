COMPONENT_ADD_INCLUDEDIRS := esp_chinese_tts/include 


LIB_FILES := $(shell ls $(COMPONENT_PATH)/esp_chinese_tts/lib*.a) 

LIBS := $(patsubst lib%.a,-l%,$(LIB_FILES))

COMPONENT_ADD_LDFLAGS +=  -L$(COMPONENT_PATH)/esp_chinese_tts \
						  $(LIBS)

