import rtc_pb2

from rpc import RPC


class RTC:

    rtc_get_time = RPC.Method(10, 1, 1, rtc_pb2.GetTimeRequest, rtc_pb2.GetTimeResponse, False, False, 0)

    @staticmethod
    def get_time(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(RTC.rtc_get_time, request, channel, rpc, timeout)

    rtc_set_time = RPC.Method(10, 1, 2, rtc_pb2.SetTimeRequest, rtc_pb2.SetTimeResponse, False, False, 0)

    @staticmethod
    def set_time(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(RTC.rtc_set_time, request, channel, rpc, timeout)

    rtc_get_configuration = RPC.Method(10, 1, 3, rtc_pb2.GetConfigurationRequest, rtc_pb2.GetConfigurationResponse, False, False, 0)

    @staticmethod
    def get_configuration(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(RTC.rtc_get_configuration, request, channel, rpc, timeout)

    rtc_set_configuration = RPC.Method(10, 1, 4, rtc_pb2.SetConfigurationRequest, rtc_pb2.SetConfigurationResponse, False, False, 0)

    @staticmethod
    def set_configuration(request, channel=None, rpc=None, timeout=None):
        return RPC.unary_call(RTC.rtc_set_configuration, request, channel, rpc, timeout)
