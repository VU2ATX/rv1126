// SPDX-License-Identifier: GPL-2.0
/*
 * imx274 driver
 *
 * Copyright (C) 2020 APNRing Electronics Co., Ltd.
 *
 * V0.0X01.0X00 first version
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/rk-camera-module.h>
#include <linux/of_graph.h>
#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mediabus.h>
#include <linux/pinctrl/consumer.h>
#include <linux/rk-preisp.h>

#define DRIVER_VERSION			KERNEL_VERSION(0, 0x01, 0x00)
#ifndef V4L2_CID_DIGITAL_GAIN
#define V4L2_CID_DIGITAL_GAIN		V4L2_CID_GAIN
#endif

#define MIPI_FREQ_891M                  891000000
#define MIPI_FREQ_446M                  446000000
#define MIPI_FREQ_743M                  743000000

#define IMX274_4LANES			4

#define CHIP_ID				0x31          // 274 没有CHIP ID寄存器，挑了一个固定的寄存器当CHIP ID
#define IMX274_REG_CHIP_ID		0x3005    // judt for debug

#define IMX274_NAME			"imx274"
#define OF_CAMERA_PINCTRL_STATE_DEFAULT	"rockchip,camera_default"
#define OF_CAMERA_PINCTRL_STATE_SLEEP	"rockchip,camera_sleep"

#define IMX274_XVCLK_FREQ		27000000
#define IMX274_MAX_PIXEL_RATE   (MIPI_FREQ_891M / 10 * 2 * IMX274_4LANES)


#define CHIP_ID				0x31       // 274 没有CHIP ID寄存器，挑了一个固定的寄存器当CHIP ID
#define IMX274_REG_CHIP_ID		0x3005    // judt for debug

#define IMX274_REG_CTRL_MODE		0x3000
#define IMX274_MODE_SW_STANDBY		0x1
#define IMX274_MODE_STREAMING		0x0

/*
 * See "SHR, SVR Setting" in datasheet
 */
#define IMX274_DEFAULT_FRAME_LENGTH		(4550)
#define IMX274_MAX_FRAME_LENGTH			(0x000fffff)




/*
 * See "Frame Rate Adjustment" in datasheet
 */
#define IMX274_PIXCLK_CONST1			(72000000)
#define IMX274_PIXCLK_CONST2			(1000000)

/*
 * The input gain is shifted by IMX274_GAIN_SHIFT to get
 * decimal number. The real gain is
 * (float)input_gain_value / (1 << IMX274_GAIN_SHIFT)
 */
#define IMX274_GAIN_SHIFT			(8)
#define IMX274_GAIN_SHIFT_MASK			((1 << IMX274_GAIN_SHIFT) - 1)

/*
 * See "Analog Gain" and "Digital Gain" in datasheet
 * min gain is 1X
 * max gain is calculated based on IMX274_GAIN_REG_MAX
 */
#define IMX274_GAIN_REG_MAX			(1957)
#define IMX274_MIN_GAIN				(0x01 << IMX274_GAIN_SHIFT)
#define IMX274_MAX_ANALOG_GAIN			((2048 << IMX274_GAIN_SHIFT)\
					/ (2048 - IMX274_GAIN_REG_MAX))
#define IMX274_MAX_DIGITAL_GAIN			(8)
#define IMX274_DEF_GAIN				(20 << IMX274_GAIN_SHIFT)
#define IMX274_GAIN_CONST			(2048) /* for gain formula */

/*
 * 1 line time in us = (HMAX / 72), minimal is 4 lines
 */
#define IMX274_MIN_EXPOSURE_TIME		(4 * 260 / 72)

#define IMX274_DEFAULT_MODE			IMX274_BINNING_OFF
#define IMX274_MAX_WIDTH			(3840)
#define IMX274_MAX_HEIGHT			(2160)
#define IMX274_MAX_FRAME_RATE			(120)
#define IMX274_MIN_FRAME_RATE			(5)
#define IMX274_DEF_FRAME_RATE			(30)  // 60->30 by lwx

/*
 * register SHR is limited to (SVR value + 1) x VMAX value - 4
 */
#define IMX274_SHR_LIMIT_CONST			(4)

/*
 * Min and max sensor reset delay (microseconds)
 */
#define IMX274_RESET_DELAY1			(2000)
#define IMX274_RESET_DELAY2			(2200)

/*
 * shift and mask constants
 */
#define IMX274_SHIFT_8_BITS			(8)
#define IMX274_SHIFT_16_BITS			(16)
#define IMX274_MASK_LSB_2_BITS			(0x03)
#define IMX274_MASK_LSB_3_BITS			(0x07)
#define IMX274_MASK_LSB_4_BITS			(0x0f)
#define IMX274_MASK_LSB_8_BITS			(0x00ff)

#define DRIVER_NAME "imx274"

/*
 * IMX274 register definitions
 */
#define IMX274_SHR_REG_MSB			0x300D /* SHR */
#define IMX274_SHR_REG_LSB			0x300C /* SHR */
#define IMX274_SVR_REG_MSB			0x300F /* SVR */
#define IMX274_SVR_REG_LSB			0x300E /* SVR */
#define IMX274_HTRIM_EN_REG			0x3037
#define IMX274_HTRIM_START_REG_LSB		0x3038
#define IMX274_HTRIM_START_REG_MSB		0x3039
#define IMX274_HTRIM_END_REG_LSB		0x303A
#define IMX274_HTRIM_END_REG_MSB		0x303B
#define IMX274_VWIDCUTEN_REG			0x30DD
#define IMX274_VWIDCUT_REG_LSB			0x30DE
#define IMX274_VWIDCUT_REG_MSB			0x30DF
#define IMX274_VWINPOS_REG_LSB			0x30E0
#define IMX274_VWINPOS_REG_MSB			0x30E1
#define IMX274_WRITE_VSIZE_REG_LSB		0x3130
#define IMX274_WRITE_VSIZE_REG_MSB		0x3131
#define IMX274_Y_OUT_SIZE_REG_LSB		0x3132
#define IMX274_Y_OUT_SIZE_REG_MSB		0x3133
#define IMX274_VMAX_REG_1			0x30FA /* VMAX, MSB */
#define IMX274_VMAX_REG_2			0x30F9 /* VMAX */
#define IMX274_VMAX_REG_3			0x30F8 /* VMAX, LSB */
#define IMX274_HMAX_REG_MSB			0x30F7 /* HMAX */
#define IMX274_HMAX_REG_LSB			0x30F6 /* HMAX */
#define IMX274_ANALOG_GAIN_ADDR_LSB		0x300A /* ANALOG GAIN LSB */
#define IMX274_ANALOG_GAIN_ADDR_MSB		0x300B /* ANALOG GAIN MSB */
#define IMX274_DIGITAL_GAIN_REG			0x3012 /* Digital Gain */
#define IMX274_VFLIP_REG			0x301A /* VERTICAL FLIP */
#define IMX274_TEST_PATTERN_REG			0x303D /* TEST PATTERN */
#define IMX274_STANDBY_REG			0x3000 /* STANDBY */

#define IMX274_TABLE_WAIT_MS			0
#define IMX274_TABLE_END			0xffff

/*
 * imx274 I2C operation related structure
 */
struct reg_8 {
	u16 addr;
	u8 val;
};

enum imx274_binning {
	IMX274_BINNING_OFF,
	IMX274_BINNING_2_1,
	IMX274_BINNING_3_1,
};

static const char * const imx274_supply_names[] = {
	"avdd",		/* Analog power */
	"dovdd",	/* Digital I/O power */
	"dvdd",		/* Digital core power */
};

#define IMX274_NUM_SUPPLIES ARRAY_SIZE(imx274_supply_names)


/*
 * Parameters for each imx274 readout mode.
 *
 * These are the values to configure the sensor in one of the
 * implemented modes.
 *
 * @init_regs: registers to initialize the mode
 * @bin_ratio: downscale factor (e.g. 3 for 3:1 binning)
 * @min_frame_len: Minimum frame length for each mode (see "Frame Rate
 *                 Adjustment (CSI-2)" in the datasheet)
 * @min_SHR: Minimum SHR register value (see "Shutter Setting (CSI-2)" in the
 *           datasheet)
 * @max_fps: Maximum frames per second
 * @nocpiop: Number of clocks per internal offset period (see "Integration Time
 *           in Each Readout Drive Mode (CSI-2)" in the datasheet)
 */
struct imx274_mode {
	u32 bus_fmt;
	const struct reg_8 *init_regs;
	unsigned int bin_ratio;
	int min_frame_len;
	int min_SHR;
	struct v4l2_fract  max_fps;
	int nocpiop;
	int width;
	int height;
	int hdr_mode;
	u32 mipi_freq_idx;
	u32 bpp;


};

/*
 * imx274 test pattern related structure
 */
enum {
	TEST_PATTERN_DISABLED = 0,
	TEST_PATTERN_ALL_000H,
	TEST_PATTERN_ALL_FFFH,
	TEST_PATTERN_ALL_555H,
	TEST_PATTERN_ALL_AAAH,
	TEST_PATTERN_VSP_5AH, /* VERTICAL STRIPE PATTERN 555H/AAAH */
	TEST_PATTERN_VSP_A5H, /* VERTICAL STRIPE PATTERN AAAH/555H */
	TEST_PATTERN_VSP_05H, /* VERTICAL STRIPE PATTERN 000H/555H */
	TEST_PATTERN_VSP_50H, /* VERTICAL STRIPE PATTERN 555H/000H */
	TEST_PATTERN_VSP_0FH, /* VERTICAL STRIPE PATTERN 000H/FFFH */
	TEST_PATTERN_VSP_F0H, /* VERTICAL STRIPE PATTERN FFFH/000H */
	TEST_PATTERN_H_COLOR_BARS,
	TEST_PATTERN_V_COLOR_BARS,
};

static const char * const tp_qmenu[] = {
	"Disabled",
	"All 000h Pattern",
	"All FFFh Pattern",
	"All 555h Pattern",
	"All AAAh Pattern",
	"Vertical Stripe (555h / AAAh)",
	"Vertical Stripe (AAAh / 555h)",
	"Vertical Stripe (000h / 555h)",
	"Vertical Stripe (555h / 000h)",
	"Vertical Stripe (000h / FFFh)",
	"Vertical Stripe (FFFh / 000h)",
	"Horizontal Color Bars",
	"Vertical Color Bars",
};

/*
 * All-pixel scan mode (10-bit)
 * imx274 mode1(refer to datasheet) register configuration with
 * 3840x2160 resolution, raw10 data and mipi four lane output
 */
static const struct reg_8 imx274_mode1_3840x2160_raw10[] = {
	{0x3004, 0x01},
	{0x3005, 0x01},
	{0x3006, 0x00},
	{0x3007, 0xa2},

	{0x3018, 0xA2}, /* output XVS, HVS */

	{0x306B, 0x05},
	{0x30E2, 0x01},

	{0x30EE, 0x01},
	{0x3342, 0x0A},
	{0x3343, 0x00},
	{0x3344, 0x16},
	{0x3345, 0x00},
	{0x33A6, 0x01},
	{0x3528, 0x0E},
	{0x3554, 0x1F},
	{0x3555, 0x01},
	{0x3556, 0x01},
	{0x3557, 0x01},
	{0x3558, 0x01},
	{0x3559, 0x00},
	{0x355A, 0x00},
	{0x35BA, 0x0E},
	{0x366A, 0x1B},
	{0x366B, 0x1A},
	{0x366C, 0x19},
	{0x366D, 0x17},
	{0x3A41, 0x08},

	{IMX274_TABLE_END, 0x00}
};

/*
 * Horizontal/vertical 2/2-line binning
 * (Horizontal and vertical weightedbinning, 10-bit)
 * imx274 mode3(refer to datasheet) register configuration with
 * 1920x1080 resolution, raw10 data and mipi four lane output
 */
static const struct reg_8 imx274_mode3_1920x1080_raw10[] = {
	{0x3004, 0x02},
	{0x3005, 0x21},
	{0x3006, 0x00},
	{0x3007, 0xb1},

	{0x3018, 0xA2}, /* output XVS, HVS */

	{0x306B, 0x05},
	{0x30E2, 0x02},

	{0x30EE, 0x01},
	{0x3342, 0x0A},
	{0x3343, 0x00},
	{0x3344, 0x1A},
	{0x3345, 0x00},
	{0x33A6, 0x01},
	{0x3528, 0x0E},
	{0x3554, 0x00},
	{0x3555, 0x01},
	{0x3556, 0x01},
	{0x3557, 0x01},
	{0x3558, 0x01},
	{0x3559, 0x00},
	{0x355A, 0x00},
	{0x35BA, 0x0E},
	{0x366A, 0x1B},
	{0x366B, 0x1A},
	{0x366C, 0x19},
	{0x366D, 0x17},
	{0x3A41, 0x08},

	{IMX274_TABLE_END, 0x00}
};

/*
 * Vertical 2/3 subsampling binning horizontal 3 binning
 * imx274 mode5(refer to datasheet) register configuration with
 * 1280x720 resolution, raw10 data and mipi four lane output
 */
static const struct reg_8 imx274_mode5_1280x720_raw10[] = {
	{0x3004, 0x03},
	{0x3005, 0x31},
	{0x3006, 0x00},
	{0x3007, 0xa9},

	{0x3018, 0xA2}, /* output XVS, HVS */

	{0x306B, 0x05},
	{0x30E2, 0x03},

	{0x30EE, 0x01},
	{0x3342, 0x0A},
	{0x3343, 0x00},
	{0x3344, 0x1B},
	{0x3345, 0x00},
	{0x33A6, 0x01},
	{0x3528, 0x0E},
	{0x3554, 0x00},
	{0x3555, 0x01},
	{0x3556, 0x01},
	{0x3557, 0x01},
	{0x3558, 0x01},
	{0x3559, 0x00},
	{0x355A, 0x00},
	{0x35BA, 0x0E},
	{0x366A, 0x1B},
	{0x366B, 0x19},
	{0x366C, 0x17},
	{0x366D, 0x17},
	{0x3A41, 0x04},

	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 first step register configuration for
 * starting stream
 */
static const struct reg_8 imx274_start_1[] = {
	{IMX274_STANDBY_REG, 0x12},
	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 second step register configuration for
 * starting stream
 */
static const struct reg_8 imx274_start_2[] = {
	{0x3120, 0xF0}, /* clock settings */
	{0x3121, 0x00}, /* clock settings */
	{0x3122, 0x02}, /* clock settings */
	{0x3129, 0x9C}, /* clock settings */
	{0x312A, 0x02}, /* clock settings */
	{0x312D, 0x02}, /* clock settings */

	{0x310B, 0x00},

	/* PLSTMG */
	{0x304C, 0x00}, /* PLSTMG01 */
	{0x304D, 0x03},
	{0x331C, 0x1A},
	{0x331D, 0x00},
	{0x3502, 0x02},
	{0x3529, 0x0E},
	{0x352A, 0x0E},
	{0x352B, 0x0E},
	{0x3538, 0x0E},
	{0x3539, 0x0E},
	{0x3553, 0x00},
	{0x357D, 0x05},
	{0x357F, 0x05},
	{0x3581, 0x04},
	{0x3583, 0x76},
	{0x3587, 0x01},
	{0x35BB, 0x0E},
	{0x35BC, 0x0E},
	{0x35BD, 0x0E},
	{0x35BE, 0x0E},
	{0x35BF, 0x0E},
	{0x366E, 0x00},
	{0x366F, 0x00},
	{0x3670, 0x00},
	{0x3671, 0x00},

	/* PSMIPI */
	{0x3304, 0x32}, /* PSMIPI1 */
	{0x3305, 0x00},
	{0x3306, 0x32},
	{0x3307, 0x00},
	{0x3590, 0x32},
	{0x3591, 0x00},
	{0x3686, 0x32},
	{0x3687, 0x00},

	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 third step register configuration for
 * starting stream
 */
static const struct reg_8 imx274_start_3[] = {
	{IMX274_STANDBY_REG, 0x00},
	{0x303E, 0x02}, /* SYS_MODE = 2 */
	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 forth step register configuration for
 * starting stream
 */
static const struct reg_8 imx274_start_4[] = {
	{0x30F4, 0x00},
	{0x3018, 0xA2}, /* XHS VHS OUTUPT */
	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 register configuration for stoping stream
 */
static const struct reg_8 imx274_stop[] = {
	{IMX274_STANDBY_REG, 0x01},
	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 disable test pattern register configuration
 */
static const struct reg_8 imx274_tp_disabled[] = {
	{0x303C, 0x00},
	{0x377F, 0x00},
	{0x3781, 0x00},
	{0x370B, 0x00},
	{IMX274_TABLE_END, 0x00}
};

/*
 * imx274 test pattern register configuration
 * reg 0x303D defines the test pattern modes
 */
static const struct reg_8 imx274_tp_regs[] = {
	{0x303C, 0x11},
	{0x370E, 0x01},
	{0x377F, 0x01},
	{0x3781, 0x01},
	{0x370B, 0x11},
	{IMX274_TABLE_END, 0x00}
};

/* nocpiop happens to be the same number for the implemented modes */
static const struct imx274_mode imx274_formats[] = {
	{
		/* mode 1, 4K */
		.bus_fmt = MEDIA_BUS_FMT_SRGGB10_1X10,
		.bin_ratio = 1,
		.init_regs = imx274_mode1_3840x2160_raw10,
		.min_frame_len = 4550,
		.min_SHR = 12,
		.max_fps = {
	                .numerator = 10000,
	                .denominator = 300000,
		},
		.nocpiop = 112,
		.width = 3840,
		.height = 2160,
		.hdr_mode = NO_HDR,
        .mipi_freq_idx = 0,
        .bpp = 10,

	},
	{
		/* mode 3, 1080p */
		.bus_fmt = MEDIA_BUS_FMT_SRGGB10_1X10,
		.bin_ratio = 2,
		.init_regs = imx274_mode3_1920x1080_raw10,
		.min_frame_len = 2310,
		.min_SHR = 8,
		.max_fps = {
	                .numerator = 10000,
	                .denominator = 1200000,
		},
		.nocpiop = 112,
		.width = 1920,
		.height = 1080,
		.hdr_mode = NO_HDR,
        .mipi_freq_idx = 0,
        .bpp = 10,
	},
	{
		/* mode 5, 720p */
		.bus_fmt = MEDIA_BUS_FMT_SRGGB10_1X10,
		.bin_ratio = 3,
		.init_regs = imx274_mode5_1280x720_raw10,
		.min_frame_len = 2310,
		.min_SHR = 8,
		.max_fps = {
	                .numerator = 10000,
	                .denominator = 1200000,
		},
		.nocpiop = 112,
		.width = 1280,
		.height = 720,
		.hdr_mode = NO_HDR,
        .mipi_freq_idx = 1,
        .bpp = 10,
	},
};

/*
 * struct imx274_ctrls - imx274 ctrl structure
 * @handler: V4L2 ctrl handler structure
 * @exposure: Pointer to expsure ctrl structure
 * @gain: Pointer to gain ctrl structure
 * @vflip: Pointer to vflip ctrl structure
 * @test_pattern: Pointer to test pattern ctrl structure
 */
struct imx274_ctrls {
	struct v4l2_ctrl_handler handler;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *gain;
	struct v4l2_ctrl *vflip;
	struct v4l2_ctrl *test_pattern;
    struct v4l2_ctrl *pixel_rate;
    struct v4l2_ctrl *link_freq;

};

/*
 * struct stim274 - imx274 device structure
 * @sd: V4L2 subdevice structure
 * @pad: Media pad structure
 * @client: Pointer to I2C client
 * @ctrls: imx274 control structure
 * @crop: rect to be captured
 * @compose: compose rect, i.e. output resolution
 * @format: V4L2 media bus frame format structure
 *          (width and height are in sync with the compose rect)
 * @frame_rate: V4L2 frame rate structure
 * @regmap: Pointer to regmap structure
 * @reset_gpio: Pointer to reset gpio
 * @lock: Mutex structure
 * @mode: Parameters for the selected readout mode
 */
struct imx274 {
	struct v4l2_subdev sd;

	struct clk		*xvclk;
	struct gpio_desc	*pwdn_gpio;
	struct regulator_bulk_data supplies[IMX274_NUM_SUPPLIES];

	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_sleep;

	struct media_pad pad;
	struct i2c_client *client;
	struct imx274_ctrls ctrls;
	struct v4l2_rect crop;
	struct v4l2_mbus_framefmt format;
	struct v4l2_fract frame_interval;
	struct regmap *regmap;
	struct gpio_desc *reset_gpio;
	struct mutex lock; /* mutex lock for operations */
	const struct imx274_mode *support_modes;
	u32			support_modes_num;
	u32			module_index;
	const char *module_facing;
	const char *module_name;
	const char *len_name;
	int   power_on;
	int   streaming;
	struct v4l2_fwnode_endpoint bus_cfg;
	const struct imx274_mode *mode;
};


static const s64 link_freq_items[] = {
        MIPI_FREQ_446M,
        MIPI_FREQ_743M,
        MIPI_FREQ_891M,
};


#define IMX274_ROUND(dim, step, flags)			\
	((flags) & V4L2_SEL_FLAG_GE			\
	 ? roundup((dim), (step))			\
	 : ((flags) & V4L2_SEL_FLAG_LE			\
	    ? rounddown((dim), (step))			\
	    : rounddown((dim) + (step) / 2, (step))))

/*
 * Function declaration
 */
static int imx274_set_gain(struct imx274 *priv, struct v4l2_ctrl *ctrl);
static int imx274_set_exposure(struct imx274 *priv, int val);
static int imx274_set_vflip(struct imx274 *priv, int val);
static int imx274_set_test_pattern(struct imx274 *priv, int val);
static int imx274_set_frame_interval(struct imx274 *priv,
				     struct v4l2_fract frame_interval);

static inline void msleep_range(unsigned int delay_base)
{
	usleep_range(delay_base * 1000, delay_base * 1000 + 500);
}

/*
 * v4l2_ctrl and v4l2_subdev related operations
 */
static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler,
			     struct imx274, ctrls.handler)->sd;
}

static inline struct imx274 *to_imx274(struct v4l2_subdev *sd)
{
	return container_of(sd, struct imx274, sd);
}


static inline int imx274_read_reg(struct imx274 *priv, u16 addr, u8 *val)
{
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	__be32 data_be = 0;
	__be16 reg_addr_be = cpu_to_be16(addr);
	int ret;
	struct i2c_client *client = priv->client;


	data_be_p = (u8 *)&data_be;
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = &data_be_p[4 - 1];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be) & 0xff;
	return 0;
}

static inline int imx274_write_reg(struct imx274 *priv, u16 addr, u8 val)
{

	u8 buf[6];
	struct i2c_client *client = priv->client;

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;
	buf[2] = val;

	if (i2c_master_send(client, buf, 3) != 3)
		return -EIO;

	return 0;
}


/*
 * Writing a register table
 *
 * @priv: Pointer to device
 * @table: Table containing register values (with optional delays)
 *
 * This is used to write register table into sensor's reg map.
 *
 * Return: 0 on success, errors otherwise
 */
static int imx274_write_table(struct imx274 *priv, const struct reg_8 regs[])
{
#if 0
	struct regmap *regmap = priv->regmap;
	int err = 0;
	const struct reg_8 *next;
	u8 val;

	int range_start = -1;
	int range_count = 0;
	u8 range_vals[16];
	int max_range_vals = ARRAY_SIZE(range_vals);

	for (next = table;; next++) {
		if ((next->addr != range_start + range_count) ||
		    (next->addr == IMX274_TABLE_END) ||
		    (next->addr == IMX274_TABLE_WAIT_MS) ||
		    (range_count == max_range_vals)) {
			if (range_count == 1)
				err = regmap_write(regmap,
						   range_start, range_vals[0]);
			else if (range_count > 1)
				err = regmap_bulk_write(regmap, range_start,
							&range_vals[0],
							range_count);
			else
				err = 0;

			if (err)
				return err;

			range_start = -1;
			range_count = 0;

			/* Handle special address values */
			if (next->addr == IMX274_TABLE_END)
				break;

			if (next->addr == IMX274_TABLE_WAIT_MS) {
				msleep_range(next->val);
				continue;
			}
		}

		val = next->val;

		if (range_start == -1)
			range_start = next->addr;

		range_vals[range_count++] = val;
	}
	return 0;
#else
	u32 i;
	int ret = 0;

	for (i = 0; ret == 0 && regs[i].addr != IMX274_TABLE_END; i++)
		if (unlikely(regs[i].addr == IMX274_TABLE_WAIT_MS))
//			usleep_range(regs[i].val * 1000, regs[i].val * 2000);
			msleep_range(regs[i].val);
		else
			ret = imx274_write_reg(priv, regs[i].addr, regs[i].val);

	return ret;
#endif
}

/**
 * Write a multibyte register.
 *
 * Uses a bulk write where possible.
 *
 * @priv: Pointer to device structure
 * @addr: Address of the LSB register.  Other registers must be
 *        consecutive, least-to-most significant.
 * @val: Value to be written to the register (cpu endianness)
 * @nbytes: Number of bits to write (range: [1..3])
 */
static int imx274_write_mbreg(struct imx274 *priv, u16 addr, u32 val,
			      size_t nbytes)
{
#if 0
	__le32 val_le = cpu_to_le32(val);
	int err;

	err = regmap_bulk_write(priv->regmap, addr, &val_le, nbytes);
	if (err)
		dev_err(&priv->client->dev,
			"%s : i2c bulk write failed, %x = %x (%zu bytes)\n",
			__func__, addr, val, nbytes);
	else
		dev_dbg(&priv->client->dev,
			"%s : addr 0x%x, val=0x%x (%zu bytes)\n",
			__func__, addr, val, nbytes);
	return err;
#else
	u32 buf_i, val_i;
	u8 buf[6];
	u8 *val_p;
	__be32 val_be;

	struct i2c_client *client = priv->client;

	if (nbytes > 4)
		return -EINVAL;

	buf[0] = addr >> 8;
	buf[1] = addr & 0xff;

	val_be = cpu_to_be32(val);
	val_p = (u8 *)&val_be;
	buf_i = 2;
	val_i = 4 - nbytes;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, nbytes + 2) != nbytes + 2)
		return -EIO;

	return 0;
#endif
}

/*
 * Set mode registers to start stream.
 * @priv: Pointer to device structure
 *
 * Return: 0 on success, errors otherwise
 */
static int imx274_mode_regs(struct imx274 *priv)
{
	int err = 0;

	err = imx274_write_table(priv, imx274_start_1);
	if (err)
		return err;

	err = imx274_write_table(priv, imx274_start_2);
	if (err)
		return err;

	err = imx274_write_table(priv, priv->mode->init_regs);

	return err;
}

/*
 * imx274_start_stream - Function for starting stream per mode index
 * @priv: Pointer to device structure
 *
 * Return: 0 on success, errors otherwise
 */
static int imx274_start_stream(struct imx274 *priv)
{
	int err = 0;

	/*
	 * Refer to "Standby Cancel Sequence when using CSI-2" in
	 * imx274 datasheet, it should wait 10ms or more here.
	 * give it 1 extra ms for margin
	 */
	msleep_range(11);
	err = imx274_write_table(priv, imx274_start_3);
	if (err)
		return err;

	/*
	 * Refer to "Standby Cancel Sequence when using CSI-2" in
	 * imx274 datasheet, it should wait 7ms or more here.
	 * give it 1 extra ms for margin
	 */
	msleep_range(8);
	err = imx274_write_table(priv, imx274_start_4);
	if (err)
		return err;

	return 0;
}

/*
 * imx274_reset - Function called to reset the sensor
 * @priv: Pointer to device structure
 * @rst: Input value for determining the sensor's end state after reset
 *
 * Set the senor in reset and then
 * if rst = 0, keep it in reset;
 * if rst = 1, bring it out of reset.
 *
 */
#if 0
static void imx274_reset(struct imx274 *priv, int rst)
{
	gpiod_set_value_cansleep(priv->reset_gpio, 0);
	usleep_range(IMX274_RESET_DELAY1, IMX274_RESET_DELAY2);
	gpiod_set_value_cansleep(priv->reset_gpio, !!rst);
	usleep_range(IMX274_RESET_DELAY1, IMX274_RESET_DELAY2);
}
#endif
/**
 * imx274_s_ctrl - This is used to set the imx274 V4L2 controls
 * @ctrl: V4L2 control to be set
 *
 * This function is used to set the V4L2 controls for the imx274 sensor.
 *
 * Return: 0 on success, errors otherwise
 */
static int imx274_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct imx274 *imx274 = to_imx274(sd);
	int ret = -EINVAL;

	dev_err(&imx274->client->dev,
		"%s : s_ctrl: %s, value: %d\n", __func__,
		ctrl->name, ctrl->val);

	if (pm_runtime_get(&imx274->client->dev) <= 0)
                return 0;


	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		dev_dbg(&imx274->client->dev,
			"%s : set V4L2_CID_EXPOSURE\n", __func__);
		ret = imx274_set_exposure(imx274, ctrl->val);
		break;

	case V4L2_CID_GAIN:
		dev_dbg(&imx274->client->dev,
			"%s : set V4L2_CID_GAIN\n", __func__);
		ret = imx274_set_gain(imx274, ctrl);
		break;

	case V4L2_CID_VFLIP:
		dev_dbg(&imx274->client->dev,
			"%s : set V4L2_CID_VFLIP\n", __func__);
		ret = imx274_set_vflip(imx274, ctrl->val);
		break;

	case V4L2_CID_TEST_PATTERN:
		dev_dbg(&imx274->client->dev,
			"%s : set V4L2_CID_TEST_PATTERN\n", __func__);
		ret = imx274_set_test_pattern(imx274, ctrl->val);
		break;
	}
	pm_runtime_put(&imx274->client->dev);
	return ret;
}

static int imx274_binning_goodness(struct imx274 *imx274,
				   int w, int ask_w,
				   int h, int ask_h, u32 flags)
{
	struct device *dev = &imx274->client->dev;
	const int goodness = 100000;
	int val = 0;

	if (flags & V4L2_SEL_FLAG_GE) {
		if (w < ask_w)
			val -= goodness;
		if (h < ask_h)
			val -= goodness;
	}

	if (flags & V4L2_SEL_FLAG_LE) {
		if (w > ask_w)
			val -= goodness;
		if (h > ask_h)
			val -= goodness;
	}

	val -= abs(w - ask_w);
	val -= abs(h - ask_h);

	dev_dbg(dev, "%s: ask %dx%d, size %dx%d, goodness %d\n",
		__func__, ask_w, ask_h, w, h, val);

	return val;
}

/**
 * Helper function to change binning and set both compose and format.
 *
 * We have two entry points to change binning: set_fmt and
 * set_selection(COMPOSE). Both have to compute the new output size
 * and set it in both the compose rect and the frame format size. We
 * also need to do the same things after setting cropping to restore
 * 1:1 binning.
 *
 * This function contains the common code for these three cases, it
 * has many arguments in order to accommodate the needs of all of
 * them.
 *
 * Must be called with imx274->lock locked.
 *
 * @imx274: The device object
 * @cfg:    The pad config we are editing for TRY requests
 * @which:  V4L2_SUBDEV_FORMAT_ACTIVE or V4L2_SUBDEV_FORMAT_TRY from the caller
 * @width:  Input-output parameter: set to the desired width before
 *          the call, contains the chosen value after returning successfully
 * @height: Input-output parameter for height (see @width)
 * @flags:  Selection flags from struct v4l2_subdev_selection, or 0 if not
 *          available (when called from set_fmt)
 */
static int __imx274_change_compose(struct imx274 *imx274,
				   struct v4l2_subdev_pad_config *cfg,
				   u32 which,
				   u32 *width,
				   u32 *height,
				   u32 flags)
{
	struct device *dev = &imx274->client->dev;
	const struct v4l2_rect *cur_crop;
	struct v4l2_mbus_framefmt *tgt_fmt;
	unsigned int i;
	const struct imx274_mode *best_mode = &imx274_formats[0];
	int best_goodness = INT_MIN;

	if (which == V4L2_SUBDEV_FORMAT_TRY) {
		cur_crop = &cfg->try_crop;
		tgt_fmt = &cfg->try_fmt;
	} else {
		cur_crop = &imx274->crop;
		tgt_fmt = &imx274->format;
	}

	for (i = 0; i < ARRAY_SIZE(imx274_formats); i++) {
		unsigned int ratio = imx274_formats[i].bin_ratio;

		int goodness = imx274_binning_goodness(
			imx274,
			cur_crop->width / ratio, *width,
			cur_crop->height / ratio, *height,
			flags);

		if (goodness >= best_goodness) {
			best_goodness = goodness;
			best_mode = &imx274_formats[i];
		}
	}

	*width = cur_crop->width / best_mode->bin_ratio;
	*height = cur_crop->height / best_mode->bin_ratio;

	if (which == V4L2_SUBDEV_FORMAT_ACTIVE)
		imx274->mode = best_mode;

	dev_dbg(dev, "%s: selected %u:1 binning\n",
		__func__, best_mode->bin_ratio);

	tgt_fmt->width = *width;
	tgt_fmt->height = *height;
	tgt_fmt->field = V4L2_FIELD_NONE;

	return 0;
}



static int imx274_enum_mbus_code(struct v4l2_subdev *sd,
                                 struct v4l2_subdev_pad_config *cfg,
                                 struct v4l2_subdev_mbus_code_enum *code)
{
        struct imx274 *imx274 = to_imx274(sd);
        const struct imx274_mode *mode = imx274->mode;

        if (code->index != 0)
                return -EINVAL;
        code->code = mode->bus_fmt;

        return 0;
}

static int imx274_enum_frame_sizes(struct v4l2_subdev *sd,
                                   struct v4l2_subdev_pad_config *cfg,
                                   struct v4l2_subdev_frame_size_enum *fse)
{
        struct imx274 *imx274 = to_imx274(sd);

        if (fse->index >= imx274->support_modes_num)
                return -EINVAL;

        if (fse->code != imx274->support_modes[fse->index].bus_fmt)
                return -EINVAL;

        fse->min_width  = imx274->support_modes[fse->index].width;
        fse->max_width  = imx274->support_modes[fse->index].width;
        fse->max_height = imx274->support_modes[fse->index].height;
        fse->min_height = imx274->support_modes[fse->index].height;

        return 0;
}

static int imx274_enum_frame_interval(struct v4l2_subdev *sd,
                                       struct v4l2_subdev_pad_config *cfg,
                                       struct v4l2_subdev_frame_interval_enum *fie)
{
        struct imx274 *imx274 = to_imx274(sd);

        if (fie->index >= imx274->support_modes_num)
                return -EINVAL;

        fie->code = imx274->support_modes[fie->index].bus_fmt;
        fie->width = imx274->support_modes[fie->index].width;
        fie->height = imx274->support_modes[fie->index].height;
        fie->interval = imx274->support_modes[fie->index].max_fps;
        fie->reserved[0] = imx274->support_modes[fie->index].hdr_mode;
        return 0;
}



/**
 * imx274_get_fmt - Get the pad format
 * @sd: Pointer to V4L2 Sub device structure
 * @cfg: Pointer to sub device pad information structure
 * @fmt: Pointer to pad level media bus format
 *
 * This function is used to get the pad format information.
 *
 * Return: 0 on success
 */
static int imx274_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct imx274 *imx274 = to_imx274(sd);

	mutex_lock(&imx274->lock);
	fmt->format = imx274->format;
	mutex_unlock(&imx274->lock);
	return 0;
}

/**
 * imx274_set_fmt - This is used to set the pad format
 * @sd: Pointer to V4L2 Sub device structure
 * @cfg: Pointer to sub device pad information structure
 * @format: Pointer to pad level media bus format
 *
 * This function is used to set the pad format.
 *
 * Return: 0 on success
 */
static int imx274_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *fmt = &format->format;
	struct imx274 *imx274 = to_imx274(sd);
	int err = 0;

	mutex_lock(&imx274->lock);

	err = __imx274_change_compose(imx274, cfg, format->which,
				      &fmt->width, &fmt->height, 0);

	if (err)
		goto out;

	/*
	 * __imx274_change_compose already set width and height in the
	 * applicable format, but we need to keep all other format
	 * values, so do a full copy here
	 */
	fmt->field = V4L2_FIELD_NONE;
	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		cfg->try_fmt = *fmt;
	else
		imx274->format = *fmt;

out:
	mutex_unlock(&imx274->lock);

	return err;
}

static int imx274_get_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_selection *sel)
{
	struct imx274 *imx274 = to_imx274(sd);
	const struct v4l2_rect *src_crop;
	const struct v4l2_mbus_framefmt *src_fmt;
	int ret = 0;

	if (sel->pad != 0)
		return -EINVAL;

	if (sel->target == V4L2_SEL_TGT_CROP_BOUNDS) {
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = IMX274_MAX_WIDTH;
		sel->r.height = IMX274_MAX_HEIGHT;
		return 0;
	}

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY) {
		src_crop = &cfg->try_crop;
		src_fmt = &cfg->try_fmt;
	} else {
		src_crop = &imx274->crop;
		src_fmt = &imx274->format;
	}

	mutex_lock(&imx274->lock);

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
		sel->r = *src_crop;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		sel->r.top = 0;
		sel->r.left = 0;
		sel->r.width = src_crop->width;
		sel->r.height = src_crop->height;
		break;
	case V4L2_SEL_TGT_COMPOSE:
		sel->r.top = 0;
		sel->r.left = 0;
		sel->r.width = src_fmt->width;
		sel->r.height = src_fmt->height;
		break;
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&imx274->lock);

	return ret;
}

static int imx274_set_selection_crop(struct imx274 *imx274,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_selection *sel)
{
	struct v4l2_rect *tgt_crop;
	struct v4l2_rect new_crop;
	bool size_changed;

	/*
	 * h_step could be 12 or 24 depending on the binning. But we
	 * won't know the binning until we choose the mode later in
	 * __imx274_change_compose(). Thus let's be safe and use the
	 * most conservative value in all cases.
	 */
	const u32 h_step = 24;

	new_crop.width = min_t(u32,
			       IMX274_ROUND(sel->r.width, h_step, sel->flags),
			       IMX274_MAX_WIDTH);

	/* Constraint: HTRIMMING_END - HTRIMMING_START >= 144 */
	if (new_crop.width < 144)
		new_crop.width = 144;

	new_crop.left = min_t(u32,
			      IMX274_ROUND(sel->r.left, h_step, 0),
			      IMX274_MAX_WIDTH - new_crop.width);

	new_crop.height = min_t(u32,
				IMX274_ROUND(sel->r.height, 2, sel->flags),
				IMX274_MAX_HEIGHT);

	new_crop.top = min_t(u32, IMX274_ROUND(sel->r.top, 2, 0),
			     IMX274_MAX_HEIGHT - new_crop.height);

	sel->r = new_crop;

	if (sel->which == V4L2_SUBDEV_FORMAT_TRY)
		tgt_crop = &cfg->try_crop;
	else
		tgt_crop = &imx274->crop;

	mutex_lock(&imx274->lock);

	size_changed = (new_crop.width != tgt_crop->width ||
			new_crop.height != tgt_crop->height);

	/* __imx274_change_compose needs the new size in *tgt_crop */
	*tgt_crop = new_crop;

	/* if crop size changed then reset the output image size */
	if (size_changed)
		__imx274_change_compose(imx274, cfg, sel->which,
					&new_crop.width, &new_crop.height,
					sel->flags);

	mutex_unlock(&imx274->lock);

	return 0;
}

static int imx274_set_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_selection *sel)
{
	struct imx274 *imx274 = to_imx274(sd);

	if (sel->pad != 0)
		return -EINVAL;

	if (sel->target == V4L2_SEL_TGT_CROP)
		return imx274_set_selection_crop(imx274, cfg, sel);

	if (sel->target == V4L2_SEL_TGT_COMPOSE) {
		int err;

		mutex_lock(&imx274->lock);
		err =  __imx274_change_compose(imx274, cfg, sel->which,
					       &sel->r.width, &sel->r.height,
					       sel->flags);
		mutex_unlock(&imx274->lock);

		/*
		 * __imx274_change_compose already set width and
		 * height in set->r, we still need to set top-left
		 */
		if (!err) {
			sel->r.top = 0;
			sel->r.left = 0;
		}

		return err;
	}

	return -EINVAL;
}

static int imx274_apply_trimming(struct imx274 *imx274)
{
	u32 h_start;
	u32 h_end;
	u32 hmax;
	u32 v_cut;
	s32 v_pos;
	u32 write_v_size;
	u32 y_out_size;
	int err;

	h_start = imx274->crop.left + 12;
	h_end = h_start + imx274->crop.width;

	/* Use the minimum allowed value of HMAX */
	/* Note: except in mode 1, (width / 16 + 23) is always < hmax_min */
	/* Note: 260 is the minimum HMAX in all implemented modes */
	hmax = max_t(u32, 260, (imx274->crop.width) / 16 + 23);

	/* invert v_pos if VFLIP */
	v_pos = imx274->ctrls.vflip->cur.val ?
		(-imx274->crop.top / 2) : (imx274->crop.top / 2);
	v_cut = (IMX274_MAX_HEIGHT - imx274->crop.height) / 2;
	write_v_size = imx274->crop.height + 22;
	y_out_size   = imx274->crop.height + 14;

	err = imx274_write_mbreg(imx274, IMX274_HMAX_REG_LSB, hmax, 2);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_HTRIM_EN_REG, 1, 1);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_HTRIM_START_REG_LSB,
					 h_start, 2);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_HTRIM_END_REG_LSB,
					 h_end, 2);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_VWIDCUTEN_REG, 1, 1);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_VWIDCUT_REG_LSB,
					 v_cut, 2);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_VWINPOS_REG_LSB,
					 v_pos, 2);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_WRITE_VSIZE_REG_LSB,
					 write_v_size, 2);
	if (!err)
		err = imx274_write_mbreg(imx274, IMX274_Y_OUT_SIZE_REG_LSB,
					 y_out_size, 2);

	return err;
}

/**
 * imx274_g_frame_interval - Get the frame interval
 * @sd: Pointer to V4L2 Sub device structure
 * @fi: Pointer to V4l2 Sub device frame interval structure
 *
 * This function is used to get the frame interval.
 *
 * Return: 0 on success
 */
static int imx274_g_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct imx274 *imx274 = to_imx274(sd);

	mutex_lock(&imx274->lock);
	fi->interval = imx274->frame_interval;	
	mutex_unlock(&imx274->lock);

	dev_err(&imx274->client->dev, "%s frame rate = %d / %d\n",
		__func__, imx274->frame_interval.numerator,
		imx274->frame_interval.denominator);

	return 0;
}

/**
 * imx274_s_frame_interval - Set the frame interval
 * @sd: Pointer to V4L2 Sub device structure
 * @fi: Pointer to V4l2 Sub device frame interval structure
 *
 * This function is used to set the frame intervavl.
 *
 * Return: 0 on success
 */
static int imx274_s_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_frame_interval *fi)
{
	struct imx274 *imx274 = to_imx274(sd);
	struct v4l2_ctrl *ctrl = imx274->ctrls.exposure;
	int min, max, def;
	int ret;

	mutex_lock(&imx274->lock);
	ret = imx274_set_frame_interval(imx274, fi->interval);

	if (!ret) {
		/*
		 * exposure time range is decided by frame interval
		 * need to update it after frame interval changes
		 */
		min = IMX274_MIN_EXPOSURE_TIME;
		max = fi->interval.numerator * 1000000 / fi->interval.denominator;
		def = max;
		if (__v4l2_ctrl_modify_range(ctrl, min, max, 1, def)) {
			dev_err(&imx274->client->dev,
				"Exposure ctrl range update failed\n");
			goto unlock;
		}

		/* update exposure time accordingly */
		imx274_set_exposure(imx274, ctrl->val);

		dev_err(&imx274->client->dev, "set frame interval to %uus\n",
			fi->interval.numerator * 1000000
			/ fi->interval.denominator);
	}

unlock:
	mutex_unlock(&imx274->lock);

	return ret;
}

/**
 * imx274_load_default - load default control values
 * @priv: Pointer to device structure
 *
 * Return: 0 on success, errors otherwise
 */
static int imx274_load_default(struct imx274 *priv)
{
	int ret;

	/* load default control values */
//	priv->frame_interval.numerator = 1;
//	priv->frame_interval.denominator = IMX274_DEF_FRAME_RATE;
	priv->frame_interval = priv->mode->max_fps;

	priv->ctrls.exposure->val = 1000000 / IMX274_DEF_FRAME_RATE;
	priv->ctrls.gain->val = IMX274_DEF_GAIN;
	priv->ctrls.vflip->val = 0;
	priv->ctrls.test_pattern->val = TEST_PATTERN_DISABLED;

	/* update frame rate */
	ret = imx274_set_frame_interval(priv,
					priv->frame_interval);
	if (ret)
		return ret;

	/* update exposure time */
	ret = v4l2_ctrl_s_ctrl(priv->ctrls.exposure, priv->ctrls.exposure->val);
	if (ret)
		return ret;

	/* update gain */
	ret = v4l2_ctrl_s_ctrl(priv->ctrls.gain, priv->ctrls.gain->val);
	if (ret)
		return ret;

	/* update vflip */
	ret = v4l2_ctrl_s_ctrl(priv->ctrls.vflip, priv->ctrls.vflip->val);
	if (ret)
		return ret;

	return 0;
}

static int __imx274_start_stream(struct imx274 *imx274)
{
	int ret; 
	u8 value = 0;

	/* load mode registers */
	ret = imx274_mode_regs(imx274);
	if (ret)
		return ret;

	ret = imx274_apply_trimming(imx274);
	if (ret)
		return ret;

	/*
	 * update frame rate & expsoure. if the last mode is different,
	 * HMAX could be changed. As the result, frame rate & exposure
	 * are changed.
	 * gain is not affected.
	 */
	ret = imx274_set_frame_interval(imx274,
			imx274->frame_interval);
	if (ret)
		return ret;

	/* update exposure time */
	ret = __v4l2_ctrl_s_ctrl(imx274->ctrls.exposure,
			imx274->ctrls.exposure->val);
	if (ret)
		return ret;

	/* start stream */
	ret = imx274_start_stream(imx274);
	if (ret)
		return ret;

	/* In case these controls are set before streaming */
	ret = __v4l2_ctrl_handler_setup(&imx274->ctrls.handler);
	if (ret) {
		dev_err(&imx274->client->dev,
				"init exp fail in hdr mode\n");
		return ret; 
	}


	imx274_read_reg(imx274, IMX274_REG_CTRL_MODE, &value);
	value = value & 0xfe;
	return imx274_write_reg(imx274, IMX274_REG_CTRL_MODE, value);
}

static int __imx274_stop_stream(struct imx274 *imx274)
{
	u8 value =0;
#if 0
        return imx327_write_reg(imx274->client,
                IMX327_REG_CTRL_MODE,
                IMX327_REG_VALUE_08BIT,
                1);
#endif
	imx274_write_table(imx274, imx274_stop);
	imx274_read_reg(imx274, IMX274_REG_CTRL_MODE, &value);
	value |= 0x01;
	
	return imx274_write_reg(imx274,IMX274_REG_CTRL_MODE, value);
}



/**
 * imx274_s_stream - It is used to start/stop the streaming.
 * @sd: V4L2 Sub device
 * @on: Flag (True / False)
 *
 * This function controls the start or stop of streaming for the
 * imx274 sensor.
 *
 * Return: 0 on success, errors otherwise
 */
static int imx274_s_stream(struct v4l2_subdev *sd, int on)
{
	struct imx274 *imx274 = to_imx274(sd);
	struct i2c_client *client = imx274->client;

	int ret = 0;
	printk("lwx:  imx274_s_stream \n");
	dev_err(&imx274->client->dev, "%s : %s, mode index = %td\n", __func__,
		on ? "Stream Start" : "Stream Stop",
		imx274->mode - &imx274_formats[0]);

	mutex_lock(&imx274->lock);
	on = !!on;
	if(on == imx274->streaming) {
		printk("lwx:stream is starting  just return \n");
		goto unlock_and_return;
	}

	if (on) {
                ret = pm_runtime_get_sync(&client->dev);
                if (ret < 0) {
			printk("lwx pm_runtime_get_sync failed\n");
                        pm_runtime_put_noidle(&client->dev);
                        goto unlock_and_return;
                }


		printk("lwx imx274 start stream \n");

		ret = __imx274_start_stream(imx274);
		if(ret) {
			v4l2_err(sd, "imx274 start stream failed while write reg\n");
			pm_runtime_put(&client->dev);
			goto unlock_and_return;
		}
		printk("imx274 start stream  ok\n");
	} else {
	 	/* stop stream */
		__imx274_stop_stream(imx274);
		pm_runtime_put(&client->dev);

	}
	imx274->streaming = on;

unlock_and_return:
	mutex_unlock(&imx274->lock);
	return ret;
}

static int imx274_g_mbus_config(struct v4l2_subdev *sd,
                                struct v4l2_mbus_config *config)
{
        struct imx274 *imx274 = to_imx274(sd);
        u32 val = 0;

        val = 1 << (IMX274_4LANES - 1) |
                        V4L2_MBUS_CSI2_CHANNEL_0 |
                        V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;
        if (imx274->bus_cfg.bus_type == 3)
                config->type = V4L2_MBUS_CCP2;
        else
                config->type = V4L2_MBUS_CSI2;
        config->flags = val;

        return 0;
}


/*
 * imx274_get_frame_length - Function for obtaining current frame length
 * @priv: Pointer to device structure
 * @val: Pointer to obainted value
 *
 * frame_length = vmax x (svr + 1), in unit of hmax.
 *
 * Return: 0 on success
 */
static int imx274_get_frame_length(struct imx274 *priv, u32 *val)
{
	int err;
	u16 svr;
	u32 vmax;
	u8 reg_val[3];

	/* svr */
	err = imx274_read_reg(priv, IMX274_SVR_REG_LSB, &reg_val[0]);
	if (err)
		goto fail;

	err = imx274_read_reg(priv, IMX274_SVR_REG_MSB, &reg_val[1]);
	if (err)
		goto fail;

	svr = (reg_val[1] << IMX274_SHIFT_8_BITS) + reg_val[0];

	/* vmax */
	err = imx274_read_reg(priv, IMX274_VMAX_REG_3, &reg_val[0]);
	if (err)
		goto fail;

	err = imx274_read_reg(priv, IMX274_VMAX_REG_2, &reg_val[1]);
	if (err)
		goto fail;

	err = imx274_read_reg(priv, IMX274_VMAX_REG_1, &reg_val[2]);
	if (err)
		goto fail;

	vmax = ((reg_val[2] & IMX274_MASK_LSB_3_BITS) << IMX274_SHIFT_16_BITS)
		+ (reg_val[1] << IMX274_SHIFT_8_BITS) + reg_val[0];

	*val = vmax * (svr + 1);

	return 0;

fail:
	dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);
	return err;
}

static int imx274_clamp_coarse_time(struct imx274 *priv, u32 *val,
				    u32 *frame_length)
{
	int err;

	err = imx274_get_frame_length(priv, frame_length);
	if (err)
		return err;

	if (*frame_length < priv->mode->min_frame_len)
		*frame_length =  priv->mode->min_frame_len;

	*val = *frame_length - *val; /* convert to raw shr */
	if (*val > *frame_length - IMX274_SHR_LIMIT_CONST)
		*val = *frame_length - IMX274_SHR_LIMIT_CONST;
	else if (*val < priv->mode->min_SHR)
		*val = priv->mode->min_SHR;

	return 0;
}

/*
 * imx274_set_digital gain - Function called when setting digital gain
 * @priv: Pointer to device structure
 * @dgain: Value of digital gain.
 *
 * Digital gain has only 4 steps: 1x, 2x, 4x, and 8x
 *
 * Return: 0 on success
 */
static int imx274_set_digital_gain(struct imx274 *priv, u32 dgain)
{
	u8 reg_val;

	reg_val = ffs(dgain);

	if (reg_val)
		reg_val--;

	reg_val = clamp(reg_val, (u8)0, (u8)3);

	return imx274_write_reg(priv, IMX274_DIGITAL_GAIN_REG,
				reg_val & IMX274_MASK_LSB_4_BITS);
}

/*
 * imx274_set_gain - Function called when setting gain
 * @priv: Pointer to device structure
 * @val: Value of gain. the real value = val << IMX274_GAIN_SHIFT;
 * @ctrl: v4l2 control pointer
 *
 * Set the gain based on input value.
 * The caller should hold the mutex lock imx274->lock if necessary
 *
 * Return: 0 on success
 */
static int imx274_set_gain(struct imx274 *priv, struct v4l2_ctrl *ctrl)
{
	int err;
	u32 gain, analog_gain, digital_gain, gain_reg;

	gain = (u32)(ctrl->val);

	dev_dbg(&priv->client->dev,
		"%s : input gain = %d.%d\n", __func__,
		gain >> IMX274_GAIN_SHIFT,
		((gain & IMX274_GAIN_SHIFT_MASK) * 100) >> IMX274_GAIN_SHIFT);

	if (gain > IMX274_MAX_DIGITAL_GAIN * IMX274_MAX_ANALOG_GAIN)
		gain = IMX274_MAX_DIGITAL_GAIN * IMX274_MAX_ANALOG_GAIN;
	else if (gain < IMX274_MIN_GAIN)
		gain = IMX274_MIN_GAIN;

	if (gain <= IMX274_MAX_ANALOG_GAIN)
		digital_gain = 1;
	else if (gain <= IMX274_MAX_ANALOG_GAIN * 2)
		digital_gain = 2;
	else if (gain <= IMX274_MAX_ANALOG_GAIN * 4)
		digital_gain = 4;
	else
		digital_gain = IMX274_MAX_DIGITAL_GAIN;

	analog_gain = gain / digital_gain;

	dev_dbg(&priv->client->dev,
		"%s : digital gain = %d, analog gain = %d.%d\n",
		__func__, digital_gain, analog_gain >> IMX274_GAIN_SHIFT,
		((analog_gain & IMX274_GAIN_SHIFT_MASK) * 100)
		>> IMX274_GAIN_SHIFT);

	err = imx274_set_digital_gain(priv, digital_gain);
	if (err)
		goto fail;

	/* convert to register value, refer to imx274 datasheet */
	gain_reg = (u32)IMX274_GAIN_CONST -
		(IMX274_GAIN_CONST << IMX274_GAIN_SHIFT) / analog_gain;
	if (gain_reg > IMX274_GAIN_REG_MAX)
		gain_reg = IMX274_GAIN_REG_MAX;

	err = imx274_write_mbreg(priv, IMX274_ANALOG_GAIN_ADDR_LSB, gain_reg,
				 2);
	if (err)
		goto fail;

	if (IMX274_GAIN_CONST - gain_reg == 0) {
		err = -EINVAL;
		goto fail;
	}

	/* convert register value back to gain value */
	ctrl->val = (IMX274_GAIN_CONST << IMX274_GAIN_SHIFT)
			/ (IMX274_GAIN_CONST - gain_reg) * digital_gain;

	dev_dbg(&priv->client->dev,
		"%s : GAIN control success, gain_reg = %d, new gain = %d\n",
		__func__, gain_reg, ctrl->val);

	return 0;

fail:
	dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);
	return err;
}

/*
 * imx274_set_coarse_time - Function called when setting SHR value
 * @priv: Pointer to device structure
 * @val: Value for exposure time in number of line_length, or [HMAX]
 *
 * Set SHR value based on input value.
 *
 * Return: 0 on success
 */
static int imx274_set_coarse_time(struct imx274 *priv, u32 *val)
{
	int err;
	u32 coarse_time, frame_length;

	coarse_time = *val;

	/* convert exposure_time to appropriate SHR value */
	err = imx274_clamp_coarse_time(priv, &coarse_time, &frame_length);
	if (err)
		goto fail;

	err = imx274_write_mbreg(priv, IMX274_SHR_REG_LSB, coarse_time, 2);
	if (err)
		goto fail;

	*val = frame_length - coarse_time;
	return 0;

fail:
	dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);
	return err;
}

/*
 * imx274_set_exposure - Function called when setting exposure time
 * @priv: Pointer to device structure
 * @val: Variable for exposure time, in the unit of micro-second
 *
 * Set exposure time based on input value.
 * The caller should hold the mutex lock imx274->lock if necessary
 *
 * Return: 0 on success
 */
static int imx274_set_exposure(struct imx274 *priv, int val)
{
	int err;
	u16 hmax;
	u8 reg_val[2];
	u32 coarse_time; /* exposure time in unit of line (HMAX)*/

	dev_dbg(&priv->client->dev,
		"%s : EXPOSURE control input = %d\n", __func__, val);

	/* step 1: convert input exposure_time (val) into number of 1[HMAX] */

	/* obtain HMAX value */
	err = imx274_read_reg(priv, IMX274_HMAX_REG_LSB, &reg_val[0]);
	if (err)
		goto fail;
	err = imx274_read_reg(priv, IMX274_HMAX_REG_MSB, &reg_val[1]);
	if (err)
		goto fail;
	hmax = (reg_val[1] << IMX274_SHIFT_8_BITS) + reg_val[0];
	if (hmax == 0) {
		err = -EINVAL;
		goto fail;
	}

	coarse_time = (IMX274_PIXCLK_CONST1 / IMX274_PIXCLK_CONST2 * val
			- priv->mode->nocpiop) / hmax;

	/* step 2: convert exposure_time into SHR value */

	/* set SHR */
	err = imx274_set_coarse_time(priv, &coarse_time);
	if (err)
		goto fail;

	priv->ctrls.exposure->val =
			(coarse_time * hmax + priv->mode->nocpiop)
			/ (IMX274_PIXCLK_CONST1 / IMX274_PIXCLK_CONST2);

	dev_dbg(&priv->client->dev,
		"%s : EXPOSURE control success\n", __func__);
	return 0;

fail:
	dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);

	return err;
}

/*
 * imx274_set_vflip - Function called when setting vertical flip
 * @priv: Pointer to device structure
 * @val: Value for vflip setting
 *
 * Set vertical flip based on input value.
 * val = 0: normal, no vertical flip
 * val = 1: vertical flip enabled
 * The caller should hold the mutex lock imx274->lock if necessary
 *
 * Return: 0 on success
 */
static int imx274_set_vflip(struct imx274 *priv, int val)
{
	int err;

	err = imx274_write_reg(priv, IMX274_VFLIP_REG, val);
	if (err) {
		dev_err(&priv->client->dev, "VFLIP control error\n");
		return err;
	}

	dev_dbg(&priv->client->dev,
		"%s : VFLIP control success\n", __func__);

	return 0;
}

/*
 * imx274_set_test_pattern - Function called when setting test pattern
 * @priv: Pointer to device structure
 * @val: Variable for test pattern
 *
 * Set to different test patterns based on input value.
 *
 * Return: 0 on success
 */
static int imx274_set_test_pattern(struct imx274 *priv, int val)
{
	int err = 0;

	if (val == TEST_PATTERN_DISABLED) {
		err = imx274_write_table(priv, imx274_tp_disabled);
	} else if (val <= TEST_PATTERN_V_COLOR_BARS) {
		err = imx274_write_reg(priv, IMX274_TEST_PATTERN_REG, val - 1);
		if (!err)
			err = imx274_write_table(priv, imx274_tp_regs);
	} else {
		err = -EINVAL;
	}

	if (!err)
		dev_dbg(&priv->client->dev,
			"%s : TEST PATTERN control success\n", __func__);
	else
		dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);

	return err;
}

/*
 * imx274_set_frame_length - Function called when setting frame length
 * @priv: Pointer to device structure
 * @val: Variable for frame length (= VMAX, i.e. vertical drive period length)
 *
 * Set frame length based on input value.
 *
 * Return: 0 on success
 */
static int imx274_set_frame_length(struct imx274 *priv, u32 val)
{
	int err;
	u32 frame_length;

	dev_dbg(&priv->client->dev, "%s : input length = %d\n",
		__func__, val);

	frame_length = (u32)val;

	err = imx274_write_mbreg(priv, IMX274_VMAX_REG_3, frame_length, 3);
	if (err)
		goto fail;

	return 0;

fail:
	dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);
	return err;
}

/*
 * imx274_set_frame_interval - Function called when setting frame interval
 * @priv: Pointer to device structure
 * @frame_interval: Variable for frame interval
 *
 * Change frame interval by updating VMAX value
 * The caller should hold the mutex lock imx274->lock if necessary
 *
 * Return: 0 on success
 */
static int imx274_set_frame_interval(struct imx274 *priv,
				     struct v4l2_fract frame_interval)
{
	int err;
	u32 frame_length, req_frame_rate, max_fps;
	u16 svr;
	u16 hmax;
	u8 reg_val[2];

	dev_err(&priv->client->dev, "%s: input frame interval = %d / %d",
		__func__, frame_interval.numerator,
		frame_interval.denominator);

	if (frame_interval.numerator == 0) {
		err = -EINVAL;
		goto fail;
	}

	req_frame_rate = (u32)(frame_interval.denominator
				/ frame_interval.numerator);
	max_fps = (u32)(priv->mode->max_fps.denominator
			/priv->mode->max_fps.numerator);

	/* boundary check */
	if (req_frame_rate > max_fps) {
		frame_interval = priv->mode->max_fps;
	//	frame_interval.numerator = 1;
	//	frame_interval.denominator = priv->mode->max_fps;
	} else if (req_frame_rate < IMX274_MIN_FRAME_RATE) {
		frame_interval.numerator = 10000;
		frame_interval.denominator = 50000; //IMX274_MIN_FRAME_RATE;
	}

	/*
	 * VMAX = 1/frame_rate x 72M / (SVR+1) / HMAX
	 * frame_length (i.e. VMAX) = (frame_interval) x 72M /(SVR+1) / HMAX
	 */

	/* SVR */
	err = imx274_read_reg(priv, IMX274_SVR_REG_LSB, &reg_val[0]);
	if (err)
		goto fail;
	err = imx274_read_reg(priv, IMX274_SVR_REG_MSB, &reg_val[1]);
	if (err)
		goto fail;
	svr = (reg_val[1] << IMX274_SHIFT_8_BITS) + reg_val[0];
	dev_dbg(&priv->client->dev,
		"%s : register SVR = %d\n", __func__, svr);

	/* HMAX */
	err = imx274_read_reg(priv, IMX274_HMAX_REG_LSB, &reg_val[0]);
	if (err)
		goto fail;
	err = imx274_read_reg(priv, IMX274_HMAX_REG_MSB, &reg_val[1]);
	if (err)
		goto fail;
	hmax = (reg_val[1] << IMX274_SHIFT_8_BITS) + reg_val[0];
	dev_dbg(&priv->client->dev,
		"%s : register HMAX = %d\n", __func__, hmax);

	if (hmax == 0 || frame_interval.denominator == 0) {
		err = -EINVAL;
		goto fail;
	}

	frame_length = IMX274_PIXCLK_CONST1 / (svr + 1) / hmax
					* frame_interval.numerator
					/ frame_interval.denominator;

	err = imx274_set_frame_length(priv, frame_length);
	if (err)
		goto fail;

	priv->frame_interval = frame_interval;
	return 0;

fail:
	dev_err(&priv->client->dev, "%s error = %d\n", __func__, err);
	return err;
}

/* Calculate the delay in us by clock rate and clock cycles */
static inline u32 imx274_cal_delay(u32 cycles)
{
	return DIV_ROUND_UP(cycles, IMX274_XVCLK_FREQ / 1000 / 1000);
}

static int __imx274_power_on(struct imx274 *imx274)
{
	int ret;
	u32 delay_us;
	struct device *dev = &imx274->client->dev;

	if (!IS_ERR_OR_NULL(imx274->pins_default)) {
		ret = pinctrl_select_state(imx274->pinctrl,
					   imx274->pins_default);
		if (ret < 0)
			dev_err(dev, "could not set pins\n");
	}

	ret = clk_set_rate(imx274->xvclk, IMX274_XVCLK_FREQ);
	if (ret < 0)
		dev_warn(dev, "Failed to set xvclk rate (37.125M Hz)\n");

	if (clk_get_rate(imx274->xvclk) != IMX274_XVCLK_FREQ)
		dev_warn(dev, "xvclk mismatched,based on 24M Hz\n");

	ret = clk_prepare_enable(imx274->xvclk);
	if (ret < 0) {
		dev_err(dev, "Failed to enable xvclk\n");
		return ret;
	}


	ret = regulator_bulk_enable(IMX274_NUM_SUPPLIES, imx274->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators\n");
		goto disable_clk;
	}

	if (!IS_ERR(imx274->reset_gpio))
		gpiod_set_value_cansleep(imx274->reset_gpio, 0);
	usleep_range(500, 1000);
	if (!IS_ERR(imx274->reset_gpio))
		gpiod_set_value_cansleep(imx274->reset_gpio, 1);

	if (!IS_ERR(imx274->pwdn_gpio))
		gpiod_set_value_cansleep(imx274->pwdn_gpio, 1);

	/* 8192 cycles prior to first SCCB transaction */
	delay_us = imx274_cal_delay(8192);
	usleep_range(delay_us, delay_us * 2);
	usleep_range(5000, 10000);
	return 0;

disable_clk:
	clk_disable_unprepare(imx274->xvclk);

	return ret;
}

static void __imx274_power_off(struct imx274 *imx274)
{
#if 1	
	int ret;
	struct device *dev = &imx274->client->dev;

	if (!IS_ERR(imx274->pwdn_gpio))
		gpiod_set_value_cansleep(imx274->pwdn_gpio, 0);
	clk_disable_unprepare(imx274->xvclk);
	if (!IS_ERR(imx274->reset_gpio))
		gpiod_set_value_cansleep(imx274->reset_gpio, 0);
	if (!IS_ERR_OR_NULL(imx274->pins_sleep)) {
		ret = pinctrl_select_state(imx274->pinctrl,
					   imx274->pins_sleep);
		if (ret < 0)
			dev_dbg(dev, "could not set pins\n");
	}
	regulator_bulk_disable(IMX274_NUM_SUPPLIES, imx274->supplies);
#endif
}


static int imx274_runtime_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx274 *imx274 = to_imx274(sd);
	return __imx274_power_on(imx274);
}

static int imx274_runtime_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx274 *imx274 = to_imx274(sd);

	__imx274_power_off(imx274);

	return 0;
}

static void imx274_get_module_inf(struct imx274 *imx274,
				  struct rkmodule_inf *inf)
{
	memset(inf, 0, sizeof(*inf));
	strlcpy(inf->base.sensor, IMX274_NAME, sizeof(inf->base.sensor));
	strlcpy(inf->base.module, imx274->module_name,
		sizeof(inf->base.module));
	strlcpy(inf->base.lens, imx274->len_name, sizeof(inf->base.lens));
}

static long imx274_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct imx274 *imx274 = to_imx274(sd);
//	struct rkmodule_hdr_cfg *hdr;
//	struct rkmodule_lvds_cfg *lvds_cfg;
	//u32 i, h, w;
	long ret = 0;
	//s64 dst_pixel_rate = 0;
	//s32 dst_link_freq = 0;
	int stream = 0;
	u8 value = 0;

	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		imx274_get_module_inf(imx274, (struct rkmodule_inf *)arg);
		break;
/*
	case PREISP_CMD_SET_HDRAE_EXP:
		ret = imx327_set_hdrae(imx327, arg);
		break;
	case RKMODULE_GET_HDR_CFG:
		hdr = (struct rkmodule_hdr_cfg *)arg;
		if (imx327->cur_mode->hdr_mode == NO_HDR)
			hdr->esp.mode = HDR_NORMAL_VC;
		else
			hdr->esp.mode = HDR_ID_CODE;
		hdr->hdr_mode = imx327->cur_mode->hdr_mode;
		break;
	case RKMODULE_SET_HDR_CFG:
		hdr = (struct rkmodule_hdr_cfg *)arg;
		for (i = 0; i < imx327->support_modes_num; i++) {
			if (imx327->support_modes[i].hdr_mode == hdr->hdr_mode) {
				imx327->cur_mode = &imx327->support_modes[i];
				break;
			}
		}
		if (i == imx327->support_modes_num) {
			dev_err(&imx327->client->dev,
				"not find hdr mode:%d config\n",
				hdr->hdr_mode);
			ret = -EINVAL;
		} else {
			w = imx327->cur_mode->hts_def - imx327->cur_mode->width;
			h = imx327->cur_mode->vts_def - imx327->cur_mode->height;
			__v4l2_ctrl_modify_range(imx327->hblank, w, w, 1, w);
			__v4l2_ctrl_modify_range(imx327->vblank, h,
				IMX327_VTS_MAX - imx327->cur_mode->height,
				1, h);
			if (imx327->cur_mode->hdr_mode == NO_HDR) {
				dst_link_freq = 0;
				dst_pixel_rate = IMX327_PIXEL_RATE_NORMAL;
			} else {
				dst_link_freq = 1;
				dst_pixel_rate = IMX327_PIXEL_RATE_HDR;
			}
			__v4l2_ctrl_s_ctrl_int64(imx327->pixel_rate,
						 dst_pixel_rate);
			__v4l2_ctrl_s_ctrl(imx327->link_freq,
					   dst_link_freq);
			imx327->cur_vts = imx327->cur_mode->vts_def;
		}
		break;
*/
	case RKMODULE_SET_CONVERSION_GAIN:
/*
		ret = imx327_set_conversion_gain(imx327, (u32 *)arg);
*/
		printk("lwx:set gain\n");
		break;

/*
	case RKMODULE_GET_LVDS_CFG:
		lvds_cfg = (struct rkmodule_lvds_cfg *)arg;
		if (imx327->bus_cfg.bus_type == 3)
			memcpy(lvds_cfg, &imx327->cur_mode->lvds_cfg,
				sizeof(struct rkmodule_lvds_cfg));
		else
			ret = -ENOIOCTLCMD;
		break;
*/
	case RKMODULE_SET_QUICK_STREAM:
		stream = *((u32 *)arg);
	        imx274_read_reg(imx274, IMX274_REG_CTRL_MODE, &value);

		if(stream)
	        	value = value & 0xfe;
		else
			value = value | 0x01;

		ret = imx274_write_reg(imx274, IMX274_REG_CTRL_MODE, value);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static long imx274_compat_ioctl32(struct v4l2_subdev *sd,
				  unsigned int cmd, unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	struct rkmodule_inf *inf;
	struct rkmodule_awb_cfg *cfg;
	struct rkmodule_hdr_cfg *hdr;
	struct preisp_hdrae_exp_s *hdrae;
	long ret;
	u32 cg = 0;

	switch (cmd) {
	case RKMODULE_GET_MODULE_INFO:
		inf = kzalloc(sizeof(*inf), GFP_KERNEL);
		if (!inf) {
			ret = -ENOMEM;
			return ret;
		}

		ret = imx274_ioctl(sd, cmd, inf);
		if (!ret)
			ret = copy_to_user(up, inf, sizeof(*inf));
		kfree(inf);
		break;
	case RKMODULE_AWB_CFG:
		cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
		if (!cfg) {
			ret = -ENOMEM;
			return ret;
		}
		ret = copy_from_user(cfg, up, sizeof(*cfg));
		if (!ret)
			ret = imx274_ioctl(sd, cmd, cfg);
		kfree(cfg);
		break;
	case RKMODULE_GET_HDR_CFG:
		hdr = kzalloc(sizeof(*hdr), GFP_KERNEL);
		if (!hdr) {
			ret = -ENOMEM;
			return ret;
		}

		ret = imx274_ioctl(sd, cmd, hdr);
		if (!ret)
			ret = copy_to_user(up, hdr, sizeof(*hdr));
		kfree(hdr);
		break;
	case RKMODULE_SET_HDR_CFG:
		hdr = kzalloc(sizeof(*hdr), GFP_KERNEL);
		if (!hdr) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(hdr, up, sizeof(*hdr));
		if (!ret)
			ret = imx274_ioctl(sd, cmd, hdr);
		kfree(hdr);
		break;
	case PREISP_CMD_SET_HDRAE_EXP:
		hdrae = kzalloc(sizeof(*hdrae), GFP_KERNEL);
		if (!hdrae) {
			ret = -ENOMEM;
			return ret;
		}

		ret = copy_from_user(hdrae, up, sizeof(*hdrae));
		if (!ret)
			ret = imx274_ioctl(sd, cmd, hdrae);
		kfree(hdrae);
		break;
	case RKMODULE_SET_CONVERSION_GAIN:
		ret = copy_from_user(&cg, up, sizeof(cg));
		if (!ret)
			ret = imx274_ioctl(sd, cmd, &cg);
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif


#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static int imx274_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{

	struct imx274 *imx274 = to_imx274(sd);
	struct v4l2_mbus_framefmt *try_fmt =
				v4l2_subdev_get_try_format(sd, fh->pad, 0);
	const struct imx274_mode *def_mode = &imx274->support_modes[0];

	mutex_lock(&imx274->lock);
	/* Initialize try_fmt */
#if 1
	try_fmt->width = imx274->format.width;
	try_fmt->height = 	imx274->format.height;
	try_fmt->code = def_mode->bus_fmt;//MEDIA_BUS_FMT_SRGGB10_1X10;
	try_fmt->field = V4L2_FIELD_NONE;
	printk("lwx: in imx274_open  width:%d height:%d\n",imx274->format.width, imx274->format.height);
#endif
	mutex_unlock(&imx274->lock);
	/* No crop or compose */

	return 0;
}
#endif


static int imx274_s_power(struct v4l2_subdev *sd, int on)
{
	struct imx274 *imx274 = to_imx274(sd);
	struct i2c_client *client = imx274->client;
	int ret = 0;
	printk("lwx: imx274  mx274->power_on=%d power=%d\n",imx274->power_on, on);
	mutex_lock(&imx274->lock);

	/* If the power state is not modified - no work to do. */
	if (imx274->power_on == !!on)
		goto unlock_and_return;

	if (on) {
		ret = pm_runtime_get_sync(&client->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&client->dev);
			goto unlock_and_return;
		}
#if 0  // by lwx
		ret = imx274_write_array(imx274->client, imx274_global_regs);
		if (ret) {
			v4l2_err(sd, "could not set init registers\n");
			pm_runtime_put_noidle(&client->dev);
			goto unlock_and_return;
		}
#endif
		imx274->power_on = true;
	} else {
		pm_runtime_put(&client->dev);
		imx274->power_on = false;
	}

unlock_and_return:
	mutex_unlock(&imx274->lock);

	return ret;
}

static const struct dev_pm_ops imx274_pm_ops = {
	SET_RUNTIME_PM_OPS(imx274_runtime_suspend,
			   imx274_runtime_resume, NULL)
};

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
static const struct v4l2_subdev_internal_ops imx274_internal_ops = {
	.open = imx274_open,
};
#endif


static const struct v4l2_subdev_core_ops imx274_core_ops = {
	.s_power = imx274_s_power,
	.ioctl = imx274_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = imx274_compat_ioctl32,
#endif
};


static const struct v4l2_subdev_video_ops imx274_video_ops = {
	.g_frame_interval = imx274_g_frame_interval,
	.s_frame_interval = imx274_s_frame_interval,
	.s_stream = imx274_s_stream,
	.g_mbus_config = imx274_g_mbus_config

};

static const struct v4l2_subdev_pad_ops imx274_pad_ops = {
        .enum_mbus_code = imx274_enum_mbus_code,
        .enum_frame_size = imx274_enum_frame_sizes,
        .enum_frame_interval = imx274_enum_frame_interval,
	.get_fmt = imx274_get_fmt,
	.set_fmt = imx274_set_fmt,
	.get_selection = imx274_get_selection,
	.set_selection = imx274_set_selection,
};


static const struct v4l2_subdev_ops imx274_subdev_ops = {
	.pad = &imx274_pad_ops,
	.video = &imx274_video_ops,
	.core = &imx274_core_ops
};

static const struct v4l2_ctrl_ops imx274_ctrl_ops = {
	.s_ctrl	= imx274_s_ctrl,
};


static int imx274_check_sensor_id(struct imx274 *imx274,
				  struct i2c_client *client)
{
	struct device *dev = &imx274->client->dev;
	u8 id = 0;
	int ret;

	ret = imx274_read_reg(imx274, IMX274_REG_CHIP_ID, &id);

	if (id != CHIP_ID) {
		dev_err(dev, "Unexpected sensor id(%06x), ret(%d)\n", id, ret);
		return -EINVAL;
	}
	else {
	 	dev_info(dev, "Get sensor id(%06x)\n",id);
	}
	return ret;
}

static int imx274_configure_regulators(struct imx274 *imx274)
{
	unsigned int i;

	for (i = 0; i < IMX274_NUM_SUPPLIES; i++)
		imx274->supplies[i].supply = imx274_supply_names[i];

	return devm_regulator_bulk_get(&imx274->client->dev,
				       IMX274_NUM_SUPPLIES,
				       imx274->supplies);
}


static int imx274_initialize_controls(struct imx274 *imx274)
{
	struct v4l2_ctrl_handler *handler;
	const struct imx274_mode *mode;
	int ret;
	u64 pixel_rate;

	handler = &imx274->ctrls.handler;
	mode = imx274->mode;
	ret = v4l2_ctrl_handler_init(handler, 6);
	if (ret)
		return ret;


	imx274->ctrls.handler.lock = &imx274->lock;

	/* add new controls */

	imx274->ctrls.link_freq = v4l2_ctrl_new_int_menu(handler, NULL,
                                V4L2_CID_LINK_FREQ,
                                ARRAY_SIZE(link_freq_items) - 1, 0,
                                link_freq_items);
	__v4l2_ctrl_s_ctrl(imx274->ctrls.link_freq, mode->mipi_freq_idx);


    /* pixel rate = link frequency * 2 * lanes / BITS_PER_SAMPLE */
    pixel_rate = (u32)link_freq_items[mode->mipi_freq_idx] / mode->bpp * 2 * IMX274_4LANES;
    imx274->ctrls.pixel_rate = v4l2_ctrl_new_std(handler, NULL,
            V4L2_CID_PIXEL_RATE, 0, IMX274_MAX_PIXEL_RATE,
            1, pixel_rate);



	imx274->ctrls.test_pattern = v4l2_ctrl_new_std_menu_items(
		&imx274->ctrls.handler, &imx274_ctrl_ops,
		V4L2_CID_TEST_PATTERN,
		ARRAY_SIZE(tp_qmenu) - 1, 0, 0, tp_qmenu);

	imx274->ctrls.gain = v4l2_ctrl_new_std(
		&imx274->ctrls.handler,
		&imx274_ctrl_ops,
		V4L2_CID_GAIN, IMX274_MIN_GAIN,
		IMX274_MAX_DIGITAL_GAIN * IMX274_MAX_ANALOG_GAIN, 1,
		IMX274_DEF_GAIN);

	imx274->ctrls.exposure = v4l2_ctrl_new_std(
		&imx274->ctrls.handler,
		&imx274_ctrl_ops,
		V4L2_CID_EXPOSURE, IMX274_MIN_EXPOSURE_TIME,
		1000000 / IMX274_DEF_FRAME_RATE, 1,
		IMX274_MIN_EXPOSURE_TIME);

	imx274->ctrls.vflip = v4l2_ctrl_new_std(
		&imx274->ctrls.handler,
		&imx274_ctrl_ops,
		V4L2_CID_VFLIP, 0, 1, 1, 0);

	if (imx274->ctrls.handler.error) {
		ret = imx274->ctrls.handler.error;
		goto err_free_handler;
	}

	imx274->sd.ctrl_handler = &imx274->ctrls.handler;
	return 0;

	
err_free_handler:
	v4l2_ctrl_handler_free(handler);
	return ret;
}

static int imx274_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct imx274 *imx274;
	int ret;
	char facing[2];
//	struct device_node *endpoint;
	struct device *dev = &client->dev;
	struct device_node *node = dev->of_node;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);


	/* initialize imx274 */
	imx274 = devm_kzalloc(&client->dev, sizeof(*imx274), GFP_KERNEL);
	if (!imx274)
		return -ENOMEM;
        imx274->streaming = 0;
	imx274->power_on = 0;
	ret = of_property_read_u32(node, RKMODULE_CAMERA_MODULE_INDEX,
				   &imx274->module_index);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_FACING,
				       &imx274->module_facing);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_MODULE_NAME,
				       &imx274->module_name);
	ret |= of_property_read_string(node, RKMODULE_CAMERA_LENS_NAME,
				       &imx274->len_name);
	if (ret) {
		dev_err(dev, "could not get module information!\n");
		return -EINVAL;
	}

	imx274->client = client;
/*
	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint),
		&imx274->bus_cfg);
*/
	imx274->bus_cfg.bus_type = 2;
	if (imx274->bus_cfg.bus_type == 3) {
		//imx274->support_modes = lvds_supported_modes;
		//imx274->support_modes_num = ARRAY_SIZE(lvds_supported_modes);
		dev_err(dev, "imx274 not support lvds mode!\n");
		return -EINVAL;

	} else {
		imx274->support_modes = imx274_formats;
		imx274->support_modes_num = ARRAY_SIZE(imx274_formats);
		imx274->mode = &imx274_formats[IMX274_DEFAULT_MODE];
	}


	imx274->xvclk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(imx274->xvclk)) {
		dev_err(dev, "Failed to get xvclk\n");
		return -EINVAL;
	}

/*
	imx274->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(imx274->reset_gpio)) {
		if (PTR_ERR(imx274->reset_gpio) != -EPROBE_DEFER)
			dev_err(&client->dev, "Reset GPIO not setup in DT");
		ret = PTR_ERR(imx274->reset_gpio);
		goto err_me;
	}
*/

	imx274->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);      // org 274 is GPIOD_OUT_HIGH
	if (IS_ERR(imx274->reset_gpio))
		dev_warn(dev, "Failed to get reset-gpios\n");

	imx274->pwdn_gpio = devm_gpiod_get(dev, "pwdn", GPIOD_OUT_LOW);
	if (IS_ERR(imx274->pwdn_gpio))
		dev_warn(dev, "Failed to get pwdn-gpios\n");

	ret = imx274_configure_regulators(imx274);
	if (ret) {
		dev_err(dev, "Failed to get power regulators\n");
		return ret;
	}

	imx274->pinctrl = devm_pinctrl_get(dev);
	if (!IS_ERR(imx274->pinctrl)) {
		imx274->pins_default =
			pinctrl_lookup_state(imx274->pinctrl,
					     OF_CAMERA_PINCTRL_STATE_DEFAULT);
		if (IS_ERR(imx274->pins_default))
			dev_err(dev, "could not get default pinstate\n");

		imx274->pins_sleep =
			pinctrl_lookup_state(imx274->pinctrl,
					     OF_CAMERA_PINCTRL_STATE_SLEEP);
		if (IS_ERR(imx274->pins_sleep))
			dev_err(dev, "could not get sleep pinstate\n");
	}

	mutex_init(&imx274->lock);

	/* initialize format */

	imx274->crop.width = IMX274_MAX_WIDTH;
	imx274->crop.height = IMX274_MAX_HEIGHT;
	imx274->format.width = imx274->crop.width / imx274->mode->bin_ratio;
	imx274->format.height = imx274->crop.height / imx274->mode->bin_ratio;
	imx274->format.field = V4L2_FIELD_NONE;
	imx274->format.code = imx274->mode->bus_fmt;//MEDIA_BUS_FMT_SRGGB10_1X10;
	imx274->format.colorspace = V4L2_COLORSPACE_SRGB;
	imx274->frame_interval = imx274->mode->max_fps;
	//imx274->frame_interval.numerator = 10000;
	//imx274->frame_interval.denominator = 300000; //IMX274_DEF_FRAME_RATE;

	strlcpy(imx274->sd.name, DRIVER_NAME, sizeof(sd->name));
	/* initialize subdevice */
	sd = &imx274->sd;
	v4l2_i2c_subdev_init(sd, client, &imx274_subdev_ops);
	if(imx274_initialize_controls(imx274) !=0) {
		goto err_me;
	}
	
	/* pull sensor out of reset */
	ret = __imx274_power_on(imx274);
	if (ret)
		goto err_ctrls;

	ret = imx274_check_sensor_id(imx274, client);
	if (ret)
		goto err_ctrls;

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	dev_err(dev, "set the video v4l2 subdev api\n");
	sd->internal_ops = &imx274_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
#endif

#if defined(CONFIG_MEDIA_CONTROLLER)
	/* initialize subdev media pad */
	imx274->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sd->entity, 1, &imx274->pad);
	if (ret < 0) {
		dev_err(&client->dev,
			"%s : media entity init Failed %d\n", __func__, ret);
		goto err_regmap;
	}
#endif

	/* load default control values */
	ret = imx274_load_default(imx274);
	if (ret) {
		dev_err(&client->dev,
			"%s : imx274_load_default failed %d\n",
			__func__, ret);
		goto err_ctrls;
	}


	memset(facing, 0, sizeof(facing));
	if (strcmp(imx274->module_facing, "back") == 0)
		facing[0] = 'b';
	else
		facing[0] = 'f';

	snprintf(sd->name, sizeof(sd->name), "m%02d_%s_%s %s",
		 imx274->module_index, facing,
		 IMX274_NAME, dev_name(sd->dev));



	/* register subdevice */
	ret = v4l2_async_register_subdev(sd);
	if (ret < 0) {
		dev_err(&client->dev,
			"%s : v4l2_async_register_subdev failed %d\n",
			__func__, ret);
		goto err_ctrls;
	}

         pm_runtime_set_active(dev);
         pm_runtime_enable(dev);
         pm_runtime_idle(dev);


	dev_info(&client->dev, "imx274 : imx274 probe success !\n");
	return 0;

err_ctrls:
	v4l2_ctrl_handler_free(&imx274->ctrls.handler);
err_me:
	media_entity_cleanup(&sd->entity);
err_regmap:
	mutex_destroy(&imx274->lock);
	return ret;
}

static int imx274_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx274 *imx274 = to_imx274(sd);

	/* stop stream */
	imx274_write_table(imx274, imx274_stop);

	v4l2_async_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&imx274->ctrls.handler);
	media_entity_cleanup(&sd->entity);
	mutex_destroy(&imx274->lock);
	return 0;
}


#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id imx274_of_match[] = {
	{ .compatible = "sony,imx274" },
	{ }
};
MODULE_DEVICE_TABLE(of, imx274_of_match);
#endif

static const struct i2c_device_id imx274_match_id[] = {
	{ "sony,imx274", 0 },
	{ },
};


static struct i2c_driver imx274_i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.pm = &imx274_pm_ops,
		.of_match_table = of_match_ptr(imx274_of_match),
	},

	.probe		= imx274_probe,
	.remove		= imx274_remove,
	.id_table	= imx274_match_id,
};


static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&imx274_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&imx274_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("Sony imx327 sensor driver");
MODULE_LICENSE("GPL v2");
