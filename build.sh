export ARCH=arm64  
export PATH=$PATH:/root/aarch64-linux-android-4.9/bin  
export CROSS_COMPILE=aarch64-linux-android-

mkdir out
make ARCH=arm64 O=out merge_kirin980_defconfig
make ARCH=arm64 O=out -j$(nproc --all) 2>&1 | tee build.log

echo 复制Image.gz
cp out/arch/arm64/boot/Image.gz  tools/Image.gz

tools/mkbootimg --kernel tools/Image.gz --base 0x0 --cmdline "loglevel=4 initcall_debug=n page_tracker=on ssbd=kernel printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=enforcing unmovable_isolate1=2:192M,3:224M,4:256M buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2019-03-01  --output Kirin980_EMUI9.1_KSU_Enforcing.img

tools/mkbootimg --kernel tools/Image.gz --base 0x0 --cmdline "loglevel=4 initcall_debug=n page_tracker=on ssbd=kernel printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=permissive unmovable_isolate1=2:192M,3:224M,4:256M buildvariant=user" --tags_offset 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --header_version 1 --os_version 9 --os_patch_level 2019-03-01  --output Kirin980_EMUI9.1_KSU_Permissive.img
