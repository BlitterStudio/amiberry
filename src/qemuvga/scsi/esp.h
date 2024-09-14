#ifndef QEMU_HW_ESP_H
#define QEMU_HW_ESP_H

/* esp.c */
#define ESP_MAX_DEVS 7
typedef int (*ESPDMAMemoryReadWriteFunc)(void *opaque, uint8_t *buf, int len);
void esp_init(hwaddr espaddr, int it_shift,
              ESPDMAMemoryReadWriteFunc dma_memory_read,
              ESPDMAMemoryReadWriteFunc dma_memory_write,
              void *dma_opaque, qemu_irq irq, qemu_irq *reset,
              qemu_irq *dma_enable);

#define ESP_REGS 16
#define TI_BUFSZ 16

typedef struct ESPState ESPState;

struct ESPState {
    uint8_t rregs[ESP_REGS * 2];
    uint8_t wregs[ESP_REGS * 2];
    qemu_irq irq;
	int irq_raised;
    uint8_t chip_id;
    uint8_t tchi_has_id;
    int32_t ti_size;
	int32_t dma_len;
    int32_t ti_rptr, ti_wptr;
    uint32_t status;
    uint32_t dma;
    uint8_t ti_buf[TI_BUFSZ];
    SCSIBus bus;
    SCSIDevice *current_dev;
    SCSIRequest *current_req;
    uint8_t cmdbuf[TI_BUFSZ];
    uint32_t cmdlen;
    uint32_t do_cmd;

    /* The amount of data left in the current DMA transfer.  */
    uint32_t dma_left;
	uint32_t dma_pending; // fakedma pending count
    /* The size of the current DMA transfer.  Zero if no transfer is in
       progress.  */
    int32_t dma_counter;
    int dma_enabled;
	int pio_on;
	int fifo_on;
	int transfer_complete;

    int32_t async_len;
    uint8_t *async_buf;

    ESPDMAMemoryReadWriteFunc dma_memory_read;
    ESPDMAMemoryReadWriteFunc dma_memory_write;
    void *dma_opaque;
    int (*dma_cb)(ESPState *s);
	int fas4xxextra;
	int fas408sig;
	uint8_t fas408_buffer[128+1];
	int fas408_buffer_size;
	int fas408_buffer_offset;
};

#define ESP_TCLO   0x0
#define ESP_TCMID  0x1
#define ESP_FIFO   0x2
#define ESP_CMD    0x3
#define ESP_RSTAT  0x4
#define ESP_WBUSID 0x4
#define ESP_RINTR  0x5
#define ESP_WSEL   0x5
#define ESP_RSEQ   0x6
#define ESP_WSYNTP 0x6
#define ESP_RFLAGS 0x7
#define ESP_WSYNO  0x7
#define ESP_CFG1   0x8
#define ESP_RRES1  0x9
#define ESP_WCCF   0x9
#define ESP_RRES2  0xa
#define ESP_WTEST  0xa
#define ESP_CFG2   0xb
#define ESP_CFG3   0xc
#define ESP_RES3   0xd
#define ESP_TCHI   0xe
#define ESP_RES4   0xf

#define CMD_DMA 0x80
#define CMD_CMD 0x7f

#define CMD_NOP      0x00
#define CMD_FLUSH    0x01
#define CMD_RESET    0x02
#define CMD_BUSRESET 0x03
#define CMD_TI       0x10
#define CMD_ICCS     0x11
#define CMD_MSGACC   0x12
#define CMD_PAD      0x18
#define CMD_SATN     0x1a
#define CMD_RSTATN   0x1b
#define CMD_SEL      0x41
#define CMD_SELATN   0x42
#define CMD_SELATNS  0x43
#define CMD_ENSEL    0x44
#define CMD_DISSEL   0x45

#define STAT_DO 0x00
#define STAT_DI 0x01
#define STAT_CD 0x02
#define STAT_ST 0x03
#define STAT_MO 0x06
#define STAT_MI 0x07
#define STAT_PIO_MASK 0x06

#define STAT_TC 0x10
#define STAT_PE 0x20
#define STAT_GE 0x40
#define STAT_INT 0x80

#define BUSID_DID 0x07

#define INTR_FC 0x08
#define INTR_BS 0x10
#define INTR_DC 0x20
#define INTR_RST 0x80

#define SEQ_0 0x0
#define SEQ_CD 0x4

#define CFG1_RESREPT 0x40

#define TCHI_FAS100A 0x4
#define TCHI_AM53C974 0x12


/*
The following registers are only on the ESP406/FAS408.  The
documentation refers to them as "Control Register Set #1".
These are the registers that are visible when bit 7 of
register 0x0d is set.  This bit is common to both register sets.
*/

#define	NCR_JMP		0x00		/* RO - Jumper Sense Register	*/
#define	 NCRJMP_RSVD	0xc0		/*	reserved		*/
#define	 NCRJMP_ROMSZ	0x20		/*	ROM Size 1=16K, 0=32K	*/
#define	 NCRJMP_J4	0x10		/*	Jumper #4		*/
#define	 NCRJMP_J3	0x08		/*	Jumper #3		*/
#define	 NCRJMP_J2	0x04		/*	Jumper #2		*/
#define	 NCRJMP_J1	0x02		/*	Jumper #1		*/
#define	 NCRJMP_J0	0x01		/*	Jumper #0		*/

#define	NCR_PIOFIFO	0x04		/* WO - PIO FIFO, 4 bytes deep	*/

#define	NCR_PSTAT	0x08		/* RW - PIO Status Register	*/
#define	 NCRPSTAT_PERR	0x80		/*	PIO Error		*/
#define	 NCRPSTAT_SIRQ	0x40		/*	Active High of SCSI IRQ */
#define	 NCRPSTAT_ATAI	0x20		/*	ATA IRQ			*/
#define	 NCRPSTAT_FEMPT	0x10		/*	PIO FIFO Empty		*/
#define	 NCRPSTAT_F13	0x08		/*	PIO FIFO 1/3		*/
#define	 NCRPSTAT_F23	0x04		/*	PIO FIFO 2/3		*/
#define	 NCRPSTAT_FFULL	0x02		/*	PIO FIFO Full		*/
#define	 NCRPSTAT_PIOM	0x01		/*	PIO/DMA Mode		*/

#define	NCR_PIOI	0x0b		/* RW - PIO Interrupt Enable	*/
#define	 NCRPIOI_RSVD	0xe0		/*	reserved		*/
#define	 NCRPIOI_EMPTY	0x10		/*	IRQ When Empty		*/
#define	 NCRPIOI_13	0x08		/*	IRQ When 1/3		*/
#define	 NCRPIOI_23	0x04		/*	IRQ When 2/3		*/
#define	 NCRPIOI_FULL	0x02		/*	IRQ When Full		*/
#define	 NCRPIOI_FINV	0x01		/*	Flag Invert		*/

#define	NCR_CFG5	0x0d		/* RW - Configuration #5	*/
#define	 NCRCFG5_CRS1	0x80		/*	Select Register Set #1	*/
#define	 NCRCFG5_SRAM	0x40		/*	SRAM Memory Map		*/
#define	 NCRCFG5_AADDR	0x20		/*	Auto Address		*/
#define	 NCRCFG5_PTRINC	0x10		/*	Pointer Increment	*/
#define	 NCRCFG5_LOWPWR	0x08		/*	Low Power Mode		*/
#define	 NCRCFG5_SINT	0x04		/*	SCSI Interrupt Enable	*/
#define	 NCRCFG5_INTP	0x02		/*	INT Polarity		*/
#define	 NCRCFG5_AINT	0x01		/*	ATA Interrupt Enable	*/

#define	NCR_SIGNTR	0x0e		/* RO - Signature */

void esp_dma_enable(ESPState *s, int irq, int level);
void esp_request_cancelled(SCSIRequest *req);
void esp_command_complete(SCSIRequest *req, uint32_t status, size_t resid);
void esp_transfer_data(SCSIRequest *req, uint32_t len);
void esp_hard_reset(ESPState *s);
uint64_t esp_reg_read(ESPState *s, uint32_t saddr);
void esp_reg_write(ESPState *s, uint32_t saddr, uint64_t val);
uint64_t fas408_read_fifo(void *opaque);
void fas408_write_fifo(void *opaque, uint64_t val);
extern const VMStateDescription vmstate_esp;

extern void esp_irq_raise(qemu_irq);
extern void esp_irq_lower(qemu_irq);

void scsiesp_req_continue(SCSIRequest *req);
SCSIRequest *scsiesp_req_new(SCSIDevice *d, uint32_t tag, uint32_t lun, uint8_t *buf, void *hba_private);
int32_t scsiesp_req_enqueue(SCSIRequest *req);
void scsiesp_req_unref(SCSIRequest *req);
uint8_t *scsiesp_req_get_buf(SCSIRequest *req);
SCSIDevice *scsiesp_device_find(SCSIBus *bus, int channel, int target, int lun);
void scsiesp_req_cancel(SCSIRequest *req);

uint64_t esp_reg_read(void *s, uint32_t saddr);
void esp_reg_write(void *s, uint32_t saddr, uint64_t val);

void esp_dma_enable(void *opaque, int level);
void esp_fake_dma_done(void *opaque);
void esp_fake_dma_put(void *opaque, uint8_t v);

void esp_request_cancelled(SCSIRequest *req);
void esp_command_complete(SCSIRequest *req, uint32_t status, size_t resid);
void esp_transfer_data(SCSIRequest *req, uint32_t len);
void esp_scsi_init(DeviceState *dev, ESPDMAMemoryReadWriteFunc read, ESPDMAMemoryReadWriteFunc write, int fas4xxextra);
void esp_scsi_reset(DeviceState *dev, void *privdata);
bool esp_dreq(DeviceState *dev);

#endif
