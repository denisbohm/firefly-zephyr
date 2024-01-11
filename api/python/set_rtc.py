from rpc import Transport
from rpc import RPC
from rpc import SerialChannel

import rtc_pb2
from firefly_rtc_v1_rtc_pb2 import RTC

from datetime import datetime
from datetime import timezone

import sys


class Main:

    def __init__(self):
        self.thread = None
        self.transport = Transport()
        self.rpc = RPC(self.transport)
        self.channel = None

    def get_utc(self):
        return datetime.now(timezone.utc).timestamp()

    def get_utc_offset(self):
        now = datetime.now()
        delta = now.replace(tzinfo=timezone.utc) - now.astimezone(timezone.utc)
        return int(delta.total_seconds())

    def run(self):
        self.channel = SerialChannel(self.transport)
        self.channel.run_client()

        utc_offset = self.get_utc_offset()
        request = rtc_pb2.SetConfigurationRequest()
        request.configuration.time_zone_offset = utc_offset
        request.configuration.display_format = rtc_pb2.Use12HourClock
        RTC.set_configuration(request)

        time = self.get_utc()
        request = rtc_pb2.SetTimeRequest()
        request.utc = int(time)
        request.us = int((time - request.utc) * 1e6)
        print(f"Set UTC Time: {request.utc} {request.us}")
        RTC.set_time(request)

        self.channel.close()


if __name__ == "__main__":
    main = Main()
    main.run()
    sys.exit(0)
