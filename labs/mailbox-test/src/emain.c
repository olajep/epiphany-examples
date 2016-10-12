#include <e-lib.h>
#include <stdint.h>

#include "common.h"

#define FOO_ADDR 0x8e000000
#define MAILBOX_ADDR 0x810F0730
/* TODO: Move to e-lib */
void e_send_message(uint32_t data)
{
	volatile struct e_message *mailbox = (struct e_message *) MAILBOX_ADDR;
	struct e_message msg;
	int i;

	msg.from = e_get_coreid();
	msg.data = data;
	msg.from = data;

	/* FIXME: 64-bit burst writes to same address is broken in FPGA elink.
	 * For now resort to 32-bit messages */
	__asm__ __volatile__ (
		"str %[msg],[%[mailbox]]"
		:
		: [msg] "r" (msg), [mailbox] "r" (mailbox)
		: "memory");
}

/* access external ram to create some interleavings between queues in FPGA
 * elink */
int make_some_noise()
{
    volatile uint32_t *foo = (uint32_t *) 0x8f100000;

    while (true)
        *foo += 5;

    return 0;
}

int main()
{
	volatile uint32_t *step = (uint32_t *) STEP_ADDR;
	volatile uint32_t *stop = (uint32_t *) STOP_ADDR;
	volatile uint32_t *foop = (uint32_t *) FOO_ADDR;

	//uint32_t foo = *foop;
	uint32_t foo = 0;
	uint32_t prev_step = 0;


    // create som elink traffic
    if (e_get_coreid() != 0x808)
        return make_some_noise();

	int i;

	while (!(*stop)) {
		while (prev_step == *step && !(*stop))
			;

		if (*stop)
			break;

		/* Create delay so host app have to wait */
		for (i = 0; i < 100000000; i++)
			__asm__ __volatile__ ("nop" ::: "memory");

		e_send_message(prev_step + foo + 1);

		prev_step++;
	}

	for (i = 0; i < NMESSAGES; i++)
		e_send_message(i);
}
