// #define REGDBG 1

#include "../rtl837x_sfr.h"
#include "../rtl837x_common.h"
#include "../rtl837x_regs.h"
#include "../rtl837x_port.h"
#include "uip.h"
#include "../html_data.h"
#include <stdint.h>

#pragma codeseg BANK1

extern __xdata uint8_t outbuf[TCP_OUTBUF_SIZE];
extern __xdata uint16_t slen;
extern __code uint8_t * __code hex;
extern __xdata uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask;
extern __code struct uip_eth_addr uip_ethaddr;
extern __code uint8_t log_to_phys_port[9];
extern __code uint8_t phys_to_log_port[6];

extern __xdata uint8_t minPort;
extern __xdata uint8_t maxPort;
extern __xdata uint8_t nSFPPorts;
extern __xdata uint8_t sfr_data[4];
extern __xdata uint8_t cpuPort;
extern __xdata uint8_t isRTL8373;
extern __xdata uint8_t sfp_pins_last;
extern __xdata uint8_t vlan_names[VLAN_NAMES_SIZE];


/*  Convert only the lower nibble to ascii HEX char.
    For convenience the upper nibble is masked out.
*/
inline char itohex(uint8_t val) {
	// Ignore upper nibble for convenience.
	val &= 0x0f;
	val -= 10;

	// 10 or above
	if ((int8_t)val >= 0)
		val += ('a' - '0' - 10);

	return val + ('0' + 10);
}


void char_to_html(char c)
{
	outbuf[slen++] = c;
}


//  Convert uint8_t to ascii HEX char.
void byte_to_html(uint8_t val)
{
	uint8_t cnt = 2;
	do {
		val = (val >> 4) | (val << 4);
		char_to_html(itohex(val));
		cnt -= 2;
	} while(cnt);
}


void itoa_html(uint8_t v)
{
	uint8_t t = (v / 100) % 10;
	if (t)
		char_to_html('0' + t);
	t = (v / 10) % 10;
	if (t)
		char_to_html('0' + t);
	char_to_html('0' + (v % 10));
}


uint16_t stat_content(void)
{
	print_string("stat_content called\n");
	return 0;
}


uint16_t port_status(void)
{
	print_string("port_status called\n");
	return 0;
}


/* Converts sfr_data[] into raw hex string.
   Suppress leading zeros.
*/
void sfr_data_to_html(void)
{
 	uint8_t print_zeros = 0;
	uint8_t val = 0;

	for (uint8_t nibble = 0; nibble < 8; nibble++) {
	  	if (!(nibble & 1))
	        val = sfr_data[nibble>>1];
		// force the swap instruction, itohex() ignores the upper nibble.
		val = (val << 4) | (val >> 4);
		// when print_zeros is not zero, we know that a non-zero number has printed.
		// That have to print all the next numbers.
		print_zeros |= val;
		// only care about lower nibble, that is what is printed.
		print_zeros &= 0x0f;
		if (print_zeros)
			charhex_to_html(val);
	}
	if (print_zeros == 0) {
	    char_to_html('0');
	}
}


void reg_to_html(register uint16_t reg)
{
	reg_read_m(reg);
	sfr_data_to_html();
}


uint16_t html_index(void)
{
	print_string("html_index called\n");
	slen += strtox(outbuf + slen, "<tr><td>IP Address</td><td>");
	itoa_html(uip_hostaddr[0]); char_to_html('.');
	itoa_html(uip_hostaddr[0] >> 8); char_to_html('.');
	itoa_html(uip_hostaddr[1]); char_to_html('.');
	itoa_html(uip_hostaddr[1]);
	slen += strtox(outbuf + slen, "</td></tr><tr><td>Gateway</td><td>");
	itoa_html(uip_draddr[0]); char_to_html('.');
	itoa_html(uip_draddr[0] >> 8); char_to_html('.');
	itoa_html(uip_draddr[1]); char_to_html('.');
	itoa_html(uip_draddr[1] >> 8);
	slen += strtox(outbuf + slen, "</td></tr><tr><td>Netmask</td><td>");
	itoa_html(uip_netmask[0]); char_to_html('.');
	itoa_html(uip_netmask[0] >> 8); char_to_html('.');
	itoa_html(uip_netmask[1]); char_to_html('.');
	itoa_html(uip_netmask[1] >> 8);
	slen += strtox(outbuf + slen, "</td></tr><tr><td>MAC Address</td><td>");
	byte_to_html(uip_ethaddr.addr[0]); char_to_html(':');
	byte_to_html(uip_ethaddr.addr[1]); char_to_html(':');
	byte_to_html(uip_ethaddr.addr[2]); char_to_html(':');
	byte_to_html(uip_ethaddr.addr[3]); char_to_html(':');
	byte_to_html(uip_ethaddr.addr[4]); char_to_html(':');
	byte_to_html(uip_ethaddr.addr[5]);
	slen += strtox(outbuf + slen, "</td></tr>");
	return 0;
}


void send_vlan(__xdata uint16_t vlan)
{
	slen = strtox(outbuf, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
	print_string("sending VLAN\n");
	//{"members":"0x00060011"}
	slen += strtox(outbuf + slen, "{\"members\":\"0x");
	vlan_get(vlan);
	sfr_data_to_html();
	slen += strtox(outbuf + slen, "\",\"name\":\"");
	__xdata uint16_t n = vlan_name(vlan);
	if (n== 0xffff) {
		print_string("VLAN has no name\n");
	} else {
		while(vlan_names[n] && vlan_names[n] != ' ')
			char_to_html(vlan_names[n++]);
	}
	slen += strtox(outbuf + slen, "\"}");
}


void send_counters(char port)
{
	print_string("send_counters called: "); print_byte(port); write_char('\n');
	slen = strtox(outbuf, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
	print_string("sending counters\n");

	port--;
	uint8_t i = isRTL8373 ? port - 1: phys_to_log_port[port];
	slen += strtox(outbuf + slen, "{\"portNum\":");
	itoa_html(i + 1);
	for (uint8_t j = 0; j < 0x3f; j++) {
		STAT_GET(j, i);
		slen += strtox(outbuf + slen, ",\"cnt_");
		itoa_html(j);
		slen += strtox(outbuf + slen, "\":\"0x");
		reg_to_html(RTL837X_STAT_V_HIGH);
		reg_to_html(RTL837X_STAT_V_LOW);
		char_to_html('\"');
	}
	char_to_html('}');
}


void send_status(void)
{
	slen = strtox(outbuf, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
	print_string("sending status\n");
	char_to_html('[');

	for (uint8_t i = minPort; i <= maxPort; i++) {
		slen += strtox(outbuf + slen, "{\"portNum\":");
		if (!isRTL8373)
			itoa_html(log_to_phys_port[i]);
		else
			itoa_html(i + 1);

		slen += strtox(outbuf + slen, ",\"isSFP\":");
		if (IS_SFP(i)) {
			char_to_html('1');
			slen += strtox(outbuf + slen, ",\"enabled\":");
			if (!((sfp_pins_last >> (i == maxPort ? 0 : 4)) & 1)) {
				char_to_html('1');
			} else {
				char_to_html('0');
			}
			slen += strtox(outbuf + slen, ",\"link\":");
			uint8_t rate = sfp_read_reg(i == maxPort ? 0 : 1, 12);
			if (rate == 0xd)
				char_to_html('2'); // 1000BX
			else if (rate == 0x1f)
				char_to_html('5'); // 2G5
			else if (rate > 0x65 && rate < 0x70)
				char_to_html('4'); // 10G "4" is not a valid value for port LINK speed
			else
				char_to_html('1'); // 100M ???
		} else {
			char_to_html('0');
			slen += strtox(outbuf + slen, ",\"enabled\":");
			phy_read(i, 0x1f, 0xa610);
			if (SFR_DATA_8 == 0x20)
				char_to_html('1');
			else
				char_to_html('0');
			slen += strtox(outbuf + slen, ",\"link\":");
			reg_read_m(RTL837X_REG_LINKS);
			uint8_t b = sfr_data[3 - (i >> 1)];
			b = (i & 1) ? b >> 4 : b & 0xf;
			char_to_html('0' + b);
		}
		STAT_GET(STAT_COUNTER_TX_PKTS, i);
		slen += strtox(outbuf + slen, ",\"txG\":\"0x");
		reg_to_html(RTL837X_STAT_V_HIGH);
		reg_to_html(RTL837X_STAT_V_LOW);

		slen += strtox(outbuf + slen, "\",\"txB\":\"0x");
		STAT_GET(STAT_COUNTER_ERR_PKTS, i);
		reg_to_html(RTL837X_STAT_V_LOW);	// 32 bit Tx Packet errors

		slen += strtox(outbuf + slen, "\",\"rxG\":\"0x");
		STAT_GET(STAT_COUNTER_RX_PKTS, i);
		reg_to_html(RTL837X_STAT_V_HIGH);
		reg_to_html(RTL837X_STAT_V_LOW);

		slen += strtox(outbuf + slen, "\",\"rxB\":\"0x");
		STAT_GET(STAT_COUNTER_ERR_PKTS, i);
		reg_to_html(RTL837X_STAT_V_HIGH);	// 32bit RX packet errors
		slen += strtox(outbuf + slen, "\"}");
		if (i < maxPort)
			char_to_html(',');
		else
			char_to_html(']');
	}
}
