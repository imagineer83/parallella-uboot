/*
 * Just to satisfy init routines..
 */

#include <common.h>
#include <xparameters.h>

#define PARPORT_CRTL_BASEADDR                   XPSS_CRTL_PARPORT_BASEADDR
#define NOR_FLASH_BASEADDR                      XPSS_PARPORT0_BASEADDR

#define PARPORT_MC_DIRECT_CMD                   0x010
#define PARPORT_MC_SET_CYCLES                   0x014
#define PARPORT_MC_SET_OPMODE                   0x018

#define BOOT_MODE_REG     (XPSS_SYS_CTRL_BASEADDR + 0x25C)
#define BOOT_MODES_MASK    0x0000000F
#define NOR_FLASH_MODE    (0x00000002)            /**< NOR  */
#define NAND_FLASH_MODE   (0x00000004)            /**< NAND */
#define QSPI_MODE         (0x00000008)            /**< QSPI */
#define SD_MODE           (0x0000000C)            /**< Secure Digital card */


DECLARE_GLOBAL_DATA_PTR;

/* Where should these really go? */

static void Out32(u32 OutAddress, u32 Value)
{
    *(volatile u32 *) OutAddress = Value;
}

static u32 In32(u32 InAddress)
{
    return *(u32 *) InAddress;
}

static void Out8(u32 OutAddress, u8 Value)
{
    *(volatile u8 *) OutAddress = Value;
}

static u8 In8(u32 InAddress)
{
    return *(u8 *) InAddress;
}

/*
 * init_nor_flash init the parameters of pl353 for the M29EW Flash
 */
void init_nor_flash()
{
  /* Init variables */

   /* Write timing info to set_cycles registers */
  u32 set_cycles_reg = (0x0 << 20) | /* Set_t6 or we_time from sram_cycles */
                       (0x1 << 17) | /* Set_t5 or t_tr from sram_cycles */
                       (0x2 << 14) | /* Set_t4 or t_pc from sram_cycles */
                       (0x5 << 11) | /* Set_t3 or t_wp from sram_cycles */
                       (0x2 << 8) |  /* Set_t2 t_ceoe from sram_cycles */
                       (0x7 << 4) |  /* Set_t1 t_wc from sram_cycles */
                       (0x7);        /* Set_t0 t_rc from sram_cycles */

  Out32(PARPORT_CRTL_BASEADDR + PARPORT_MC_SET_CYCLES, set_cycles_reg);

  /* write operation mode to set_opmode registers */
  u32 set_opmode_reg = (0x1 << 13) | /* set_burst_align, see to 32 beats */
                       (0x1 << 12) | /* set_bls, set to default */
                       (0x0 << 11) | /* set_adv bit, set to default */
                       (0x0 << 10) | /* set_baa, I guess we don't use baa_n */
                       (0x0 << 7) |  /* set_wr_bl, write brust length, set to 0 */
                       (0x0 << 6) |  /* set_wr_sync, set to 0 */
                       (0x0 << 3) |  /* set_rd_bl, read brust lenght, set to 0 */
                       (0x0 << 2) |  /* set_rd_sync, set to 0 */
                       (0x0 );       /* set_mw, memory width, 16bits width*/
  Out32(PARPORT_CRTL_BASEADDR + PARPORT_MC_SET_OPMODE, set_opmode_reg);

  /*
   * Issue a direct_cmd by writing to direct_cmd register
   * This is needed becuase the UpdatesReg flag in direct_cmd updates the
   * state of SMC
   */
  u32 direct_cmd_reg = (0x0 << 23) | /* chip 1 from interface 0 */
                       (0x2 << 21) | /* UpdateRegs operation, to update the two reg we wrote earlier*/
                       (0x0 << 20) | /* cre */
                       (0x0);        /* addr, not use in UpdateRegs */
  Out32(PARPORT_CRTL_BASEADDR + PARPORT_MC_DIRECT_CMD, direct_cmd_reg);

  /* reset the flash itself so that it's ready to be accessed */

  Out8(NOR_FLASH_BASEADDR + 0xAAA, 0xAA);
  Out8(NOR_FLASH_BASEADDR + 0x555, 0x55);
  Out8(NOR_FLASH_BASEADDR,         0xF0);
}

int board_init(void)
{
	icache_enable();
	init_nor_flash();

	return 0;
}

int board_late_init (void)
{
	u32 boot_mode;

	boot_mode = (In32(BOOT_MODE_REG) & BOOT_MODES_MASK);
	switch(boot_mode) {
	case QSPI_MODE:
		setenv("modeboot", "run qspiboot");
		break;
	case NAND_FLASH_MODE:
		setenv("modeboot", "run nandboot");
		break;
	case NOR_FLASH_MODE:
		setenv("modeboot", "run norboot");
		break;
	case SD_MODE:
		setenv("modeboot", "run sdboot");
		break;
	default:
		setenv("modeboot", "");
		break;
	}
	return 0;
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

/*
 * OK, and resets too.
 */
void reset_cpu(ulong addr)
{
	u32 *slcr_p;

	slcr_p = (u32*)XPSS_SYS_CTRL_BASEADDR;

	/* unlock SLCR */
	*(slcr_p + 2) = 0xDF0D;
	/* Tickle soft reset bit */
	*(slcr_p + 128) = 1;

	while(1) {;}
}

void do_reset(void)
{
	reset_cpu(0);
}