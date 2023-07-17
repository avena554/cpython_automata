class DynamicEncoder:

    def __init__(self):
        self.next_id = 0
        self._decoder = []
        self._encoder = {}

    def encode(self, symb):
        if symb not in self._encoder:
            self._decoder.append(symb)
            self._encoder[symb] = self.next_id
            self.next_id += 1

        return self._encoder[symb]

    def decode(self, symb):
        return self._decoder[symb]

    def as_maps(self):
        return {v: i for (v, i) in self._encoder.items()}, {i: v for (i, v) in enumerate(self._decoder)}

    def known(self):
        return self._encoder.keys()


class StaticDecoder:

    def __init__(self, decoding_map):
        self.decoding_map = decoding_map

    def decode(self, symb):
        return self.decoding_map[symb]

