# ESP32 serial port troubleshooting (Linux)

If flashing or monitoring fails with an error such as
`could not open port /dev/ttyACM0: No such file or directory`, the selected
serial device is not currently present. `/dev/ttyACM0` is only an example; the
board may appear under a different name.

1. Connect the board directly to the computer with a known-good **USB data**
   cable. Try another cable or USB port if needed; some USB cables provide
   power only.
2. Check which serial ports exist:

   ```sh
   python -m serial.tools.list_ports -v
   ```

   You can also check the common device names:

   ```sh
   ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
   ```

   Unplug and reconnect the board and run the check again. Use the port that
   appears (for example, `/dev/ttyUSB0`) instead of `/dev/ttyACM0`.
3. Pass that port explicitly to ESP-IDF commands, for example:

   ```sh
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

   Replace `/dev/ttyUSB0` with the actual port found above.
4. If **no port appears** even though flashing works, the monitor's USB serial
   interface may not have its Linux driver attached. Check the USB driver:

   ```sh
   lsusb -t
   sudo modprobe cdc_acm
   ```

   Unplug and reconnect the board, then check for `/dev/ttyACM*` again. If
   `modprobe` reports that the module is missing, reinstall the extra modules
   for the running kernel and reboot:

   ```sh
   sudo apt install --reinstall linux-modules-extra-$(uname -r)
   ```

   If the USB device itself is not listed, or the serial port still does not
   appear, check the cable/connector and watch kernel messages while
   reconnecting:

   ```sh
   dmesg --follow
   ```

   Press `Ctrl+C` to stop. If there are no USB connection messages, try another
   cable/port and check that the board is powered. The board may expose either
   native USB serial or a USB-to-serial bridge, so its device name can differ.
5. If the port exists but opening it reports **Permission denied**, add your
   user to the serial-access group and log out and back in:

   ```sh
   sudo usermod -aG dialout "$USER"
   ```

   Then retry. A missing `/dev/ttyACM0` is usually a device detection or wrong
   port-name issue, not a permissions issue.
6. If the port is present but flashing cannot connect, close other programs
   using the port and try putting the board in download mode: hold **BOOT**,
   tap **RESET/EN**, then release **BOOT**. Retry the flash command.
