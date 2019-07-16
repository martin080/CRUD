#include "getmac.h"

void mac_eth0(unsigned char MAC_str[13])
{
    #define HWADDR_len 6
    int s,i;
    struct ifreq ifr;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "br-wlan");
    ioctl(s, SIOCGIFHWADDR, &ifr);
    for (i=0; i<HWADDR_len; i++)
        sprintf(&MAC_str[i*2],"%02X",((unsigned char*)ifr.ifr_hwaddr.sa_data)[i]);
    MAC_str[12]='\0';

    int j = 0;
    while (MAC_str[j])
    {
	    MAC_str[j] = tolower(MAC_str[j]);
	    j++;
    }
}