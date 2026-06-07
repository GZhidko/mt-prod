#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "const-c.inc"

#include "uammsg.h"
#include "uamclt.h"
#include "sweetuam.h"

int global_debug_flag = 0;

MODULE = SweetAuth		PACKAGE = SweetAuth		PREFIX = swa_

INCLUDE: const-xs.inc

int
swa_sweetauth(...)
	PROTOTYPE: $$;$$$$$$$$$$$
	PPCODE:
	{
    sw_uamclt_group_t *group;
	char *sw_argv[SW_UAM_ARG_MAX];
	int rc;
	char *errmsg;
	int i;

	for (i=0;i<SW_UAM_ARG_MAX;i++) {
		sw_argv[i]="";
	}
	// warn("sweetauth items=%d",items);
	for (i=0;i<SW_UAM_ARG_MAX && i<items;i++) {
		sw_argv[i] = (char*)SvPV_nolen(ST(i));
		// warn("sweetauth arg[%d]=\"%s\" (%d)",
		//		i,sw_argv[i],sw_largv[i]);
	}
	sw_argv[i]=NULL;
	if (sw_config_load(sw_uamclt_default_config_file) < 0) {
		rc=-1;
		errmsg="config-load-falure";
	} else if (sw_uam_create_client(&group) != -1) {
		// warn("authorize: before send");
		rc=sw_uam_send_msg(group, sw_argv);
		// warn("authorize: after send rc=%d",rc);
		if (rc == 0) rc=sw_uam_recv_msg(group, sw_argv);
		else errmsg="send-msg-failure";
		if (rc) errmsg="recv-msg-failure";
		(void)sw_uam_destroy_client(group);
	} else {
		rc=-1;
		errmsg="create-client-failure";
	}

	// warn("authorize: after recv rc=%d",rc);
	if (rc == 0) {
		for (i=0;i<SW_UAM_ARG_MAX && sw_argv[i];i++) {
			EXTEND(sp, 1);
			PUSHs(sv_2mortal(newSVpv(sw_argv[i],strlen(sw_argv[i]))));
		}
	} else {
		EXTEND(sp, 3);
		PUSHs(sv_2mortal(newSVpv("ER",strlen("ER"))));
		PUSHs(sv_2mortal(newSVpv(sw_argv[1],strlen(sw_argv[1]))));
		PUSHs(sv_2mortal(newSVpv(errmsg,strlen(errmsg))));
	}
	}
