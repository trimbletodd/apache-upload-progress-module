mod_upload_progress.la: mod_upload_progress.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_upload_progress.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_upload_progress.la
