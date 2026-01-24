import hid
import sys
import time

VENDOR_ID = 0x303A
PRODUCT_ID = 0x1324
REPORT_ID = 0x03
OUT_PAYLOAD = bytes([0x00, 0x03, 0x01, 0x00, 0x07, 0x00, 0x00, 0x05]) + b'hello' + bytes(48 - 8 - 5)



def find_device():
    print("Enumerating HID devices:")
    comms_path = None
    for d in hid.enumerate():
        if d['vendor_id'] == VENDOR_ID and d['product_id'] == PRODUCT_ID:
            print(f"  path={d['path']} interface_number={d.get('interface_number')} usage_page={d.get('usage_page')} usage={d.get('usage')}")
            if d.get('interface_number', 0) == 1:
                comms_path = d['path']
    return comms_path

def main():
    path = find_device()
    if not path:
        print("Device not found.")
        sys.exit(1)

    dev = hid.device()
    dev.open_path(path)
    print(f"Opened device: {path}")
    # Send output report (prepend report ID)
    out_report = bytes([REPORT_ID]) + OUT_PAYLOAD
    print(f"Writing {len(out_report)} bytes: {out_report.hex()}")
    dev.write(out_report)
    time.sleep(0.5)
    # Read input report
    print("Reading input report...")
    in_report = dev.read(49, timeout_ms=500)
    if in_report:
        print(f"Received ({len(in_report)} bytes): {in_report}")
    else:
        print("No response received.")
    dev.close()

if __name__ == "__main__":
    main()
