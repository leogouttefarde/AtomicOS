


# A mettre sur clé USB via le logiciel Windows Rufus
all:
	grub2-mkrescue -o AtomicOS.iso files
	cp -f AtomicOS.iso ~/Bureau

# Ne marche pas étrangement, utiliser le logiciel Windows Rufus
# usb:
# 	sudo dd if=AtomicOS.iso of=/dev/sdc1 && sync


# Ne marche pas
#sudo mkfs.vfat -F 32 -n AtomicOS -I /dev/sdc1
#sudo grub2-install --root-directory=/run/media/leo/AtomicOS --no-floppy --recheck --force /dev/sdc1 --target=i386-pc


# Ne marche pas
# mke2fs /dev/sdc1
# mount -t ext2 /dev/sdc1 /mnt
# mkdir /mnt/boot
# grub2-install --boot-directory=/mnt/boot /dev/sdc1 --target=i386-pc
# umount /mnt


