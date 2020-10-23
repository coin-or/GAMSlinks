../utils/optcc.c : $(GAMS_PATH)/apifiles/C/api/optcc.c
	@cp $< $@

../utils/gmomcc.c : $(GAMS_PATH)/apifiles/C/api/gmomcc.c
	@cp $< $@

../utils/gevmcc.c : $(GAMS_PATH)/apifiles/C/api/gevmcc.c
	@cp $< $@

../utils/gclgms.c : $(GAMS_PATH)/apifiles/C/api/gclgms.c
	@cp $< $@

../utils/gdxcc.c : $(GAMS_PATH)/apifiles/C/api/gdxcc.c
	@cp $< $@

../utils/palmcc.c : $(GAMS_PATH)/apifiles/C/api/palmcc.c
	@cp $< $@

../utils/cfgmcc.c : $(GAMS_PATH)/apifiles/C/api/cfgmcc.c
	@cp $< $@

CLEANFILES += ../utils/optcc.c ../utils/gmomcc.c ../utils/gevmcc.c ../utils/gclgms.c ../utils/gdxcc.c ../utils/palmcc.c ../utils/cfgmcc.c

AM_CPPFLAGS += -I"$(GAMS_PATH)/apifiles/C/api" $(GAMSLIBCFLAGS)

## for api files from GAMS >= 33
## eventually, this should be removed, gcmt.c should be compiled as well, and enabling threadSafeIndic in gamsconfig.yaml should be considered
AM_CPPFLAGS += -DGC_NO_MUTEX
