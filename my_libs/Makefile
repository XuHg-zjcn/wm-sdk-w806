TOP_DIR = ..
sinclude $(TOP_DIR)/tools/W806/conf.mk

ifndef PDIR
GEN_LIBS = libmylibs$(LIB_EXT)
COMPONENTS_libmylibs = w806/libw806$(LIB_EXT) 
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/W806/rules.mk
INCLUDES := $(INCLUDES) -I $(PDIR)include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

#TODO:软链接文件夹无法编译，以后再搞
