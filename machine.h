#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <stdint.h>
#include <stdbool.h>

#define MACHINE_STR_HELPER(x)  #x
#define MACHINE_STR(x) MACHINE_STR_HELPER(x)


#define MACHINE_DEF_DIR machine

#ifndef MACHINE_NAME
	#define MACHINE_NAME default_machine
#endif

#define MACHINE_INC(dir,file) MACHINE_STR(dir/file)

#define RTL8370 0x70
#define RTL8372 0x72
#define RTL8373 0x73

#include  MACHINE_INC(MACHINE_DEF_DIR,MACHINE_NAME.h)


#ifndef MACHINE_CHIPSET
#error "MACHINE_CHIPSET shall be defined in <machine>.h file"
#endif

#ifndef MACHINE_MIN_PORT
#if MACHINE_CHIPSET == RTL8372
#define MACHINE_MIN_PORT 3
#else
#define MACHINE_MIN_PORT 0
#endif
#endif

#ifndef MACHINE_MAX_PORT
#if MACHINE_CHIPSET == RTL8370
#define MACHINE_MAX_PORT 7
#else
#define MACHINE_MAX_PORT 8
#endif
#endif

#define MACHINE_PORT_VALID(p) \
	((uint8_t)((p) - MACHINE_MIN_PORT) <= (MACHINE_MAX_PORT - MACHINE_MIN_PORT))

// auto derive SOC_FAMILY
#define RTL836X 6
#define RTL837X 7

#ifdef SOC_FAMILY
#error "SOC_FAMILY can not be manually defined, shall be auto derived"
#endif
#if (MACHINE_CHIPSET == RTL8372) || (MACHINE_CHIPSET == RTL8373)
#define SOC_FAMILY RTL837X
#elif (MACHINE_CHIPSET == RTL8370 )
#define SOC_FAMILY RTL836X
#else
#error "Unsupported MACHINE_CHIPSET is specified!"
#endif

#define LED_27 1
// SYSTEM LED
#define LED_28_SYS 2
#define LED_29 4

struct high_leds {
	// Defines MUX and LED enabling for pins 27-29
	uint8_t mux : 3;
	uint8_t enable : 3;
	uint8_t reserved : 2;
};

// Port/Mac not used/connected,
#define NOP (0x00)
#define IS_PHYS_PORT_INVALID(port) (port == NOP)

// SDSx in on PORT/MACx
#define MAC_SDS0 (3)
#define MAC_SDS1 (8)

enum sds_type {
	SDS_UNUSED = 0, 
	SDS_SFP,
	SDS_EPHY,
	SDS_FIXED_LINK,

	// Used as error code
	SDS_NOT_A_SDS_PORT = -1
};


// External PHY types.
enum phy_type {
	RTL8224,   // Default 4-port 2.5Gbit PHY which normaly used with a RTL8382
	RTL8221B,  // 1-port 2.5Gbit PHY
	RTL8261BE, // 1-port  10gbit PHY
};

struct sfp_port
{
	uint8_t pin_detect; // gpio number 0-63, 0xFF = don't have it?
	uint8_t pin_los; // gpio number 0-63, 0xFF = don't have it?
	uint8_t pin_tx_disable; // gpio number 0-63, 0xFF = not present
	uint8_t i2c;
};

struct ext_phy
{
	enum phy_type type;
	uint8_t phy_addr;
	uint8_t reset_pin;
};

struct sds_settings {
	enum sds_type usage;
	union {
		struct sfp_port sfp;
		struct ext_phy  ephy;
	} sds_settings_t;
};

struct machine {
	char machine_name[30];
	uint8_t isRTL8373;
	uint8_t log_to_phys_port[9];
	// sfp_port[0] is directly linked to MAC 3 / SDS0 (MAC_SDS0)
	// sfp_port[1] is directly linked to MAC 8 / SDS1 (MAC_SDS1)
	// sfp_port struct holds the settings for the devices connected to the SDSx-port.
	// - In case of SFP-CAGE, i2c-setting, gpio to detect the device etc.
	// - In case of external PHY, MDIO-address, max-linkspeed etc.
	// - In case of fixed-link, linkspeed.
	struct sds_settings sds_settings[2];
	uint8_t reset_pin;
	struct high_leds high_leds;
	// Defines which led-set (0-3) will be used for given logical port
	// led-set is physical group of LEDs that can be configured to show different port status combinations (see port_led_set below)
	uint8_t port_led_set[9];
	// Defines led-set configuration, applied to all ports using particular led-set
	// Each led-set can have 4 different hardware LED configurations. Which one should be used, depends how LED is wired on the board
	// See stock RTL837X_REG_LED3_2_SETx and RTL837X_REG_LED1_0_SETx registers for reference configuration
	uint32_t led_sets[4][4];
	uint8_t led_mux_custom;
	uint8_t led_mux[28];
	uint32_t mac_flash_offset;
	// Read the flash with single-IO (CMD_FREAD) instead of DIO for MMIO/code
	// fetch. Set for boards whose flash chip is not usable in DIO mode, which
	// shows up as a board that does not boot at all, garbled console output or
	// random resets. Costs management-plane speed only, the ASIC keeps
	// forwarding in hardware.
	uint8_t flash_sio;
};

struct machine_runtime
{
	uint8_t isRTL8373 : 1;
	uint8_t isN : 1;
};

void machine_custom_init(void) __banked;
int8_t phys_to_log_port(uint8_t phys_port);
bool is_slot_sfp(uint8_t slot);
int8_t port_to_sds(uint8_t log_port);
enum sds_type port_to_sds_usage(uint8_t log_port);

#endif
