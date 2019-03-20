/**
 * A trigger that causes the PC speaker to continuously beep
 *
 * Copyright (c) 2019 voidstarzero
 * Copyright (c) 2002 Vojtech Pavlik
 * Copyright (c) 1992 Orest Zborowski
 */

/**
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/i8253.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("voidstarzero <46990705+voidstarzero@users.noreply.github.com>");
MODULE_DESCRIPTION("A trigger that beeps continuously");
MODULE_LICENSE("GPL");

static void do_start_scream(int freq)
{
    unsigned long flags;
    unsigned int count = PIT_TICK_RATE / freq;

    raw_spin_lock_irqsave(&i8253_lock, flags);

    outb_p(0xb6, 0x43);
    outb_p(count & 0xff, 0x42);
    outb((count >> 8) & 0xff, 0x42);

    outb_p(inb_p(0x61) | 3, 0x61);

    raw_spin_unlock_irqrestore(&i8253_lock, flags);
}

static void do_stop_scream(void)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&i8253_lock, flags);

    outb_p(inb_p(0x61) & 0xfc, 0x61);

    raw_spin_unlock_irqrestore(&i8253_lock, flags);
}

#define FREQ_BUFSIZE 7

static ssize_t scream_start(struct file* file, const char __user* ubuf,
                            size_t count, loff_t* ppos)
{
    int freq;
    char freq_buf[FREQ_BUFSIZE + 1] = {0};

    if (*ppos > 0 || count > FREQ_BUFSIZE) {
        return -EFAULT;
    }
    if (copy_from_user(freq_buf, ubuf, count)) {
        return -EFAULT;
    }

    if (kstrtoint(freq_buf, 10, &freq)) {
        return -EINVAL;
    }
    if (freq < 20 || freq > 32767) {
        freq = 1760;
    }

    do_start_scream(freq);

    *ppos = strlen(freq_buf);
    return *ppos;
}

static struct file_operations scream_ops = {
    .owner = THIS_MODULE,
    .write = scream_start,
};

static struct proc_dir_entry* proc_ent;

static int scream_init(void)
{
    printk(KERN_INFO "Loading module Aargh!...\n");

    proc_ent = proc_create("scream", 0200, NULL, &scream_ops);
    if (!proc_ent) {
        return -ENOMEM;
    }

    printk(KERN_INFO "Module Aargh! loaded\n");
    return 0;
}

static void scream_exit(void)
{
    printk(KERN_INFO "Removing module Aargh!...\n");

    do_stop_scream();
    proc_remove(proc_ent);

    printk(KERN_INFO "Module Aargh! removed\n");
}

module_init(scream_init);
module_exit(scream_exit);
