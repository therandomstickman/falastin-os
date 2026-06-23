#include "lwip_port.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/init.h"
#include "lwip/dns.h"
#include "netif/etharp.h"

#include "rtl8139.h"
#include "screen.h"
#include "timer.h"
#include "malloc.h"
#include "libc.h"

static struct netif* netif = 0;
static int lwip_ready = 0;

u32_t sys_now(void) {
    return timer_ticks() * 10;
}

static err_t low_level_output(struct netif *netif, struct pbuf *p) {
    (void)netif;
    if (!lwip_ready) return ERR_IF;
    
    struct pbuf *q;
    u8_t *buffer = (u8_t*)malloc(p->tot_len);
    if (!buffer) return ERR_MEM;
    
    int len = 0;
    for (q = p; q != NULL; q = q->next) {
        memcpy(buffer + len, q->payload, q->len);
        len += q->len;
    }
    
    rtl8139_send_packet(buffer, len);
    free(buffer);
    return ERR_OK;
}

static struct pbuf* low_level_input(struct netif *netif) {
    (void)netif;
    if (!lwip_ready) return NULL;
    
    u8_t buffer[2048];
    int len = rtl8139_receive_packet(buffer, sizeof(buffer));
    if (len <= 0) return NULL;
    
    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) return NULL;
    
    pbuf_take(p, buffer, len);
    return p;
}

static err_t lwip_input_fn(struct pbuf *p, struct netif *inp) {
    (void)inp;
    if (!lwip_ready) return ERR_IF;
    return inp->input(p, inp);
}

static err_t netif_init_cb(struct netif *netif) {
    netif->hwaddr_len = 6;
    rtl8139_get_mac(netif->hwaddr);
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
    return ERR_OK;
}

static void netif_poll(struct netif *netif) {
    struct pbuf *p = low_level_input(netif);
    if (p != NULL) {
        if (lwip_input_fn(p, netif) != ERR_OK) {
            pbuf_free(p);
        }
    }
}

void lwip_init_port(void) {
    rtl8139_init();
    
    netif = (struct netif*)malloc(sizeof(struct netif));
    if (!netif) return;
    
    lwip_init();
    
    ip_addr_t ip, netmask, gw;
    IP4_ADDR(&ip, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 10, 0, 2, 2);

    netif_add(netif, &ip, &netmask, &gw, NULL, netif_init_cb, lwip_input_fn);
    netif_set_default(netif);
    netif_set_up(netif);
    
    ip_addr_t dns_server;
    IP4_ADDR(&dns_server, 8, 8, 8, 8);
    dns_setserver(0, &dns_server);
    
    lwip_ready = 1;
}

void lwip_poll(void) {
    if (!lwip_ready) return;
    sys_check_timeouts();
    netif_poll(netif);
}

void lwip_ping_test(void) {
    if (!lwip_ready) {
        print("lwIP not initialized. Run 'lwip' first.\n");
        return;
    }
    print("lwIP is ready!\n");
}