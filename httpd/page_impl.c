#include "../rtl837x_common.h"
#include "uip.h"
#include "../html_data.h"

#pragma codeseg BANK1

extern __code uint8_t * __code hex;
extern __xdata uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask;
extern __code uint8_t ownMAC[];

#define PUTC(c) *outbuf++ = c;

#define PUTBYTE(a) { *outbuf++ = hex[(a >> 4) & 0xf]; *outbuf++ = hex[a & 0xf]; }

inline uint8_t itoa_html(uint8_t v, __xdata uint8_t *outbuf)
{
	uint8_t t = (v / 100) % 10;
	if (t)
		PUTC('0' + t);
	t = (v / 10) % 10;
	if (t)
		PUTC('0' + t);
	PUTC('0' + (v % 10));

	if (v >= 100)
		return 3;
	else if (v >= 10)
		return 2;
	else
		return 1;
}


uint16_t stat_content(__xdata uint8_t *outbuf)
{
	print_string("stat_content called\n");
	return 0;
}


uint16_t port_status(__xdata uint8_t *outbuf)
{
	print_string("port_status called\n");
	return 0;
}


uint16_t html_index(__xdata uint8_t *outbuf)
{
	__xdata uint8_t *oldptr = outbuf;

	print_string("html_index called\n");
	outbuf += strtox(outbuf, "<tr><td>IP Address</td><td>");
	outbuf += itoa_html(uip_hostaddr[0], outbuf); PUTC('.');
	outbuf += itoa_html(uip_hostaddr[0] >> 8, outbuf); PUTC('.');
	outbuf += itoa_html(uip_hostaddr[1], outbuf); PUTC('.');
	outbuf += itoa_html(uip_hostaddr[1] >> 8, outbuf);
	outbuf += strtox(outbuf, "</td></tr><tr><td>Gateway</td><td>");
	outbuf += itoa_html(uip_draddr[0], outbuf); PUTC('.');
	outbuf += itoa_html(uip_draddr[0] >> 8, outbuf); PUTC('.');
	outbuf += itoa_html(uip_draddr[1], outbuf); PUTC('.');
	outbuf += itoa_html(uip_draddr[1] >> 8, outbuf);
	outbuf += strtox(outbuf, "</td></tr><tr><td>Netmask</td><td>");
	outbuf += itoa_html(uip_netmask[0], outbuf); PUTC('.');
	outbuf += itoa_html(uip_netmask[0] >> 8, outbuf); PUTC('.');
	outbuf += itoa_html(uip_netmask[1], outbuf); PUTC('.');
	outbuf += itoa_html(uip_netmask[1] >> 8, outbuf);
	outbuf += strtox(outbuf, "</td></tr><tr><td>MAC Address</td><td>");
	PUTBYTE(ownMAC[0]); PUTC(':');
	PUTBYTE(ownMAC[1]); PUTC(':');
	PUTBYTE(ownMAC[2]); PUTC(':');
	PUTBYTE(ownMAC[3]); PUTC(':');
	PUTBYTE(ownMAC[4]); PUTC(':');
	PUTBYTE(ownMAC[5]);
	outbuf += strtox(outbuf, "</td></tr>");

	return outbuf - oldptr;
}
