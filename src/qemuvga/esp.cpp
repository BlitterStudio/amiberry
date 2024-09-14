/*
 * QEMU ESP/NCR53C9x emulation
 *
 * Copyright (c) 2005-2006 Fabrice Bellard
 * Copyright (c) 2012 Herve Poussineau
 *
 * Copyright (c) 2014-2016 Toni Wilen (pseudo-dma, fas408 PIO buffer)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <assert.h>

#include "qemuuaeglue.h"
#include "queue.h"

//#include "hw/sysbus.h"
#include "scsi/scsi.h"
#include "scsi/esp.h"
//#include "trace.h"
//#include "qemu/log.h"

/*
 * On Sparc32, this is the ESP (NCR53C90) part of chip STP2000 (Master I/O),
 * also produced as NCR89C100. See
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C100.txt
 * and
 * http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR53C9X.txt
 */

#define TYPE_ESP "esp"
//#define ESP(obj) OBJECT_CHECK(SysBusESPState, (obj), TYPE_ESP)
#define ESP(obj) (ESPState*)obj->lsistate

#define ESPLOG 0

static void esp_raise_ext_irq(ESPState * s)
{
	if (s->irq_raised)
		return;
	s->irq_raised = 1;
	esp_irq_raise(s->irq);
}

static void esp_lower_ext_irq(ESPState * s)
{
	if (!s->irq_raised)
		return;
	s->irq_raised = 0;
	esp_irq_lower(s->irq);
}


static void esp_raise_irq(ESPState *s)
{
    if (!(s->rregs[ESP_RSTAT] & STAT_INT)) {
        s->rregs[ESP_RSTAT] |= STAT_INT;
#if ESPLOG
        write_log("irq->1\n");
#endif
        esp_raise_ext_irq(s);
    }
}

static void esp_lower_irq(ESPState *s)
{
    if (s->rregs[ESP_RSTAT] & STAT_INT) {
        s->rregs[ESP_RSTAT] &= ~STAT_INT;
#if ESPLOG
        write_log("irq->0\n");
#endif
        esp_lower_ext_irq(s);
    }
}

static void fas408_raise_irq(ESPState *s)
{
	if (!(s->rregs[ESP_REGS + NCR_PSTAT] & NCRPSTAT_SIRQ)) {
		s->rregs[ESP_REGS + NCR_PSTAT] |= NCRPSTAT_SIRQ;
#if ESPLOG
        write_log("irq408->1\n");
#endif
		esp_raise_ext_irq(s);
	}
}

static void fas408_lower_irq(ESPState *s)
{
	if (s->rregs[ESP_REGS + NCR_PSTAT] & NCRPSTAT_SIRQ) {
		s->rregs[ESP_REGS + NCR_PSTAT] &= ~NCRPSTAT_SIRQ;
#if ESPLOG
        write_log("irq408->0\n");
#endif
        esp_lower_ext_irq(s);
	}
}

static void fas408_check(ESPState *s)
{
	if (!(s->fas4xxextra & 1))
		return;
	bool irq = false;
	int v = 0;
	int len = 0;
	if (s->fas408_buffer_size > 0) {
		len = s->fas408_buffer_size - s->fas408_buffer_offset;
	} else if ((s->rregs[ESP_RSTAT] & STAT_PIO_MASK) == 0) {
		len = s->ti_size;
	}
	if (s->wregs[ESP_REGS + NCR_PSTAT] & 1) {
		v |= len == 0 ? NCRPSTAT_FEMPT : 0;
		v |= len >= 42 ? NCRPSTAT_F13 : 0;
		v |= len >= 84 ? NCRPSTAT_F23 : 0;
		v |= len >= 128 ? NCRPSTAT_FFULL : 0;
		if ((s->wregs[ESP_REGS + NCR_PIOI] & v) & (NCRPSTAT_FEMPT | NCRPSTAT_F13 | NCRPSTAT_F23 | NCRPSTAT_FFULL)) {
			irq = true;
		}
	}
	if (irq) {
		v |= NCRPSTAT_SIRQ;
		fas408_raise_irq(s);
	} else {
		fas408_lower_irq(s);
	}
	s->rregs[ESP_REGS + NCR_PSTAT] = v;
}

void esp_dma_enable(void *opaque, int level)
{
	ESPState *s = (ESPState*)opaque;
	if (level) {
        s->dma_enabled = 1;
        if (s->dma_cb) {
            if (s->dma_cb(s))
				s->dma_cb = NULL;
        }
    } else {
        s->dma_enabled = 0;
    }
}

void esp_request_cancelled(SCSIRequest *req)
{
	ESPState *s = (ESPState*)req->hba_private;

    if (req == s->current_req) {
		scsiesp_req_unref(s->current_req);
        s->current_req = NULL;
        s->current_dev = NULL;
    }
}

static uint32_t get_cmd(ESPState *s, uint8_t *buf)
{
    uint32_t dmalen;
    int target;

    target = s->wregs[ESP_WBUSID] & BUSID_DID;
    if (s->dma) {
        dmalen = s->rregs[ESP_TCLO];
        dmalen |= s->rregs[ESP_TCMID] << 8;
        dmalen |= s->rregs[ESP_TCHI] << 16;
        s->dma_memory_read(s->dma_opaque, buf, dmalen);
    } else {
        dmalen = s->ti_size;
        memcpy(buf, s->ti_buf, dmalen);
        //buf[0] = buf[2] >> 5; // This makes no sense!
    }

    s->ti_size = 0;
    s->ti_rptr = 0;
    s->ti_wptr = 0;

    if (s->current_req) {
        /* Started a new command before the old one finished.  Cancel it.  */
		scsiesp_req_cancel(s->current_req);
        s->async_len = 0;
    }

	s->current_dev = scsiesp_device_find(&s->bus, 0, target, 0);
    if (!s->current_dev) {
        // No such drive
        s->rregs[ESP_RSTAT] = 0;
        s->rregs[ESP_RINTR] = INTR_DC;
        s->rregs[ESP_RSEQ] = SEQ_0;
        esp_raise_irq(s);
        return 0;
    }
    return dmalen;
}

static void do_busid_cmd(ESPState *s, uint8_t *buf, uint8_t busid)
{
    int32_t datalen;
    int lun;
    SCSIDevice *current_lun;

    lun = busid & 7;
    current_lun = scsiesp_device_find(&s->bus, 0, s->current_dev->id, lun);
	if (!current_lun) {
        s->rregs[ESP_RSTAT] = 0;
        s->rregs[ESP_RINTR] = INTR_DC;
        s->rregs[ESP_RSEQ] = SEQ_0;
	    esp_raise_irq(s);
		return;
	}
    s->current_req = scsiesp_req_new(current_lun, 0, lun, buf, s);
    datalen = scsiesp_req_enqueue(s->current_req);
    s->ti_size = datalen;
	s->transfer_complete = 0;
	if (datalen != 0) {
		s->rregs[ESP_RSTAT] = 0;
		if (s->dma) {
	        s->rregs[ESP_RSTAT] = STAT_TC;
	        s->dma_left = 0;
	        s->dma_counter = 0;
		}
		if (datalen > 0) {
            s->rregs[ESP_RSTAT] |= STAT_DI;
        } else {
            s->rregs[ESP_RSTAT] |= STAT_DO;
        }
        scsiesp_req_continue(s->current_req);
    }
    s->rregs[ESP_RINTR] = INTR_BS | INTR_FC;
    s->rregs[ESP_RSEQ] = SEQ_CD;
    esp_raise_irq(s);
}

static void do_cmd(ESPState *s, uint8_t *buf)
{
    uint8_t busid = buf[0];

    do_busid_cmd(s, &buf[1], busid);
}

static int handle_satn(ESPState *s)
{
    uint8_t buf[32];
    int len;

    if (s->dma && !s->dma_enabled) {
        s->dma_cb = handle_satn;
        return 1;
    }
    len = get_cmd(s, buf);
    if (len)
        do_cmd(s, buf);
	return 1;
}

static int handle_s_without_atn(ESPState *s)
{
    uint8_t buf[32];
    int len;

    if (s->dma && !s->dma_enabled) {
        s->dma_cb = handle_s_without_atn;
        return 1;
    }
    len = get_cmd(s, buf);
    if (len) {
        do_busid_cmd(s, buf, 0);
    }
	return 1;
}

static int handle_satn_stop(ESPState *s)
{
    if (s->dma && !s->dma_enabled) {
        s->dma_cb = handle_satn_stop;
        return 1;
    }
    s->cmdlen = get_cmd(s, s->cmdbuf);
    if (s->cmdlen) {
        s->do_cmd = 1;
        s->rregs[ESP_RSTAT] = STAT_TC | STAT_CD;
        s->rregs[ESP_RINTR] = INTR_BS | INTR_FC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
        esp_raise_irq(s);
    }
	return 1;
}

static void init_status_phase(ESPState *s, int st)
{
	// Multi Evolution driver reads FIFO after
	// Message Accepted command. This makes
	// sure wrong buffer is not read.
	s->pio_on = 0;
	s->async_buf = NULL;
	s->fifo_on = 2;

	if (st) {
		// status + message
		s->ti_buf[0] = s->status;
		s->ti_buf[1] = 0;
		s->ti_size = 2;
	} else {
		// message
		s->ti_buf[0] = 0;
		s->ti_size = 1;
	}
	s->ti_rptr = 0;
	s->ti_wptr = 0;
}

static void write_response(ESPState *s)
{
	init_status_phase(s, 1);

    if (s->dma) {
        s->dma_memory_write(s->dma_opaque, s->ti_buf, 2);
        s->rregs[ESP_RSTAT] = STAT_TC | STAT_ST;
        s->rregs[ESP_RINTR] = INTR_FC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
    } else {
        s->ti_size = 2;
        s->ti_rptr = 0;
        s->ti_wptr = 0;
		s->rregs[ESP_RSTAT] |= STAT_MI;
        s->rregs[ESP_RINTR] = INTR_FC;
    }
    esp_raise_irq(s);
}

static void esp_dma_done(ESPState *s)
{
	s->transfer_complete = 1;
    s->rregs[ESP_RSTAT] |= STAT_TC;
    s->rregs[ESP_RINTR] = INTR_BS;
    s->rregs[ESP_RSEQ] = 0;

	s->dma_counter -= s->dma_len;
    s->rregs[ESP_TCLO] = s->dma_counter;
    s->rregs[ESP_TCMID] = s->dma_counter >> 8;
	if ((s->wregs[ESP_CFG2] & 0x40) || (s->fas4xxextra & 2))
		s->rregs[ESP_TCHI] = s->dma_counter >> 16;
    
	esp_raise_irq(s);
}

static int esp_do_dma(ESPState *s)
{
    int len, len2;
    int to_device;

    to_device = (s->ti_size < 0);
    len = s->dma_left;
    if (s->do_cmd) {
        s->dma_memory_read(s->dma_opaque, &s->cmdbuf[s->cmdlen], len);
        s->ti_size = 0;
        s->cmdlen = 0;
        s->do_cmd = 0;
        do_cmd(s, s->cmdbuf);
        return 1;
    }
    if (s->async_len == 0) {
        /* Defer until data is available.  */
        return 1;
    }
    if (len > s->async_len) {
        len = s->async_len;
    }
	len2 = len;
	s->dma_pending = len2;
    if (to_device) {
		// if data in fifo, transfer it first
		if (s->ti_wptr > 0) {
			int l = s->ti_wptr > len ? len : s->ti_wptr;
			memcpy(s->async_buf, s->ti_buf, l);
			s->async_buf += l;
			s->async_len -= l;
			s->ti_wptr -= l;
		}
		len = s->dma_memory_read(s->dma_opaque, s->async_buf, len2);
		// if dma counter is larger than transfer size, fill the FIFO
		// Masoboshi needs this.
		if (len < 0) {
			int diff = s->dma_counter - (len2 + s->dma_len);
			if (diff > TI_BUFSZ)
				diff = TI_BUFSZ;
			if (diff > 0) {
				s->dma_memory_read(s->dma_opaque, s->ti_buf, diff);
				s->ti_rptr = 0;
				s->ti_size = diff;
				s->ti_wptr = diff;
				s->fifo_on = 3;
			}
		}
    } else {
        len = s->dma_memory_write(s->dma_opaque, s->async_buf, len2);
    }
	if (len < 0)
		len = len2;
    s->dma_left -= len;
	s->dma_len += len;
    s->async_buf += len;
    s->async_len -= len;
    if (to_device)
        s->ti_size += len;
    else
        s->ti_size -= len;
    if (s->async_len == 0) {
		scsiesp_req_continue(s->current_req);
        /* If there is still data to be read from the device then
           complete the DMA operation immediately.  Otherwise defer
           until the scsi layer has completed.  */
        if (to_device || s->dma_left != 0 || s->ti_size == 0) {
            return 1;
        }
    }

	if (len2 > len && s->dma_left > 0)
		return 0;

    /* Partially filled a scsi buffer. Complete immediately.  */
    esp_dma_done(s);
	return 1;
}

void esp_fake_dma_put(void *opaque, uint8_t v)
{
	ESPState *s = (ESPState*)opaque;
	if (s->transfer_complete) {
		if (!s->fifo_on) {
			s->fifo_on = 1;
			s->ti_rptr = s->ti_wptr = 0;
			s->ti_size = 0;
		}
		esp_reg_write(opaque, ESP_FIFO, v);
	} else {
		esp_dma_enable(opaque, 1);
	}
}

void esp_fake_dma_done(void *opaque)
{
	ESPState *s = (ESPState*)opaque;
	int to_device = (s->ti_size < 0);
	int len = s->dma_pending;

	s->dma_pending = 0;
	s->dma_left -= len;
	s->dma_len += len;
	s->async_buf += len;
	s->async_len -= len;
	if (to_device)
		s->ti_size += len;
	else
		s->ti_size -= len;
	if (s->async_len == 0) {
		scsiesp_req_continue(s->current_req);
	} else {
		esp_do_dma(s);
	}
}

void esp_command_complete(SCSIRequest *req, uint32_t status,
                                 size_t resid)
{
	ESPState *s = (ESPState*)req->hba_private;
	bool dma = s->dma != 0;

	if (s->fifo_on != 3) {
		s->fifo_on = 0;
	    s->ti_size = 0;
	}
    s->dma_left = 0;
	s->dma_pending = 0;
    s->async_len = 0;
    s->status = status;
	fas408_check(s);
	s->rregs[ESP_RSTAT] = STAT_ST;
    esp_dma_done(s);
	// TC is not set if transfer stopped due to phase change, not counter becoming zero.
	if (dma && s->dma_len < s->dma_counter)
		s->rregs[ESP_RSTAT] &= ~STAT_TC;
    if (s->current_req) {
		scsiesp_req_unref(s->current_req);
        s->current_req = NULL;
        s->current_dev = NULL;
    }
}

void esp_transfer_data(SCSIRequest *req, uint32_t len)
{
	ESPState *s = (ESPState*)req->hba_private;

    s->async_len = len;
	s->async_buf = scsiesp_req_get_buf(req);
    if (s->dma_left) {
        esp_do_dma(s);
    } else if (s->dma_counter != 0 && s->ti_size == 0) {
        /* If this was the last part of a DMA transfer then the
           completion interrupt is deferred to here.  */
        esp_dma_done(s);
    }
}

bool esp_dreq(DeviceState *dev)
{
	ESPState *s = ESP(dev);
	return s->dma_cb != NULL;
}

static int handle_ti(ESPState *s)
{
    int32_t dmalen, minlen;

	s->fifo_on = 1;
	s->transfer_complete = 0;

	fas408_check(s);

    if (s->dma && !s->dma_enabled) {
        s->dma_cb = handle_ti;
        return 1;
    }

    dmalen = s->rregs[ESP_TCLO];
    dmalen |= s->rregs[ESP_TCMID] << 8;
    dmalen |= s->rregs[ESP_TCHI] << 16;
    if (dmalen == 0) {
        dmalen = 0x10000;
    }
    s->dma_counter = dmalen;

    if (s->do_cmd)
        minlen = (dmalen < 32) ? dmalen : 32;
    else if (s->ti_size < 0)
        minlen = (dmalen < -s->ti_size) ? dmalen : -s->ti_size;
    else
        minlen = (dmalen < s->ti_size) ? dmalen : s->ti_size;
    if (s->dma) {
		if (s->dma == 1) {
			s->dma_left = minlen;
			s->dma_len = 0;
		}
		s->dma = 2;
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        if (!esp_do_dma(s)) {
	        s->dma_cb = handle_ti;
			return 0;
		}
		return 1;
    } else if (s->do_cmd) {
        s->ti_size = 0;
        s->cmdlen = 0;
        s->do_cmd = 0;
        do_cmd(s, s->cmdbuf);
        return 1;
    } else {
		// no dma
		s->rregs[ESP_RINTR] = INTR_BS;
		esp_raise_irq(s);
	}
	return 1;
}

void esp_hard_reset(ESPState *s)
{
    memset(s->rregs, 0, ESP_REGS);
    memset(s->wregs, 0, ESP_REGS);
    if (!(s->fas4xxextra & 2)) {
        s->rregs[ESP_TCHI] = s->chip_id;
    }
    s->ti_size = 0;
    s->ti_rptr = 0;
    s->ti_wptr = 0;
    s->dma = 0;
    s->do_cmd = 0;
    s->dma_cb = NULL;
	s->fas408_buffer_offset = 0;
	s->fas408_buffer_size = 0;
    s->rregs[ESP_CFG1] = 7;
}

static void esp_soft_reset(ESPState *s)
{
    esp_hard_reset(s);
}

static void parent_esp_reset(ESPState *s, int irq, int level)
{
    if (level) {
        esp_soft_reset(s);
    }
}

uint64_t fas408_read_fifo(void *opaque)
{
	ESPState *s = (ESPState*)opaque;
	s->rregs[ESP_FIFO] = 0;
	if ((s->fas4xxextra & 1) && (s->wregs[ESP_REGS + NCR_PSTAT] & NCRPSTAT_PIOM) && (s->fas408_buffer_size > 0 || s->fas408_buffer_offset > 0 || (s->rregs[ESP_RSTAT] & STAT_PIO_MASK) == STAT_DO)) {
		bool refill = true;
		if (s->ti_size > 128) {
			s->rregs[ESP_FIFO] = s->async_buf[s->ti_rptr++];
			s->ti_size--;
		} else if (!s->fas408_buffer_size) {
			if (s->ti_size) {
				memcpy(s->fas408_buffer, &s->async_buf[s->ti_rptr], s->ti_size);
				s->fas408_buffer_offset = 0;
				s->fas408_buffer_size = s->ti_size;
				s->ti_size = 0;
				if (s->current_req) {
					scsiesp_req_continue(s->current_req);
				}
				s->ti_rptr = 0;
				s->ti_wptr = 0;
				s->pio_on = 0;
				s->fifo_on = 0;
				s->rregs[ESP_FIFO] = s->fas408_buffer[s->fas408_buffer_offset++];
			}
		} else {
			s->rregs[ESP_FIFO] = s->fas408_buffer[s->fas408_buffer_offset++];
			if (s->fas408_buffer_offset >= s->fas408_buffer_size) {
				s->fas408_buffer_offset = s->fas408_buffer_size = 0;
			}
		}
		fas408_check(s);
		return s->rregs[ESP_FIFO];
	}
	return 0;
}

static uint64_t esp_reg_read2(void *opaque, uint32_t saddr)
{
	ESPState *s = (ESPState*)opaque;
	uint32_t old_val;

	if ((s->fas4xxextra & 1) && (s->wregs[0x0d] & 0x80)) {
		saddr += ESP_REGS;
	}

    switch (saddr) {
    case ESP_FIFO:
		if (s->fifo_on) {
			// FIFO can be only read in PIO mode when any transfer command is active.
			if (s->ti_size > 0) {
				s->ti_size--;
				if ((s->rregs[ESP_RSTAT] & 7) == STAT_DI || (s->rregs[ESP_RSTAT] & 7) == STAT_MI || (s->rregs[ESP_RSTAT] & 7) == STAT_ST || s->pio_on) {
					/* Data out.  */
					if ((s->rregs[ESP_RSTAT] & 7) == STAT_DI && s->async_buf) {
						s->rregs[ESP_FIFO] = s->async_buf[s->ti_rptr++];
						s->pio_on = 1;
					} else if ((s->rregs[ESP_RSTAT] & 7) == STAT_ST) {
						s->rregs[ESP_FIFO] = s->ti_buf[s->ti_rptr++];
						s->pio_on = 1;
						// -> Message In
						s->rregs[ESP_RSTAT] &= ~7;
						s->rregs[ESP_RSTAT] |= STAT_MI;
					} else if ((s->rregs[ESP_RSTAT] & 7) == STAT_MI) {
						s->rregs[ESP_FIFO] = s->ti_buf[s->ti_rptr++];
						s->pio_on = 1;
					} else {
						s->rregs[ESP_FIFO] = 0;
					}
					if (s->ti_size <= 1 && s->current_req) {
						// last byte is now going to FIFO, transfer ends.
						scsiesp_req_continue(s->current_req);
						// set ti_size back to 1, last byte is now in FIFO.
						s->ti_size = 1;
						s->fifo_on = 1;
					} else {
						esp_raise_irq(s);
					}
				}
			}
			if (s->ti_size == 0) {
				s->ti_rptr = 0;
				s->ti_wptr = 0;
				s->pio_on = 0;
				s->fifo_on = 0;
			}
		}
#if	ESPLOG
		write_log("<-FIFO %02x %d %d %d\n", s->rregs[ESP_FIFO], s->pio_on, s->ti_size, s->ti_rptr);
#endif		
		break;
    case ESP_RINTR:
        /* Clear sequence step, interrupt register and all status bits
           except TC */
        old_val = s->rregs[ESP_RINTR];
        s->rregs[ESP_RINTR] = 0;
        s->rregs[ESP_RSTAT] &= ~STAT_TC;
        s->rregs[ESP_RSEQ] = SEQ_CD;
        esp_lower_irq(s);

        return old_val;
	case ESP_RFLAGS:
	{
		int v = 0;
		if (s->fifo_on) {
			if (s->ti_size >= 16)
				v = 16;
			else
				v = s->ti_size;
		}
		if (!s->dma && v > 1 && s->fifo_on < 2)
			v = 1;
		return v | (s->rregs[ESP_RSEQ] << 5);
	}
	case ESP_RES4:
		return 0x80 | 0x20 | 0x2;
	case ESP_TCHI:
		if (!(s->wregs[ESP_CFG2] & 0x40) && !(s->fas4xxextra & 2))
			return 0;
		break;

	// FAS408
	case ESP_REGS + NCR_PIOFIFO:
		return fas408_read_fifo(opaque);
	case ESP_REGS + NCR_PSTAT:
		fas408_check(s);
		return s->rregs[ESP_REGS + NCR_PSTAT];
	case ESP_REGS + NCR_SIGNTR:
		s->fas408sig ^= 7;
		return 0x58 | s->fas408sig;

	default:
#if	ESPLOG > 1
		write_log("read unknown 53c94 register %02x\n", saddr);
#endif
		break;
    }
    return s->rregs[saddr];
}

uint64_t esp_reg_read(void *opaque, uint32_t saddr)
{
	uint64_t v = esp_reg_read2(opaque, saddr);
#if ESPLOG
	write_log("ESP_READ  %02x %02x\n", saddr & 0xff, v & 0xff);
#endif
	return v;
}

void fas408_write_fifo(void *opaque, uint64_t val)
{
	ESPState *s = (ESPState*)opaque;
	if (!(s->fas4xxextra & 1))
		return;
	s->fas408_buffer_offset = 0;
	if (s->fas408_buffer_size < 128) {
		s->fas408_buffer[s->fas408_buffer_size++] = (uint8_t)val;
	}
	fas408_check(s);
	while ((s->wregs[ESP_REGS + NCR_PSTAT] & NCRPSTAT_PIOM) && s->ti_size < 0 && s->fas408_buffer_size > 0) {
		s->async_buf[s->dma_pending++] = s->fas408_buffer[0];
		s->fas408_buffer_size--;
		if (s->fas408_buffer_size > 0) {
			memmove(s->fas408_buffer, s->fas408_buffer + 1, s->fas408_buffer_size);
		}
		if (s->dma_pending == s->async_len) {
			esp_fake_dma_done(opaque);
			break;
		}
	}
}


void esp_reg_write(void *opaque, uint32_t saddr, uint64_t val64)
{
	ESPState *s = (ESPState*)opaque;
    uint32_t val = (uint32_t)val64;

	if ((s->fas4xxextra & 1) && (s->wregs[ESP_RES3] & 0x80)) {
		saddr += ESP_REGS;
	}

#if	ESPLOG
	write_log("ESP_WRITE %02x %02x\n", saddr & 0xff, val & 0xff);
#endif

	switch (saddr) {
    case ESP_TCLO:
    case ESP_TCMID:
		s->rregs[ESP_RSTAT] &= ~STAT_TC;
		break;
	case ESP_TCHI:
		if (!(s->wregs[ESP_CFG2] & 0x40) && !(s->fas4xxextra & 2))
			val = 0;
		else
			s->rregs[ESP_RSTAT] &= ~STAT_TC;
		break;
    case ESP_FIFO:
#if	ESPLOG
		write_log("->FIFO %02x %d %d %d\n", val & 0xff, s->do_cmd, s->cmdlen, s->ti_wptr);
#endif
		if (s->do_cmd) {
			if (s->cmdlen >= TI_BUFSZ)
				return;
            s->cmdbuf[s->cmdlen++] = val & 0xff;
        } else {
			if (s->ti_wptr >= TI_BUFSZ)
				return;
            s->ti_size++;
            s->ti_buf[s->ti_wptr++] = val & 0xff;
        }
        break;
    case ESP_CMD:
        s->rregs[saddr] = val;
        if (val & CMD_DMA) {
            s->dma = 1;
            /* Reload DMA counter.  */
            s->rregs[ESP_TCLO] = s->wregs[ESP_TCLO];
            s->rregs[ESP_TCMID] = s->wregs[ESP_TCMID];
            s->rregs[ESP_TCHI] = s->wregs[ESP_TCHI];
			if (!(s->wregs[ESP_CFG2] & 0x40) && !(s->fas4xxextra & 2))
				s->rregs[ESP_TCHI] = 0;
        } else {
            s->dma = 0;
        }
        switch(val & CMD_CMD) {
        case CMD_NOP:
			if ((val & CMD_DMA) && ((s->wregs[ESP_CFG2] & 0x40) && !(s->fas4xxextra & 2)))
				s->rregs[ESP_TCHI] = s->chip_id;
			break;
        case CMD_FLUSH:
            //s->ti_size = 0;
			s->fifo_on = 0;
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSEQ] = 0;
            s->rregs[ESP_RFLAGS] = 0;
            break;
        case CMD_RESET:
            esp_soft_reset(s);
			// E-Matrix 530 detects existence of SCSI chip by
			// writing CMD_RESET and then immediately checking
			// if it reads back.
			s->rregs[saddr] = CMD_RESET;
            break;
        case CMD_BUSRESET:
            s->rregs[ESP_RINTR] = INTR_RST;
            if (!(s->wregs[ESP_CFG1] & CFG1_RESREPT)) {
                esp_raise_irq(s);
            }
            break;
        case CMD_TI:
			// transfer info and status/message:
			// make sure status/message byte is in buffer
			if ((s->rregs[ESP_RSTAT] & 7) == STAT_ST) {
				init_status_phase(s, 1);
			} else if ((s->rregs[ESP_RSTAT] & 7) == STAT_MI) {
				init_status_phase(s, 0);
			}
			handle_ti(s);
            break;
        case CMD_ICCS:
            write_response(s);
            break;
        case CMD_MSGACC:
            s->rregs[ESP_RINTR] = INTR_DC;
            s->rregs[ESP_RSEQ] = 0;
            //s->rregs[ESP_RFLAGS] = 0;
			// Masoboshi and Trifecta drivers expects phase=0
			s->rregs[ESP_RSTAT] &= ~7;
            esp_raise_irq(s);
            break;
        case CMD_PAD:
            s->rregs[ESP_RSTAT] = STAT_TC;
            s->rregs[ESP_RINTR] = INTR_FC;
            s->rregs[ESP_RSEQ] = 0;
			if (s->current_req) {
				scsiesp_req_continue(s->current_req);
			}
			break;
        case CMD_SATN:
            break;
        case CMD_RSTATN:
            break;
        case CMD_SEL:
            handle_s_without_atn(s);
            break;
        case CMD_SELATN:
            handle_satn(s);
            break;
        case CMD_SELATNS:
            handle_satn_stop(s);
            break;
        case CMD_ENSEL:
            s->rregs[ESP_RINTR] = 0;
            break;
        case CMD_DISSEL:
			// Masoboshi driver expects Function Complete.
            s->rregs[ESP_RINTR] = INTR_FC;
            esp_raise_irq(s);
            break;
        default:
            break;
        }
        break;
	case ESP_WBUSID:
	case ESP_WSEL:
	case ESP_WSYNTP:
	case ESP_WSYNO:
        break;
    case ESP_CFG1:
    case ESP_CFG2:
	case ESP_CFG3:
    case ESP_RES4:
        s->rregs[saddr] = val;
        break;
	case ESP_RES3:
		s->rregs[saddr] = val;
		s->rregs[ESP_REGS + NCR_CFG5] = val;
#if	ESPLOG
		write_log("ESP_RES3 = %02x\n", (uint8_t)val);
#endif
		break;
	case ESP_WCCF:
	case ESP_WTEST:
        break;
 
	// FAS408
	case ESP_REGS + NCR_PIOFIFO:
		fas408_write_fifo(opaque, val);
		break;
	case ESP_REGS + NCR_PSTAT: // RW - PIO Status Register
		s->rregs[ESP_REGS + NCR_PSTAT] = val;
		fas408_check(s);
#if	ESPLOG
		write_log("NCR_PSTAT = %02x\n", (uint8_t)val);
#endif
		break;
	case ESP_REGS + NCR_PIOI: // RW - PIO Interrupt Enable
		s->rregs[ESP_REGS + NCR_PIOI] = val;
		fas408_check(s);
#if	ESPLOG
		write_log("NCR_PIOI = %02x\n", (uint8_t)val);
#endif
		break;
	case ESP_REGS + NCR_CFG5: // RW - Configuration #5
		s->wregs[ESP_RES3] = val;
		s->rregs[ESP_REGS + NCR_CFG5] = val;
#if	ESPLOG
		write_log("NCR_CFG5 = %02x\n", (uint8_t)val);
#endif
		break;
	
	default:
#if	ESPLOG > 1
		write_log("write unknown 53c94 register %02x\n", saddr);
#endif
        return;
    }
    s->wregs[saddr] = val;
}

static bool esp_mem_accepts(void *opaque, hwaddr addr,
                            unsigned size, bool is_write)
{
    return (size == 1) || (is_write && size == 4);
}

#if 0
const VMStateDescription vmstate_esp = {
    .name ="esp",
    .version_id = 3,
    .minimum_version_id = 3,
    .minimum_version_id_old = 3,
    .fields      = (VMStateField []) {
        VMSTATE_BUFFER(rregs, ESPState),
        VMSTATE_BUFFER(wregs, ESPState),
        VMSTATE_INT32(ti_size, ESPState),
        VMSTATE_UINT32(ti_rptr, ESPState),
        VMSTATE_UINT32(ti_wptr, ESPState),
        VMSTATE_BUFFER(ti_buf, ESPState),
        VMSTATE_UINT32(status, ESPState),
        VMSTATE_UINT32(dma, ESPState),
        VMSTATE_BUFFER(cmdbuf, ESPState),
        VMSTATE_UINT32(cmdlen, ESPState),
        VMSTATE_UINT32(do_cmd, ESPState),
        VMSTATE_UINT32(dma_left, ESPState),
        VMSTATE_END_OF_LIST()
    }
};
#endif

typedef struct {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion iomem;
    uint32_t it_shift;
    ESPState esp;
} SysBusESPState;

#if 0
static void sysbus_esp_mem_write(void *opaque, hwaddr addr,
                                 uint64_t val, unsigned int size)
{
	SysBusESPState *sysbus = (SysBusESPState*)opaque;
    uint32_t saddr;

    saddr = addr >> sysbus->it_shift;
    esp_reg_write(&sysbus->esp, saddr, val);
}

static uint64_t sysbus_esp_mem_read(void *opaque, hwaddr addr,
                                    unsigned int size)
{
	SysBusESPState *sysbus = (SysBusESPState*)opaque;
    uint32_t saddr;

    saddr = addr >> sysbus->it_shift;
    return esp_reg_read(&sysbus->esp, saddr);
}

static const MemoryRegionOps sysbus_esp_mem_ops = {
    .read = sysbus_esp_mem_read,
    .write = sysbus_esp_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid.accepts = esp_mem_accepts,
};

void esp_init(hwaddr espaddr, int it_shift,
              ESPDMAMemoryReadWriteFunc dma_memory_read,
              ESPDMAMemoryReadWriteFunc dma_memory_write,
              void *dma_opaque, qemu_irq irq, qemu_irq *reset,
              qemu_irq *dma_enable)
{
    DeviceState *dev;
    SysBusDevice *s;
    SysBusESPState *sysbus;
    ESPState *esp;

    dev = qdev_create(NULL, TYPE_ESP);
    sysbus = ESP(dev);
    esp = &sysbus->esp;
    esp->dma_memory_read = dma_memory_read;
    esp->dma_memory_write = dma_memory_write;
    esp->dma_opaque = dma_opaque;
    sysbus->it_shift = it_shift;
    /* XXX for now until rc4030 has been changed to use DMA enable signal */
    esp->dma_enabled = 1;
    qdev_init_nofail(dev);
    s = SYS_BUS_DEVICE(dev);
    sysbus_connect_irq(s, 0, irq);
    sysbus_mmio_map(s, 0, espaddr);
    *reset = qdev_get_gpio_in(dev, 0);
    *dma_enable = qdev_get_gpio_in(dev, 1);
}

static const struct SCSIBusInfo esp_scsi_info = {
    .tcq = false,
    .max_target = ESP_MAX_DEVS,
    .max_lun = 7,

    .transfer_data = esp_transfer_data,
    .complete = esp_command_complete,
    .cancel = esp_request_cancelled
};

static void sysbus_esp_gpio_demux(void *opaque, int irq, int level)
{
    SysBusESPState *sysbus = ESP(opaque);
    ESPState *s = &sysbus->esp;

    switch (irq) {
    case 0:
        parent_esp_reset(s, irq, level);
        break;
    case 1:
        esp_dma_enable(opaque, irq, level);
        break;
    }
}

static void sysbus_esp_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    SysBusESPState *sysbus = ESP(dev);
    ESPState *s = &sysbus->esp;
    Error *err = NULL;

    sysbus_init_irq(sbd, &s->irq);
    assert(sysbus->it_shift != -1);

    s->chip_id = TCHI_FAS100A;
    memory_region_init_io(&sysbus->iomem, OBJECT(sysbus), &sysbus_esp_mem_ops,
                          sysbus, "esp", ESP_REGS << sysbus->it_shift);
    sysbus_init_mmio(sbd, &sysbus->iomem);

    qdev_init_gpio_in(dev, sysbus_esp_gpio_demux, 2);

    scsi_bus_new(&s->bus, sizeof(s->bus), dev, &esp_scsi_info, NULL);
    scsi_bus_legacy_handle_cmdline(&s->bus, &err);
    if (err != NULL) {
        error_propagate(errp, err);
        return;
    }
}

static void sysbus_esp_hard_reset(DeviceState *dev)
{
    SysBusESPState *sysbus = ESP(dev);
    esp_hard_reset(&sysbus->esp);
}

static const VMStateDescription vmstate_sysbus_esp_scsi = {
    .name = "sysbusespscsi",
    .version_id = 0,
    .minimum_version_id = 0,
    .minimum_version_id_old = 0,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(esp, SysBusESPState, 0, vmstate_esp, ESPState),
        VMSTATE_END_OF_LIST()
    }
};

static void sysbus_esp_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = sysbus_esp_realize;
    dc->reset = sysbus_esp_hard_reset;
    dc->vmsd = &vmstate_sysbus_esp_scsi;
    set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
}

static const TypeInfo sysbus_esp_info = {
    .name          = TYPE_ESP,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SysBusESPState),
    .class_init    = sysbus_esp_class_init,
};

static void esp_register_types(void)
{
    type_register_static(&sysbus_esp_info);
}

type_init(esp_register_types)
#endif

void esp_scsi_init(DeviceState *dev, ESPDMAMemoryReadWriteFunc read, ESPDMAMemoryReadWriteFunc write, int fas4xxextra)
{
	dev->lsistate = calloc(sizeof(ESPState), 1);
	ESPState *s = ESP(dev);
	s->dma_memory_read = read;
	s->dma_memory_write = write;
	s->fas4xxextra = fas4xxextra;
	s->chip_id = 0x12 | 0x80;
}

void esp_scsi_reset(DeviceState *dev, void *privdata)
{
	ESPState *s = ESP(dev);

	esp_soft_reset(s);
	s->bus.privdata = privdata;
	s->irq = privdata;
	s->dma_opaque = privdata;
}
