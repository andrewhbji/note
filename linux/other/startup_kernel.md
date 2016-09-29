https://github.com/notro/rpi-firmware/wiki/start_kernel

```
start_kernel
|
|--> smp_setup_processor_id
|    |
|    `--> printk(KERN_INFO "Booting Linux on physical CPU 0x%x\n", mpidr);
|
|--> cgroup_init_early
|    |
|    `--> printk(KERN_ERR "cgroup: Subsys %s id == %d\n",
|
|--> pr_notice("%s", linux_banner);
|
|--> setup_arch
|    |
|    |--> setup_processor
|    |    |
|    |    `--> cpu_init
|    |    |
|    |    `--> printk("CPU: %s [%08x] revision %d (ARMv%s), cr=%08lx\n", cpu_name, read_cpuid_id(), read_cpuid_id() & 15,proc_arch[cpu_architecture()], cr_alignment);
|    |
|    |--> setup_machine_fdt
|    |    |
|    |    `--> pr_info("Machine: %s, model: %s\n", mdesc_best->name, model);
|    |
|    |--> arm_memblock_init
|    |    |
|    |    |--> arm_mm_memblock_reserve
|    |    |
|    |    |--> arm_dt_memblock_reserve
|    |    |
|    |    `--> mdesc->reserve
|    |
|    |--> paging_init
|    |    |
|    |    `--> devicemaps_init
|    |         |
|    |         `--> mdesc->map_io
|    |
|    `--> mdesc->init_early
|
|--> pr_notice("Kernel command line: %s\n", boot_command_line);
|
|--> init_IRQ
|    |
|    `--> machine_desc->init_irq
|
|--> time_init
|    |
|    `--> machine_desc->init_time
|
|--> console_init
|    |
|    `--> console_initcall's
|         :
|         `--> con_init
|              |
|              `--> pr_info("Console: %s %s %dx%d\n", ...);
|                   Console: colour dummy device 80x30
|
`--> rest_init
     |
     `--> kernel_init
          |
          |--> kernel_init_freeable
          |    |
          |    |--> do_basic_setup
          |    |    |
          |    |    `--> do_initcalls
          |    |         :
          |    |         :--> early init calls
          |    |         :--> core init calls
          |    |         :--> post core init calls
          |    |         :--> arch init calls
          |    |         :    :
          |    |         :    `--> customize_machine
          |    |         :         |
          |    |         :         `--> machine_desc->init_machine
          |    |         :
          |    |         :--> subsys init calls
          |    |         :--> fs init calls
          |    |         :--> device init calls
          |    |         :    :
          |    |         :    `--> module_init() entries
          |    |         :         drivers are probed during driver registration
          |    |         :
          |    |         `--> late init calls
          |    |         :         |
          |    |         :         `--> machine_desc->init_late        
          |    |         :
          |    |         :--> smp_init
          |    |         :         |
          |    |         :         `--> cpu_up
          |    |         :                  |
          |    |         :                  `--> _cpu_up
          |    |         :                          |
          |    |         :                          `--> __cpu_up 
          |    |         :                                      |                      
          |    |         :                                      `--> boot_secondary
          |    |         :                                                  |
          |    |         :                                                  `-->smp_ops.smp_boot_secondary 
          |    |         :                                                                  |
          |    |         :                                                                  `-->secondary_startup 
          |    |         :                                                                              |
          |    |         :                                                                              `-->__secondary_switched 
          |    |         :                                                                                          |
          |    |         :                                                                                          `-->secondary_start_kernel
          |    |         :                                                                                                        |  
          |    |         :                                                                                                        `-->cpu_init        
          |    |
          |    |--> prepare_namespace
          |    |    |
          |    |    `--> mount_root
          |    |         |
          |    |         `--> mount_block_root
          |    |              |
          |    |              `--> do_mount_root
          |    |                   |
          |    |                   `--> printk(KERN_INFO "VFS: Mounted root (%s filesystem)%s on device %u:%u.\n", ...);
          |    |
          |    `--> load_default_modules
          |
          |--> free_initmem
          |    |
          |    `--> free_initmem_default
          |         |
      |         `--> free_reserved_areafree_reserved_area
          |              |
      |              `--> printk: Freeing unused kernel memory: xxxK
          |
          `--> run_init_process
```