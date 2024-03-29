DESCRIPTION:

this is a wip reverse engineer'd implementation of the sx modchip firmware.  
you have to either extract the bct's and payloads from the sx firmware or provide your own ones.


DISCLAIMER:

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


REQUIREMENTS:

- devkitPro with devkitARM(https://devkitpro.org/wiki/Getting_Started)
- python to run gw_flash.py


OPTIONAL STEPS (in case you want to use the sx bct/payload):

- download the sx core/lite firmware v1.3
- download gw_flash.py(https://gist.github.com/SciresM/cd40c3d1b5dfafbf1fe8f7e5cf13f91e)
- decrypt the firmware using "gw_flash.py -d encrypted_firmware_name.bin decrypted_firmware_name.bin"
- extract the erista bct from the decrypted firmware at file offset 0x4220 with size 0x2800 (00 02 00 00 0F 0E 00 00...)
- extract the mariko bct from the decrypted firmware at file offset 0x6A20 with size 0x2800 (00 08 00 00 00 00 00 00...)
- extract the stage 0 payload from the decrypted firmware at file offset 0xCAA0 with size 0x5C0 (00 00 00 00 00 00 00 00...)
- extract the stage 1 payload from the decrypted firmware at file offset 0x9220 with size 0x3880 (09 00 00 EA 00 00 03 40...)
- fill them into firmware\include\bct.h
- the firmware part should now compile


TODO:

- figure out correct names for spi data interaction functions
- figure out correct name for the following error codes
  - 0xBAD0000B		(fpga related, returned by spi0_read_status/sub_8005A94)
  - 0xBAD0000F		(fpga related, returned by spi0_read_status/sub_8005A94)
  - 0xBAD00010		(fpga related, returned by spi0_read_status/sub_8005A94)
  - 0xBAD00011		(fpga related, returned by spi0_read_status/sub_8005A94)
- figure out correct name for the following usb commands
  - 0xFACE003D
