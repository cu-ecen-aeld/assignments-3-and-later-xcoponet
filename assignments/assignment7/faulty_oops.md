This is the log of faulty driver fail
```
echo "Hello world" > /dev/faulty 
[   57.541183] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[   57.541343] Mem abort info:
[   57.541381]   ESR = 0x0000000096000045
[   57.541420]   EC = 0x25: DABT (current EL), IL = 32 bits
[   57.541485]   SET = 0, FnV = 0
[   57.541524]   EA = 0, S1PTW = 0
[   57.541564]   FSC = 0x05: level 1 translation fault
[   57.541627] Data abort info:
[   57.541668]   ISV = 0, ISS = 0x00000045
[   57.541712]   CM = 0, WnR = 1
[   57.541754] user pgtable: 4k pages, 39-bit VAs, pgdp=0000000043dc7000
[   57.541822] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[   57.541938] Internal error: Oops: 0000000096000045 [#2] PREEMPT SMP
[   57.542166] Modules linked in: scull(O) faulty(O) hello(O)
[   57.542250] CPU: 2 PID: 451 Comm: sh Tainted: G      D    O      5.15.150-yocto-standard #1
[   57.542335] Hardware name: linux,dummy-virt (DT)
[   57.542403] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[   57.542518] pc : faulty_write+0x18/0x20 [faulty]
[   57.542602] lr : vfs_write+0xf8/0x29c
[   57.542670] sp : ffffffc00a19bd80
[   57.542714] x29: ffffffc00a19bd80 x28: ffffff80020c6040 x27: 0000000000000000
[   57.542839] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[   57.542944] x23: 0000000000000000 x22: ffffffc00a19bdc0 x21: 00000055748f6cb0
[   57.543052] x20: ffffff8003730d00 x19: 000000000000000c x18: 0000000000000000
[   57.543152] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[   57.543255] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[   57.543351] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008269fac
[   57.543486] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[   57.543605] x5 : 0000000000000001 x4 : ffffffc000b75000 x3 : ffffffc00a19bdc0
[   57.543721] x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
[   57.543838] Call trace:
[   57.543885]  faulty_write+0x18/0x20 [faulty]
[   57.543950]  ksys_write+0x74/0x10c
[   57.543996]  __arm64_sys_write+0x24/0x30
[   57.544047]  invoke_syscall+0x5c/0x130
[   57.544098]  el0_svc_common.constprop.0+0x4c/0x100
[   57.544164]  do_el0_svc+0x4c/0xb4
[   57.544206]  el0_svc+0x28/0x80
[   57.544255]  el0t_64_sync_handler+0xa4/0x130
[   57.544321]  el0t_64_sync+0x1a0/0x1a4
[   57.544379] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[   57.544538] ---[ end trace 178e03387000deee ]---
Segmentation fault
```

The following lines tell that the crash occured in the funtion "faulty_write"
```
[   57.543838] Call trace:
[   57.543885]  faulty_write+0x18/0x20 [faulty]
```

The line `[   57.541183] Unable to handle kernel NULL pointer dereference at virtual address ` tells the crash is due to operation on a NULL pointer.
