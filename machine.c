#include <stdint.h>
#include <stdbool.h>
#include <machine.h>

extern __code const struct machine machine;

/* Looks-up the physical port.
 * The index into machine.log_to_phys_port = {0, 0, 0, 5, 1, 2, 3, 4, 6}, is equal the logical port.
 * Returns the positive number when found
 * Returns -1 when not found
 */
int8_t phys_to_log_port(uint8_t phys_port) {
	uint8_t port = MACHINE_MIN_PORT;
	uint8_t port_max = MACHINE_MAX_PORT;

	// .log_to_phys_port = {1, 2, 3, 4, 5, 6, 7, 8, 9},
	do {
		uint8_t phys_port_val = machine.log_to_phys_port[port];
		if (phys_port_val == phys_port && phys_port_val != NOP)
			return (int8_t)port;
		port++;
	} while(port <= port_max);

	return -1;
}

/* return a bool is a slot is a marked as SFP-port. */
bool is_slot_sfp(uint8_t slot) {
	return machine.sds_settings[slot].usage == SDS_SFP;
}

/* Check if the port is a SDS port
 * Returns sds-number when port has SDS
 * Returns -1 on error
*/
int8_t port_to_sds(uint8_t log_port) {
	bool sds = log_port == MAC_SDS1;
	if (sds || log_port == MAC_SDS0)
		return sds;
	// return -1, subtracking is cheaper.
	return sds - 1;
}

/* Check if the port is a SDS port
 * Returns sds-usage when port has SDS
 * Returns -1 / SDS_NOT_A_SDS_PORT on error
*/
enum sds_type port_to_sds_usage(uint8_t log_port) {
	bool sds = log_port == MAC_SDS1;
	if (sds || log_port == MAC_SDS0)
		return machine.sds_settings[sds].usage;
	// return -1, subtracking is cheaper.
	return SDS_NOT_A_SDS_PORT;
}
