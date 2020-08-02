.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT must be set in the environment.")
endif

include $(PSL1GHT)/Makefile.base

TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCE		:=	source
INCLUDE		:=	include
DATA		:=	data
LIBS		:=	-lzip -lz -lfont -ltiny3d -lgcm_sys -lreality -lsysutil -lio -lm -lpngdec -ljpgdec -lnet -lsysmodule

ICON0       = $(SOURCE)/../ICON0.PNG
APPID		:=	PSL145310
CONTENTID	:=	UP0001-$(APPID)_00-0000000000000000
SFOXML		:=	package.xml

CFLAGS		+= -g -O2 -Wall --std=gnu99
CXXFLAGS	+= -g -O2 -Wall

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCE),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export BUILDDIR	:=	$(CURDIR)/$(BUILD)
export DEPSDIR	:=	$(BUILDDIR)

CFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
CXXFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
VCGFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.vcg)))

export OFILES	:=	$(CFILES:.c=.o) \
					$(CXXFILES:.cpp=.o) \
					$(SFILES:.S=.o) \
					$(BINFILES:.bin=.bin.o) \
					$(VCGFILES:.vcg=.vcg.o)

export BINFILES	:=	$(BINFILES:.bin=.bin.h)
export VCGFILES	:=	$(VCGFILES:.vcg=.vcg.h)

export INCLUDES	:=	$(foreach dir,$(INCLUDE),-I$(CURDIR)/$(dir)) \
					-I$(CURDIR)/$(BUILD)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo Clean...
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).a $(OUTPUT).pkg

pkg: $(BUILD)
	@echo Creating PKG...
	@mkdir -p $(BUILD)/pkg
	@mkdir -p $(BUILD)/pkg/USRDIR
	@cp $(ICON0) $(BUILD)/pkg/
	@cp $(BUILD)/../$(TARGET).self $(BUILD)/pkg/RELOAD.SELF
	#@$(FSELF) -n $(BUILD)/$(TARGET).elf $(BUILD)/pkg/USRDIR/EBOOT.BIN
	@$(SELF_NPDRM) $(BUILD)/$(TARGET).elf $(BUILD)/pkg/USRDIR/EBOOT.BIN $(CONTENTID)
	@$(SFO) -f $(SFOXML) $(BUILD)/pkg/PARAM.SFO
	@$(PKG) --contentid $(CONTENTID) $(BUILD)/pkg/ $(OUTPUT).pkg
	@cp $(OUTPUT).pkg $(OUTPUT).geohot.pkg
	@$(PKG_GEO) $(OUTPUT).geohot.pkg

pkgh341: $(BUILD)
	@echo Creating PKG...
	@mkdir -p $(BUILD)/pkg
	@mkdir -p $(BUILD)/pkg/USRDIR
	@cp $(ICON0) $(BUILD)/pkg/
	@cp $(BUILD)/../$(TARGET).self $(BUILD)/pkg/RELOAD.SELF
	@cp $(BUILD)/../payload/EBOOT.BIN $ $(BUILD)/pkg/USRDIR/EBOOT.BIN
	@$(SFO) -f $(SFOXML) $(BUILD)/pkg/PARAM.SFO
	@$(PKG) --contentid $(CONTENTID) $(BUILD)/pkg/ $(OUTPUT).cfw_hermes_3.41.pkg

run: $(BUILD)
	@$(PS3LOADAPP) $(OUTPUT).self

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
$(OFILES): $(BINFILES) $(VCGFILES)

-include $(DEPENDS)

endif
