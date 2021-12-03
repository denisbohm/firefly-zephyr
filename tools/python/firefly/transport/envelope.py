class Envelope:

    system_firefly = 0

    subsystem_system = 0
    subsystem_i2cm   = 1
    subsystem_spim   = 2

    type_event    = 0
    type_request  = 1
    type_response = 2

    def __init__(self, crc16=0, length=0, target=0, source=0, system=0, subsystem=0, type=0, reserved0=0):
        self.crc16 = crc16
        self.length = length
        self.target = target
        self.source = source
        self.system = system
        self.subsystem = subsystem
        self.type = type
        self.reserved0 = reserved0
