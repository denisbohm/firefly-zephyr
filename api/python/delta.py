class BitDecoder:

    def __init__(self, data, bit_count):
        self.data = data
        self.bit_count = bit_count
        self.bit_index = 0

    def read(self, bit_count):
        if (self.bit_index + bit_count) > self.bit_count:
            raise Exception()
        extracted_bit_count = 0
        mask = 0xffffffff >> (32 - bit_count)
        value = 0
        while extracted_bit_count < bit_count:
            index = self.bit_index // 8
            if index >= len(self.data):
                raise Exception()
            byte = self.data[index]
            byte_bit_index = self.bit_index % 8
            byte_bit_count = 8 - byte_bit_index
            remaining_bit_count = bit_count - extracted_bit_count
            if byte_bit_count > remaining_bit_count:
                byte_bit_count = remaining_bit_count
            value |= (mask & (byte >> byte_bit_index)) << extracted_bit_count
            mask = mask >> byte_bit_count
            extracted_bit_count += byte_bit_count
            self.bit_index += byte_bit_count
        return value


class BitEncoder:
    
    def __init__(self):
        self.data = bytearray()
        self.bit_count = 0
        self.byte = 0

    def write(self, value, bit_count):
        if (value >> bit_count) != 0:
            raise Exception()
        bit_index = self.bit_count % 8
        self.byte |= value << bit_index
        bit_index += bit_count
        while bit_index >= 8:
            self.data.append(self.byte & 0xff)
            self.byte >>= 8
            bit_index -= 8
        if self.byte > 256:
            raise Exception()
        self.bit_count += bit_count

    def finalize(self):
        bit_index = self.bit_count % 8
        if bit_index > 0:
            self.data.append(self.byte)
        if len(self.data) != (self.bit_count + 7) // 8:
            raise Exception()


class Delta:

    @staticmethod
    def get_resolution(value):
        return value.bit_length()

    @staticmethod
    def zigzag_encode(value):
        return (value << 1) if value >= 0 else ((-value << 1) - 1)

    @staticmethod
    def zigzag_decode(value):
        return (-value >> 1) if (value & 1) != 0 else (value >> 1)

    @staticmethod
    def get_delta_resolution(values):
        resolution = 0
        previous_value = values[0]
        for i in range(1, len(values)):
            value = values[i]
            delta = value - previous_value
            encoded_delta = Delta.zigzag_encode(delta)
            encoded_delta_resolution = Delta.get_resolution(encoded_delta)
            if encoded_delta_resolution > resolution:
                resolution = encoded_delta_resolution
            previous_value = value
        return resolution

    @staticmethod
    def encode(bit_encoder, values, value_resolution, delta_resolution):
        previous_value = values[0]
        bit_encoder.write(Delta.zigzag_encode(previous_value), value_resolution)
        if delta_resolution == 0:
            return
        for i in range(1, len(values)):
            value = values[i]
            delta = value - previous_value
            encoded_delta = Delta.zigzag_encode(delta)
            bit_encoder.write(encoded_delta, delta_resolution)
            previous_value = value

    @staticmethod
    def decode(bit_decoder, value_resolution, count, delta_resolution):
        values = []
        previous_value = Delta.zigzag_decode(bit_decoder.read(value_resolution))
        values.append(previous_value)
        if delta_resolution == 0:
            for i in range(1, count):
                values.append(previous_value)
            return values
        for i in range(1, count):
            encoded_delta = bit_decoder.read(delta_resolution)
            delta = Delta.zigzag_decode(encoded_delta)
            value = previous_value + delta
            values.append(value)
            previous_value = value
        return values


class SensorParameters:

    def __init__(self, value_resolution, channels):
        self.value_resolution = value_resolution
        self.channels = channels


class SensingDeltaEncoding:

    @staticmethod
    def decode(bit_decoder, delta_encoding_parameters, sensor_parameters_by_sensor):
        dataset = sensing_pb2.Dataset()
        del dataset.data[:]
        sensor_count = bit_decoder.read(delta_encoding_parameters.sensor_count_resolution)
        for _ in range(0, sensor_count):
            sensor = bit_decoder.read(delta_encoding_parameters.sensor_resolution)
            data = sensing_pb2.Data()
            data.sensor = sensor
            del data.channels[:]
            sample_count = bit_decoder.read(delta_encoding_parameters.sample_count_resolution)
            channel_count = len(sensor_parameters_by_sensor[sensor].channels)
            value_resolution = sensor_parameters_by_sensor[sensor].value_resolution
            data.ordinal = bit_decoder.read(delta_encoding_parameters.ordinal_resolution)
            for i in range(0, channel_count):
                delta_resolution = bit_decoder.read(delta_encoding_parameters.delta_resolution_resolution)
                values = Delta.decode(bit_decoder, value_resolution, sample_count, delta_resolution)
                channel = sensing_pb2.Channel()
                del channel.values[:]
                channel.values.extend(values)
                data.channels.extend([channel])
            dataset.data.extend([data])
        return dataset

    @staticmethod
    def encode(bit_encoder, delta_encoding_parameters, sensor_parameters_by_sensor, dataset):
        bit_encoder.write(len(dataset.data), delta_encoding_parameters.sensor_count_resolution)
        for data in dataset.data:
            sensor = data.sensor
            value_resolution = sensor_parameters_by_sensor[sensor].value_resolution
            sample_count = len(data.channels[0].values)
            bit_encoder.write(sensor, delta_encoding_parameters.sensor_resolution)
            bit_encoder.write(sample_count, delta_encoding_parameters.sample_count_resolution)
            bit_encoder.write(data.ordinal, delta_encoding_parameters.ordinal_resolution)
            for channel in data.channels:
                values = channel.values
                delta_resolution = Delta.get_delta_resolution(values)
                bit_encoder.write(delta_resolution, delta_encoding_parameters.delta_resolution_resolution)
                Delta.encode(bit_encoder, values, value_resolution, delta_resolution)

    @staticmethod
    def print_dataset(dataset):
        for data in dataset.data:
            print(f'sensor: {data.sensor}')
            print(f'  ordinal: {data.ordinal}')
            for i in range(0, len(data.channels)):
                print(f'  channel: {i}')
                channel = data.channels[i]
                for value in channel.values:
                    print(f'    {value}')


class DeltaTests:

    @staticmethod
    def test_zigzag():
        # 10 = -2 zz 11
        # 11 = -1 zz 01
        # 00 = 0 zz 00
        # 01 = 1 zz 10
        print("zigzag:")
        value_resolution = 2
        for value in range(-2, 2):
            bit_encoder = BitEncoder()
            zigzag = Delta.zigzag_encode(value)
            if zigzag.bit_length() > value_resolution:
                raise Exception("invalid zigzag representation")
            bit_encoder.write(zigzag, value_resolution)
            bit_encoder.finalize()
            if bit_encoder.bit_count > value_resolution:
                raise Exception("invalid bit encoder representation")
            bit_decoder = BitDecoder(bit_encoder.data, bit_encoder.bit_count)
            result = Delta.zigzag_decode(bit_decoder.read(value_resolution))
            if result != value:
                raise Exception("incorrect zigzag transformation")
            print(f"  {value} -> {zigzag} -> {result}")

    @staticmethod
    def test_dataset():
        sensor_parameters_by_sensor = {
            sensing_pb2.Photoplethysmography: SensorParameters(20, [0])
        }

        delta_encoding_parameters = sensing_pb2.DeltaEncodingParameters()
        delta_encoding_parameters.sensor_count_resolution = 4
        delta_encoding_parameters.sensor_resolution = 4
        delta_encoding_parameters.sample_count_resolution = 8
        delta_encoding_parameters.ordinal_resolution = 32
        delta_encoding_parameters.delta_resolution_resolution = 6

        dataset = sensing_pb2.Dataset()
        del dataset.data[:]
        data = sensing_pb2.Data()
        data.sensor = sensing_pb2.Photoplethysmography
        data.ordinal = 4
        del data.channels[:]
        channel = sensing_pb2.Channel()
        del channel.values[:]
        offset = 0
        noise = 2**18
        for _ in range(0, 64):
            channel.values.extend([random.randrange(0, noise) + offset])
        data.channels.extend([channel])
        dataset.data.extend([data])
        SensingDeltaEncoding.print_dataset(dataset)

        protobuf_bytes = dataset.SerializeToString()
        print(f"protobuf byte count: {len(protobuf_bytes)}")

        bit_encoder = BitEncoder()
        SensingDeltaEncoding.encode(bit_encoder, delta_encoding_parameters, sensor_parameters_by_sensor, dataset)
        bit_encoder.finalize()
        print(f"encoded byte count: {len(bit_encoder.data)}")
        bit_decoder = BitDecoder(bit_encoder.data, bit_encoder.bit_count)
        result = SensingDeltaEncoding.decode(bit_decoder, delta_encoding_parameters, sensor_parameters_by_sensor)

        SensingDeltaEncoding.print_dataset(result)

    @staticmethod
    def test():
        DeltaTests.test_zigzag()
        DeltaTests.test_dataset()


if __name__ == "__main__":
    DeltaTests.test()
