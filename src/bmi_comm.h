/*
 * bmi_comm.h
 * Synchronous BMI calls. BMI communication is inherently asynchronous. We use
 * wrapper functions to create synchronous send/recv calls.
 *
 * Nawab Ali <alin@cse.ohio-state.edu>
 */

#ifndef _BMI_COMM_H_
#define _BMI_COMM_H_

#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>

#include "bmi.h"

#define MAX_IDLE_TIME 10
#define ION_ENV "ZOIDFS_ION_NAME"

int32_t bmi_comm_send(BMI_addr_t, void *, bmi_size_t, bmi_msg_tag_t,
                      bmi_context_id);
int32_t bmi_comm_recv(BMI_addr_t, void *, bmi_size_t, bmi_msg_tag_t,
                      bmi_context_id);
int32_t bmi_comm_sendu(BMI_addr_t, void *, bmi_size_t, bmi_msg_tag_t,
                       bmi_context_id);

#endif

/*
 * Local variables:
 *  mode: c
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=4 sts=4 sw=4 expandtab
 */
