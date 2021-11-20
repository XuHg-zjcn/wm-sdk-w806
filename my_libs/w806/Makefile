TOP_DIR = ../..
sinclude $(TOP_DIR)/tools/W806/conf.mk

ifndef PDIR
GEN_LIBS = libw806$(LIB_EXT)
COMPONENTS_libw806 = Src/libw806src$(LIB_EXT) 
endif

#DEFINES +=

sinclude $(TOP_DIR)/tools/W806/rules.mk
INCLUDES := $(INCLUDES) -I $(PDIR)include

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile


