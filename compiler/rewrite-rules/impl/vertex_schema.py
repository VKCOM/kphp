import json
import jsonschema
from typing import Dict, Tuple, Optional, List, Union


class VertexArgumentInfo:
    def __init__(self, name: str, id: Union[int, List[int]]):
        self.name: str = name
        self.id: int = id  # index or range values
        self.type: str = ""
        self.optional: bool = False


class VertexInfo:
    def __init__(self, data: Dict[str, any]):
        self.name: str = data["name"]
        self.base: Optional[VertexInfo] = None
        self.range: Optional[VertexArgumentInfo] = None
        if "ranges" in data:
            ranges = data["ranges"]
            if len(ranges) != 1:
                raise RuntimeError(f"{self.name} multiple entries in 'ranges' are not implemented")
            rng_name, rng_data = list(ranges.items())[0]
            self.range = VertexArgumentInfo(rng_name, rng_data)
        self.args: List[VertexArgumentInfo] = []
        for son_name, son_data in data.get("sons", {}).items():
            if isinstance(son_data, int):
                self.args.append(VertexArgumentInfo(son_name, son_data))
                continue
            arg = VertexArgumentInfo(son_name, son_data["id"])
            if "type" in son_data:
                arg.type = son_data["type"]
            if "optional" in son_data:
                arg.optional = son_data["optional"]
            self.args.append(arg)

    def get_arg(self, i: int, length: int) -> Tuple[VertexArgumentInfo, Optional[int]]:
        for arg in self.args:
            if arg.id == i:
                return arg, None
            if arg.id < 0 and (length + arg.id) == i:
                return arg, None
        if self.is_variadic():
            range_min_index, range_max_index = self.get_range_bounds(length)
            if range_min_index <= i <= range_max_index:
                offset = i - range_min_index
                return self.get_range(), offset
        if self.base:
            return self.base.get_arg(i, length)
        raise RuntimeError(f"{self.name} get_arg(i={i}, length={length}): can't find an argument")

    def get_range_bounds(self, length: int) -> Tuple[int, int]:
        range_from, range_to = self.get_range().id
        max_index = length - 1
        if range_to > 0:
            max_index = range_to
        elif range_to < 0:
            max_index = (length - 1) + range_to
        return range_from, max_index

    def get_range(self) -> Optional[VertexArgumentInfo]:
        if self.range:
            return self.range
        return self.base.get_range() if self.base else None

    def has_children(self) -> bool:
        return bool(self.args or (self.base and self.base.has_children())) or self.is_variadic()

    def is_variadic(self):
        return self.get_range() is not None


# TODO: use this class in both vertex-gen.py and rules-gen.py?
class VertexSchema:
    def __init__(self, filename: str):
        with open(filename) as f:
            raw_data = json.load(f)
        self.__validate(raw_data, filename)
        self.vertex_map = {}
        # create the initial objects
        for vertex_data in raw_data:
            self.vertex_map[vertex_data["name"]] = VertexInfo(vertex_data)
        # now bind the parents
        for vertex_data in raw_data:
            v = self.vertex_map[vertex_data["name"]]
            if "base_name" in vertex_data:
                v.base = self.vertex_map[vertex_data["base_name"]]

    def get(self, op: str) -> VertexInfo:
        return self.vertex_map[op]

    @staticmethod
    def __validate(raw_data: Dict[str, any], filename: str):
        with open(filename.replace(".json", ".config.json")) as f:
            schema = json.load(f)
            jsonschema.validators.Draft4Validator.check_schema(schema)
            jsonschema.validate(raw_data, schema)

