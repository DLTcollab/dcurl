# Execute dcurl remote worker on the [DE10-Nano](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=1046) board
## Write the image file to the micro SD card
Prepare a micro SD card reader for writing the image file.

The command for writing the image file to the micro SD card:
### Linux
```bash
$ sudo dd if=[image-file] of=/dev/sd[x] bs=1M status=progress
```
**Important**:\
Please make sure to fill the correct **[x]** value in `/dev/sd[x]`.\
It must map to the micro SD card. Otherwise, your hard disk will be broken.\
You can check it with the command `$ sudo fdisk -l`.

### macOS
```bash
$ sudo dd if=[image-file] of=/dev/mmcblk[x] bs=1M status=progress
```
**Important**:\
Please make sure to fill the correct **[x]** value in `/dev/mmcblk[x]`.


## Hardware manual setting
Change the FPGA configuration mode switch as shown in the image\
<img src="https://forum.uvm.io/uploads/default/original/1X/e09be635519a0af23ad88baed8ef99d06941eca4.png" width="600">


## Connect with the DE10-Nano board
### Hardware
Please make sure the following list are well set or connected:
- 5V DC Power Jack
- UART to USB (USB Mini-B)
- HPS Gigabit Ethernet
- MicroSD Card Socket
<img src="https://software.intel.com/sites/default/files/did_feeds_images/76220cfd-83d7-49b0-a8c3-be8a2728a6e6/76220cfd-83d7-49b0-a8c3-be8a2728a6e6-imageId=7376634d-da28-4a76-bf64-7c97648f4fe6.jpg" width="600">
<img src="https://software.intel.com/sites/default/files/did_feeds_images/76220cfd-83d7-49b0-a8c3-be8a2728a6e6/76220cfd-83d7-49b0-a8c3-be8a2728a6e6-imageId=5fba4448-b09b-4aa4-afaa-99572005b99e.jpg" width="600">

### Minicom
Use **minicom** to connect with the **DE10-Nano** board.
#### Install minicom
##### Linux
```bash
$ sudo apt install minicom
```
##### macOS
```bash
$ sudo brew install minicom
```

#### Configure minicom
```bash
$ sudo minicom -s
```
Select **Serial port setup** and configure as the following
##### Linux
```
┌─────────────────────────────────────────────────────────────────┐
│ A -    Serial Device      : /dev/ttyUSB0                        │
│ B - Lockfile Location     : /var/lock                           │
│ C -   Callin Program      :                                     │
│ D -  Callout Program      :                                     │
│ E -    Bps/Par/Bits       : 115200 8N1                          │
│ F - Hardware Flow Control : No                                  │
│ G - Software Flow Control : No                                  │
│                                                                 │
│    Change which setting?                                        │
└─────────────────────────────────────────────────────────────────┘
```

##### macOS
```
┌─────────────────────────────────────────────────────────────────┐
│ A -    Serial Device      : /dev/tty.usbserial-A107T0EA         │
│ B - Lockfile Location     : /usr/local/Cellar/minicom/2.7.1/var │
│ C -   Callin Program      :                                     │
│ D -  Callout Program      :                                     │
│ E -    Bps/Par/Bits       : 115200 8N1                          │
│ F - Hardware Flow Control : No                                  │
│ G - Software Flow Control : No                                  │
│                                                                 │
│    Change which setting?                                        │
└─────────────────────────────────────────────────────────────────┘
```

If you want to change the value of `Serial Device`, press **Ctrl-A**.

After configuration, select **Save setup as dfl** and select **Exit from Minicom** to exit.

#### Connect with minicom
```bash
$ sudo minicom
```
Then enter the user account and the password to login.

### MAC address setting (optional)
For monitoring or using [Ansible Playbooks](https://docs.ansible.com/ansible/latest/user_guide/playbooks.html) on remote workers,
the IP address of them should be fixed.
The router can assign the IP address by detecting the hardware MAC address.
However, each time the **DE10-Nano** board is rebooted, the MAC address would be different.

The steps to set the MAC address:
- Open the file `/etc/network/interfaces`
- Add the following text
  ```
  auto eth0
  iface eth0 inet dhcp
  hwaddress ether xx:xx:xx:xx:xx:xx
  ```
  xx:xx:xx:xx:xx:xx is the assigned MAC address
- Reboot


## Build and execute the remote worker
### Load the driver
You need to load the driver first for the further execution of the remote worker.\
Otherwise, it will crash.
```bash
$ sudo insmod cpowdrv.ko
```

### Clone the repository
```bash
$ git clone git@github.com:DLTcollab/dcurl.git
```

### Build the remote worker
```bash
$ cd dcurl
$ make BUILD_REMOTE=1 BOARD=de10nano BUILD_FPGA=1
```

### Execute the remote worker
```bash
$ sudo ./build/remote-worker --broker [rabbitmq-broker-hostname]
```
Use **-b** or **--broker** to assign the hostname of the RabbitMQ broker.\
The default value is **localhost**.


## Reference
- [Terasic DE10-Nano Product Brief](https://www.mouser.tw/pdfdocs/de10NanoBrief.pdf)
- [Terasic DE10-Nano Get Started Guide](https://software.intel.com/en-us/terasic-de10-nano-get-started-guide)
- [Linux on ARM - DE10-Nano Kit](https://www.digikey.com/eewiki/display/linuxonarm/DE10-Nano+Kit)
