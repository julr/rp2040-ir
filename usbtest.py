try:
    from winusbcdc import *
except:
    print('winusbcdc is missing, install with "pip install winusbcdc"')
    exit()

DEVICE_VID = 0xF055
DEVICE_PID = 0xB195

api = WinUsbPy()
if api.list_usb_devices(deviceinterface=True, present=True):
    if api.init_winusb_device("IR receiver", DEVICE_VID, DEVICE_PID):
        pipeInfo = api.query_pipe(0)
        write_pipe = pipeInfo.pipe_id & 0x7F
        read_pipe = write_pipe | 0x80

        api.set_timeout(write_pipe, 0.1)
        api.set_timeout(read_pipe, 0.1)
        api.flush(read_pipe)

        print('Device opened, waiting for packets...')
        while True:
            ret = api.read(read_pipe, 64)
            if ret:
                if ret.raw[0] == 0x00:
                    print(f'Got IR signal: address: 0x{ret.raw[1]:02X}, command: 0x{ret.raw[2]:02X}, repeating: {ret.raw[3] == 1}')
                else:
                    print(f'Got unknown packet: {ret.raw}')
    else:
        print('Unable to open device')
else:
    print('No matching device found')