void pci_init(int type);
void pci_slot(int card);

void pci_add_specific(int card, uint8_t (*read)(int func, int addr, void *priv), void (*write)(int func, int addr, uint8_t val, void *priv), void *priv);
int pci_add(uint8_t (*read)(int func, int addr, void *priv), void (*write)(int func, int addr, uint8_t val, void *priv), void *priv);
void pci_add_card(uint8_t add_type, uint8_t(*read)(int func, int addr, void *priv), void (*write)(int func, int addr, uint8_t val, void *priv), void *priv, uint8_t *slot);

void pci_set_irq_routing(int card, int irq);
void pci_set_card_routing(int card, int pci_int);
void pci_set_irq(int card, int pci_int, uint8_t *state);
void pci_clear_irq(int card, int pci_int, uint8_t *state);


#define PCI_REG_COMMAND 0x04

#define PCI_COMMAND_IO  0x01
#define PCI_COMMAND_MEM 0x02

#define PCI_CONFIG_TYPE_1 1
#define PCI_CONFIG_TYPE_2 2

#define PCI_INTA 1
#define PCI_INTB 2
#define PCI_INTC 3
#define PCI_INTD 4

#define PCI_IRQ_DISABLED -1

enum {
    PCI_ADD_NORTHBRIDGE = 0,
    PCI_ADD_NORTHBRIDGE_SEC = 1,
    PCI_ADD_AGPBRIDGE = 2,
    PCI_ADD_SOUTHBRIDGE = 3,
    PCI_ADD_SOUTHBRIDGE_IDE = 4,
    PCI_ADD_SOUTHBRIDGE_PMU = 5,
    PCI_ADD_SOUTHBRIDGE_USB = 6,
    PCI_ADD_AGP = 0x0f,
    PCI_ADD_NORMAL = 0x10,
    PCI_ADD_VIDEO = 0x11,
    PCI_ADD_HANGUL = 0x12,
    PCI_ADD_IDE = 0x13,
    PCI_ADD_SCSI = 0x14,
    PCI_ADD_SOUND = 0x15,
    PCI_ADD_MODEM = 0x16,
    PCI_ADD_NETWORK = 0x17,
    PCI_ADD_UART = 0x18,
    PCI_ADD_USB = 0x19,
    PCI_ADD_BRIDGE = 0x1a
};

extern int pci_burst_time, pci_nonburst_time;
