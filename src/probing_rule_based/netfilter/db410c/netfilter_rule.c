// Decision Tree using netfilter and LKM

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>	//Needed for LINUX_VERSION_CODE <= KERNEL_VERSION
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

static struct nf_hook_ops simpleFilterHook;

// implementation of Filter callback function - Netfilter Hook
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0)
static unsigned int simpleFilter(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *skb))
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
static unsigned int simpleFilter(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *skb))
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0)
static unsigned int simpleFilter(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct nf_hook_state *state)
#else
static unsigned int simpleFilter(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
#endif
{

    struct ethhdr *ethh;
    struct iphdr  *iph; 	// ip header struct
    struct tcphdr *tcph;	// tcp header struct
    struct udphdr *udph;	// udp header struct

    ethh = eth_hdr(skb);
    iph = ip_hdr(skb);

    if (!(iph)){
	return NF_ACCEPT;
    }

    if (iph->protocol == IPPROTO_TCP){ // TCP Protocol
	tcph = tcp_hdr(skb);
	// rule-based probing block
	/* tcp flags =  XXX_ ____ ____ = Reserved
			___X ____ ____ = Nonce
			____ X___ ____ = CWR - Congestion Window Reduced
			____ _X__ ____ = ECN-Echo
			____ __X_ ____ = Urgent
			____ ___X ____ = Acknowledgment
			____ ____ X___ = Push
			____ ____ _X__ = Reset
			____ ____ __X_ = Syn
			____ ____ ___X = Fin
	*/
	if (tcph->window == htons(1024) || tcph->window == htons(2048) || tcph->window == htons(3072) || tcph->window == htons(4096)){
	    if (tcph->fin == 1 && tcph->psh == 1 && tcph->urg == 1){
		// XMAS SCAN
		printk(KERN_INFO "TCP XMAS Scan blocked\n");
		return NF_DROP;
	    } else if (tcph->fin == 1 && tcph->cwr == 0 && tcph->ece == 0 && tcph->urg == 0 && tcph->ack == 0 && tcph->psh == 0 && tcph->rst==0 && tcph->syn==0) {
		// FIN SCAN
		printk(KERN_INFO "TCP FIN Scan blocked\n");
		return NF_DROP;
	    } else if (tcph->fin == 0 && tcph->cwr == 0 && tcph->ece == 0 && tcph->urg == 0 && tcph->ack == 0 && tcph->psh == 0 && tcph->rst==0 && tcph->syn==0) {
		// NULL SCAN
		printk(KERN_INFO "TCP NULL Scan blocked\n");
		return NF_DROP;
	    } else if (tcph->fin == 1){
		// SYN SCAN
		printk(KERN_INFO "TCP SYN Scan blocked\n");
		return NF_DROP;
	    }
	} else {
	    return NF_ACCEPT;
	}
    } else if (iph->protocol == IPPROTO_UDP) { // UDP Protocol
	udph = udp_hdr(skb);
	return NF_ACCEPT;
    } 
    return NF_ACCEPT;
}

// Netfilter hook

static struct nf_hook_ops simpleFilterHook = {
    .hook	= simpleFilter,
    .hooknum	= NF_INET_POST_ROUTING,
    .pf		= PF_INET,
    .priority	= NF_IP_PRI_FIRST,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0)
    .owner	= THIS_MODULE
#endif
};

int setUpFilter(void){
    printk(KERN_INFO "Registering Simple Filter.\n");

    //register the hook
    #if LINUX_VERSION_CODE <= KERNEL_VERSION(4,12,14)
    	nf_register_hook(&simpleFilterHook);
    #else
	nf_register_net_hook(&init_net, &simpleFilterHook);
    #endif
    return 0;
}

void removeFilter(void){
    printk(KERN_INFO "Simple Filter is being removed.\n");
    
    //unregister the hook
    #if LINUX_VERSION_CODE <= KERNEL_VERSION(4,12,14)
	nf_register_hook(&simpleFilterHook);
    #else
        nf_unregister_net_hook(&init_net, &simpleFilterHook);
    #endif
}

module_init(setUpFilter);
module_exit(removeFilter);

MODULE_LICENSE("GPL");
