/* Proof-of-concept verification for a specific GT915 configuration.

   To perform an analysis we need to define a main function and stubs.
   There are two kind of stubs:
    1. The stubs needed to simulate the Linux Kernel APIs
    2. The stubs needed to simulate the behavior of the Hardware

   At the Kernel level no parallelism is simulated, i.e. the Kernel is
   supposed not to be running in a SMP system.
   The interrupt handlers are called sequentially after the initialization of
   the driver.
   We do not check if the Linux Kernel APIs are called properly by the driver.
   The Kernel simulation is very coarse.

   At the hardware level we based our simulation on the GT915 spec sheet.

   The main driver is implemented in function main.
   - It calls the initialization function of the driver
   - It forces the probing of the hardware
   - It calls infinitely the IRQ handler

   The behavior of the driver is very dependent on the Linux devicetree.
   A fragment of this device tree is defined here.
*/

#include <tis_builtin.h>

#include <linux/i2c.h>
#include "drivers/input/touchscreen/gt9xx/gt9xx.h"
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/firmware.h>
#include "fs/proc/internal.h"

int printf(const char *format, ...);
int scnprintf(char *buf, size_t size, char const *fmt , ...)
{
    tis_make_unknown(buf, size);
    return size;
}

/* 1. The Kernel level stubs */

/* Basic Kernel workqueues. We only need to handle one element in the queue. */
bool
queue_work_on(int cpu, struct workqueue_struct *wq,
              struct work_struct *work)
{
    if (work)
        work->func(work);
    return 0;
}
bool queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
                           struct delayed_work *work,
                           unsigned long delay)
{
    if (work)
        work->work.func(&work->work);
    return 0;
}
void __init_work(struct work_struct *work, int onstack)
{
    return;
}

struct workqueue_struct *
__alloc_workqueue_key(const char *fmt, unsigned int flags, int max_active,
                      struct lock_class_key *key, const char *lock_name, ...)
{
    return NULL;
}

bool cancel_work_sync(struct work_struct *work)
{
    return 0;
}

//@ assigns \nothing;
void flush_workqueue();

//@ assigns \nothing;
void destroy_workqueue();

/* Kernel mutex. They are ignored in this analysis. */
void mutex_lock(struct mutex *lock)
{
    return;
}

void mutex_unlock(struct mutex *lock)
{
    return;
}

void __mutex_init(struct mutex *lock, char const *name,
                  struct lock_class_key *key)
{
    return;
}

/* Kernel spinlocks. Ignored in this analysis. */
void __raw_spin_lock_init(raw_spinlock_t *lock, char const *name,
                          struct lock_class_key *key)
{
    return;
}

void _raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags)
{
    return;
}

unsigned long _raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
    return 0;
}

/* Linux input driver */
void input_event(struct input_dev *dev, unsigned int type,
                 unsigned int code, int value)
{
    return;
}

void input_mt_report_slot_state(struct input_dev *dev,
                                unsigned int tool_type, bool active)
{
    return;
}

void input_set_abs_params(struct input_dev *dev, unsigned int axis,
                          int min, int max, int fuzz, int flat)
{
    return;
}

int input_mt_init_slots(struct input_dev *dev, unsigned int num_slots,
                        unsigned int flags)
{
    return 0;
}

struct input_dev tis_input_dev;

struct input_dev *input_allocate_device(void)
{
    return &tis_input_dev;
}

int input_register_device(struct input_dev *)
{
    return 0;
}

/* Linux device */
int dev_set_name(struct device *dev, const char *name, ...)
{
    return 0;
}
void device_initialize(struct device *dev)
{
    return;
}

/* Linux GPIO */
int gpiod_direction_input(struct gpio_desc *p)
{
    return 0;
}

int gpiod_direction_output_raw(struct gpio_desc *desc, int value) {
    return 0;
}

struct gpio_desc *gpio_to_desc(unsigned gpio)
{
    return NULL;
}

int gpio_request( unsigned x,char const *  y)
{
    return 0;
}

//@ assigns \nothing;
void gpio_free(unsigned int gpio);

//@ assigns \result \from \nothing;
struct gpio_desc *gpio_to_desc();

//@ assigns \result \from \nothing;
int gpiod_direction_output_raw();

/* Direct access to de Device tree. */
int of_get_named_gpio_flags(struct device_node *np,
                            char const *list_name, int index,
                            enum of_gpio_flags *flags)
{
    /* Hard-coding some nodes of the device tree in order
       to short-cut the Kernel behaviour. */
    if (strcmp(list_name, "reset-gpios") == 0) {
        return 16;
    } else if (strcmp(list_name, "interrupt-gpios") == 0) {
        return 2;
    }
    return -1;
}

#define TIS_FIRMWARE_SIZE 43008
uint8_t tis_firmware_data[TIS_FIRMWARE_SIZE] = { 1 };

/* Firmware related functions */
int request_firmware(const struct firmware **fw, const char *name,
		     struct device *device)
{
    static struct firmware tis_firmware = { .size = TIS_FIRMWARE_SIZE,
                                            .data = tis_firmware_data };
    *fw = &tis_firmware;
    return 0;
}

void release_firmware(struct firmware const *fw)
{
    return;
}

/* IRQ management */
#define TIS_MAX_IRQ 2
irqreturn_t (*tis_irqs_handlers[TIS_MAX_IRQ])(int , void *);
void *tis_irq_args[TIS_MAX_IRQ];

int request_threaded_irq(unsigned int irq,
                         irqreturn_t (*handler)(int , void *),
                         irqreturn_t (*thread_fn)(int , void *),
                         unsigned long flags, char const *name,
                         void *dev)
{
    if (irq >= TIS_MAX_IRQ || tis_irqs_handlers[irq]) {
        printf("Cannot allocate irq %u", irq);
        return 1;
    }
    tis_irqs_handlers[irq] = thread_fn;
    tis_irq_args[irq] = (void *)dev;
    return 0;
}

void enable_irq(unsigned int irq)
{
    return;
}

void disable_irq_nosync(unsigned int irq)
{
    return;
}

/* I2C Kernel simulation */
static struct i2c_driver TIS_I2C_DRIVER = {0};

int i2c_register_driver(struct module *module, struct i2c_driver *driver) {
    TIS_I2C_DRIVER = *driver;
    return 0;
}

int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
                 int num)
{
    return adap->algo->master_xfer(adap, msgs, num);
}

struct proc_dir_entry tis_dummy_proc_entry = 0;
struct file_operations tis_proc_fops;

struct proc_dir_entry *proc_create_data(const char *c, umode_t m,
                                        struct proc_dir_entry *d,
                                        const struct file_operations *f,
                                        void *v)
{
    tis_proc_fops = *f;
    return &tis_dummy_proc_entry;
}

/* Kernel memory management */
void *calloc(size_t nmemb, size_t size);

void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp) {
    return calloc(size, 1);
}

void *__kmalloc(size_t size, gfp_t gfp) {
    return calloc(size, 1);
}

extern void free(void*);

void devm_kfree(struct device *dev, void *p)
{
    free(p);
}

/* Kernel timer management */
void init_timer_key(struct timer_list *timer, unsigned int flags,
                    char const *name, struct lock_class_key *key) {
    return;
}

//@ assigns \nothing;
void msleep(unsigned int);

//@ assigns \nothing;
int usleep(int);

/* Devicetree definition: a linked list of properties.
   Here we hardcode the properties that are supposed to be provided
   by the DT at boot time.
   The properties and their values are extracted from
   Documentation/devicetree/bindings/input/touchscreen/gt9xx/gt9xx.txt
   Changing this will impact the coverage of the analysis.
*/
uint32_t tis_XXX[] = { 6666 } ; // Dummy property for testing purpose.

struct property tis_prop_XXX =
{ .name = "XXXX",
  .length = sizeof(tis_XXX),
  .value = tis_XXX,
  .next = 0
};

char tis_product_id[] = "915" ;

struct property tis_prop_product_id =
{ .name = "goodix,product-id",
  .length = sizeof(tis_product_id),
  .value = tis_product_id,
  .next = &tis_prop_XXX
};

struct property tis_prop_driver_send_cfg =
{ .name = "goodix,driver-send-cfg",
  .length = sizeof(char),
  .value = 0,
  .next = &tis_prop_product_id
};

uint8_t tis_cfg_data0[] = {
    0x41, 0xD0, 0x02, 0x00, 0x05, 0x0A, 0x05, 0x01, 0x01, 0x08, 0x12,
    0x58, 0x50, 0x41, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x8C, 0x2E, 0x0E, 0x28, 0x24, 0x73,
    0x13, 0x00, 0x00, 0x00, 0x83, 0x03, 0x1D, 0x40, 0x02, 0x00, 0x00,
    0x00, 0x03, 0x64, 0x32, 0x00, 0x00, 0x00, 0x1A, 0x38, 0x94, 0xC0,
    0x02, 0x00, 0x00, 0x00, 0x04, 0x9E, 0x1C, 0x00, 0x8D, 0x20, 0x00,
    0x7A, 0x26, 0x00, 0x6D, 0x2C, 0x00, 0x60, 0x34, 0x00, 0x60, 0x10,
    0x38, 0x68, 0x00, 0xF0, 0x50, 0x35, 0xFF, 0xFF, 0x27, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x1B, 0x14, 0x0C, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12,
    0x14, 0x16, 0x18, 0x1A, 0x1C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0F, 0x10, 0x12, 0x13, 0x14,
    0x16, 0x18, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x24, 0x26,
    0x28, 0x29, 0x2A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0xFF, 0x07, 0x01
};

struct property tis_prop_cfg_data0 =
{ .name = "goodix,cfg-data0",
  .length = sizeof(tis_cfg_data0),
  .value = tis_cfg_data0,
  .next = &tis_prop_driver_send_cfg
};

uint32_t tis_buttons[3]  = { 55, 67, 77 };

struct property tis_prop_button =
{ .name = "goodix,button-map",
  .length = 3*4,
  .value = tis_buttons,
  .next = &tis_prop_cfg_data0
};

uint32_t reset_gpios[]  = { 16 };

struct property tis_prop_reset_gpios =
{ .name = "reset-gpios",
  .length = sizeof(reset_gpios),
  .value = reset_gpios,
  .next = &tis_prop_button
};

uint32_t display_coords[4]  = { 666 };

struct property tis_prop_display_coords =
{ .name = "goodix,display-coords",
  .length = 4*4,
  .value = display_coords,
  .next = &tis_prop_reset_gpios
};

uint32_t panel_coords[4]  = { 666 };

struct property tis_prop_panel_coords =
{ .name = "goodix,panel-coords",
  .length = 4*4,
  .value = panel_coords,
  .next = &tis_prop_display_coords
};

/* 2. Hardware level stub */

/* Goodix Hardware simulation inspired from the datasheet for GT915.
   We simulate:
   - a fixed configuration
   - any user events on the device
   We ignore the GUP_REG_FW_MSG (0x41E4) register because we could not
   find any documentation about it.
*/

/* Only one 16 bits register defines the state of the hardware.
   It contains the address of the HW register that needs to be read when
   a I2C read is requested.
   It is written whenever the I2C bus is sending a write command containing
   at least 2 bytes.
*/
uint16_t tis_goodix_register = 0;

int tis_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    for (int message_nb=0 ; message_nb < num ; ++message_nb) {
        struct i2c_msg message = msgs[message_nb];
        printf("TIS_Flags %d: %x ", message_nb, (unsigned int)message.flags);
        if (message.flags & I2C_M_RD) {
            switch (tis_goodix_register) {
            case 0x8047: // Cf. GT915 Doc page 11 Section 6.2.b
                printf("TIS_Config_Version %x", (unsigned)tis_goodix_register);
                message.buf[0] = 42;
                break;
            case 0x8144: // Cf. GT915 Doc page 16 Section 6.2.c
                printf("TIS_Firmware_Version %x", (unsigned)tis_goodix_register);
                message.buf[0] = 0x1;
                message.buf[1] = 0x2;
                break;
            case 0x8140: // Cf. GT915 Doc page 16 Section 6.2.c
                printf("TIS_Firmware_Version %x", (unsigned)tis_goodix_register);
                message.buf[0] = '9';
                message.buf[1] = '1';
                message.buf[2] = '5';
                message.buf[3] = '\0';
                break;
            case 0x814e: // Cf. GT915 Doc page 17 Section 6.2.c
                printf("TIS_get_hw_point_1 %x", (unsigned)tis_goodix_register);
                tis_make_unknown(message.buf, message.len);
                break;
            case 0x8158: // Cf. GT915 Doc page 17 Section 6.2.c
                printf("TIS_get_hw_point_2 %x", (unsigned)tis_goodix_register);
                tis_make_unknown(message.buf, message.len);
                break;
            case 0x4180: // SS51 dsp ?
                printf("SS51 DSP %x", (unsigned)tis_goodix_register);
                message.buf[0] = 0x0C; // Wild guess to enforce success.
                break;
            default:
                printf("TIS_Unknown_Register %x",
                       (unsigned)tis_goodix_register);
            }
        } else {
            if (message.len >= 2) {
                tis_goodix_register = (message.buf[0] << 8U) + message.buf[1];
                printf("TIS_set_hw_register: %x ",
                       (unsigned int)tis_goodix_register);
                if (message.len > 2)
                    printf("TIS_writing %d other bytes", message.len - 2);
            } else {
                printf("TIS_Writing command too small %d", message.len);
            }
        }
    }
    return num;
}

u32 tis_functionality(struct i2c_adapter *adapt) {
    return 1;
}

//@ assigns ((char*)dest)[0 .. n-1] \from ((char*)src)[0.. n-1];
void *__builtin_memcpy(void *dest, const void *src, size_t n);

//@ assigns ((char*)dest)[0 .. n-1] \from ((char*)src)[0..];
void *tis_memcpy(void *dest, const void *src, size_t n);

unsigned long __copy_from_user(
    void *to,
    void const __attribute__((__noderef__, __address_space__(1))) *from,
    unsigned long n)
{
    tis_memcpy(to, from, n);
    return 0;
}

ssize_t simple_read_from_buffer(
    void __user *to,
    size_t count,
    loff_t *ppos,
    const void *from,
    size_t available)
{
    /*@ assert *ppos == 0 ; */
    ssize_t transmit = (count > available) ? available : count;
    tis_memcpy(to, from, transmit);
    return transmit;
}

/* 3. Main function */

/* Definition of the basic data structures used by the Kernel to support
   the hardware/ */
struct goodix_ts_data tis_goodix_ts_data = { .abs_x_max = 4096 };

struct device_node tis_dev_node = {
    .properties = &tis_prop_panel_coords,
    .data = &tis_goodix_ts_data
};

struct i2c_algorithm tis_algo = {
    .functionality = &tis_functionality,
    .master_xfer = &tis_master_xfer,
    .smbus_xfer = 0
};

struct i2c_adapter tis_adapter =
{ .owner = 0,
  .algo = &tis_algo,
  .name = "TIS Adapter"
};

struct i2c_client tis_client =
{ .flags = 0,
  .addr = 0,
  .name = "tis_client",
  .adapter = &tis_adapter,
  .dev = { .of_node = &tis_dev_node,
           .driver_data = 0
    },
  .irq = 0,
  .detected = { 0 }
};

struct tis_cmd_head_header {
	u8  wr;		/* write read flag 0:R 1:W 2:PID 3: */
	u8  flag;	/* 0:no need flag/int 1: need flag  2:need int */
	u8 flag_addr[2];/* flag address */
	u8  flag_val;	/* flag val */
	u8  flag_relation; /* flag_val:flag 0:not equal 1:equal 2:> 3:< */
	u16 circle;	/* polling cycle */
	u8  times;	/* polling times */
	u8  retry;	/* I2C retry times */
	u16 delay;	/* delay before read or after write */
	u16 data_len;	/* data length */
	u8  addr_len;	/* address length */
	u8  addr[2];	/* address */
	u8  res[3];	/* reserved */
	u8  *data;	/* data pointer */
} __packed;

int tis_proc_read(){
    static struct file tis_file_to_read;
    char tis_buf_for_user_read[1000];
    size_t tis_count = 1000;
    loff_t tis_ppos = 0;
    int result;
    result = (*(tis_proc_fops.read))(&tis_file_to_read,
                                     tis_buf_for_user_read,
                                     tis_count,
                                     &tis_ppos);
    return result;
}

struct regulator *regulator_get(struct device *dev,
	                            const char *id)
{
	return NULL;
}

struct dentry *debugfs_create_dir(const char *name, struct dentry *parent)
{
    static struct dentry tis_dentry;
    return &tis_dentry;
}

struct dentry *debugfs_create_file(const char *name, umode_t mode,
				   struct dentry *parent, void *data,
				   const struct file_operations *fops)
{
    static struct dentry tis_dentry_file;
    return &tis_dentry_file;
}

int fb_register_client(struct notifier_block *nb)
{
    return 0;
}

int sysfs_create_group(struct kobject *kobj,
                       const struct attribute_group *grp) {
    return 0;
}

#include <strings.h>
#define TIS_WRITE_BUFFER_SIZE 4096
#define TIS_WRITE_SIZE 500

int tis_proc_write() {
    struct file tis_file_to_write;
    char tis_buf_user_order[TIS_WRITE_BUFFER_SIZE];
    size_t tis_count = TIS_WRITE_SIZE;
    loff_t tis_ppos = 0;
    int result;
    // Fill-up the zero-initialized buffer with tis_count arbitrary bytes.
    tis_make_unknown(tis_buf_user_order, TIS_WRITE_BUFFER_SIZE);
#ifdef TIS_WR
    ((struct tis_cmd_head_header *)tis_buf_user_order)->wr = TIS_WR;
#endif
#ifdef TIS_FLAG
    ((struct tis_cmd_head_header *)tis_buf_user_order)->flag = TIS_FLAG;
#endif
    ((struct tis_cmd_head_header *)tis_buf_user_order)->retry = 2;
#ifdef TIS_ADDR_LEN
    ((struct tis_cmd_head_header *)tis_buf_user_order)->addr_len = TIS_ADDR_LEN;
#endif
#ifdef TIS_DATA_LEN
    ((struct tis_cmd_head_header *)tis_buf_user_order)->data_len = TIS_DATA_LEN;
#endif
    ((struct tis_cmd_head_header *)tis_buf_user_order)->res[0] = 13;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->times = 1;
    if (tis_proc_fops.write)
        result = (*(tis_proc_fops.write))(&tis_file_to_write,
                                         tis_buf_user_order,
                                         tis_count,
                                         &tis_ppos);
    else
        result = -1;
    return result;
}

/* This is the entry point for the analysis */
int main() {
    /* Force this initialization of the driver */
    int result = goodix_ts_init();
    /* Probe the hardware */
    int probe_ok = TIS_I2C_DRIVER.probe(&tis_client, 0) ;
    if (probe_ok){
        printf("[TIS modprobe failed with %d\n", probe_ok);
        return probe_ok;
    }

#ifdef TIS_SCENARIO_IRQ
    /* Scenario 1: infinite loop calling the interrupt handlers. */
    while(1)
    {
        for (unsigned int irq = 0 ; irq < TIS_MAX_IRQ; ++irq)
            if (tis_irqs_handlers[irq])
                (*tis_irqs_handlers[irq])(irq, tis_irq_args[irq]);
    }
#endif /* TIS_SCENARIO_IRQ */

#ifdef TIS_SCENARIO_PROC
    /* Scenario 2: Try writing and reading the proc file. */
    tis_proc_write();
    tis_proc_read();
#endif /* TIS_SCENARIO_PROC */

    return result;
}
