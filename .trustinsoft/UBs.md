# UBs

## Fixed UBs

2 UBs that were fixed along the way. Can be seen in the intermediary commits.

### FixedUB#1

```bash
tis_driver.c:517:[kernel] warning: Function pointer must point to function with compatible type.
                  Pointer type: ssize_t (struct file *,
                                         char __attribute__((__noderef__, __address_space__(1))) *,
                                         size_t, loff_t *)
                  Non-function: {0}
                  assert \valid_function(tis_proc_fops.read);
                  stack: tis_proc_read :: tis_driver.c:612 <- main
```

This one happens in all the tests for Scenario 2.

The root cause for this UB is that in file `drivers/input/touchscreen/gt9xx/gt9xx.c` in function `goodix_ts_probe` we call `init_wr_node` without checking if it succeeds or fails. And if it fails then not everything is properly allocated and initialized which indirectly leads to this UB in `tis_proc_read`.

### FixedUB#2

```bash
drivers/input/touchscreen/gt9xx/goodix_tool.c:176:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:176 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:176:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:176 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:177:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:177 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:177:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:177 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:178:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:178 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:178:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:178 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:179:[kernel] warning: may not point to a valid string:
                  assert \points_to_valid_string((void *)((s8 *)ic_type));
                  stack: strcmp :: drivers/input/touchscreen/gt9xx/goodix_tool.c:179 <-
                         register_i2c_func :: drivers/input/touchscreen/gt9xx/goodix_tool.c:403 <-
                         goodix_tool_write :: tis_driver.c:579 <-
                         tis_proc_write :: tis_driver.c:611 <-
                         main
```

This one happens in the last test for Scenario 2 - "Scenario 2 (proc) - UB#3".

The reason is that the content of the variable `ic_type` comes from outside, so we cannot guarantee that it contains a valid null-terminated string, and when it is used as argument in `strcmp` it causes an UB. The fix is to replace the calls to `strcmp` with calls to `strncmp` using appropriate size in each case.

## Remaining UBs

UBs that remain in the last commit.

### No UB

For comparison - the executions with these contents of `tis_buf_user_order` are correct - no UB is triggered.

In `tis_proc_write`:

```C
    tis_make_unknown(tis_buf_user_order, TIS_WRITE_BUFFER_SIZE);
    ((struct tis_cmd_head_header *)tis_buf_user_order)->wr = 0;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->flag = 0;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->retry = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->addr_len = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->data_len = 256;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->res[0] = 13;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->times = 1;
```

### UB#1

In `tis_proc_write`:

```C
    tis_make_unknown(tis_buf_user_order, TIS_WRITE_BUFFER_SIZE);
    ((struct tis_cmd_head_header *)tis_buf_user_order)->wr = 0;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->flag = 0;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->retry = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->addr_len = 10; // I'm equal 10!
    ((struct tis_cmd_head_header *)tis_buf_user_order)->data_len = 256;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->res[0] = 13;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->times = 1;
```

Result:

```bash
drivers/input/touchscreen/gt9xx/goodix_tool.c:522:[kernel] warning: out of bounds read.
                  assert
                  \valid_read((u8 *)cmd_head.addr+(0 .. (unsigned int)cmd_head.addr_len-1));
                  stack: memcpy :: drivers/input/touchscreen/gt9xx/goodix_tool.c:522 <-
                         goodix_tool_read :: tis_driver.c:517 <-
                         tis_proc_read :: tis_driver.c:605 <-
                         main
```

Seems to be a real UB.

### UB#2

In `tis_proc_write`:

```C
    tis_make_unknown(tis_buf_user_order, TIS_WRITE_BUFFER_SIZE);
    ((struct tis_cmd_head_header *)tis_buf_user_order)->wr = 0;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->flag = 1; // I'm equal 1!
    ((struct tis_cmd_head_header *)tis_buf_user_order)->retry = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->addr_len = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->data_len = 256;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->res[0] = 13;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->times = 1;
```

Result:

```bash
drivers/input/touchscreen/gt9xx/goodix_tool.c:279:[kernel] warning: accessing uninitialized left-value: assert \initialized(&buf[2]);
                  stack: comfirm :: drivers/input/touchscreen/gt9xx/goodix_tool.c:513 <-
                         goodix_tool_read :: tis_driver.c:517 <-
                         tis_proc_read :: tis_driver.c:605 <-
                         main
drivers/input/touchscreen/gt9xx/goodix_tool.c:279:[value] completely invalid value in evaluation of
        argument buf[2]
```

I did not investigate this one.

### False Positive #1

In `tis_proc_write`:

```C
    tis_make_unknown(tis_buf_user_order, TIS_WRITE_BUFFER_SIZE);
    ((struct tis_cmd_head_header *)tis_buf_user_order)->wr = 0;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->flag = 1;  // I'm equal 1!
    ((struct tis_cmd_head_header *)tis_buf_user_order)->retry = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->addr_len = tis_interval(0,2); // I'm equal 0, 1, or 2!
    ((struct tis_cmd_head_header *)tis_buf_user_order)->data_len = 256;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->res[0] = 13;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->times = 1;
```

With `tis_interval` we get an UB:

```bash
tis_driver.c:423:[kernel] warning: accessing uninitialized left-value: assert \initialized(message.buf+0);
                  stack: tis_master_xfer :: tis_driver.c:238 <-
                         i2c_transfer :: drivers/input/touchscreen/gt9xx/goodix_tool.c:110 <-
                         tool_i2c_read_no_extra :: drivers/input/touchscreen/gt9xx/goodix_tool.c:155 <-
                         tool_i2c_read_with_extra :: drivers/input/touchscreen/gt9xx/goodix_tool.c:275 <-
                         comfirm :: drivers/input/touchscreen/gt9xx/goodix_tool.c:513 <-
                         goodix_tool_read :: tis_driver.c:517 <-
                         tis_proc_read :: tis_driver.c:605 <-
                         main
```

But if we change that to `tis_interval_split` it disappears, we get UB#2 instead:

```bash
drivers/input/touchscreen/gt9xx/goodix_tool.c:279:[kernel] warning: accessing uninitialized left-value: assert \initialized(&buf[2]);
                  stack: comfirm :: drivers/input/touchscreen/gt9xx/goodix_tool.c:513 <-
                         goodix_tool_read :: tis_driver.c:517 <-
                         tis_proc_read :: tis_driver.c:605 <-
                         main
```

### UB#3 - or probably - False Positive #2

In `tis_proc_write`:

```C
    tis_make_unknown(tis_buf_user_order, TIS_WRITE_BUFFER_SIZE);
    ((struct tis_cmd_head_header *)tis_buf_user_order)->flag = 0
    ((struct tis_cmd_head_header *)tis_buf_user_order)->retry = 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->addr_len 2;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->res[0] = 13;
    ((struct tis_cmd_head_header *)tis_buf_user_order)->times = 1;
```

Result:

```bash
tis_driver.c:461:[kernel] warning: out of bounds read.
                  assert \valid_read((char *)from+(0 .. (unsigned int)transmit-1));
                  stack: tis_memcpy :: tis_driver.c:461 <-
                         simple_read_from_buffer :: drivers/input/touchscreen/gt9xx/goodix_tool.c:563 <-
                         goodix_tool_read :: tis_driver.c:517 <-
                         tis_proc_read :: tis_driver.c:612 <-
                         main
```

This seems to be a false positive caused by a precision loss and merging of states.

The allocated buffer (pointed by the variable `from`) can have different lengths, but its length matches correctly the value of the variable `available` (which gets its value from `data_len`) in every execution, so everything is actually fine here. However, these values get merged and the Analyzer does not know that they match correctly.

This patch can be applied in order to separate correctly the intervals of lengths (stored in `data_len`):

```C
+++ b/drivers/input/touchscreen/gt9xx/goodix_tool.c
@@ -530,7 +530,6 @@ static ssize_t goodix_tool_read(struct file *file, char __user *user_buf,
                        msleep(cmd_head.delay);
 
                data_len = cmd_head.data_len;
-               //@ assert data_len <= 512 || 512 < data_len <= 1024 || 1024 < data_len <= 1536 || 1536 < data_len <= 2048 || 2048 < data_len;
                if (data_len <= 0 || (data_len > data_length)) {
                        dev_err(&gt_client->dev, "Invalid data length %d\n",
                                data_len);
```

I don't know how to separate correctly the different allocated buffers, so that the Analyzer manages to find that they match with the lengths.
